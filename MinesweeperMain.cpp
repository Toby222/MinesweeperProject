#define OLC_PGE_APPLICATION
#define OLC_PGEX_CONTROLS
// #define OLC_GFX_OPENGL10

#include "olcPixelGameEngine.h"
#include "olcPGEX_Controls.h"
#include <vector>
#include <random>
#include <algorithm>

namespace minesweeper {
	constexpr auto MS_FIELD_SIZE = 16;
	constexpr auto MS_TOPBAR_SIZE = 31;
	constexpr auto MS_SCALE = 2;

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

	struct Graphics {
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

		static olc::Decal** IntToDecals(int value) {
			olc::Decal** decals = new olc::Decal * [3]{ Graphics::digitDecals[9], Graphics::digitDecals[9], Graphics::digitDecals[9] };
			if (0 <= value && value < 999)
			{
				decals[0] = digitDecals[value / 100 % 10];
				decals[1] = digitDecals[value / 10 % 10];
				decals[2] = digitDecals[value % 10];
			}
			if (-99 < value && value < 0) {
				decals[0] = digitDecals[10];
				decals[1] = digitDecals[-value / 10 % 10];
				decals[2] = digitDecals[-value % 10];
			}
			if (value <= -99) {
				decals[0] = digitDecals[10];
				decals[1] = digitDecals[9];
				decals[2] = digitDecals[9];
			}
			return decals;
		}

		Graphics() {
			std::cout << "Loading sprites...\n";
			try
			{
				std::stringstream filename;
				for (int i = 0; i < 9; i++) {
					filename.str("");
					filename << "field" << i << ".png";
					fieldSprites[i] = LoadSprite(filename.str(), olc::vi2d(MS_FIELD_SIZE, MS_FIELD_SIZE));
					fieldDecals[i] = new olc::Decal(fieldSprites[i]);
					filename.str("");
					filename << "digit" << i << ".png";
					digitSprites[i] = LoadSprite(filename.str(), olc::vi2d(13, 23));
					digitDecals[i] = new olc::Decal(digitSprites[i]);
				}
				digitSprites[9] = LoadSprite("digit9.png");
				digitDecals[9] = new olc::Decal(digitSprites[9]);
				digitSprites[10] = LoadSprite("digit-.png");
				digitDecals[10] = new olc::Decal(digitSprites[10]);
				closedSprite = LoadSprite("closed.png");
				closedDecal = new olc::Decal(closedSprite);
				falsemineSprite = LoadSprite("falsemine.png");
				closedDecal = new olc::Decal(closedSprite);
				flaggedSprite = LoadSprite("flagged.png");
				flaggedDecal = new olc::Decal(flaggedSprite);
				revealedmineSprite = LoadSprite("revealedmine.png");
				revealedmineDecal = new olc::Decal(revealedmineSprite);
				clickedmineSprite = LoadSprite("clickedmine.png");
				clickedmineDecal = new olc::Decal(clickedmineSprite);
			}
			catch (const std::exception& err)
			{
				std::cerr << "Terminating during loading process:\n  " << err.what() << std::endl;
				std::cin.get();
				exit(EXIT_FAILURE);
			}
		}
		//                  012345678          0123456789-
		static olc::Decal* fieldDecals[9], * digitDecals[11], * closedDecal, * flaggedDecal, * revealedmineDecal, * falsemineDecal, * clickedmineDecal;
	private:
		static olc::Sprite* fieldSprites[9], * digitSprites[11], * closedSprite, * flaggedSprite, * revealedmineSprite, * falsemineSprite, * clickedmineSprite;
	};
	olc::Sprite* Graphics::fieldSprites[9], * Graphics::digitSprites[11], * Graphics::closedSprite, * Graphics::flaggedSprite, * Graphics::revealedmineSprite, * Graphics::falsemineSprite, * Graphics::clickedmineSprite;
	olc::Decal* Graphics::fieldDecals[9], * Graphics::digitDecals[11], * Graphics::closedDecal, * Graphics::flaggedDecal, * Graphics::revealedmineDecal, * Graphics::falsemineDecal, * Graphics::clickedmineDecal;

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

		olc::Decal* getDecal(bool gameOver = false) {
			if (gameOver) {
				if (this->isMine) {
					switch (this->state) {
					case(State::flagged):
						return Graphics::flaggedDecal;
					case(State::closed):
						return Graphics::revealedmineDecal;
					case(State::pressed):
						std::cerr << "Pressed square after gameOver";
					case(State::open):
						return Graphics::clickedmineDecal;
					default:
						throw std::exception("Invalid Square state");
					}
				}
				else {
					switch (this->state) {
					case(State::flagged):
						return Graphics::falsemineDecal;
					case(State::pressed):
						std::cerr << "Pressed square after gameOver";
					case(State::open):
					case(State::closed):
						return Graphics::fieldDecals[this->value];
					default:
						throw std::exception("Invalid Square state");
					}
				}
			}
			else {
				switch (state) {
				case(State::closed):
					return Graphics::closedDecal;
				case(State::flagged):
					return Graphics::flaggedDecal;
				case(State::open):
					return Graphics::fieldDecals[value];
				case(State::pressed):
					return Graphics::fieldDecals[0];
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

		// returns difference in unflagged Squares
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

		olc::ctrls::Slider* amountSlider;

		enum class State {
			newgame,
			playing,
			gameOver
		} gameState = State::newgame;
		enum class Display {
			help,
			config,
			game
		} display = Display::help;

		int minecount;
		int flaggedSquares;
		int openedSquares;

	public:
		Minesweeper(int minecount = 35)
		{
			this->minecount = minecount;
			sAppName = "Minesweeper";
		}

		int getMaxMines() {
			return this->field.size() * this->field[0].size();
		}

		// Called once at the start, so create things here
		bool OnUserCreate() override
		{
			olc::ctrls::Initialize(this);

			Graphics::Graphics();
			std::uniform_real_distribution<> random(0, 1);
			std::default_random_engine generator;

			if ((MS_FIELD_SIZE & MS_FIELD_SIZE >> 1) != 0)
				throw std::exception("Invalid field size. Has to be power of 2.");

			if (ScreenWidth() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen width.");
			if ((ScreenHeight() - MS_TOPBAR_SIZE) % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen height.");

			CreateField(0);

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
			if (minecount >= (getMaxMines()))
				throw std::exception("Invalid mine count for field size.");

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
		}

		bool OnUserUpdate(float fElapsedTime) override
		{
			if (fElapsedTime >= 0 && gameState == State::playing)
				passedSeconds += fElapsedTime;

			if (!Minesweeper::IsFocused() && !((int)(passedSeconds - fElapsedTime) < (int)passedSeconds || fElapsedTime < 0))
				return true;
			if (GetKey(olc::Key::ESCAPE).bPressed)
				return false;

			if (GetKey(olc::Key::F5).bPressed && fElapsedTime >= 0) {
				display = Display::game;
				CreateField(this->minecount);
				for (int y = 0; y < MS_TOPBAR_SIZE; y++)
					for (int x = 0; x < ScreenWidth(); x++)
						Draw(x, y, olc::DARK_GREY);
			}

			prevHoveredSquare = hoveredSquare;
			hoveredSquare = nullptr;

			auto mousePos = olc::vi2d(GetMouseX(), GetMouseY() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE;

#ifndef NDEBUG
			if (GetKey(olc::Key::T).bHeld && GetKey(olc::Key::B).bPressed)
				for (auto row : this->field)
					for (auto square : row) {
						this->flaggedSquares += square->TryFlag();
						redrawField = true;
					}
#endif // !NDEBUG

			if (GetKey(olc::Key::F2).bPressed)
			{
				if (display == Display::config) {
					if (fElapsedTime >= 0)
						this->CreateField();
					display = Display::game;
					for (int y = 0; y < MS_TOPBAR_SIZE; y++)
						for (int x = 0; x < ScreenWidth(); x++)
							Draw(x, y, olc::DARK_GREY);
				}
				else {
					display = Display::config;

					// Why doesn't this cause a memory leak?
					amountSlider = new olc::ctrls::Slider({ 5, 32 + 15 + 8 }, ScreenWidth() - 10, olc::ctrls::Orientation::HORIZONTAL, olc::DARK_GREY, olc::GREY);
					amountSlider->SetHeadOffset((float)(this->minecount - 1) / ((float)(getMaxMines() - 2)) * amountSlider->GetWidth());
				}
			}
			if (GetKey(olc::Key::F1).bPressed) {
				display = Display::help;
				Clear(olc::BLACK);
			}
			switch (this->display)
			{
				// HILFE
			case(Display::help):
			{
				DrawStringDecal({ 8,8 }, "Keybinds\n------------------------------\nESC: Exit Game\nF1: Help\nF2: Settings\nF5: New Game\n\n\nControls\n------------------------------\nLMB: Reveal Square\nRMB: Flag Square\nMMB: Reveal adjacent Squares");

#ifndef NDEBUG
				DrawStringDecal({ 8, 8 * 16 }, "TODO?\n------------------------------\nSound\nAlternate graphics");
#endif // !NDEBUG

				DrawStringDecal({ 8, (float)ScreenHeight() - 8 * (3 + 1) }, "Created by Tobias Berger\nusing the\nOneLoneCoder PixelGameEngine");
				return true;
			}
			// EINSTELLUNGEN
			case(Display::config):
			{
				Clear(olc::BLACK);
				DrawStringDecal({ 8,8 }, "Settings\n------------------------------\n\nMines:");
				amountSlider->Update();
				amountSlider->SetHeadOffset(std::clamp(amountSlider->GetHeadOffset(), 0.0f, amountSlider->GetWidth()));
#ifndef NDEBUG
				printf_s("%f\n", amountSlider->GetPercent());
#endif // !NDEBUG

				minecount = std::roundf(this->amountSlider->Value(getMaxMines() - 2) + 1);

				DrawStringDecal({ 64,32 }, std::to_string(minecount));

				return true;
			}

			// SPIELFELD
			case(Display::game):
			{
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
						if (gameState == State::newgame)
						{
							byte tries = 0;
							while (hoveredSquare->isMine && (++tries) <= 255) {
								CreateField(this->minecount);
								hoveredSquare = this->field[mousePos.y][mousePos.x];
							}
#ifndef NDEBUG
							printf_s("Generated field after %hhu tries\n", tries);
#endif // !NDEBUG
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

				// Playing field
				for (auto row : this->field)
					for (auto square : row)
						DrawDecal({ (float)(square->position.x) * MS_FIELD_SIZE, (float)(square->position.y) * MS_FIELD_SIZE + MS_TOPBAR_SIZE }, square->getDecal(gameState == State::gameOver));
				
				// Mines counter
				olc::Decal** mineCounterDecals = Graphics::IntToDecals(this->minecount - this->flaggedSquares);
				for (int i = 0; i < 3; i++)
					DrawDecal({ (float)(this->ScreenWidth() - 4 - mineCounterDecals[0]->sprite->width * (3 - i)), 4.0f }, mineCounterDecals[i]);

				// Seconds counter
				olc::Decal** secondCounterDecals = Graphics::IntToDecals((int)passedSeconds);
				for (int i = 0; i < 3; i++)
					DrawDecal({ (float)(4.0f + secondCounterDecals[0]->sprite->width * i), 4.0f }, secondCounterDecals[i]);

				return true;
			}
			}
		}
	};
}

int main()
{
	minesweeper::Minesweeper mainWindow(50);

	if (mainWindow.Construct(minesweeper::MS_FIELD_SIZE * 16, minesweeper::MS_FIELD_SIZE * 16 + minesweeper::MS_TOPBAR_SIZE, minesweeper::MS_SCALE, minesweeper::MS_SCALE))
		mainWindow.Start();

	return 0;
}