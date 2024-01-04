/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_wins.c

#include "winsock.h"
#include "../qcommon/qcommon.h"

#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;


//cvar_t		*net_shownet; //braxi -- not used anywhere
static cvar_t	*net_noudp;

loopback_t	loopbacks[2];
int			ip_sockets[2];

char *NET_ErrorString (void);

//=============================================================================

void NetadrToSockadr (netadr_t *a, struct sockaddr *s)
{
	memset (s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (a->type == NA_IP)
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
}

void SockadrToNetadr(struct sockaddr* s, netadr_t* a)
{
	if (s->sa_family == AF_INET)
	{
		a->type = NA_IP;
		*(int*)&a->ip = ((struct sockaddr_in*)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in*)s)->sin_port;
	}
}

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
			return true;
	}

	return false;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
	}
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];

	if (a.type == NA_LOOPBACK)
		Com_sprintf (s, sizeof(s), "loopback");
	else /*if (a.type == NA_IP)*/
		Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
	return s;
}


/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
#define DO(src,dest)	\
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf (copy, "%x", &val);	

qboolean	NET_StringToSockaddr (char *s, struct sockaddr *sadr)
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];
	
	memset (sadr, 0, sizeof(*sadr));

	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
		
	((struct sockaddr_in *)sadr)->sin_port = 0;

	strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy; *colon; colon++)
	{
		if (*colon == ':')
		{
			*colon = 0;
			((struct sockaddr_in*)sadr)->sin_port = htons((short)atoi(colon + 1));
		}
	}
	
	if (copy[0] >= '0' && copy[0] <= '9')
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(copy);
	}
	else
	{
		h = gethostbyname(copy);
		if (!h)
			return 0;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}

	
	return true;
}

#undef DO

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	struct sockaddr sadr;
	
	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockaddr (s, &sadr))
		return false;
	
	SockadrToNetadr (&sadr, a);

	return true;
}


qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return adr.type == NA_LOOPBACK;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *netFrom, sizebuf_t *netMessage)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (netMessage->data, loop->msgs[i].data, loop->msgs[i].datalen);
	netMessage->cursize = loop->msgs[i].datalen;
	memset (netFrom, 0, sizeof(*netFrom));
	netFrom->type = NA_LOOPBACK;
	return true;

}


void NET_SendLoopPacket (netsrc_t sock, int length, void *data/*, netadr_t to*/)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *netFrom, sizebuf_t *netmessage)
{
	int 	ret;
	struct sockaddr from;
	int		fromlen;
	int		net_socket;
//	int		protocol;
	int		err;

	if (NET_GetLoopPacket (sock, netFrom, netmessage))
		return true;

//	for (protocol = 0 ; protocol < 2 ; protocol++) // braxi -- NO IPX, maybe add ipv6 some time
//	{
		//if (protocol == 0)
			net_socket = ip_sockets[sock];

			if (!net_socket)
				return false;

		fromlen = sizeof(from);
		ret = recvfrom (net_socket, netmessage->data, netmessage->maxsize
			, 0, (struct sockaddr *)&from, &fromlen);
		if (ret == -1)
		{
			err = WSAGetLastError();

			if (err == WSAEWOULDBLOCK)
				return false;

#if 1
			if (!dedicated->value)	// let dedicated servers continue after errors
				Com_Error(ERR_DROP, "NET_GetPacket: %s", NET_ErrorString());
#else
			if (dedicated->value)	// let dedicated servers continue after errors
				Com_Printf ("NET_GetPacket: %s", NET_ErrorString()); //braxi -- should be same behaviour for localplayer servers
			else
				Com_Error (ERR_DROP, "NET_GetPacket: %s", NET_ErrorString());
#endif
			return false;
		}

		SockadrToNetadr (&from, netFrom);

		if (ret == netmessage->maxsize)
		{
			Com_Printf ("Oversize packet from %s\n", NET_AdrToString (*netFrom));
			return false;
		}

		netmessage->cursize = ret;
		return true;
//	}
//	return false;
}

//=============================================================================

void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		ret;
	struct sockaddr	addr;
	int		net_socket;

	if ( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket (sock, length, data /*, to*/);
		return;
	}

	net_socket = 123; // silence compiler warning..

	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else
	{
		Com_Error(ERR_FATAL, "NET_SendPacket: bad address type");
	}

	NetadrToSockadr (&to, &addr);

	ret = sendto (net_socket, data, length, 0, &addr, sizeof(addr) );
	if (ret == -1)
	{
		int err = WSAGetLastError();

		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// some PPP links dont allow broadcasts
		if ((err == WSAEADDRNOTAVAIL) && (to.type == NA_BROADCAST))
			return;

		if (dedicated->value)	// let dedicated servers continue after errors
		{
			Com_Printf ("NET_SendPacket ERROR: %s\n", NET_ErrorString());
		}
		else
		{
			if (err == WSAEADDRNOTAVAIL)
			{
				Com_DPrintf (DP_NET, "NET_SendPacket Warning: %s : %s\n", NET_ErrorString(), NET_AdrToString (to));
			}
			else
			{
				Com_Error (ERR_DROP, "NET_SendPacket ERROR: %s\n", NET_ErrorString());
			}
		}
	}
}


//=============================================================================


/*
====================
NET_Socket
====================
*/
int NET_IPSocket (char *net_interface, int port)
{
	int					newsocket;
	struct sockaddr_in	address;
	qboolean			_true = true;
	int					i = 1;
	int					err;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			Com_Printf ("WARNING: UDP_OpenSocket: socket: %s", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	if (ioctlsocket (newsocket, FIONBIO, (u_long*)&_true) == -1)
	{
		Com_Printf ("WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		Com_Printf ("WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		NET_StringToSockaddr (net_interface, (struct sockaddr *)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	address.sin_family = AF_INET;

	if( bind (newsocket, (void *)&address, sizeof(address)) == -1)
	{
		Com_Printf ("WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString());
		closesocket (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void)
{
	cvar_t	*ip;
	int		port;
	int		isdedicated;

	ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);

	isdedicated = Cvar_VariableValue ("dedicated");

	if (!ip_sockets[NS_SERVER])
	{
		port = Cvar_Get("ip_hostport", "0", CVAR_NOSET)->value;
		if (!port)
		{
			port = Cvar_Get("hostport", "0", CVAR_NOSET)->value;
			if (!port)
			{
				port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_NOSET)->value;
			}
		}
		ip_sockets[NS_SERVER] = NET_IPSocket (ip->string, port);
		if (!ip_sockets[NS_SERVER] && isdedicated)
			Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port");
	}


	// dedicated servers don't need client ports
	if (isdedicated)
		return;

	if (!ip_sockets[NS_CLIENT])
	{
		port = Cvar_Get("ip_clientport", "0", CVAR_NOSET)->value;
		if (!port)
		{
			port = Cvar_Get("clientport", va("%i", PORT_CLIENT), CVAR_NOSET)->value;
			if (!port)
				port = PORT_ANY;
		}
		ip_sockets[NS_CLIENT] = NET_IPSocket (ip->string, port);
		if (!ip_sockets[NS_CLIENT])
			ip_sockets[NS_CLIENT] = NET_IPSocket (ip->string, PORT_ANY);
	}
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config (qboolean multiplayer)
{
	int		i;
	static	qboolean	old_config;

	if (old_config == multiplayer)
		return;

	old_config = multiplayer;

	if (!multiplayer)
	{	// shut down any existing sockets
		for (i=0 ; i<2 ; i++)
		{
			if (ip_sockets[i])
			{
				closesocket (ip_sockets[i]);
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		if (! net_noudp->value)
			NET_OpenIP ();
	}
}

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
    struct timeval timeout;
	fd_set	fdset;
	extern cvar_t *dedicated;
	int i;

	if (!dedicated || !dedicated->value)
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	i = 0;
	if (ip_sockets[NS_SERVER]) 
	{
		FD_SET(ip_sockets[NS_SERVER], &fdset); // network socket
		i = ip_sockets[NS_SERVER];
	}
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(i+1, &fdset, NULL, NULL, &timeout);
}

//===================================================================


static WSADATA		winsockdata;

/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
	WORD	wVersionRequested; 
	int		r;

	wVersionRequested = MAKEWORD(1, 1); 

	r = WSAStartup (wVersionRequested, &winsockdata);

	if (r)
		Com_Error(ERR_FATAL, "Winsock initialization failed.");

	Com_Printf("Winsock Initialized\n");

	net_noudp = Cvar_Get ("net_noudp", "0", CVAR_NOSET);

//	net_shownet = Cvar_Get ("net_shownet", "0", 0); //braxi -- not used anywhere
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown (void)
{
	NET_Config (false);	// close sockets

	WSACleanup ();
}


/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString (void)
{
	int		code;

	code = WSAGetLastError ();
	switch (code)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}
