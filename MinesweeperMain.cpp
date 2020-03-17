#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <array>

#define MS_FIELD_SIZE 16
#define MS_TOPBAR_SIZE 32

namespace minesweeper {
	struct Sprites {
		olc::Sprite* LoadSprite(std::string filename) {
			std::cout << filename << std::endl;
			std::stringstream filepath;
			filepath << "Sprites/" << filename;

			auto newSprite = new olc::Sprite(filepath.str());

			if (newSprite->width == 0 || newSprite->height == 0)
				throw std::exception("Invalid file.");

			if (newSprite->width != MS_FIELD_SIZE || newSprite->height != MS_FIELD_SIZE) {
				std::stringstream exceptionString;
				exceptionString << "Loaded sprite is invalid size. Needs to be " << MS_FIELD_SIZE << "x" << MS_FIELD_SIZE << std::endl;
				throw std::exception(exceptionString.str().c_str());
			}

			return newSprite;
		}

		Sprites() {
			std::cout << "Loading sprites...\n";
			try
			{
				for (int i = 0; i < 9; i++) {
					std::stringstream filename;
					filename << "field" << i << ".png";
					fieldSprites[i] = LoadSprite(filename.str());
				}
				flagged = LoadSprite("flagged.png");
				closed = LoadSprite("closed.png");
			}
			catch (const std::exception & err)
			{
				std::cerr << "Terminating during loading process:\n  " << err.what() << std::endl;
				std::cin.get();
				exit(EXIT_FAILURE);
			}
		}

		olc::Sprite* fieldSprites[9];
		olc::Sprite* closed;
		olc::Sprite* flagged;
	};

	class Minesweeper : public olc::PixelGameEngine {
	private:
		struct Square {
		private:
			enum class State {
				open,
				closed,
				flagged
			};

		public:
			Square(bool mine, olc::vi2d position) : position(position)
			{
				this->mine = mine;
			}

			const olc::vi2d position;

			bool mine;
			int value = 0;
			bool flagged = false;
			State state = State::closed;

			olc::Sprite getSprite() {
				return olc::Sprite("Sprites/closed.png");
				/*switch (state) {
					case(State::closed):
						return sprites.closed;
					case(State::flagged):
						return sprites.flagged;
					case(State::open):
						return sprites.fieldSprites[value];
					default:
						throw std::exception("Invalid Square state");
				}*/
			};
		};

	public:
		std::vector<std::vector<Square>> field;

		Minesweeper()
		{
			for (int y = 0; y < 16; y++) {
				field.push_back(std::vector<Square>());
				for (int x = 0; x < 16; x++)
					field[y].push_back(Square(false, olc::vi2d(x, y)));
			}

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

			for (int y = 0; y < MS_TOPBAR_SIZE; y++)
				for (int x = 0; x < ScreenWidth(); x++)
					Draw(x, y, olc::DARK_GREY);

			for (auto row : field)
				for (auto square : row)
					DrawSprite(square.position.x * MS_FIELD_SIZE, square.position.y * MS_FIELD_SIZE + MS_TOPBAR_SIZE, &square.getSprite());

			return true;
		}
	};
}

int main()
{
	minesweeper::Sprites::Sprites();
	minesweeper::Minesweeper mainWindow;
	if (mainWindow.Construct(256, 256 + MS_TOPBAR_SIZE, 2, 2))
		mainWindow.Start();

	return 0;
}