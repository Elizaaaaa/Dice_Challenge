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

class InvadersCore {
public:

	void setup(IDiceInvaders* _system) {
		// Get the system
		system = _system;
		// Setup Sprites
		player = system->createSprite("data/player.bmp");
		alien1 = system->createSprite("data/enemy1.bmp");
		alien2 = system->createSprite("data/enemy2.bmp");
		rocket = system->createSprite("data/rocket.bmp");
		bomb = system->createSprite("data/bomb.bmp");
		// Game start time
		lastTime = system->getElapsedTime();
		lastAlienMoveTime = system->getElapsedTime();
		lastDropBomb = system->getElapsedTime();
		// Player start positon
		horizontalPosition = 320;

		calculateMoves();
		initAliens();
	}

	void draw() {

			drawAliens();
			drawRockets();
			drawBombs();
			drawPlayer();
			drawText();

	}

	void initAliens() {
		for (int i = 0; i < row_num; ++i) {
			for (int j = 0; j < col_num; ++j) {
				if (i % 2 == 0)
					aliens[j + i * col_num] = alien1;
				else
					aliens[j + i * col_num] = alien2;
			}
		}

		for (int i = 0; i < row_num; ++i)
			alienHorisontalMove[i] = DIS_WITH_WINDOW;

		alienMoveTime = 0.2f;
		dropBombTimeGap = 4.0f;
		alienStartPosition = 50;
		alienNum = col_num * ROW_NUM;
	}

	void restart() {
		initAliens();
		rocketNum = 0;
		playerHealth = MAX_HEALTH;
		score = 0;
		gameOver = false;
		// To avoid launching rocket when restarting
		launchRocket = true;
	}

	bool hitByRocket(float alienX, float alienY, float rocketX, float rocketY) {
		if (rocketX >= alienX - 16 && rocketX <= alienX + 32 - 16)
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

	bool isGameOver() {
		return gameOver;
	}

	int getScore() {
		return score;
	}

	void initRocket() {
		if (rocketNum < MAX_ROCKETS) {
			Weapon* newRocket = (struct Weapon*) malloc(sizeof(struct Weapon));
			newRocket->sprite = rocket;
			newRocket->x = horizontalPosition;
			newRocket->y = WINDOW_HEIGHT - 32;
			rockets.push_back(newRocket);
			rocketNum++;
		}
	}

	bool isCreateBomb() {
		if (newTime - lastDropBomb > dropBombTimeGap) {
			dropBomb = rand() % 11;
			if (dropBomb < 1.0f)
				return true;
		}

		return false;
	}

	void initBomb(float x, float y) {
		Weapon* newBomb = (struct Weapon*) malloc(sizeof(struct Weapon));
		newBomb->sprite = bomb;
		newBomb->x = x;
		newBomb->y = y + 32;
		bombs.push_back(newBomb);
	}

	void calculateMoves() {
		move = (newTime - lastTime) * 160.0f;
		rocketMove = (newTime - lastTime) * 160.f;
		bombMove = (newTime - lastTime) * 100.0f;
	}

	void moveAlienRow(int row) {
		if (newTime - lastAlienMoveTime > alienMoveTime && row == nextMoveRow) {
			alienHorisontalMove[row] += (alienMoveDir * alienSpeed);
			nextMoveRow = nextMoveRow > 0 ? nextMoveRow - 1 : ROW_NUM - 1;
			lastAlienMoveTime = newTime;
		}
	}

	void checkAlienRockets(float x, float y, int index) {
		auto r = rockets.begin();
		while (r != rockets.end()) {
			if (hitByRocket(x, y, (*r)->x, (*r)->y)) {
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
	}

	void checkAlienPlayer(float x, float y, int index) {
		/* Check whether the aliens hits the player
		** Since the player is fixed at the bottom, and game is over when aliens reach the bottom
		** it's hard to have this situation
		*/
		if (x >= horizontalPosition && x <= horizontalPosition + 32 && y >= WINDOW_HEIGHT - 32) {
			aliens[index] = NULL;
			alienNum--;
			playerHealth--;
			if (playerHealth <= 0)
				gameOver = true;
		}
	}

	void drawPlayer() {
		player->draw(horizontalPosition, WINDOW_HEIGHT - 32);
	}

	void drawAliens() {
		for (int i = row_num - 1; i >= 0; --i) {
			moveAlienRow(i);
			for (int j = 0; j < col_num; ++j) {
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
						gameOver = true;

					// Update aliens position
					xPos = j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i];
					yPos = i * (32 + DIS_WITH_ALIEN) + alienStartPosition;

					aliens[index]->draw(xPos, yPos);

					// Drop a bomb?
					if (isCreateBomb()) {
						initBomb(xPos, yPos);
						lastDropBomb = newTime;
					}

					checkAlienRockets(xPos, yPos, index);
					checkAlienPlayer(xPos, yPos, index);
				}
			}
		}
	}

	void drawRockets() {
		auto r = rockets.begin();
		while (r != rockets.end()) {
			(*r)->y -= rocketMove;
			// Out of the window
			if ((*r)->y + 32 <= 0) {
				(*r)->sprite = NULL;
				(*r) = NULL;
				rockets.erase(r++);
				rocketNum--;
			}
			else {
				if (*r && (*r)->sprite)
					(*r)->sprite->draw((*r)->x, (*r)->y);
				++r;
			}
		}
	}

	void drawBombs() {
		auto b = bombs.begin();
		while (b != bombs.end()) {
			(*b)->y += bombMove;
			// Out of the window
			if ((*b)->y >= WINDOW_HEIGHT) {
				(*b)->sprite = NULL;
				*b = NULL;
				bombs.erase(b++);
			}
			// Bomb hits player
			else if (hitByBomb(horizontalPosition, WINDOW_HEIGHT - 32, (*b)->x, (*b)->y)) {
				(*b)->sprite = NULL;
				*b = NULL;
				bombs.erase(b++);
				playerHealth--;
				if (playerHealth <= 0)
					gameOver = true;
			}
			else {
				if (*b && (*b)->sprite)
					(*b)->sprite->draw((*b)->x, (*b)->y);
				++b;
			}
		}
	}

	void drawText() {
		if (gameOver) {
			string overMessage = "GAME OVER";
			system->drawText((WINDOW_WIDTH - 50) / 2, (WINDOW_HEIGHT - 100) / 2, overMessage.c_str());
			string scoreMessage = "Total Score: " + to_string(score);
			system->drawText((WINDOW_WIDTH - 80) / 2, (WINDOW_HEIGHT) / 2, scoreMessage.c_str());
			string restartMessage = "Press SPACE to restart";
			system->drawText((WINDOW_WIDTH - 116) / 2, (WINDOW_HEIGHT + 200) / 2, restartMessage.c_str());

		}
		else {
			string lifeInfo = "Life: " + to_string(playerHealth) + "/" + to_string(MAX_HEALTH);
			system->drawText(32, WINDOW_HEIGHT - 32, lifeInfo.c_str());
			string scoreInfo = "Score: " + to_string(score);
			system->drawText(WINDOW_WIDTH - 150, 15, scoreInfo.c_str());
		}
	}

	void clear() {
		for (int i = 0; i < row_num; ++i)
			for (int j = 0; j < col_num; ++j)
				if (aliens[j + i * col_num])
					aliens[j + i * col_num] = NULL;
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
	}


private:
	IDiceInvaders* system;

	// Set Sprites
	ISprite* alien1; ISprite* alien2; ISprite* rocket; ISprite* bomb;
	ISprite* player;

	// Game start time
	float newTime;
	float lastTime;
	float lastAlienMoveTime;
	float lastDropBomb;
	// Player start positon
	float horizontalPosition = 320;

	// Movements
	float move;
	float rocketMove;
	float bombMove;

	int playerHealth = MAX_HEALTH;
	int score = 0;

	static const int row_num = ROW_NUM;
	static const int col_num = (WINDOW_WIDTH - 2 * DIS_WITH_WINDOW) / (32 + DIS_WITH_ALIEN); // Calculate the col according to actual window width
	int alienNum = row_num * col_num;

	ISprite* aliens[row_num * col_num]; // Use array to store aliens. The size is known and won't change.
	float alienStartPosition = 50;					// Y-position for the first line
	float alienHorisontalMove[row_num];				// X-Movement for each line
	int nextMoveRow = ROW_NUM - 1;					// The last row moves first
	int alienMoveDir = 1;
	float alienSpeed = 5;
	float alienMoveTime = 0.2f;

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

	bool gameOver = false;
};
