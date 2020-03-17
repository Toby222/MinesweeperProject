#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <array>

#define MS_FIELD_SIZE 16
#define TOP_BAR_HEIGHT 32

class Field {
private:
	static struct Sprites {
		Sprites() {
			std::cout << "Loading sprites...\n";
			for (int i = 0; i < fieldSprites.size(); i++) {
				std::stringstream filename;
				filename << "Sprites/field" << i << ".png";
				try
				{
					std::cout << filename.str() << std::endl;
					auto newSprite = new olc::Sprite(filename.str());

					if (newSprite->width == 0 || newSprite->height == 0)
						throw std::exception("Invalid file.");
					if (newSprite->width != newSprite->height || (newSprite->height & newSprite->height >> 1) != 0)
						throw std::exception("Loaded sprite is invalid size. Needs to be power of 2, square");

					fieldSprites[i] = *newSprite;
				}
				catch (const std::exception & err)
				{
					std::cerr << "Terminating during loading process:\n  " << err.what() << std::endl;
					std::cin.get();
					exit(EXIT_FAILURE);
				}
			}
			flagged = olc::Sprite("Sprites/flagged.png");
			closed = olc::Sprite("Sprites/closed.png");
		};

		static std::array<olc::Sprite, 9> fieldSprites=nullptr;
		static olc::Sprite flagged =nullptr;
		static olc::Sprite closed =nullptr;
	};

public:
	enum class State {
		flagged,
		open,
		closed,
	} state = State::closed;

	olc::vi2d position;
	std::vector<Field> neighbors;
	int value = 0;
	bool isMine = false;

	olc::Sprite getSprite() {
		switch (state) {
			case (State::flagged):
				return Sprites::flagged;
			case(State::closed):
				return Sprites::closed;
			case(State::open):
				return Sprites::fieldSprites[value];
			default:
				throw new std::exception("Invalid field state.");
		}
	}
};

class Minesweeper : public olc::PixelGameEngine
{
public:
	Minesweeper()
	{
		sAppName = "Minesweeper";
	}

	// Called once at the start, so create things here
	bool OnUserCreate() override
	{
		return true;
	}

	// Called once per frame
	bool OnUserUpdate(float fElapsedTime) override
	{
		if (GetKey(olc::Key::ESCAPE).bPressed)
			return false;

		Field().getSprite();

		for (int x = ScreenWidth() - 1; x >= 0; x--) {
			for (int y = 0; y < 32; y++)
				Draw(x, y, olc::DARK_GREY);
		}

		return true;
	}
};

int main()
{
	Minesweeper mainWindow;
	if (mainWindow.Construct(256, 256, 2, 2))
		mainWindow.Start();

	return 0;
}
