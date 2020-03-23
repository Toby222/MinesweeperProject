#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <random>

#define MS_FIELD_SIZE 16
#define MS_TOPBAR_SIZE 31
#define MS_SCALE 2

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

		static olc::Sprite** IntToSprites(int value) {
			olc::Sprite** sprites = new olc::Sprite * [3]{ Sprites::digitSprites[9], Sprites::digitSprites[9], Sprites::digitSprites[9] };
			if (0 <= value && value < 999)
			{
				sprites[0] = digitSprites[value / 100 % 10];
				sprites[1] = digitSprites[value / 10 % 10];
				sprites[2] = digitSprites[value % 10];
			}
			if (-99 < value && value < 0) {
				sprites[0] = digitSprites[10];
				sprites[1] = digitSprites[-value / 10 % 10];
				sprites[2] = digitSprites[-value % 10];
			}
			if (value <= -99) {
				sprites[0] = digitSprites[10];
				sprites[1] = digitSprites[9];
				sprites[2] = digitSprites[9];
			}
			return sprites;
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
				digitSprites[10] = LoadSprite("digit-.png");
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

		static olc::Sprite* fieldSprites[9], * digitSprites[11], * closed, * flagged, * revealedmine, * falsemine, * clickedmine;
	};
	olc::Sprite* Sprites::fieldSprites[9], * Sprites::digitSprites[11], * Sprites::closed, * Sprites::flagged, * Sprites::revealedmine, * Sprites::falsemine, * Sprites::clickedmine;

	struct Square {
		enum class State {
			open,
			closed,
			flagged,
			pressed
		};

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

		Square* neighbors[8] = {};

		void setNeighbors(std::vector<std::vector<Square*>>* field) {
			for (int i = 0; i < 8; i++) {
				auto neighborPos = this->position + offsets[i];
				if (neighborPos.y >= field->size() || neighborPos.y < 0 ||
					neighborPos.x >= (*field)[0].size() || neighborPos.x < 0)
					neighbors[i] = nullptr;
				else {
					neighbors[i] = (*field)[neighborPos.y][neighborPos.x];
					if (neighbors[i]->isMine)
						this->value++;
				}
			}
		}

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
						default:
							throw std::exception("Invalid Square state");
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
						default:
							throw std::exception("Invalid Square state");
					}
				}
			}
			else {
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

		// Returns amount of opened squares
		// -1 if it was a mine.
		int TryOpen(std::vector<std::vector<Square*>> field, std::vector<Square*>* opened = nullptr) {
			if (this->state == State::closed ||
				this->state == State::pressed) {
				this->state = State::open;
				if (opened == nullptr)
					opened = new std::vector<Square*>();
				opened->push_back(this);
				if (this->isMine)
					return -1;
				if (this->value == 0 && std::find(opened->begin(), opened->end(), this) != opened->end()) {
					int result = 1;
					for (auto neighbor : this->neighbors) {
						if (neighbor == nullptr)
							continue;
						result += field[neighbor->position.y][neighbor->position.x]->TryOpen(field, opened);
					}
					return result;
				}
				else {
					return 1;
				}
			}
			return 0;
		}

		int TryFlag() {
			switch (this->state) {
				case(State::closed):
					this->state = State::flagged;
					return +1;
					break;
				case(State::flagged):
					this->state = State::closed;
					return -1;
					break;
				case(State::pressed):
				case(State::open):
					return +0;
					break;
				default:
					throw std::exception("Invalid Square state");
			}
		}
	};

	class Minesweeper : public olc::PixelGameEngine {
	private:
		std::vector<std::vector<Square*>> field;
		Square* hoveredSquare;
		Square* prevHoveredSquare;
		float passedSeconds;

		enum class State {
			newgame,
			playing,
			gameOver
		} gameState = State::newgame;
		int minecount;
		int flaggedSquares;
		int openedSquares;

	public:
		Minesweeper(int minecount = 35)
		{
			this->minecount = minecount;
			sAppName = "Minesweeper";
		}

		// Called once at the start, so create things here
		bool OnUserCreate() override
		{
			Sprites::Sprites();
			std::uniform_real_distribution<> random(0, 1);
			std::default_random_engine generator;

			if ((MS_FIELD_SIZE & MS_FIELD_SIZE >> 1) != 0)
				throw std::exception("Invalid this->field size. Has to be power of 2.");

			if (ScreenWidth() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen width.");
			if ((ScreenHeight() - MS_TOPBAR_SIZE) % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen height.");

			CreateField(this->minecount);

			return true;
		}

		void CreateField(int minecount = -1) {
			this->gameState = State::newgame;
			this->passedSeconds = 0;
			this->flaggedSquares = 0;
			this->openedSquares = 0;
			this->prevHoveredSquare = nullptr;
			this->hoveredSquare = nullptr;

			for (auto row : this->field)
				for (auto square : row)
					delete square;

			this->field.clear();

			for (int y = 0; y < (ScreenHeight() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE; y++) {
				this->field.push_back(std::vector<Square*>());
				for (int x = 0; x < ScreenWidth() / MS_FIELD_SIZE; x++) {
					this->field[y].push_back(new Square(olc::vi2d(x, y)));
				}
			}

			if (minecount == -1)
				minecount = this->minecount;
			if (minecount >= (this->field.size() * this->field[0].size()))
				throw std::exception("Invalid mine count for this this->field size.");

			std::uniform_int_distribution<> randomX(0, (int)this->field[0].size() - 1);
			std::uniform_int_distribution<> randomY(0, (int)this->field.size() - 1);

			std::random_device rd;
			std::default_random_engine gen(rd());

			for (int i = 0; i < minecount; i++) {
				auto newMine = this->field[randomY(gen)][randomX(gen)];
				if (newMine->isMine)
					i--;
				else
					newMine->isMine = true;
			}

			for (auto row : this->field)
				for (auto square : row)
					square->setNeighbors(&this->field);

			for (int y = 0; y < MS_TOPBAR_SIZE; y++)
				for (int x = 0; x < ScreenWidth(); x++)
					Draw(x, y, olc::DARK_GREY);
			OnUserUpdate(-1);
		}

		// Called once per frame; fElapsedTime is dT in seconds
		// Hacky: negative fElapsedTime forces redraw
		bool OnUserUpdate(float fElapsedTime) override
		{
			if (fElapsedTime >= 0 && gameState == State::playing)
				passedSeconds += fElapsedTime;

			if (!Minesweeper::IsFocused() && !((int)(passedSeconds - fElapsedTime) < (int)passedSeconds || fElapsedTime < 0))
				return true;
			if (GetKey(olc::Key::ESCAPE).bPressed)
				return false;
			if (GetKey(olc::Key::F5).bPressed && fElapsedTime >= 0)
				CreateField(this->minecount);

			prevHoveredSquare = hoveredSquare;
			hoveredSquare = nullptr;
			bool redrawField = fElapsedTime < 0;

			auto mousePos = olc::vi2d(GetMouseX(), GetMouseY() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE;

#ifndef NDEBUG
			if (GetKey(olc::Key::T).bHeld && GetKey(olc::Key::B).bHeld)
				for (auto row : this->field)
					for (auto square : row) {
						this->flaggedSquares += square->TryFlag();
						redrawField = true;
					}
#endif

			if (!(gameState == State::gameOver || prevHoveredSquare == nullptr)) {
				prevHoveredSquare->TryRelease();
				for (auto neighbor : prevHoveredSquare->neighbors)
					if (neighbor != nullptr)
						neighbor->TryRelease();
			}

			if (mousePos.y >= 0 && mousePos.y < this->field.size() &&
				mousePos.x >= 0 && mousePos.x < this->field[0].size() &&
				GetMouseY() > MS_TOPBAR_SIZE) {
				hoveredSquare = this->field[mousePos.y][mousePos.x];
				if (GetMouse(1).bPressed && !(gameState == State::gameOver)) {
					this->gameState = State::playing;
					this->flaggedSquares += hoveredSquare->TryFlag();
				}

				if (GetMouse(2).bHeld && !(gameState == State::gameOver)) {
					for (auto neighbor : hoveredSquare->neighbors)
						if (neighbor != nullptr)
							neighbor->TryPress();
				}
				else if (GetMouse(2).bReleased && !(gameState == State::gameOver) && hoveredSquare->state == Square::State::open) {
					int flaggedNeighbors = 0;
					for (auto neighbor : hoveredSquare->neighbors)
						if (neighbor != nullptr && neighbor->state == Square::State::flagged)
							flaggedNeighbors++;

					if (flaggedNeighbors == hoveredSquare->value)
						for (auto neighbor : hoveredSquare->neighbors)
						{
							if (neighbor != nullptr) {
								int opened = neighbor->TryOpen(this->field);
								this->openedSquares += opened;
								if (opened == -1)
									gameState = State::gameOver;
							}
						}
				}

				if ((GetMouse(0).bReleased) && !(gameState == State::gameOver)) {
					while (gameState == State::newgame && hoveredSquare->isMine)
					{
						CreateField(this->minecount);
						hoveredSquare = this->field[mousePos.y][mousePos.x];
					}
					gameState = State::playing;
					int opened = hoveredSquare->TryOpen(this->field);
					this->openedSquares += opened;
					if (opened == -1)
						gameState = State::gameOver;
				}
				else if ((GetMouse(0).bHeld || GetMouse(2).bHeld) && !(gameState == State::gameOver))
					hoveredSquare->TryPress();

				if ((this->field[0].size() * this->field.size()) - this->openedSquares == this->minecount)
					this->gameState = State::gameOver;
			}

			if (GetMouse(0).bPressed || GetMouse(0).bHeld || GetMouse(0).bReleased ||
				GetMouse(1).bPressed || GetMouse(1).bHeld || GetMouse(1).bReleased ||
				GetMouse(2).bPressed || GetMouse(2).bHeld || GetMouse(2).bReleased) {
				redrawField = true;
			}

			if (redrawField) {
				for (auto row : this->field)
					for (auto square : row)
						DrawSprite(square->position.x * MS_FIELD_SIZE, square->position.y * MS_FIELD_SIZE + MS_TOPBAR_SIZE, square->getSprite(gameState == State::gameOver));
				olc::Sprite** mineCounterSprites = Sprites::IntToSprites(this->minecount - this->flaggedSquares);
				for (int i = 0; i < 3; i++)
					DrawSprite(this->ScreenWidth() - 4 - mineCounterSprites[0]->width * (3 - i), 4, mineCounterSprites[i]);
			}

			// if             new second passed                          or redraw is forced
			if ((int)(passedSeconds - fElapsedTime) < (int)passedSeconds || fElapsedTime < 0) {
				olc::Sprite** counterSprites = Sprites::IntToSprites((int)passedSeconds);
				for (int i = 0; i < 3; i++)
					DrawSprite(4 + counterSprites[0]->width * i, 4, counterSprites[i]);
			}

			return true;
		}
	};
}

int main()
{
	minesweeper::Minesweeper mainWindow(50);

	if (mainWindow.Construct(MS_FIELD_SIZE * 16, MS_FIELD_SIZE * 16 + MS_TOPBAR_SIZE, MS_SCALE, MS_SCALE))
		mainWindow.Start();

	return 0;
}
