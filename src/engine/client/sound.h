/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

struct sfx_s;

void S_Init (void);
void S_Shutdown (void);

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound (vec3_t origin, int entnum, int entchannel, struct sfx_s *sfx, float fvol,  float attenuation, float timeofs);
void S_StopEntitySounds(int entnum);
void S_StartLocalSound (char *s);

void S_RawSamples (int samples, int rate, int width, int channels, byte *data);

void S_StopAllSounds(void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

void S_Activate (qboolean active);

void S_BeginRegistration (void);
struct sfx_s *S_RegisterSound (char *sample);
void S_EndRegistration (void);

struct sfx_s *S_FindName (char *name, qboolean create);

// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CL_GetEntitySoundOrigin (int ent, vec3_t org);
