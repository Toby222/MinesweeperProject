#define OLC_PGE_APPLICATION
#define OLC_PGEX_CONTROLS
// #define OLC_GFX_OPENGL10

#include "olcPixelGameEngine.h"
#include <vector>
#include <random>

namespace minesweeper {

	// Slider
	// Taken and modified from "olcPGEX_Controls.h"
	// fixing a few bugs and reducing unnecessary overhead for this project
	class Slider
	{
	public:
		olc::PixelGameEngine* pge;

		int x;
		int y;

		float width;
		float height;
		float headOffset;
		olc::Pixel backgroundColor;
		olc::Pixel foregroundColor;
		bool IsSelected = false;
		Slider(olc::PixelGameEngine* pixelGameEngine, olc::vf2d position = { 0.0f, 0.0f }, float pWidth = 0.0f, float pHeight = 30.0f, olc::Pixel pBackgroundColor = olc::VERY_DARK_GREY, olc::Pixel pForegroundColor = olc::GREY)
		{
			pge = pixelGameEngine;

			x = position.x;
			y = position.y;
			width = pWidth;
			height = pHeight;
			headOffset = 0;
			backgroundColor = pBackgroundColor;
			foregroundColor = pForegroundColor;
		}
		float GetWidth()
		{
			return width;
		}
		float GetHeight()
		{
			return height;
		}
		void SetHeadOffset(float hOffset)
		{
			headOffset = hOffset;
		}
		float GetHeadOffset()
		{
			return headOffset;
		}
		float GetPercent()
		{
			return headOffset / width * 100;
		}
		float Value(float maxv)
		{
			return maxv * GetPercent() / 100;
		}
		void Update()
		{
			bool inBounds = pge->GetMouseX() >= x && pge->GetMouseX() <= x + headOffset + 5 &&
							pge->GetMouseY() >= y && pge->GetMouseY() <= y + height;

			if (inBounds && (pge->GetMouse(0).bPressed || pge->GetMouse(1).bPressed || pge->GetMouse(2).bPressed))
			{
				IsSelected = true;
			}
			if (pge->GetMouse(0).bReleased || pge->GetMouse(1).bReleased || pge->GetMouse(2).bReleased)
			{
				IsSelected = false;
			}

			if (IsSelected)
			{
				int newOffset = pge->GetMouseX() - x;
				if (newOffset >= 0 && newOffset <= (int)width)
				{
					headOffset = newOffset;
				}
			}

			// Empty bar
			pge->FillRect(x, y, width, 5, backgroundColor);

			// Head
			pge->FillRect(x + headOffset - 5, y - height / 2, 10, height, foregroundColor);

			// Filled bar
			pge->FillRect(x, y, headOffset, 5, foregroundColor);
		}
	};

	constexpr auto MS_FIELD_SIZE = 16;
	constexpr auto MS_TOPBAR_SIZE = 31;
	constexpr auto MS_SCALE = 2;

	// For finding Square neighbors
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
		// Load a Sprite from a filename, optionally with a given expected size
		olc::Sprite* LoadSprite(std::string filename, olc::vi2d expectedSize = { -1, -1 }) {
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

		// Turn an integer into an array of 3 Decals
		// Accurate for numbers -99 to 999, and clamped to those values
		static olc::Decal** IntToDecals(int value) {
			olc::Decal** decals = new olc::Decal * [3]{ Graphics::digitDecals[9], Graphics::digitDecals[9], Graphics::digitDecals[9] };
			if (0 <= value && value < 999)
			{
				decals[0] = digitDecals[value / 100 % 10]; // 0-9
				decals[1] = digitDecals[value / 10 % 10]; // 0-9
				decals[2] = digitDecals[value % 10]; // 0-9
			}
			if (-99 < value && value < 0) {
				decals[0] = digitDecals[10]; // "-"
				decals[1] = digitDecals[-value / 10 % 10]; // 0-9
				decals[2] = digitDecals[-value % 10]; // 0-9
			}
			if (value <= -99) {
				decals[0] = digitDecals[10]; // "-"
			}
			return decals;
		}

		// Constructor
		// Goes through all sprite names in a rather ugly, manual way
		// Stores them in the sprite/decal arrays accordingly
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
			// Critical error while loading Sprites
			// Are the files there?
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
	// Assign memory for the static variables
	// Sprites are not actually used at the moment, and only stored so that Decals work correctly
	olc::Sprite* Graphics::fieldSprites[9], * Graphics::digitSprites[11], * Graphics::closedSprite, * Graphics::flaggedSprite, * Graphics::revealedmineSprite, * Graphics::falsemineSprite, * Graphics::clickedmineSprite;
	olc::Decal* Graphics::fieldDecals[9], * Graphics::digitDecals[11], * Graphics::closedDecal, * Graphics::flaggedDecal, * Graphics::revealedmineDecal, * Graphics::falsemineDecal, * Graphics::clickedmineDecal;

	// Square struct
	struct Square {
		enum class State {
			open,
			closed,
			flagged,
			pressed
		};

		// Constructor
		Square(int x, int y) : position({ x,y }) {
			isMine = false;
			state = State::closed;
			value = 0;
		}

		// Field coordinates
		const olc::vi2d position;
		// Flag if this Square is a mine
		bool isMine;
		// Neigboring mines
		int value;
		// Current state
		State state;

		// Neighboring squares
		Square* neighbors[8] = {};

		// Set own neighbors based on own position inside given field
		void setNeighbors(std::vector<std::vector<Square*>>* field) {
			for (int i = 0; i < 8; i++) {
				auto neighborPos = position + offsets[i];
				if (neighborPos.y >= field->size() || neighborPos.y < 0 ||
					neighborPos.x >= (*field)[0].size() || neighborPos.x < 0)
					neighbors[i] = nullptr;
				else {
					neighbors[i] = (*field)[neighborPos.y][neighborPos.x];
					if (neighbors[i]->isMine)
						value++;
				}
			}
		}

		// Get the current Decal of this Square
		// dependant on if the game is over
		/*
		gameOver:
			mine:
				flagged: flagged Square
				pressed: prints a non-critical error to std::cerr (All Squares should be released at gameOver)
				closed: uncovered Mine
				open: clicked Mine
			not-mine:
				flagged: falsely flagged Square
				pressed: prints a non-critical error to std::cerr (All Squares should be released at gameOver)
				open | closed: appropriately numbered Square

		not gameOver:
			closed: closed Square
			flagged: flagged Square
			open: appropriately numbered Square
			pressed: 0-numbered Square (empty Square)
		*/
		olc::Decal* getDecal(bool gameOver = false) {
			if (gameOver) {
				if (isMine) {
					switch (state) {
						case(State::flagged):
							return Graphics::flaggedDecal;
						case(State::pressed):
							std::cerr << "Pressed square after gameOver";
						case(State::closed):
							return Graphics::revealedmineDecal;
						case(State::open):
							return Graphics::clickedmineDecal;
						default:
							throw std::exception("Invalid Square state");
					}
				}
				else {
					switch (state) {
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
			throw std::exception("ERROR: GetDecal() -> This should never be reached");
		};

		// Set the state to pressed
		void TryPress() {
			if (state == State::closed)
				state = State::pressed;
		}

		// Set the state to closed
		void TryRelease() {
			if (state == State::pressed)
				state = State::closed;
		}

		// Tries to open this Square
		// Returns amount of opened squares
		// -1 if this Square has a mine.
		//  0 if this Square was already opened
		// >1 if flood-open was triggered
		int TryOpen() {
			// Only works on closed or pressed Squares, as opened ones can't be opened again
			if (state == State::closed ||
				state == State::pressed) {
				state = State::open;
				if (isMine)
					return -1;
				// If value is 0, open neighbors recursively
				if (value == 0) {
					int result = 1;
					for (auto neighbor : neighbors) {
						if (neighbor == nullptr)
							continue;
						result += neighbor->TryOpen();
					}
					return result;
				}
				else
					return 1;
			}
			return 0;
		}

		// Try to flag this Square
		// Returns difference in unflagged Squares
		int TryFlag() {
			switch (state) {
				case(State::closed):
					state = State::flagged;
					return +1;
				case(State::flagged):
					state = State::closed;
					return -1;
				case(State::pressed):
				case(State::open):
					return +0;
			}
		}
	};

	class Minesweeper : public olc::PixelGameEngine {
	private:
		// The playing field
		std::vector<std::vector<Square*>> field;
		// Currently selected Square
		Square* hoveredSquare;
		// Previously selected Square
		Square* prevHoveredSquare;
		// Time in the current game
		float passedSeconds;

		// Create a new Slider slightly below the top bar
		// Y = MS_TOPBAR_SIZE + (slider.GetHeight() / 2) + (<height of the text label above it>) + 5 // as a visual buffer/whitespace
		// X = <half the width of the Slider's head
		// Width = ScreenWidth() - <width of the Slider's head>
		Slider* amountSlider = new Slider(this, { 5, MS_TOPBAR_SIZE + 10 + 8 + 5 }, ScreenWidth() - 10, 20, olc::VERY_DARK_GREY, olc::GREY);

		// Current game state
		enum class State {
			newgame,
			playing,
			gameOver
		} gameState = State::newgame;

		// Currently displayed screen
		enum class Display {
			help,
			config,
			game
		} display = Display::help;

		int minecount; // Amount of mines
		int flaggedSquares; // Amount of flags
		int openedSquares; // Amount of opened Squares

	public:
		// Constructor
		Minesweeper(int pMinecount = 35)
		{
			minecount = pMinecount;
			sAppName = "Minesweeper";
		}

		// Total amount of Squares
		int getFieldArea() {
			return field.size() * field[0].size();
		}

		// Called once at startup
		bool OnUserCreate() override
		{
			// Initialize the Graphics store to load in the Sprites
			Graphics::Graphics();

			// Check if the Values are set correctly
			// So that MS_FIELD_SIZE could be modified
			if ((MS_FIELD_SIZE & MS_FIELD_SIZE >> 1) != 0)
				throw std::exception("Invalid field size. Has to be power of 2.");

			// To check if the values passed to Minesweeper.Construct() were correct
			if (ScreenWidth() % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen width.");
			if ((ScreenHeight() - MS_TOPBAR_SIZE) % MS_FIELD_SIZE != 0)
				throw std::exception("Invalid screen height.");

			// Generate an empty field, to initialize the field vector
			CreateField(0);

			// Set the config slider to the default value
			// Offset = (current_value-min_value) / (max_value-min_value) * width
			amountSlider->SetHeadOffset(((float)minecount - 1.0f) / ((float)getFieldArea() - 1.0f) * amountSlider->GetWidth());

			// Signal that creation was successful
			return true;
		}

		void CreateField(int pMinecount = -1) {
			// Initialize a new game
			gameState = State::newgame;
			passedSeconds = 0;
			flaggedSquares = 0;
			openedSquares = 0;
			prevHoveredSquare = nullptr;
			hoveredSquare = nullptr;

			// Delete all existing current Squares
			for (auto row : field)
				for (auto square : row)
					delete square;

			// Delete all existing rows
			field.clear();


			// Generate as many new rows as fit on the screen
			for (int y = 0; y < (ScreenHeight() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE; y++) {
				field.push_back(std::vector<Square*>());
				// Generate as many new Squares for the row as fit on the screen
				for (int x = 0; x < ScreenWidth() / MS_FIELD_SIZE; x++) {
					field[y].push_back(new Square(x, y));
				}
			}

			// If minecount parameter is negative,
			// set local variable "minecount" to object variable
			// (Because default values for parameters have to be constant)
			if (pMinecount <= -1)
				pMinecount = minecount;

			// Error if you try to generate a field with more mines than would fit
			if (pMinecount >= getFieldArea())
				throw std::exception("Invalid mine count for field size.");

			// Distributions for random (x, y) coordinates inside the field
			std::uniform_int_distribution<> randomX(0, (int)field[0].size() - 1);
			std::uniform_int_distribution<> randomY(0, (int)field.size() - 1);

			// Generator for the random values
			std::random_device rd;
			std::default_random_engine gen(rd());

			// Place as many mines as given in minecount
			// If a field is already a mine, ignore it
			for (int i = 0; i < pMinecount;) {
				auto newMine = field[randomY(gen)][randomX(gen)];
				if (!newMine->isMine) {
					i++;
					newMine->isMine = true;
				}
			}

			// Go through all the Squares and set their value
			for (auto row : field)
				for (auto square : row)
					square->setNeighbors(&field);
		}

		// Called once per frame
		// fElapesTime => time in ms since the last frame
		// return value:
		// true => continue execution
		// false => end execution
		bool OnUserUpdate(float fElapsedTime) override
		{
			// Blank the screen to background color
			Clear(olc::DARK_GREY);

			// Handle out-of-focus window, as Decals won't stay on-screen
			if (!IsFocused()) {
				DrawString({ 8,8 }, "GAME PAUSED!");
				return true;
			}

			// Add seconds to the timer
			if (fElapsedTime > 0 && gameState == State::playing)
				passedSeconds += fElapsedTime;

			// If ESC is pressed, end the game
			if (GetKey(olc::Key::ESCAPE).bPressed)
				return false;

			// If F5 is pressed, create a new game
			if (GetKey(olc::Key::F5).bPressed && fElapsedTime > 0) {
				// Change window to show game field
				display = Display::game;
				CreateField();
			}

			// Update hover values
			prevHoveredSquare = hoveredSquare;
			hoveredSquare = nullptr;

			// If F2 is pressed, either open config
			// or create new game if already open
			if (GetKey(olc::Key::F2).bPressed)
			{
				if (display == Display::config) {
					if (fElapsedTime > 0)
						CreateField();
					display = Display::game;
				}
				else {
					display = Display::config;
				}
			}

			// If F1 was pressed, open help
			if (GetKey(olc::Key::F1).bPressed) {
				display = Display::help;
			}

			// Select what to render
			switch (display)
			{
				// Help screen (F1)
				case(Display::help):
					// Render Help string
					DrawStringDecal({ 8,8 }, "Keybinds\n------------------------------\nESC: Exit Game\nF1: Help\nF2: Settings\nF5: New Game\n\n\nControls\n------------------------------\nLMB: Reveal Square\nRMB: Flag Square\nMMB: Reveal adjacent Squares\n\n\nGoal\n------------------------------\nThe goal is to clear the\nentire field leaving only\nmines.");

#ifndef NDEBUG
					// In Debug-builds, render TODO list (incomplete, not updated)
					//                       vv amount of newlines in previous string
					DrawStringDecal({ 8, 8 * 23 }, "TODO?\n------------------------------\nSound\nAlternate graphics\nSpritesheet");
#endif // !NDEBUG

					// Render attribution 4 lines from the bottom
					DrawStringDecal({ 8, (float)ScreenHeight() - 8 * (3 + 1) }, "Created by Tobias Berger\nusing the\nOneLoneCoder PixelGameEngine");
					return true;

					// Config screen (F2)
				case(Display::config):
					// Render Slider label
					DrawStringDecal({ 8,8 }, "Settings\n------------------------------\n\nMines:");
					// Update Slider
					amountSlider->Update();
					// Change minecount for new field
					minecount = std::roundf(amountSlider->Value(getFieldArea() - 2) + 1);
					// Draw the current minecount after the Slider label
					DrawStringDecal({ 64,32 }, std::to_string(minecount));
					return true;

					// Game screen (F5)
				case(Display::game):
					// Get mouse position as field indeces
					auto mousePos = olc::vi2d(GetMouseX(), GetMouseY() - MS_TOPBAR_SIZE) / MS_FIELD_SIZE;

					// Un-press any Square that was pressed in previous frame
					// (If mouse position didn't change, they'll be re-pressed again)
					if (!(gameState == State::gameOver || prevHoveredSquare == nullptr)) {
						prevHoveredSquare->TryRelease();
						for (auto neighbor : prevHoveredSquare->neighbors)
							if (neighbor != nullptr)
								neighbor->TryRelease();
					}

					// If the mouse is inside the field
					if (mousePos.y >= 0 && mousePos.y < field.size() &&
						mousePos.x >= 0 && mousePos.x < field[0].size() &&
						GetMouseY() > MS_TOPBAR_SIZE) {
						// Set the currently hovered Square to the one under the mouse
						hoveredSquare = field[mousePos.y][mousePos.x];

						// If you haven't lost yet
						if (gameState != State::gameOver) {
							// If RMB (MB1) was clicked, flag the hovered Square
							if (GetMouse(1).bPressed) {
								gameState = State::playing;
								// Adjust count of flagged Squares accordingly
								flaggedSquares += hoveredSquare->TryFlag();
							}

							// If MMB (MB2) is being held, press all neighboring Squares
							if (GetMouse(2).bHeld) {
								for (auto neighbor : hoveredSquare->neighbors)
									if (neighbor != nullptr)
										neighbor->TryPress();
							}
							// If MMB (MB2) was released, and the hovered Square is already opened
							else if (GetMouse(2).bReleased && hoveredSquare->state == Square::State::open) {
								// Count the neighboring flags
								int flaggedNeighbors = 0;
								for (auto neighbor : hoveredSquare->neighbors)
									if (neighbor != nullptr && neighbor->state == Square::State::flagged)
										flaggedNeighbors++;

								// If you have as many flags as there are adjacent mines
								// Open all neighboring cells
								// Lose the game if any of them were mines
								if (flaggedNeighbors == hoveredSquare->value) {
									for (auto neighbor : hoveredSquare->neighbors)
									{
										if (neighbor != nullptr) {
											int opened = neighbor->TryOpen();
											openedSquares += opened;
											if (opened == -1)
												gameState = State::gameOver;
										}
									}
								}
							}

							// If LMB (MB1) was released, 
							if (GetMouse(0).bReleased) {
								// start the new game, if not started already
								if (gameState == State::newgame)
								{
									// Try a maximum of 255 times to generate a field
									// where the hovered square is safe
									// (High mine counts might take too long)
									byte tries = 0;
									while (hoveredSquare->isMine && (++tries) <= 255) {
										CreateField();
										hoveredSquare = field[mousePos.y][mousePos.x];
									}
#ifndef NDEBUG
									// Debug: Show how many tries it took
									printf_s("Generated field after %hhu tries\n", tries);
#endif // !NDEBUG
								}
								gameState = State::playing;
								// Count how many squares were opened
								int opened = hoveredSquare->TryOpen();
								openedSquares += opened;
								// If the field you opened was a mine, lose
								if (opened == -1) {
									gameState = State::gameOver;
									printf_s("You lost\n");
								}
							}
							// If LMB (MB0) or MMB (MB2) are being held, press the hovered square
							else if (GetMouse(0).bHeld || GetMouse(2).bHeld)
								hoveredSquare->TryPress();
						}
						// If all remaining closed Squares are mines, win
						if (getFieldArea() - this->openedSquares == this->minecount) {
							gameState = State::gameOver;
							printf_s("You won!\n");
						}
					}
					// Render:
					// Playing field
					for (auto row : field)
						for (auto square : row)
							DrawDecal({ (float)(square->position.x) * MS_FIELD_SIZE, (float)(square->position.y) * MS_FIELD_SIZE + MS_TOPBAR_SIZE },
									  square->getDecal(gameState == State::gameOver));

					// 3-digit mine counter
					olc::Decal** mineCounterDecals = Graphics::IntToDecals(this->minecount - this->flaggedSquares);
					for (int i = 0; i < 3; i++)
						DrawDecal({ (float)(this->ScreenWidth() - 4 - mineCounterDecals[0]->sprite->width * (3 - i)), 4.0f }, mineCounterDecals[i]);

					// 3-digit second counter
					olc::Decal** secondCounterDecals = Graphics::IntToDecals((int)passedSeconds);
					for (int i = 0; i < 3; i++)
						DrawDecal({ (float)(4.0f + secondCounterDecals[0]->sprite->width * i), 4.0f }, secondCounterDecals[i]);

					return true;
			}
		}
	};
}

// Main entry function
int main()
{
	// Create a new minesweeper::Minesweeper instance
	// with 50 mines
	minesweeper::Minesweeper mainWindow(50);

	// Try to create a Window that's as wide as 16 Squares
	// and as tall as 16 Squares plus the gray top bar
	// Scale it up by some factor to make it more readable
	if (mainWindow.Construct(minesweeper::MS_FIELD_SIZE * 16, minesweeper::MS_FIELD_SIZE * 16 + minesweeper::MS_TOPBAR_SIZE, minesweeper::MS_SCALE, minesweeper::MS_SCALE))
		// Open the window
		// Calls mainWindow.OnUserCreate() when finished
		mainWindow.Start();

	return 0;
}