/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================

NETWORK

Pragma's interface to the networking layer.

==============================================================
*/

#pragma once

#ifndef _PRAGMA_NETWORK_H_
#define _PRAGMA_NETWORK_H_

// minimum rate in bytes per second, was absurdly low 100 in Q2
#define NET_RATE_MIN 5000

// maximum rate in bytes per second
#define NET_RATE_MAX 25000

// default rate in bytes per second, was 5000 in Q2
#define NET_RATE_DEFAULT 15000


// ditto.
#define	PORT_ANY	-1

// master server port
#define	PORT_MASTER	27900

// default port for clients
#define	PORT_CLIENT	27901

// default port for game servers
#define	PORT_SERVER	27910

// max length of a message
#define	MAX_MSGLEN		1400	

// size of header in each packet two ints and a short
//#define	PACKET_HEADER	10		// braxi: commented out, unused	

typedef enum { NA_LOOPBACK, NA_BROADCAST, NA_IP } netadrtype_t;

typedef enum { NS_CLIENT, NS_SERVER } netsrc_t;

typedef struct
{
	netadrtype_t	type;
	byte			ip[4];
	unsigned short	port;
} netadr_t;

void		NET_Init(void);
void		NET_Shutdown(void);

void		NET_Config(qboolean multiplayer);

qboolean	NET_GetPacket(netsrc_t sock, netadr_t* net_from, sizebuf_t* net_message);
void		NET_SendPacket(netsrc_t sock, int length, void* data, netadr_t to);

qboolean	NET_CompareAdr(netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr(netadr_t a, netadr_t b);
qboolean	NET_IsLocalAddress(netadr_t adr);
char* NET_AdrToString(netadr_t a);
qboolean	NET_StringToAdr(char* s, netadr_t* a);
void		NET_Sleep(int msec);


#endif /*_PRAGMA_NETWORK_H_*/
