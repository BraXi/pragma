/*QUAKED test_solid (.5 .5 1) (-15 -15 -15) (15 15 15) 
30x30x30 solid box, DEVELOPER_SCRIPT must be enabled
*/
float() SP_test_solid =
{
	// test_solid is the classname
	// self is the new spawned entity
	local float mdl;
	if(!developer_script)
		return FALSE;
	if((mdl = ModelIndex("dev/box_30x30x30")) == 0)
		return FALSE; //no model found, delete ent
	
	SetModelIndex( self, mdl );
	SetSize( self, '-15 -15 -15', '15 15 15' );
	SetSolid( self, SOLID_BBOX );
		
	self.movetype = MOVETYPE_NONE;
	
	return TRUE;
};

// ----------------------------------------------------------------------

void() viewthing_think =
{
//	self.animFrame = (self.animFrame + 1) % 7;
	self.nextthink = time + frameTime;
};

/*QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)
Place one in a level to serve as an animation and model viewer. DEVELOPER_SCRIPT must be enabled
*/
float() SP_viewthing =
{
	local float mdl;	
	
	if(!developer_script)
		return FALSE;
	if((mdl = ModelIndex("objects/banner/tris")) == 0)
		return FALSE; // G_FreeEdict(self)

	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_BBOX;
	self.renderfx = RF_FRAMELERP;
	
	SetSize( self, '-16 -16 -24', '16 16 32' );
	SetModelIndex( self, mdl );
	
	self.nextthink = time + 0.5;
	self.think = viewthing_think;
	
	dprint( "viewthing spawned\n" );	
	return TRUE; //spawned succesfuly
};