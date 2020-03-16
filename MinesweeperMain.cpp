#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <array>

#define MS_FIELD_SIZE 16

class Minesweeper : public olc::PixelGameEngine
{
private:
	std::array<olc::Sprite, 9> fieldSprites;
public:
	Minesweeper()
	{
		sAppName = "Minesweeper";
	}

public:
	// Called once at the start, so create things here
	bool OnUserCreate() override
	{
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
		return true;
	}

	// Called once per frame
	bool OnUserUpdate(float fElapsedTime) override
	{
		if (GetKey(olc::Key::ESCAPE).bPressed)
			return false;
		for (int i = 0; i < fieldSprites.size(); i++) {
			const int x = (i * MS_FIELD_SIZE) % ScreenWidth();
			const int y = ((MS_FIELD_SIZE * i) / ScreenWidth()) * MS_FIELD_SIZE;
			DrawSprite(x, y, &fieldSprites[i], MS_FIELD_SIZE/fieldSprites[i].height);
		}

		return true;
	}
};

int main()
{
	Minesweeper mainWindow;
	if (mainWindow.Construct(256, 256, 1, 1))
		mainWindow.Start();

	return 0;
}