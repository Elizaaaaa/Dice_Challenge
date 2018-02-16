#ifndef DICEINVADERS_H
#define DICEINVADERS_H

struct ISprite
{
	// Destroys the sprite instance
	virtual void destroy() = 0;

	// Draw the sprite at the given position.
	// Valid coordinates are between (0,0) (upper left) and (width-32, height-32) (lower right).
	// (All sprites are 32*32 pixels, clipping is not supported)
	virtual void draw(int x, int y) = 0;
};

struct IDiceInvaders 
{
	// Destroys the dice invaders instance
	virtual void destroy() = 0;

	// Creates the main window. Returns true on success.
	virtual bool init(int width, int height) = 0;

	// Clears the screen and draws all sprites and texts which have been drawn
	// since the last update call.
	// If update returns false, the application should terminate.
	virtual bool update() = 0;

	// Create a sprite given a string.
	// All sprites are 32*32 pixels.
	virtual ISprite* createSprite(const char* name) = 0;

	// Draws the given text.
	virtual void drawText(int x, int y, const char* msg) = 0;

	// Return the total time spent in the game, in seconds.
	virtual float getElapsedTime() = 0;

	struct KeyStatus
	{
		bool fire; // space
		bool left; // left arrow
		bool right; // right arrow
	};

	// Returns the keyboard status. If a flag is set, the corresponding key is being held down.
	virtual void getKeyStatus(KeyStatus& keys) = 0;
};

// A factory type for creating IDiceInvaders instances.
typedef IDiceInvaders* (__cdecl DiceInvadersFactoryType)();

#endif
