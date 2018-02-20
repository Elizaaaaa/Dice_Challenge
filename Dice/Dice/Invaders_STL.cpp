#include <windows.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <array>
#include <list>
#include "DiceInvaders.h"

using namespace std;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define ROW_NUM 5
#define DIS_WITH_WINDOW 60
#define DIS_WITH_ALIEN 8

#define MAX_ROCKETS 20
#define MAX_BOMBS 20

#define MAX_HEALTH 3

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

bool hitByRocket(float alienX, float alienY, float rocketX, float rocketY) {
	if (rocketX >= alienX - 3 && rocketX <= alienX + 32 + 3)
		if (rocketY >= alienY && rocketY <= alienY + 32)
			return true;

	return false;
}

bool hitByBomb(float playerX, float playerY, float bombX, float bombY) {
	if (bombX >= playerX - 3 && bombX <= playerX + 32 + 3)
		if (bombY >= playerY - 16 && bombY <= playerY + 32)
			return true;

	return false;
}

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
	int playerHealth = MAX_HEALTH;

	// Initiate aliens
	const int col_num = (WINDOW_WIDTH - 2*DIS_WITH_WINDOW) / (32 + DIS_WITH_ALIEN);
	ISprite* aliens[ROW_NUM][col_num];

	for (int i = 0; i < ROW_NUM; ++i) {
		for (int j = 0; j < col_num; ++j) {
			if (i % 2 == 0) 
				aliens[i][j] = system->createSprite("data/enemy1.bmp");
			else
				aliens[i][j] = system->createSprite("data/enemy2.bmp");
		}
	}

	bool launchRocket = false;
	int rocketNum = 0;
	// Prepare the memory for rockets
	list<Weapon*> rockets;

	float alienStartPosition = 50;
	//Movement for each line
	float alienHorisontalMove[ROW_NUM];
	for (int i = 0; i < ROW_NUM; ++i)
		alienHorisontalMove[i] = DIS_WITH_WINDOW;
	//The last row moves first
	int nextMoveRow = ROW_NUM - 1;
	int alienMoveDir = 1;
	float alienSpeed = 5;
	int alienNum = ROW_NUM * col_num;

	// Use a random number to determine whether or not drop a bomb
	float dropBomb = 0.0f;
	// Prepare the memory for bombs
	list<Weapon*> bombs;

	float horizontalPosition = 320;
	float lastTime = system->getElapsedTime();
	float lastAlienMoveTime = system->getElapsedTime();
	// Don't drop bomb too frequently
	float lastDropBomb = system->getElapsedTime();
	
	float alienMoveTime = 0.2f;

	bool gameOver = false;

 	while (system->update())
	{
		player->draw(int(horizontalPosition), WINDOW_HEIGHT - 32);

		float newTime = system->getElapsedTime();

		// If all aliens are killed, re-generate a group of aliens
		if (alienNum == 0) {
			for (int i = 0; i < ROW_NUM; ++i) {
				for (int j = 0; j < col_num; ++j) {
					if (i % 2 == 0)
						aliens[i][j] = system->createSprite("data/enemy1.bmp");
					else
						aliens[i][j] = system->createSprite("data/enemy2.bmp");
				}
			}
			// Recover the position
			alienStartPosition = 50;
			for (int i = 0; i < ROW_NUM; ++i)
				alienHorisontalMove[i] = DIS_WITH_WINDOW;
			alienNum = col_num * ROW_NUM;
		}
		
		// Draw aliens
		for (int i = ROW_NUM - 1; i >= 0; --i) {			//y - coord
			//Move the aliens row by row
			if (newTime - lastAlienMoveTime > alienMoveTime && i == nextMoveRow) {
				alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
				nextMoveRow = nextMoveRow > 0 ? nextMoveRow - 1 : ROW_NUM - 1;
				lastAlienMoveTime = newTime;
			}
			for (int j = 0; j < col_num; ++j) {		//x - coord
				if (aliens[i][j] != NULL) {
					float xPos = j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i];
					float yPos = i * (32 + DIS_WITH_ALIEN) + alienStartPosition;
					// Aliens reach the boundary
					if (xPos + 32 >= WINDOW_WIDTH || xPos <= 0) {
						alienMoveDir *= -1;								// Change moving direction
						alienStartPosition += (32 + DIS_WITH_ALIEN);	// Move aliens down
						alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
					}
					// Aliens reach the bottom
					if (yPos + 32 >= WINDOW_HEIGHT) {
						// Game Over
						gameOver = true;
					}

					// Update aliens position
					xPos = j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i];
					yPos = i * (32 + DIS_WITH_ALIEN) + alienStartPosition;

					aliens[i][j]->draw(xPos, yPos);
					if (newTime - lastDropBomb > 5.0f) {
						// Drop a bomb?
						dropBomb = rand() % 11;
						if (dropBomb < 1.0f) {
							Weapon* newBomb = (struct Weapon*) malloc(sizeof(struct Weapon));
							newBomb->sprite = system->createSprite("data/bomb.bmp");
							newBomb->x = xPos;
							newBomb->y = yPos + 32;
							bombs.push_back(newBomb);
							lastDropBomb = newTime;
						}
					}

					// Check whether the alien is hit by a rocket
					auto r = rockets.begin();
					while (r != rockets.end()) {
						// Out of the window
						if (hitByRocket(xPos, yPos, (*r)->x, (*r)->y)) {
							(*r)->sprite->destroy();
							(*r)->sprite = NULL;
							rockets.erase(r++);
							rocketNum--;

							// Use aliens[i][j]->destroy() will cause memory warning
							//aliens[i][j]->destroy();
							aliens[i][j] = NULL;

							// Alien doesn't exist. 
							break;
						}
						else
							++r;
					}
				}
			}
		}

		float move = (newTime - lastTime) * 160.0f;
		float rocketMove = (newTime - lastTime) * 160.f;
		float bombMove = (newTime - lastTime) * 100.0f;
		lastTime = newTime;

		// Draw rockets
		auto r = rockets.begin();
		while (r != rockets.end()) { 
			(*r)->y -= rocketMove;
			// Out of the window
			if ((*r)->y + 32 <= 0) {
				(*r)->sprite->destroy();
				(*r)->sprite = NULL;
				rockets.erase(r++);
				rocketNum--;
			}
			else {
				(*r)->sprite->draw((*r)->x, (*r)->y);
				++r;
			}
		}

		// Draw bombs
		auto b = bombs.begin();
		while (b != bombs.end()) {
			(*b)->y += bombMove;
			// Out of the window
			if ((*b)->y >= WINDOW_HEIGHT ) {
				(*b)->sprite->destroy();
				(*b)->sprite = NULL;
				bombs.erase(b++);
			}
			// Bomb hits player
			else if (hitByBomb(horizontalPosition, WINDOW_HEIGHT - 32, (*b)->x, (*b)->y)) {
				(*b)->sprite->destroy();
				(*b)->sprite = NULL;
				bombs.erase(b++);
				playerHealth--;
				if (playerHealth <= 0)
					gameOver = true;
			}
			else {
				(*b)->sprite->draw((*b)->x, (*b)->y);
				++b;
			}
		}

		IDiceInvaders::KeyStatus keys;
		system->getKeyStatus(keys);
		if (keys.right)
			horizontalPosition += move;
		else if (keys.left)
			horizontalPosition -= move;
		if (keys.fire && !launchRocket) {
			if (rocketNum < MAX_ROCKETS) {
				Weapon* newRocket = (struct Weapon*) malloc(sizeof(struct Weapon));
				newRocket->sprite = system->createSprite("data/rocket.bmp");
				newRocket->x = horizontalPosition;
				newRocket->y = WINDOW_HEIGHT - 32;
				rockets.push_back(newRocket);
				rocketNum++;
			}
			launchRocket = true;
		}
		if (!keys.fire && launchRocket) {
			launchRocket = false;
		}

		string lifeInfo = "Remain Life: " + to_string(playerHealth) + "/" + to_string(MAX_HEALTH);
		system->drawText(32, WINDOW_HEIGHT - 32, lifeInfo.c_str());
	}

	player->destroy();
	system->destroy();

	return 0;
}



