/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


#ifdef CGAME_INCLUDE 
// these match engine
typedef enum
{
	XALIGN_NONE = 0,
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
} UI_AlignX;

#endif

//
// do not modify
//

// api version
#define	CGAME_API_VERSION 1

//
// functions provided by the main engine
//
typedef struct
{
	void	(*Printf) (char* fmt, ...);
	void	(*Error) (char* fmt, ...);

	char	*(*GetConfigString) (int index);

	// managed memory allocation
	void	*(*TagMalloc) (int size, int tag);
	void	(*TagFree) (void* block);
	void	(*FreeTags) (int tag);

	// net packet parsing
	int		(*MSG_ReadChar)(void);
	int		(*MSG_ReadByte)(void);
	int		(*MSG_ReadShort)(void);
	int		(*MSG_ReadLong)(void);
	float	(*MSG_ReadFloat)(void);
	float	(*MSG_ReadCoord)(void);
	void	(*MSG_ReadPos)(vec3_t pos);
	float	(*MSG_ReadAngle)(void);
	float	(*MSG_ReadAngle16)(void);
	void	(*MSG_ReadDir)(vec3_t vector);

	void	(*DrawString)(char* string, float x, float y, float fontSize, int alignx, rgba_t color);
	void	(*DrawStretchedImage)(rect_t rect, rgba_t color, char* pic);
	void	(*DrawFill) (rect_t rect, rgba_t color);

	void	(*GetCursorPos)(vec2_t *out);
	void	(*SetCursorPos)(int x, int y);

	// console variable interaction
	cvar_t* (*cvar) (char* var_name, char* value, int flags);
	cvar_t* (*cvar_set) (char* var_name, char* value);
	cvar_t* (*cvar_forceset) (char* var_name, char* value);

} cgame_import_t;

//
// functions exported by the game subsystem
//
typedef struct
{
	int			apiversion;

	int			(*Init) (void);
	void		(*Shutdown) (void);

	void		(*Frame) (float frametime, float time, float realtime, int width, int height);
	void		(*NewMap)(char* mapname);

	void		(*DrawHUD) (void);
	void		(*DrawUI) (void);

	void		(*ParseTempEnt)(int type);
} cgame_export_t;

cgame_export_t* GetClientGameAPI(cgame_import_t* import);

extern cgame_export_t* cgame;