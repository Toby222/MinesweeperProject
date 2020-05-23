/*
	olcPGEX_Controls.h
	+-------------------------------------------------------------+
	|         OneLoneCoder Pixel Game Engine Extension            |
	|                       Controls                              |
	+-------------------------------------------------------------+

	What is this?
	~~~~~~~~~~~~~
	This is an extension to the olcPixelGameEngine, which provides
	controls, event handler and other things, that help to design a GUI
	using the olcPixelGameEngine.

	Example
	~~~~~~~

	// A basic RGB color picker

	#define OLC_PGE_APPLICATION
	#include "olcPixelGameEngine.h"

	#define OLC_PGEX_CONTROLS
	#include "olcPGEX_Controls.h"

	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	class ControlsTest : public olc::PixelGameEngine
	{
	public:
		ControlsTest()
		{
		}
	private:
		olc::ctrls::Slider red;
		olc::ctrls::Slider green;
		olc::ctrls::Slider blue;
	public:

		bool OnUserCreate() override
		{
			olc::ctrls::Initialize(this);

			olc::ctrls::Slider red(olc::vf2d(200, 200), 300, olc::ctrls::HORIZONTAL, olc::GREY, olc::GREEN);
			this->red = red;

			olc::ctrls::Slider green(olc::vf2d(200, 250), 300, olc::ctrls::HORIZONTAL, olc::GREY, olc::GREEN);
			this->green = green;

			olc::ctrls::Slider blue(olc::vf2d(200, 300), 300, olc::ctrls::HORIZONTAL, olc::GREY, olc::GREEN);
			this->blue = blue;

			return true;
		}

		bool OnUserUpdate(float fElapsedTime) override
		{
			Clear(olc::BLACK);

			this->red.Update();
			this->green.Update();
			this->blue.Update();

			FillRect(690, 190, 110, 120, olc::Pixel(r, g, b));
			DrawRect(690, 190, 110, 120, olc::WHITE);

			r = this->red.Value(255);
			g = this->green.Value(255);
			b = this->blue.Value(255);
			return true;
		}
	};

	int main()
	{
		ControlsTest demo;
		if (demo.Construct(1100, 500, 1, 1))
			demo.Start();
		return 0;
	}

	What's new?
	~~~~~~~~~~~
	  - Slider looks better
	  - Box is now sensitive to events
	  - You can draw the box as well if you want to
*/

#ifdef OLC_PGEX_CONTROLS
#undef OLC_PGEX_CONTROLS

#define MOUSE_POINT  olc::vf2d (pge->GetMouseX(), pge->GetMouseY())

namespace olc
{
	/* olcPixelGameEngine Controls extension functions & classes */
	namespace ctrls
	{
		olc::PixelGameEngine* pge;

		void Initialize(olc::PixelGameEngine* pixelGameEngine)
		{
			pge = pixelGameEngine;
		}

		class BaseComponent;
		class EventHandler
		{
		public:
			virtual void MouseClicked(olc::PixelGameEngine* pge, BaseComponent* control, olc::vf2d mouse) {}
			virtual void MouseReleased(olc::PixelGameEngine* pge, BaseComponent* control, olc::vf2d mouse) {}
			virtual void MouseHover(olc::PixelGameEngine* pge, BaseComponent* control, olc::vf2d mouse) {}
			virtual void ValueChanged(olc::PixelGameEngine* pge, BaseComponent* control, float newValue) {}

			/* Any event on the component has occured */
			virtual void AnyEvent(olc::PixelGameEngine* pge, BaseComponent* control) {}
		};

		class BaseComponent
		{
		public:
			std::vector<EventHandler*> eventHandlers;
			bool lockHandlers = false;
			bool lockUpdates = false;
			int x = 0;
			int y = 0;
			void LockHandlers()
			{
				lockHandlers = true;
			}
			void FreeHandlers()
			{
				lockHandlers = false;
			}
			bool HandlersLocked()
			{
				return lockHandlers;
			}
			void LockUpdates()
			{
				lockUpdates = true;
			}
			void FreeUpdates()
			{
				lockUpdates = false;
			}
			bool UpdatesLocked()
			{
				return lockUpdates;
			}
			float GetX()
			{
				return x;
			}

			float GetY()
			{
				return y;
			}

			virtual float GetWidth()
			{
				return 0.0f;
			}

			virtual float GetHeight()
			{
				return 0.0f;
			}
			virtual void UpdateSelf() {}
			void Update()
			{
				if (!lockUpdates)
					UpdateSelf();
			}
			void AddEventHandler(EventHandler* evh)
			{
				this->eventHandlers.push_back(evh);
			}
			void CallMouseClicked(BaseComponent* ctrl)
			{
				if (!lockHandlers)
					for (EventHandler* handler : eventHandlers)
					{
						handler->MouseClicked(pge, ctrl, MOUSE_POINT);
						handler->AnyEvent(pge, ctrl);
					}
			}
			void CallMouseReleased(BaseComponent* ctrl)
			{
				if (!lockHandlers)
					for (EventHandler* handler : eventHandlers)
					{
						handler->MouseReleased(pge, ctrl, MOUSE_POINT);
						handler->AnyEvent(pge, ctrl);
					}
			}
			void CallMouseHover(BaseComponent* ctrl)
			{
				if (!lockHandlers)
					for (EventHandler* handler : eventHandlers)
					{
						handler->MouseHover(pge, ctrl, MOUSE_POINT);
						handler->AnyEvent(pge, ctrl);
					}
			}
			void CallValueChanged(BaseComponent* ctrl, float newValue)
			{
				if (!lockHandlers)
					for (EventHandler* handler : eventHandlers)
					{
						handler->ValueChanged(pge, ctrl, newValue);
						handler->AnyEvent(pge, ctrl);
					}
			}
		};

		class Box : public BaseComponent
		{
		public:
			float width;
			float height;
			bool draw;
			olc::Pixel backgroundColor;
			Box(float x = 0.0f, float y = 0.0f, float width = 0.0f, float height = 0.0f, bool draw = false, olc::Pixel backgroundColor = olc::Pixel(225, 225, 225))
			{
				this->x = x;
				this->y = y;
				this->width = width;
				this->height = height;
				this->backgroundColor = backgroundColor;
			}
			float GetWidth() override { return width; }
			float GetHeight() override { return height; }
			bool Contains(float pointX, float pointY)
			{
				if ((int)pointX > (int)x && (int)pointX <= (int)x + (int)width && (int)pointY >= (int)y && (int)pointY <= (int)y + (int)height)
				{
					return true;
				}
				return false;
			}
			void UpdateSelf() override
			{
				pge->FillRect(x, y, width, height, backgroundColor);
				if (Contains(pge->GetMouseX(), pge->GetMouseY()))
				{
					CallMouseHover(this);
					if (pge->GetMouse(0).bPressed || pge->GetMouse(1).bPressed || pge->GetMouse(2).bPressed)
						CallMouseClicked(this);
					if (pge->GetMouse(0).bReleased || pge->GetMouse(1).bReleased || pge->GetMouse(2).bReleased)
						CallMouseReleased(this);
				}
			}
		};

		class Slider : public BaseComponent
		{
		public:
			float size;
			float headOffset;
			olc::Pixel backgroundColor;
			olc::Pixel foregroundColor;
			bool IsSelected = false;
			Slider(olc::vf2d position = olc::vf2d(0.0f, 0.0f), float size = 0.0f, olc::Pixel backgroundColor = olc::Pixel(88, 92, 92), olc::Pixel foregroundColor = olc::Pixel(67, 112, 181))
			{
				this->x = position.x;
				this->y = position.y;
				this->size = size;
				this->headOffset = 0;
				this->backgroundColor = backgroundColor;
				this->foregroundColor = foregroundColor;
			}
			float GetWidth() override
			{
				return size;
			}
			float GetHeight() override
			{
				return size;
			}
			void SetHeadOffset(float hoffset)
			{
				headOffset = hoffset;
			}
			float GetHeadOffset()
			{
				return headOffset;
			}
			float GetPercent()
			{
				return headOffset / (size / 100);
			}
			float Value(float maxv)
			{
				return (maxv / 100) * GetPercent();
			}
			void UpdateSelf() override
			{
				Box* boundingBox = new Box(x + headOffset - 5, y - 30 / 2, 10, 30);
				if (boundingBox->Contains(pge->GetMouseX(), pge->GetMouseY()))
				{
					CallMouseHover(this);
					if (pge->GetMouse(0).bPressed || pge->GetMouse(1).bPressed || pge->GetMouse(2).bPressed)
					{
						IsSelected = true;
						CallMouseClicked(this);
					}
					if (pge->GetMouse(0).bReleased || pge->GetMouse(1).bReleased || pge->GetMouse(2).bReleased)
					{
						CallMouseReleased(this);
					}
				}
				if (pge->GetMouse(0).bReleased || pge->GetMouse(1).bReleased || pge->GetMouse(2).bReleased)
				{
					IsSelected = false;
				}
				if (IsSelected)
				{
					int newOffset = pge->GetMouseX() - x;
					if (newOffset >= -1 && newOffset <= (int)size + 1)
					{
						headOffset = newOffset;
						CallValueChanged(this, headOffset);
					}
				}
				pge->FillRect(x, y, size, 5, backgroundColor);
				pge->FillRect(x + headOffset - 5, y - 30 / 2, 10, 30, foregroundColor);
				pge->FillRect(x, y, headOffset, 5, foregroundColor);
				delete boundingBox;
				return;
			}
		};
	}
}
#endif