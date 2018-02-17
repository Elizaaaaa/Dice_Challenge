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

	float horizontalPosition = 320;
	float lastTime = system->getElapsedTime();
	while (system->update())
	{
		player->draw(int(horizontalPosition), 480 - 32);

		for (int i = 0; i < col_num * ROW_NUM; ++i) {
			int x = i % col_num;
			int y = i / col_num;

			aliens[i]->draw(x * (32 + DIS_WITH_ALIEN) + DIS_WITH_WINDOW, y * (32 + DIS_WITH_ALIEN) + alienStartPosition);
		}

		float newTime = system->getElapsedTime();
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



