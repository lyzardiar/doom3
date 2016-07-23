#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

class Player
{
public:
	Player();

	void CalculateRenderView();
	renderView_t* GetRenderView();

	void Think();
	void SetUserInput(const usercmd_t& usercmd);

private:
	renderView_t*	_renderView;
	usercmd_t		_usercmd;

	idAngles		viewAngles;	
	idAngles		cmdAngles;		

	idVec2 speed;

	static idCVar player_speed;
public:
	idVec3 orgin;
};

#endif