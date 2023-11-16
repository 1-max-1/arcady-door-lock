#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Servo.h>

#include "TouchHandler.h"
#include "LGFX_SPI_ILI9488_DoorLockScreen.h"

#define LOCKED_SENSOR_PIN 15
#define UNLOCKED_SENSOR_PIN 33
#define LEVER_SERVO_PIN 16
#define GEAR_SERVO_PIN 17
#define LCD_BACKLIGHT_PIN 4

#define PASSWORD "BROWN"

LGFX_SPI_ILI9488_DoorLockScreen display;
TouchHandler touchHandler(&display);

// The physical lock may be left in a halfway state where it is not touching either of the sensors. In this case we do not know the state of the lock.
enum class LockState { LOCKED, UNLOCKED, UNKNOWN };
enum class ProgramState { MAIN_MENU, PASSCODE, UNLOCKING, LOCKING };
LockState lockState = LockState::UNKNOWN;
ProgramState programState = ProgramState::MAIN_MENU;

// All passcode buttons are 80x80 squares. Coordinates start in top left corner.
const char passcodeOptions[] = { 'R', 'N', 'O', 'W', 'E', 'B' };
const uint16_t passcodeButtonX[] = { 8, 200, 392, 8, 200, 392 };
const uint16_t passcodeButtonY[] = { 119, 119, 119, 225, 225, 225 };

char currentInput[6];
uint8_t currentInputCount = 0;

// Redrawing the screen is slow, stop flickering by only redrawing the screen if the state of something changes - use this to keep track
bool mainMenuIsDirty = true;

Servo leverServo;
Servo gearServo;

LockState readLockState() {
	if (digitalRead(LOCKED_SENSOR_PIN) == HIGH)
		return LockState::LOCKED;
	if (digitalRead(UNLOCKED_SENSOR_PIN) == HIGH)
		return LockState::UNLOCKED;
	return LockState::UNKNOWN;
}

void operateLock(bool unlockDoor) {
	int gearServoPos = unlockDoor ? 0 : 180;
	int cycles = 0; // Due to the size of the gears it can take up to 2 full cycles to lock the door. Stop early if a sensor is triggered.
	while (cycles < 2 &&
	((unlockDoor && readLockState() != LockState::UNLOCKED) ||
	(!unlockDoor && readLockState() != LockState::LOCKED))) {
		gearServo.write(180 - gearServoPos);
		delay(250);
		leverServo.write(45);
		delay(250);
		gearServo.write(gearServoPos);
		delay(250);
		leverServo.write(180);
		delay(250);
		cycles++;
	}
}

void drawUI() {
	display.fillScreen(TFT_BLACK);

	switch (programState) {
		case ProgramState::MAIN_MENU:
		{
			int buttonColor = lockState == LockState::UNKNOWN ? TFT_SKYBLUE : TFT_GREEN;
			display.fillRoundRect(115, 110, 250, 100, 10, buttonColor);
			// Don't want to be locked out, so if the lock state is unknown we need to add another option to unlock the door.
			if (lockState == LockState::UNKNOWN) {
				display.fillRoundRect(115, 225, 250, 70, 10, buttonColor);
				display.drawString("Unlock", 240, 270);
			}
			display.drawString(lockState == LockState::LOCKED ? "Unlock" : "Lock", 240, 160);
			break;
		}
		case ProgramState::PASSCODE:
			display.setColor(TFT_BROWN);
			for (int i = 0; i < 5; i++) {
				display.drawFastHLine(96 * i + 8, 100, 80);
				display.drawFastHLine(96 * i + 8, 101, 80);
			}

			for (int i = 0; i < sizeof(passcodeOptions); i++) {
				display.fillRoundRect(passcodeButtonX[i], passcodeButtonY[i], 80, 80, 10, TFT_BROWN);
				display.drawChar(passcodeOptions[i], passcodeButtonX[i] + 20, passcodeButtonY[i] + 20);
			}

			break;
		
		case ProgramState::UNLOCKING:
		case ProgramState::LOCKING:
		{
			display.setTextColor(TFT_MAGENTA);
			const char* const text = programState == ProgramState::UNLOCKING ? "Unlocking..." : "Locking...";
			display.drawString(text, 240, 160);
			display.setTextColor(TFT_WHITE);
			break;
		}
	}
}

void setup() {
	Serial.begin(115200);

	esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, 0); // Attached to interrupt pin on touchscreen

  	display.init();
	display.setRotation(1); // Landscape mode, pins to the right
	uint16_t touchCalibrationData[8] = { 262, 3897, 289, 245, 3893, 3955, 3844, 315 };
	display.setTouchCalibrate(touchCalibrationData); // Calibration data was obtained from display.calibrateTouch()

	display.setTextColor(TFT_WHITE);
	display.setTextSize(5);
	display.setTextDatum(textdatum_t::middle_center);

	pinMode(LOCKED_SENSOR_PIN, INPUT_PULLDOWN);
	pinMode(UNLOCKED_SENSOR_PIN, INPUT_PULLDOWN);
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	digitalWrite(LCD_BACKLIGHT_PIN, HIGH);

	leverServo.attach(LEVER_SERVO_PIN);
	leverServo.write(180); // Disengage gear, sometimes it stops in a weird middle position
	gearServo.attach(GEAR_SERVO_PIN);

	drawUI();
}

void loop() {
	touchHandler.onLoop();

	switch (programState) {
		case ProgramState::MAIN_MENU:
			if (touchHandler.isRectButtonPressed(115, 110, 250, 100)) {
				programState = lockState == LockState::LOCKED ? ProgramState::PASSCODE : ProgramState::LOCKING;
				drawUI();
			}
			// If the lock state is unknown there is a second button for unlocking
			else if (lockState == LockState::UNKNOWN && touchHandler.isRectButtonPressed(115, 225, 250, 100)) {
				programState = ProgramState::PASSCODE;
				drawUI();
			}
			// If no buttons are pressed then we just draw the menu as normal. Only redraw if the state has changed.
			else {
				LockState prevLockState = lockState;
				lockState = readLockState();
				mainMenuIsDirty = mainMenuIsDirty || prevLockState != lockState;
				if (mainMenuIsDirty)
					drawUI();
				mainMenuIsDirty = false;
			}

			break;
		
		case ProgramState::UNLOCKING:
		case ProgramState::LOCKING:
			operateLock(programState == ProgramState::UNLOCKING);
			programState = ProgramState::MAIN_MENU;
			mainMenuIsDirty = true;
			break;
		
		case ProgramState::PASSCODE:
			for (int i = 0; i < sizeof(passcodeOptions); i++) {
				// If this button isn't being pressed then ignore it
				if (!touchHandler.isRectButtonPressed(passcodeButtonX[i], passcodeButtonY[i], 80, 80)) continue;

				display.drawChar(passcodeOptions[i], 96 * currentInputCount + 8 + 20, 20);
				currentInput[currentInputCount] = passcodeOptions[i];
				currentInputCount++;

				// True if password attempt has been fully entered. Save one slot in the currentInput array for the null terminator.
				if (currentInputCount == sizeof(currentInput) - 1) {
					if (strcmp(currentInput, PASSWORD) == 0) {
						programState = ProgramState::UNLOCKING;
						drawUI();
					}
					else
						display.fillRect(0, 0, 480, 95, TFT_BLACK); // Don't need to redraw the whole screen, just the area where the text is.
					memset(currentInput, 0, sizeof(currentInput));
					currentInputCount = 0;
				}
			}
			break;
	}
}