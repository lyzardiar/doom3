#include "../idlib/precompiled.h"
#include "Player.h"

Player::Player():renderView(NULL)
{

}

void Player::CalculateRenderView( void ) {
	int i;
	float range;

	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	// copy global shader parms
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}
	//renderView->globalMaterial = gameLocal.GetGlobalMaterial();

	// calculate size of 3D view
	renderView->x = 0;
	renderView->y = 0;
	renderView->width = SCREEN_WIDTH;
	renderView->height = SCREEN_HEIGHT;
	renderView->viewID = 0;

	renderView->vieworg = vec3_zero;
	renderView->viewaxis = mat3_identity;

	renderView->viewID = 0;

	renderView->fov_x = 4;
	renderView->fov_y = 3;
		// field of view
		//gameLocal.CalcFov( CalcFov( true ), renderView->fov_x, renderView->fov_y );

	if ( renderView->fov_y == 0 ) {
		common->Error( "renderView->fov_y == 0" );
	}

		//gameLocal.Printf( "%s : %s\n", renderView->vieworg.ToString(), renderView->viewaxis.ToAngles().ToString() );
}

renderView_t* Player::GetRenderView()
{
	return renderView;
}
