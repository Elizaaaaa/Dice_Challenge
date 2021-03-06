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
	if (rocketX >= alienX -16 && rocketX <= alienX + 32 - 16)
		if (rocketY >= alienY && rocketY <= alienY + 32)
			return true;

	return false;
}

bool hitByBomb(float playerX, float playerY, float bombX, float bombY) {
	if (bombX >= playerX - 16 && bombX <= playerX + 32 - 16)
		if (bombY >= playerY - 16 && bombY <= playerY + 32)
			return true;

	return false;
}

void initAliens(struct ISprite ** aliens, int row, int col, ISprite* alien1, ISprite* alien2) {
	for (int i = 0; i < row; ++i) {
		for (int j = 0; j < col; ++j) {
			if (i % 2 == 0)
				aliens[j + i * col] = alien1;
			else
				aliens[j + i * col] = alien2;
		}
	}
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
	int score = 0;

	// Aliens
	const int col_num = (WINDOW_WIDTH - 2 * DIS_WITH_WINDOW) / (32 + DIS_WITH_ALIEN); // Calculate the col according to actual window width
	ISprite* aliens[ROW_NUM * col_num];				// Use array to store aliens. The size is known and won't change. More efficient to access through [x][y].
	initAliens(aliens, ROW_NUM, col_num, system->createSprite("data/enemy1.bmp"), system->createSprite("data/enemy2.bmp"));
	float alienStartPosition = 50;					// Y-position for the first line
	float alienHorisontalMove[ROW_NUM];				// Movement for each line
	for (int i = 0; i < ROW_NUM; ++i)
		alienHorisontalMove[i] = DIS_WITH_WINDOW;
	int nextMoveRow = ROW_NUM - 1;					// The last row moves first
	int alienMoveDir = 1;
	float alienSpeed = 5;
	float alienMoveTime = 0.2f;
	int alienNum = ROW_NUM * col_num;

	// Weapons
	bool launchRocket = false;
	int rocketNum = 0;
	/* Use List<> to store rockets and bombs.
	** Easy to insert and delete. Dynamic memory storage
	*/
	list<Weapon*> rockets;

	float dropBomb = 0.0f;							// Use a random number to determine whether or not drop a bomb
	float dropBombTimeGap = 4.0f;
	list<Weapon*> bombs;

	float horizontalPosition = 320;
	float lastTime = system->getElapsedTime();
	float lastAlienMoveTime = system->getElapsedTime();
	float lastDropBomb = system->getElapsedTime();

	bool gameOver = false;

	while (system->update())
	{
		if (!gameOver) {
			player->draw(int(horizontalPosition), WINDOW_HEIGHT - 32);

			float newTime = system->getElapsedTime();

			// If all aliens are killed, re-generate a group of aliens
			if (alienNum == 0) {
				initAliens(aliens, ROW_NUM, col_num, system->createSprite("data/enemy1.bmp"), system->createSprite("data/enemy2.bmp"));
				alienMoveTime = 0.2f;
				dropBombTimeGap = 4.0f;
				// Reset the position variables
				alienStartPosition = 50;
				for (int i = 0; i < ROW_NUM; ++i)
					alienHorisontalMove[i] = DIS_WITH_WINDOW;
				alienNum = col_num * ROW_NUM;
			}

			// Draw aliens
			for (int i = ROW_NUM - 1; i >= 0; --i) {			// y - coord
																// Move the aliens row by row
				if (newTime - lastAlienMoveTime > alienMoveTime && i == nextMoveRow) {
					alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
					nextMoveRow = nextMoveRow > 0 ? nextMoveRow - 1 : ROW_NUM - 1;
					lastAlienMoveTime = newTime;
				}
				for (int j = 0; j < col_num; ++j) {				// x - coord
					int index = j + i * col_num;
					if (aliens[index] != NULL) {
						float xPos = j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i];
						float yPos = i * (32 + DIS_WITH_ALIEN) + alienStartPosition;

						// Aliens reach the left / right boundary
						if (xPos + 32 >= WINDOW_WIDTH || xPos <= 0) {
							alienMoveDir *= -1;									// Change moving direction
							alienStartPosition += (32 + DIS_WITH_ALIEN);		// Move aliens down
							if (alienStartPosition >= WINDOW_HEIGHT / 5) {      // Speed up aliens and dropping bombs
								alienMoveTime = 0.1f;
								dropBombTimeGap -= 1.0f;
							}
							else if (alienStartPosition >= 2 * WINDOW_HEIGHT / 5) {
								alienMoveTime = 0.02f;
								dropBombTimeGap -= 1.0f;
							}
							alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
						}
						// Aliens reach the bottom
						if (yPos >= WINDOW_HEIGHT)
							// Game Over
							gameOver = true;

						// Update aliens position
						xPos = j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i];
						yPos = i * (32 + DIS_WITH_ALIEN) + alienStartPosition;

						aliens[index]->draw(xPos, yPos);
						if (newTime - lastDropBomb > dropBombTimeGap) {
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

								/* Use aliens[i][j]->destroy() will cause memory warning
								** The memory can be used for storing the next wave of aliens.
								** Use aliens[i][j] = NULL to refuse accessing
								*/
								//aliens[i][j]->destroy();
								aliens[index] = NULL;
								alienNum--;
								score += 100;

								// Alien doesn't exist. 
								break;
							}
							else
								++r;
						}

						/* Check whether the aliens hits the player
						** Since the player is fixed at the bottom, and game is over when aliens reach the bottom
						** it's hard to have this situation
						*/
						if (xPos >= horizontalPosition && xPos <= horizontalPosition + 32 && yPos >= WINDOW_HEIGHT - 32) {
							aliens[index] = NULL;
							alienNum--;
							playerHealth--;
							if (playerHealth <= 0)
								gameOver = true;
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
				if ((*b)->y >= WINDOW_HEIGHT) {
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
			/*
			** Avoid the situation where player press and hold the space
			** Only one rocket will be created for one pressing keys.fire
			*/
			if (!keys.fire && launchRocket) {
				launchRocket = false;
			}

			string lifeInfo = "Life: " + to_string(playerHealth) + "/" + to_string(MAX_HEALTH);
			system->drawText(32, WINDOW_HEIGHT - 32, lifeInfo.c_str());
			string scoreInfo = "Score: " + to_string(score);
			system->drawText(WINDOW_WIDTH - 150, 15, scoreInfo.c_str());
		}
		/*
		** Put restart game operation at the bottom
		** to improve the readability
		*/
		else if (gameOver) {
			string overMessage = "GAME OVER";
			system->drawText((WINDOW_WIDTH - 50) / 2, (WINDOW_HEIGHT - 100) / 2, overMessage.c_str());
			string scoreMessage = "Total Score: " + to_string(score);
			system->drawText((WINDOW_WIDTH - 80) / 2, (WINDOW_HEIGHT) / 2, scoreMessage.c_str());
			string restartMessage = "Press SPACE to restart";
			system->drawText((WINDOW_WIDTH - 116) / 2, (WINDOW_HEIGHT + 200) / 2, restartMessage.c_str());

			IDiceInvaders::KeyStatus keys;
			system->getKeyStatus(keys);
			// Restart the game
			if (keys.fire && gameOver) {
				alienMoveTime = 0.2f;
				dropBombTimeGap = 4.0f;
				// Reset the position variables
				alienStartPosition = 50;
				for (int i = 0; i < ROW_NUM; ++i)
					alienHorisontalMove[i] = DIS_WITH_WINDOW;
				alienNum = col_num * ROW_NUM;
				// Clear bombs and rockets
				for (auto b : bombs)
					if (b) {
						free(b);
						b = NULL;
					}
				for (auto r : rockets)
					if (r) {
						free(r);
						r = NULL;
					}
				// Re-generate aliens
				initAliens(aliens, ROW_NUM, col_num, system->createSprite("data/enemy1.bmp"), system->createSprite("data/enemy2.bmp"));

				rocketNum = 0;
				playerHealth = MAX_HEALTH;
				score = 0;
				// To avoid launching rocket when restarting
				launchRocket = true;

				gameOver = false;
			}
			continue;
		}
	}

	player->destroy();
	system->destroy();
	// Clear the memory
	for (int i = 0; i < ROW_NUM; ++i)
		for (int j = 0; j < col_num; ++j)
			if (aliens[j + i * col_num])
				aliens[j + i * col_num]->destroy();
	for (auto b : bombs)
		if (b) {
			free(b);
			b = NULL;
		}
	for (auto r : rockets)
		if (r) {
			free(r);
			r = NULL;
		}

	return 0;
}



