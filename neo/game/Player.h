#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

class Player
{
public:
	Player();

	void CalculateRenderView();

	renderView_t* GetRenderView();

	void Think();

private:
	renderView_t* renderView;

public:
	idVec3 orgin;
};

#endif