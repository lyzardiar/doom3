#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

class Player
{
public:
	Player();

	void CalculateRenderView();

	renderView_t* GetRenderView();

private:
	renderView_t* renderView;
};

#endif