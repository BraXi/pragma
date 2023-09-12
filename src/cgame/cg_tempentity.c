/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

#include "cg_local.h"

const char *str_tent[] =
{
	"TE_GUNSHOT",
	"TE_BLOOD",
	"TE_BLASTER",
	"TE_RAILTRAIL",
	"TE_SHOTGUN",
	"TE_EXPLOSION1",
	"TE_EXPLOSION2",
	"TE_ROCKET_EXPLOSION",
	"TE_GRENADE_EXPLOSION",
	"TE_SPARKS",
	"TE_SPLASH",
	"TE_BUBBLETRAIL",
	"TE_SCREEN_SPARKS",
	"TE_SHIELD_SPARKS",
	"TE_BULLET_SPARKS",
	"TE_LASER_SPARKS",
	"TE_PARASITE_ATTACK",
	"TE_ROCKET_EXPLOSION_WATER",
	"TE_GRENADE_EXPLOSION_WATER",
	"TE_MEDIC_CABLE_ATTACK",
	"TE_BFG_EXPLOSION",
	"TE_BFG_BIGEXPLOSION",
	"TE_BOSSTPORT",
	"TE_BFG_LASER,"
	"TE_GRAPPLE_CABLE",
	"TE_WELDING_SPARKS",
	"TE_GREENBLOOD",
	"TE_BLUEHYPERBLASTER",
	"TE_PLASMA_EXPLOSION",
	"TE_TUNNEL_SPARKS",
	"TE_BLASTER2",
	"TE_RAILTRAIL2",
	"TE_FLAME",
	"TE_LIGHTNING",
	"TE_DEBUGTRAIL",
	"TE_PLAIN_EXPLOSION",
	"TE_FLASHLIGHT",
	"TE_FORCEWALL",
	"TE_HEATBEAM",
	"TE_MONSTER_HEATBEAM",
	"TE_STEAM",
	"TE_BUBBLETRAIL2",
	"TE_MOREBLOOD",
	"TE_HEATBEAM_SPARKS",
	"TE_HEATBEAM_STEAM",
	"TE_CHAINFIST_SMOKE",
	"TE_ELECTRIC_SPARKS",
	"TE_TRACKER_EXPLOSION",
	"TE_TELEPORT_EFFECT",
	"TE_DBALL_GOAL",
	"TE_WIDOWBEAMOUT",
	"TE_NUKEBLAST",
	"TE_WIDOWSPLASH",
	"TE_EXPLOSION1_BIG",
	"TE_EXPLOSION1_NP",
	"TE_FLECHETTE"
};

void CG_ParseTempEntityMessage(int type)
{
	gi.Printf("CG_ParseTempEntityMessage: %s\n", str_tent[type]);
}