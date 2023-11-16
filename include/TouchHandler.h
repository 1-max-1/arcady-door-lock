#ifndef _ILI9488_TouchHandler_
#define _ILI9488_TouchHandler_

#include <LovyanGFX.hpp>

/**
 * Handles everything to do with screen touches. Currently this is screen wakeup and button presses.
*/
class TouchHandler {
	private:
	//------------------------------------------------

	lgfx::LGFX_Device* display;
		
	short curTouchX = -1;
	short curTouchY = -1;
	int lastTouchCount = 0;
	int curTouchCount = 0;

	// The buttons work on a system that does not allow new button presses to be registered until a certain amount of time has passed.
	// This prevents the finger from going down and the program registering like a hundred presses (a new one every clock cycle) until the finger is released.
	// Even if the finger is down, buttons will not be "pressed" until the touch event is actually ready to be acted upon/consumed.
	bool touchEventReadyToConsume = false;
	unsigned long lastTouchReadyTime = 0;
	const byte PRESS_COOLDOWN = 150;       // Minimum time allowed between button presses
	
	// The esp32 will go into deepsleep and turn off the screen after a period of inactivity. Touching the screne will wake it up again.
	unsigned long timeOfLastTouch = 0;
	const unsigned int SCREEN_TIMEOUT = 30000;

	public:
	//------------------------------------------------

	TouchHandler(lgfx::LGFX_Device* display) {
		this->display = display;
	}

	/**
	 * Does the logic for handling touches, call this at the start of each loop.
	*/
	void onLoop() {
		// We have to go from no touch to touch, holding down on the screen will not repeat the button press
		lastTouchCount = curTouchCount;
		curTouchCount = display->getTouch(&curTouchX, &curTouchY);
		if (curTouchCount > 0 && lastTouchCount == 0 && millis() - lastTouchReadyTime > PRESS_COOLDOWN) {
			touchEventReadyToConsume = true;
			lastTouchReadyTime = millis();
		}
		else
			touchEventReadyToConsume = false;

		if (curTouchCount > 0)
			timeOfLastTouch = millis();
		if (millis() - timeOfLastTouch > SCREEN_TIMEOUT)
			esp_deep_sleep_start();
	}
	
	/**
	 * Returns true if the user has pressed the button with the given dimensions. x and y are the top left corner.
	*/
	bool isRectButtonPressed(int x, int y, int w, int h) {
		bool isWithinRect = curTouchX > x && curTouchX < x + w && curTouchY > y && curTouchY < y + h;
		return touchEventReadyToConsume && isWithinRect;
	}
};

#endif