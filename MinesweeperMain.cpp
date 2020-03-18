#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <random>

#define MS_FIELD_SIZE 16
#define MS_TOPBAR_SIZE 32

namespace minesweeper {
	const olc::vi2d offsets[8]{
		olc::vi2d(-1, -1),
		olc::vi2d(0, -1),
		olc::vi2d(1, -1),
		olc::vi2d(-1, 0),
		olc::vi2d(1, 0),
		olc::vi2d(-1, 1),
		olc::vi2d(0, 1),
		olc::vi2d(1, 1)
	};

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
				digitSprites[9] = LoadSprite("digit9.png");
				closed = LoadSprite("closed.png");
				falsemine = LoadSprite("falsemine.png");
				flagged = LoadSprite("flagged.png");
				revealedmine = LoadSprite("revealedmine.png");
				clickedmine = LoadSprite("clickedmine.png");
			}
			catch (const std::exception & err)
			{
				std::cerr << "Terminating during loading process:\n  " << err.what() << std::endl;
				std::cin.get();
				exit(EXIT_FAILURE);
			}
		}

		static olc::Sprite* fieldSprites[9], * digitSprites[10], * closed, * flagged, * revealedmine, * falsemine, * clickedmine;
	};
	olc::Sprite* Sprites::fieldSprites[9], * Sprites::digitSprites[10], * Sprites::closed, * Sprites::flagged, * Sprites::revealedmine, * Sprites::falsemine, * Sprites::clickedmine;

	class Minesweeper : public olc::PixelGameEngine {
	private:
		struct Square {
		private:
			enum class State {
				open,
				closed,
				flagged,
				pressed
			};

		public:
			Square(olc::vi2d position) : position(position)
			{
				this->isMine = false;
				this->state = State::closed;
				this->value = 0;
			}

			const olc::vi2d position;
			bool isMine;
			int value;
			State state;

			olc::Sprite* getSprite(bool gameOver = false) {
				if (gameOver) {
					if (this->isMine) {
						switch (this->state) {
							case(State::flagged):
								return Sprites::flagged;
							case(State::closed):
								return Sprites::revealedmine;
							case(State::pressed):
								std::cerr << "Pressed square after gameOver";
							case(State::open):
								return Sprites::clickedmine;
						}
					}
					else {
						switch (this->state) {
							case(State::flagged):
								return Sprites::falsemine;
							case(State::pressed):
								std::cerr << "Pressed square after gameOver";
							case(State::open):
							case(State::closed):
								return Sprites::fieldSprites[this->value];
						}
					}
				}
				else {
					/*if (isMine)
						return Sprites::revealedmine;*/
					switch (state) {
						case(State::closed):
							return Sprites::closed;
						case(State::flagged):
							return Sprites::flagged;
						case(State::open):
							return Sprites::fieldSprites[value];
						case(State::pressed):
							return Sprites::fieldSprites[0];
						default:
							throw std::exception("Invalid Square state");
					}
				}
			};

			void TryPress() {
				if (this->state == State::closed)
					this->state = State::pressed;
			}

			void TryRelease() {
				if (this->state == State::pressed)
					this->state = State::closed;
			}

			void TryOpen(std::vector<std::vector<Square*>> field) {
				if (this->state == State::closed ||
					this->state == State::pressed) {
					this->state = State::open;

					if (this->value == 0) {
						for (auto offset : offsets) {
							auto neighborPos = this->position + offset;
							if (neighborPos.y >= field.size() || neighborPos.y < 0 ||
								neighborPos.x >= field[0].size() || neighborPos.x < 0)
								continue;
							field[neighborPos.y][neighborPos.x]->TryOpen(field);
						}
					}
				}
			}

			void TryFlag() {
				switch (this->state) {
					case(State::closed):
						this->state = State::flagged;
						break;
					case(State::flagged):
						this->state = State::closed;
						break;
					case(State::pressed):
					case(State::open):
						break;
				}
			}
		};

	private:
		std::vector<std::vector<Square*>> field;
		Square* hoveredSquare;
		Square* prevHoveredSquare;
		bool gameOver;
		int minecount;

	public:
		Minesweeper(int minecount = 35)
		{
			this->minecount = minecount;
			gameOver = false;
			sAppName = "Minesweeper";
		}

		// Called once at the start, so create things here
		bool OnUserCreate() override
		{
			Sprites::Sprites();
			std::uniform_real_distribution<> random(0, 1);
			std::default_random_engine generator;

			if ((MS_FIELD_SIZE & MS_FIELD_SIZE >> 1) != 0)
				throw std::exception("Invalid field size. Has to be power of 2.");

			if (ScreenWidth() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen width.");
			if (ScreenHeight() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen height.");

			for (int y = 0; y < (ScreenHeight() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE; y++) {
				field.push_back(std::vector<Square*>());
				for (int x = 0; x < ScreenWidth() / MS_FIELD_SIZE; x++)
					field[y].push_back(new Square(olc::vi2d(x, y)));
			}
			CreateField(this->minecount);

			for (auto row : field)
				for (auto square : row)
					for (auto offset : offsets) {
						auto neighborPos = square->position + offset;
						if (neighborPos.y >= field.size() || neighborPos.y < 0 ||
							neighborPos.x >= field[0].size() || neighborPos.x < 0)
							continue;
						if (field[neighborPos.y][neighborPos.x]->isMine)
							square->value++;
					}

			for (int y = 0; y < MS_TOPBAR_SIZE; y++)
				for (int x = 0; x < ScreenWidth(); x++)
					Draw(x, y, olc::DARK_GREY);

			for (auto row : field)
				for (auto square : row)
					DrawSprite(square->position.x * MS_FIELD_SIZE, square->position.y * MS_FIELD_SIZE + MS_TOPBAR_SIZE, square->getSprite());

			return true;
		}

		void CreateField(int minecount) {
			if (minecount >= (field.size() * field[0].size()))
				throw std::exception("Invalid mine count for this field size.");

			std::uniform_real_distribution<> randomX(0, field[0].size());
			std::uniform_real_distribution<> randomY(0, field.size());

			std::random_device rd;
			std::default_random_engine gen(rd());

			int DBG_MINECOUNT = 0;

			for (int i = 0; i < minecount; i++) {
				auto newMine = field[randomY(gen)][randomX(gen)];
				if (newMine->isMine)
					i--;
				else 
					newMine->isMine = true;
				
			}
		}

		// Called once per frame
		bool OnUserUpdate(float fElapsedTime) override
		{
			if (!Minesweeper::IsFocused())
				return true;
			if (GetKey(olc::Key::ESCAPE).bPressed)
				return false;

			prevHoveredSquare = hoveredSquare;
			hoveredSquare = nullptr;
			bool redraw = false;

			auto mousePos = olc::vi2d(GetMouseX(), GetMouseY() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE;

			if (!(gameOver || prevHoveredSquare == nullptr))
				prevHoveredSquare->TryRelease();

			if (mousePos.y >= 0 && mousePos.y < field.size() &&
				mousePos.x >= 0 && mousePos.x < field[0].size() &&
				GetMouseY() > MS_TOPBAR_SIZE) {
				hoveredSquare = field[mousePos.y][mousePos.x];
				if (GetMouse(1).bPressed && !gameOver)
					hoveredSquare->TryFlag();
				if (GetMouse(0).bReleased && !gameOver) {
					hoveredSquare->TryOpen(field);
					if (hoveredSquare->isMine)
						gameOver = true;
				}
				else if (GetMouse(0).bHeld && !gameOver)
					hoveredSquare->TryPress();
				else if (GetMouse(3).bHeld && !gameOver) {
					for (auto offset : offsets) {
					}
				}
				else if (GetMouse(3).bReleased) {
				}
			}

			if (GetMouse(0).bPressed || GetMouse(0).bHeld || GetMouse(0).bReleased ||
				GetMouse(1).bPressed || GetMouse(1).bHeld || GetMouse(0).bReleased) {
				redraw = true;
			}

			if (redraw) {
				for (int y = 0; y < MS_TOPBAR_SIZE; y++)
					for (int x = 0; x < ScreenWidth(); x++)
						Draw(x, y, olc::DARK_GREY);

				for (auto row : field)
					for (auto square : row)
						DrawSprite(square->position.x * MS_FIELD_SIZE, square->position.y * MS_FIELD_SIZE + MS_TOPBAR_SIZE, square->getSprite(gameOver));
			}

			return true;
		}
	};
}

int main()
{
	minesweeper::Minesweeper mainWindow(200);
	if (mainWindow.Construct(1024, 512 + MS_TOPBAR_SIZE, 1, 1))
		mainWindow.Start();

	return 0;
}