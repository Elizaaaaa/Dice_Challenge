#include <windows.h>
#include <cassert>
#include <cstdio>
#include "DiceInvaders.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define ROW_NUM 5
#define DIS_WITH_WINDOW 60
#define DIS_WITH_ALIEN 8

class DiceInvadersLib
{
public:
	explicit DiceInvadersLib(LPCWSTR libraryPath)
	{
		m_lib = LoadLibrary(libraryPath);
		assert(m_lib);

		DiceInvadersFactoryType* factory = (DiceInvadersFactoryType*)GetProcAddress(
			m_lib, "DiceInvadersFactory");
		m_interface = factory();
		assert(m_interface);
	}

	~DiceInvadersLib()
	{
		FreeLibrary(m_lib);
	}

	IDiceInvaders* get() const
	{
		return m_interface;
	}

private:
	DiceInvadersLib(const DiceInvadersLib&);
	DiceInvadersLib& operator=(const DiceInvadersLib&);

private:
	IDiceInvaders * m_interface;
	HMODULE m_lib;
};

int APIENTRY WinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	LPSTR commandLine,
	int commandShow)
{
	DiceInvadersLib lib(L"DiceInvaders.dll");
	IDiceInvaders* system = lib.get();

	system->init(WINDOW_WIDTH, WINDOW_HEIGHT);

	ISprite* player = system->createSprite("data/player.bmp");
	const int col_num = (WINDOW_WIDTH - 2*DIS_WITH_WINDOW) / (32 + DIS_WITH_ALIEN);
	ISprite* aliens[col_num * ROW_NUM];
	for (int i = 0; i < col_num * ROW_NUM; ++i) {
		int y = i / col_num;

		if (y % 2 == 0)
			aliens[i] = system->createSprite("data/enemy1.bmp");
		else
			aliens[i] = system->createSprite("data/enemy2.bmp");
	}

	float alienStartPosition = 50;
	//Movement for each line
	float alienHorisontalMove[ROW_NUM];
	for (int i = 0; i < ROW_NUM; ++i)
		alienHorisontalMove[i] = DIS_WITH_WINDOW;
	//The last row moves first
	int nextMoveRow = ROW_NUM - 1;
	int alienMoveDir = 1;
	float alienSpeed = 5;

	float horizontalPosition = 320;
	float lastTime = system->getElapsedTime();
	float lastAlienMoveTime = system->getElapsedTime();
	
	float moveTime = 0.2;

	while (system->update())
	{
		player->draw(int(horizontalPosition), 480 - 32);

		float newTime = system->getElapsedTime();

		for (int i = ROW_NUM - 1; i >= 0; --i) {			//y - coord
			//Move the aliens row by row
			if (newTime - lastAlienMoveTime > moveTime && i == nextMoveRow) {
				alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
				nextMoveRow = nextMoveRow > 0 ? nextMoveRow - 1 : ROW_NUM - 1;
				lastAlienMoveTime = newTime;
			}

			for (int j = 0; j < col_num; ++j) {		//x - coord
				float xPos = j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i];
				float yPos = i * (32 + DIS_WITH_ALIEN) + alienStartPosition;
				if (xPos + 32 >= WINDOW_WIDTH || xPos <= 0) {
					alienMoveDir *= -1;
					alienStartPosition += (32 + DIS_WITH_ALIEN);
					alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
				}
				aliens[j + i * col_num]->draw(j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i], i * (32 + DIS_WITH_ALIEN) + alienStartPosition);
				
			}
		}
/*		
		for (int i = 0; i < col_num * ROW_NUM; ++i) {
			int x = i % col_num;
			int y = i / col_num;

			//float xMove = AlienMoveHorizontal(x, newTime, lastAlienMoveTime);
			if (newTime - lastAlienMoveTime > MOVE_TIME && x == nextMoveRow) {
				alienHorisontalMove = DIS_WITH_ALIEN + 10;
				nextMoveRow = nextMoveRow > 0 ? nextMoveRow - 1 : ROW_NUM - 1;
			}
			else
				alienHorisontalMove = DIS_WITH_ALIEN;

			aliens[i]->draw(x * (32 + DIS_WITH_ALIEN) + alienHorisontalMove, y * (32 + DIS_WITH_ALIEN) + alienStartPosition);
		}
*/
		float move = (newTime - lastTime) * 160.0f;
		lastTime = newTime;

		IDiceInvaders::KeyStatus keys;
		system->getKeyStatus(keys);
		if (keys.right)
			horizontalPosition += move;
		else if (keys.left)
			horizontalPosition -= move;
	}

	player->destroy();
	system->destroy();

	return 0;
}



