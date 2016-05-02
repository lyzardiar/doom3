/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Player.h"

#ifdef GAME_DLL

idSys *						sys = NULL;
idCommon *					common = NULL;
idCmdSystem *				cmdSystem = NULL;
idCVarSystem *				cvarSystem = NULL;
idFileSystem *				fileSystem = NULL;
idNetworkSystem *			networkSystem = NULL;
idRenderSystem *			renderSystem = NULL;
idRenderModelManager *		renderModelManager = NULL;
idUserInterfaceManager *	uiManager = NULL;
idDeclManager *				declManager = NULL;
idCVar *					idCVar::staticVars = NULL;

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL|CVAR_SYSTEM, "force generic platform independent SIMD" );

#endif

idRenderWorld *				gameRenderWorld = NULL;		// all drawing is done to this world

static gameExport_t			gameExport;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
idGameLocal					gameLocal;
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal

const char *idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "surftype10", "surftype11", "surftype12", "surftype13", "surftype14", "surftype15"
};


idGameLocal::idGameLocal()
{
}

void idGameLocal::Init( void )
{
	idLib::Init();

	// register static cvars declared in the game
	idCVar::RegisterStaticVars();

	// register game specific decl types
	//declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDeclModelDef> );
	declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDecl> );
	declManager->RegisterDeclType( "export",			DECL_MODELEXPORT,	idDeclAllocator<idDecl> );

	// register game specific decl folders
	declManager->RegisterDeclFolder( "def",				".def",				DECL_ENTITYDEF );
	declManager->RegisterDeclFolder( "fx",				".fx",				DECL_FX );
	declManager->RegisterDeclFolder( "particles",		".prt",				DECL_PARTICLE );
	declManager->RegisterDeclFolder( "af",				".af",				DECL_AF );
	declManager->RegisterDeclFolder( "newpdas",			".pda",				DECL_PDA );

	player = new Player;
	player->CalculateRenderView();
}

void idGameLocal::Shutdown( void )
{

}

void idGameLocal::SetLocalClient( int clientNum )
{

}

void idGameLocal::ThrottleUserInfo( void )
{

}

const idDict * idGameLocal::SetUserInfo( int clientNum, const idDict &userInfo, bool isClient, bool canModify )
{
	return NULL;
}

const idDict * idGameLocal::GetUserInfo( int clientNum )
{
	return NULL;
}

void idGameLocal::SetServerInfo( const idDict &serverInfo )
{

}

const idDict & idGameLocal::GetPersistentPlayerInfo( int clientNum )
{
	return _playerInfo;
}

void idGameLocal::SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo )
{

}

void ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight ) {
	bool	gotTarget, gotUp, gotRight;
	const char	*texture;
	idVec3	color;

	memset( renderLight, 0, sizeof( *renderLight ) );

	if (!args->GetVector("light_origin", "", renderLight->origin)) {
		args->GetVector( "origin", "", renderLight->origin );
	}

	gotTarget = args->GetVector( "light_target", "", renderLight->target );
	gotUp = args->GetVector( "light_up", "", renderLight->up );
	gotRight = args->GetVector( "light_right", "", renderLight->right );
	args->GetVector( "light_start", "0 0 0", renderLight->start );
	if ( !args->GetVector( "light_end", "", renderLight->end ) ) {
		renderLight->end = renderLight->target;
	}

	// we should have all of the target/right/up or none of them
	if ( ( gotTarget || gotUp || gotRight ) != ( gotTarget && gotUp && gotRight ) ) {
		return;
	}

	if ( !gotTarget ) {
		renderLight->pointLight = true;

		// allow an optional relative center of light and shadow offset
		args->GetVector( "light_center", "0 0 0", renderLight->lightCenter );

		// create a point light
		if (!args->GetVector( "light_radius", "300 300 300", renderLight->lightRadius ) ) {
			float radius;

			args->GetFloat( "light", "300", radius );
			renderLight->lightRadius[0] = renderLight->lightRadius[1] = renderLight->lightRadius[2] = radius;
		}
	}

	// get the rotation matrix in either full form, or single angle form
	idAngles angles;
	idMat3 mat;
	if ( !args->GetMatrix( "light_rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
		if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
			args->GetFloat( "angle", "0", angles[ 1 ] );
			angles[ 0 ] = 0;
			angles[ 1 ] = idMath::AngleNormalize360( angles[ 1 ] );
			angles[ 2 ] = 0;
			mat = angles.ToMat3();
		}
	}

	// fix degenerate identity matrices
	mat[0].FixDegenerateNormal();
	mat[1].FixDegenerateNormal();
	mat[2].FixDegenerateNormal();

	renderLight->axis = mat;

	// check for other attributes
	args->GetVector( "_color", "1 1 1", color );
	renderLight->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderLight->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderLight->shaderParms[ SHADERPARM_BLUE ]		= color[2];
	args->GetFloat( "shaderParm3", "1", renderLight->shaderParms[ SHADERPARM_TIMESCALE ] );
	if ( !args->GetFloat( "shaderParm4", "0", renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] ) ) {
		// offset the start time of the shader to sync it to the game time
		renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args->GetFloat( "shaderParm5", "0", renderLight->shaderParms[5] );
	args->GetFloat( "shaderParm6", "0", renderLight->shaderParms[6] );
	args->GetFloat( "shaderParm7", "0", renderLight->shaderParms[ SHADERPARM_MODE ] );
	args->GetBool( "noshadows", "0", renderLight->noShadows );
	args->GetBool( "nospecular", "0", renderLight->noSpecular );
	args->GetBool( "parallel", "0", renderLight->parallel );

	args->GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );
}

void idGameLocal::InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, bool isServer, bool isClient, int randSeed )
{
	gameRenderWorld = renderWorld;

	renderLight_t* renderLight = new renderLight_t;
	memset(renderLight, 0, sizeof(renderLight_t));
	renderLight->origin.Set(-128, 192, 128);
	renderLight->lightRadius.Set(226, 226, 225);
	renderLight->pointLight = true;

	renderLight->shaderParms[ SHADERPARM_RED ]		= 0.78;
	renderLight->shaderParms[ SHADERPARM_GREEN ]	= 1;
	renderLight->shaderParms[ SHADERPARM_BLUE ]		= 1;

	renderLight->shader = declManager->FindMaterial( "lights/squarelight1", false );
	renderLight->shaderParms[3] = 1;

	gameRenderWorld->AddLightDef(renderLight);
}

bool idGameLocal::InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idFile *saveGameFile )
{
	return true;
}

void idGameLocal::SaveGame( idFile *saveGameFile )
{

}

void idGameLocal::MapShutdown( void )
{

}

void idGameLocal::CacheDictionaryMedia( const idDict *dict )
{

}

void idGameLocal::SpawnPlayer( int clientNum )
{

}

gameReturn_t idGameLocal::RunFrame( const usercmd_t *clientCmds )
{
	gameReturn_t ret;
	memset(&ret, 0, sizeof(gameReturn_t));

	usercmd_t cmd = clientCmds[0];
	player->orgin.x = player->orgin.x + cmd.forwardmove;
	player->CalculateRenderView();

	gameRenderWorld->SetRenderView(player->GetRenderView());
	return ret;
}

bool idGameLocal::Draw( int clientNum )
{
	gameRenderWorld->RenderScene( player->GetRenderView() );

	/*renderSystem->SetColor4( 1, 1, 1, 1.0 );
	const idMaterial* armorMaterial = declManager->FindMaterial( "armorViewEffect" );
	renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, armorMaterial );*/

	return true;
}

escReply_t idGameLocal::HandleESC( idUserInterface **gui )
{
	escReply_t ret;
	return ret;
}

idUserInterface	* idGameLocal::StartMenu( void )
{
	return NULL;
}

const char * idGameLocal::HandleGuiCommands( const char *menuCommand )
{
	return NULL;
}

void idGameLocal::HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui )
{

}

allowReply_t idGameLocal::ServerAllowClient( int numClients, const char *IP, const char *guid, const char *password, char reason[MAX_STRING_CHARS] )
{
	allowReply_t allreply;
	return allreply;
}

void idGameLocal::ServerClientConnect( int clientNum, const char *guid )
{

}

void idGameLocal::ServerClientBegin( int clientNum )
{

}

void idGameLocal::ServerClientDisconnect( int clientNum )
{

}

void idGameLocal::ServerWriteInitialReliableMessages( int clientNum )
{

}

void idGameLocal::ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, byte *clientInPVS, int numPVSClients )
{

}

bool idGameLocal::ServerApplySnapshot( int clientNum, int sequence )
{
	return false;
}

void idGameLocal::ServerProcessReliableMessage( int clientNum, const idBitMsg &msg )
{

}

void idGameLocal::ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg )
{

}

bool idGameLocal::ClientApplySnapshot( int clientNum, int sequence )
{
	return false;
}

void idGameLocal::ClientProcessReliableMessage( int clientNum, const idBitMsg &msg )
{

}

gameReturn_t idGameLocal::ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame )
{
	gameReturn_t ret;

	return ret;
}

void idGameLocal::GetClientStats( int clientNum, char *data, const int len )
{

}

void idGameLocal::SwitchTeam( int clientNum, int team )
{

}

bool idGameLocal::DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] )
{
	return false;
}

void idGameLocal::SelectTimeGroup( int timeGroup )
{

}

int idGameLocal::GetTimeGroupTime( int timeGroup )
{
	return 0;
}

void idGameLocal::GetBestGameType( const char* map, const char* gametype, char buf[ MAX_STRING_CHARS ] )
{

}

void idGameLocal::GetMapLoadingGUI( char gui[ MAX_STRING_CHARS ] )
{

}













































/*
===========
GetGameAPI
============
*/
#if __MWERKS__
#pragma export on
#endif
#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
extern "C" gameExport_t *GetGameAPI( gameImport_t *import ) {
#if __MWERKS__
#pragma export off
#endif

	if ( import->version == GAME_API_VERSION ) {

		// set interface pointers used by the game
		sys							= import->sys;
		common						= import->common;
		cmdSystem					= import->cmdSystem;
		cvarSystem					= import->cvarSystem;
		fileSystem					= import->fileSystem;
		networkSystem				= import->networkSystem;
		renderSystem				= import->renderSystem;
		renderModelManager			= import->renderModelManager;
		uiManager					= import->uiManager;
		declManager					= import->declManager;
	}

	// set interface pointers used by idLib
	idLib::sys					= sys;
	idLib::common				= common;
	idLib::cvarSystem			= cvarSystem;
	idLib::fileSystem			= fileSystem;

	// setup export interface
	gameExport.version = GAME_API_VERSION;
	gameExport.game = game;

	return &gameExport;
}
#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif

/*
===========
TestGameAPI
============
*/
void TestGameAPI( void ) {
	gameImport_t testImport;
	gameExport_t testExport;

	testImport.sys						= ::sys;
	testImport.common					= ::common;
	testImport.cmdSystem				= ::cmdSystem;
	testImport.cvarSystem				= ::cvarSystem;
	testImport.fileSystem				= ::fileSystem;
	testImport.networkSystem			= ::networkSystem;
	testImport.renderSystem				= ::renderSystem;
	testImport.renderModelManager		= ::renderModelManager;
	testImport.uiManager				= ::uiManager;
	testImport.declManager				= ::declManager;

	testExport = *GetGameAPI( &testImport );
}

