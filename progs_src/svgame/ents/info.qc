/*

info_null
info_notnull

*/

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
removed in game
*/

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for triggers, lights, etc.
*/

float() SP_info_null =
{
	return false; // discard it, it's only used by BSP tools
};

float() SP_info_notnull =
{
	return true;
};

