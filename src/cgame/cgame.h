#pragma once

extern void CL_ShutdownClientGame();
extern void CL_InitClientGame();

extern void CG_Main();
extern void CG_Frame(float frametime, int time, float realtime);
extern void CG_DrawGUI();


extern qboolean CG_CanDrawCall();