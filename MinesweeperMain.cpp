#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <random>

#define MS_FIELD_SIZE 16
#define MS_TOPBAR_SIZE 32

namespace minesweeper {
	struct Sprites {
		olc::Sprite* LoadSprite(std::string filename, olc::vi2d expectedSize = olc::vi2d(-1, -1)) {
			std::cout << filename << std::endl;
			std::stringstream filepath;
			filepath << "Sprites/" << filename;

			auto newSprite = new olc::Sprite(filepath.str());

			if (newSprite->width == 0 || newSprite->height == 0)
				throw std::exception("Invalid file.");
			if (((newSprite->width != expectedSize.x) && (expectedSize.x != -1)) ||
				((newSprite->height != expectedSize.y) && (expectedSize.y != -1))) {
				std::stringstream exceptionString;
				exceptionString << "Loaded sprite is invalid size. Needs to be " << expectedSize.x << "x" << expectedSize.y << std::endl;
				throw std::exception(exceptionString.str().c_str());
			}

			return newSprite;
		}

		Sprites() {
			std::cout << "Loading sprites...\n";
			try
			{
				std::stringstream filename;
				for (int i = 0; i < 9; i++) {
					filename.str("");
					filename << "field" << i << ".png";
					fieldSprites[i] = LoadSprite(filename.str(), olc::vi2d(MS_FIELD_SIZE, MS_FIELD_SIZE));
					filename.str("");
					filename << "digit" << i << ".png";
					digitSprites[i] = LoadSprite(filename.str(), olc::vi2d(13, 23));
				}
				flagged = LoadSprite("flagged.png");
				closed = LoadSprite("closed.png");
				revealedmine = LoadSprite("revealedmine.png");
			}
			catch (const std::exception & err)
			{
				std::cerr << "Terminating during loading process:\n  " << err.what() << std::endl;
				std::cin.get();
				exit(EXIT_FAILURE);
			}
		}

		static olc::Sprite* fieldSprites[9], * digitSprites[10], * closed, * flagged, * revealedmine;
	};
	olc::Sprite* Sprites::fieldSprites[9], * Sprites::digitSprites[10], * Sprites::closed, * Sprites::flagged, * Sprites::revealedmine;

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
			Square(bool isMine, olc::vi2d position) : position(position)
			{
				this->isMine = isMine;
				this->flagged = false;
				this->state = State::closed;
				this->value = 0;
			}

			const olc::vi2d position;
			bool isMine;
			int value;
			bool flagged;
			State state;

			olc::Sprite* getSprite() {
				if (isMine)
					return Sprites::revealedmine;
				switch (state) {
					case(State::closed):
						return Sprites::closed;
					case(State::flagged):
						return Sprites::flagged;
					case(State::open):
						return Sprites::fieldSprites[value];
					default:
						throw std::exception("Invalid Square state");
				}
			};

			void TryOpen() {
				if (this->state == State::closed)
					this->state = State::open;
			}

			void TryFlag() {
				switch (this->state) {
					case(State::closed):
						this->state = State::flagged;
						break;
					case(State::flagged):
						this->state = State::closed;
						break;
					case(State::open):
						break;
				}
			}
		};

	public:
		std::vector<std::vector<Square*>> field;

		Minesweeper()
		{
			sAppName = "Minesweeper";
		}

		// Called once at the start, so create things here
		bool OnUserCreate() override
		{
			Sprites::Sprites();
			std::uniform_real<> random(0, 1);
			std::default_random_engine generator;

			if ((MS_FIELD_SIZE & MS_FIELD_SIZE >> 1) != 0)
				throw std::exception("Invalid field size. Has to be power of 2.");

			if (ScreenWidth() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen width.");
			if (ScreenHeight() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen height.");

			for (int y = 0; y < (ScreenHeight() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE; y++) {
				field.push_back(std::vector<Square*>());
				for (int x = 0; x < ScreenWidth() / MS_FIELD_SIZE; x++) {
					field[y].push_back(new Square(random(generator) <= 0.25, olc::vi2d(x, y)));
				}
			}

			olc::vi2d offsets[8]{
				olc::vi2d(-1, 1),
				olc::vi2d(0, 1),
				olc::vi2d(1, 1),
				olc::vi2d(-1, 0),
				olc::vi2d(1,0),
				olc::vi2d(-1,1),
				olc::vi2d(0, 1),
				olc::vi2d(1, 1)
			};

			for (auto row : field)
			{
				for (auto square : row)
				{
					square->value = 0;
					square->TryOpen();
					for (auto offset : offsets) {
						auto neighborPos = square->position + offset;
						if (neighborPos.y >= field.size() ||
							neighborPos.y < 0 ||
							neighborPos.x >= field[0].size() ||
							neighborPos.x < 0)
							continue;
						if (field[neighborPos.y][neighborPos.x]->isMine)
							square->value++;
					}
				}
			}
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
					DrawSprite(square->position.x * MS_FIELD_SIZE, square->position.y * MS_FIELD_SIZE + MS_TOPBAR_SIZE, square->getSprite());

			return true;
		}
	};
}

int main()
{
	minesweeper::Minesweeper mainWindow;
	if (mainWindow.Construct(1024, 512 + MS_TOPBAR_SIZE, 1, 1))
		mainWindow.Start();

	return 0;
}