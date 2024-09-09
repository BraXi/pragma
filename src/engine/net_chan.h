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

#ifndef _PRAGMA_NET_CHAN_H_
#define _PRAGMA_NET_CHAN_H_

typedef struct
{
	qboolean	fatal_error;

	netsrc_t	sock;

	int			dropped;			// between last packet and previous

	int			last_received;		// for timeouts
	int			last_sent;			// for retransmits

	netadr_t	remote_address;
	int			qport;				// qport value to write when transmitting

// sequencing variables
	int			incoming_sequence;
	int			incoming_acknowledged;
	int			incoming_reliable_acknowledged;	// single bit

	int			incoming_reliable_sequence;		// single bit, maintained local

	int			outgoing_sequence;
	int			reliable_sequence;			// single bit
	int			last_reliable_sequence;		// sequence number of last send

// reliable staging and holding areas
	sizebuf_t	message;		// writing buffer to send to server
	byte		message_buf[MAX_MSGLEN - 16];		// leave space for header

// message is copied to this buffer when it is first transfered
	int			reliable_length;
	byte		reliable_buf[MAX_MSGLEN - 16];	// unacked reliable message
} netchan_t;


extern	netadr_t	net_from;
extern	sizebuf_t	net_message;
extern	byte		net_message_buffer[MAX_MSGLEN];

void Netchan_Init(void);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int qport);

qboolean Netchan_NeedReliable(netchan_t* chan);
void Netchan_Transmit(netchan_t* chan, int length, byte* data);
void Netchan_OutOfBand(int net_socket, netadr_t adr, int length, byte* data);
void Netchan_OutOfBandPrint(int net_socket, netadr_t adr, char* format, ...);
qboolean Netchan_Process(netchan_t* chan, sizebuf_t* msg);

qboolean Netchan_CanReliable(netchan_t* chan);

#endif /*_PRAGMA_NET_CHAN_H_*/
