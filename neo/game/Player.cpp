#include "../idlib/precompiled.h"
#include "Player.h"

const static float PlayerSpeed = 0.1f;

Player::Player():_renderView(NULL), orgin(90, 64, 200)
{

}

void Player::CalculateRenderView( void ) {
	int i;

	if ( !_renderView ) {
		_renderView = new renderView_t;
	}
	memset( _renderView, 0, sizeof( *_renderView ) );

	// copy global shader parms
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		_renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}

	// calculate size of 3D view
	_renderView->x = 0;
	_renderView->y = 0;
	_renderView->width = SCREEN_WIDTH;
	_renderView->height = SCREEN_HEIGHT;
	_renderView->viewID = 0;

	_renderView->vieworg = orgin;
	_renderView->viewaxis = mat3_identity;

	_renderView->viewID = 0;

	// x ¼Ð½Ç
	_renderView->fov_x = 90;
	// y ¼Ð½Ç
	_renderView->fov_y = 74;

	_renderView->viewaxis = viewAngles.ToMat3() ;

	if ( _renderView->fov_y == 0 ) {
		common->Error( "renderView->fov_y == 0" );
	}
}

renderView_t* Player::GetRenderView()
{
	return _renderView;
}

void Player::Think()
{
	orgin.x += _usercmd.forwardmove * PlayerSpeed;
	orgin.y -= _usercmd.rightmove * PlayerSpeed; 
	orgin.z += _usercmd.upmove * PlayerSpeed;

	for (int i = 0; i < 3; i++ ) {
		cmdAngles[i] = SHORT2ANGLE( _usercmd.angles[i] );
		viewAngles[i] = idMath::AngleNormalize180( SHORT2ANGLE( _usercmd.angles[i]));
	}

	CalculateRenderView();
}

void Player::SetUserInput(const usercmd_t& usercmd )
{
	_usercmd = usercmd;
}
