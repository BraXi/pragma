/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

// cg_svcmd.qc

/*
=================
CG_ServerCommand

handles 'SVC_CGCMD [command (byte)] [...]' command from server

Use MSG_Read functions here...
=================
*/
void(float inCommand) CG_ServerCommand =
{
	switch(inCommand)
	{
	case 1:
		localsound("misc/beep.wav", 1.0);
		print(MSG_ReadString()); // print our message
		break;
	default:
		print("received unknown CG command from server: ", ftos(inCommand), "\n");
		break;
	}
};