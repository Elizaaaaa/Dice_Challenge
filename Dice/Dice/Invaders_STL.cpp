#include "InvadersCore.h"

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

	InvadersCore core;
	core.setup(system);

	while (system->update())
	{
		core.newTime = system->getElapsedTime();
		core.calculateMoves();
		core.draw();

		IDiceInvaders::KeyStatus keys;
		system->getKeyStatus(keys);
		if (keys.right)
			core.horizontalPosition += core.move;
		else if (keys.left)
			core.horizontalPosition -= core.move;
		/*
		** Avoid the situation where player press and hold the space
		** Only one rocket will be created for one pressing keys.fire
		*/
		if (!keys.fire && core.launchRocket) {
			core.launchRocket = false;
		}
		if (keys.fire && !core.launchRocket) {
			core.initRocket();
			core.launchRocket = true;
		}
		core.lastTime = core.newTime;
	}

	system->destroy();

	return 0;
}



