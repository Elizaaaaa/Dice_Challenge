#include <windows.h>
#include <cassert>
#include <cstdio>
#include "DiceInvaders.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define ROW_NUM 5
#define DIS_WITH_WINDOW 60
#define DIS_WITH_ALIEN 8

#define MAX_ROCKETS 20
#define MAX_BOMBS 20

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

bool HitByRocket(float alienX, float alienY, float rocketX, float rocketY) {
	if (rocketX >= alienX - 5 && rocketX <= alienX + 32 + 5)
		if (rocketY >= alienY && rocketY <= alienY + 32 + 16)
			return true;

	return false;
}

void insertNode(struct LinkedList** head, ISprite* newSprite, float x, float y) {
	struct LinkedList* newNode = (struct LinkedList*)malloc(sizeof(struct LinkedList));
	newNode->sprite = newSprite;
	newNode->x = x;
	newNode->y = y;
	// Add the node at the begining
	newNode->prev = NULL;
	newNode->next = (*head);
	if ((*head) != NULL)
		(*head)->prev = newNode;
	(*head) = newNode;
}

void deleteNode(struct LinkedList** head, struct LinkedList* node) {
	if (*head == NULL || node == NULL)
		return;
	// Case: detele the head node
	if (*head == node)
		*head = node->next;
	// Case: delete node is not the last node
	if (node->next != NULL)
		node->next->prev = node->prev;
	// Case: delete node is not the first node
	if (node->prev != NULL)
		node->prev->next = node->next;

	// Delete the memory
	free(node);
	node = NULL;
	return;
}

void DestroyRocet(ISprite* rockets, int i, int len) {

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

	// Initiate aliens
	const int col_num = (WINDOW_WIDTH - 2*DIS_WITH_WINDOW) / (32 + DIS_WITH_ALIEN);
	ISprite* aliens[col_num * ROW_NUM];
	for (int i = 0; i < col_num * ROW_NUM; ++i) {
		int y = i / col_num;

		if (y % 2 == 0)
			aliens[i] = system->createSprite("data/enemy1.bmp");
		else
			aliens[i] = system->createSprite("data/enemy2.bmp");
	}

	bool launchRocket = false;
	int rocketNum = 0;
	// Prepare the memory for rockets
	LinkedList* rockets = NULL;

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
	
	float alienMoveTime = 0.2f;

	while (system->update())
	{
		player->draw(int(horizontalPosition), WINDOW_HEIGHT - 32);

		float newTime = system->getElapsedTime();
		
		// Draw aliens
		for (int i = ROW_NUM - 1; i >= 0; --i) {			//y - coord
			//Move the aliens row by row
			if (newTime - lastAlienMoveTime > alienMoveTime && i == nextMoveRow) {
				alienHorisontalMove[i] += (alienMoveDir * alienSpeed);
				nextMoveRow = nextMoveRow > 0 ? nextMoveRow - 1 : ROW_NUM - 1;
				lastAlienMoveTime = newTime;
			}
			for (int j = 0; j < col_num; ++j) {		//x - coord
				if (aliens[j + i * col_num] != NULL) {
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
						system->destroy();
					}

					aliens[j + i * col_num]->draw(j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i], i * (32 + DIS_WITH_ALIEN) + alienStartPosition);
					
					// Check whether the alien is hit by a rocket
					LinkedList* r = rockets;
					while (r) {
						if (HitByRocket(j * (32 + DIS_WITH_ALIEN) + alienHorisontalMove[i], i * (32 + DIS_WITH_ALIEN), r->x, r->y)) {
							// Destroy the rocket
							r->sprite->destroy();
							deleteNode(&rockets, r);
							rocketNum--;
							r = rockets;

							// Destroy the alien
							if (aliens[j + i * col_num]) {
								//aliens[j + i * col_num]->destroy();
								aliens[j + i * col_num] = NULL;
							}
						}
						else
							r = r->next;
					}
				}
			}
		}

		float move = (newTime - lastTime) * 160.0f;
		float rocketMove = (newTime - lastTime) * 160.f;
		lastTime = newTime;

		LinkedList* tmp = rockets;
		while (tmp) {
			tmp->y -= rocketMove;
			// Out of the window
			if (tmp->y + 32 <= 0) {
				tmp->sprite->destroy();
				deleteNode(&rockets, tmp);
				rocketNum--;
				tmp = rockets;
				continue;
			}
			tmp->sprite->draw(tmp->x, tmp->y);
			tmp = tmp->next;
		}

		IDiceInvaders::KeyStatus keys;
		system->getKeyStatus(keys);
		if (keys.right)
			horizontalPosition += move;
		else if (keys.left)
			horizontalPosition -= move;
		if (keys.fire) 
			launchRocket = true;
		// Launch rocket after release the space
		if (!keys.fire && launchRocket) {
			if (rocketNum < MAX_ROCKETS) {
				insertNode(&rockets, system->createSprite("data/rocket.bmp"), horizontalPosition, WINDOW_HEIGHT - 32);
				rocketNum++;
				launchRocket = false;
			}
		}
	}

	player->destroy();
	system->destroy();

	return 0;
}



