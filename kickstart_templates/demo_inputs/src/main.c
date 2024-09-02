//
//  main.c
//  {}{{__KICKSTART_PLAYDATE_GAME_NAME__}}{}
//
//  {}{{__KICKSTART_AUTHOR__}}{}
//
//  Simple project that displays a real-time input/controller display and 
//  is meant to demonstrate:
//
//  - handling all inputs: D-pad, A / B buttons, Crank, Accelerometer, System menu
//  - loading and using a custom font
//  - including additional C source/header files
//

#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "text_manager.h"


static int update(void* userdata);

/** Playdate API runtime */
PlaydateAPI* pd = NULL;

const char* fontPath = "assets/fonts/hello-world-c_2024a";
LCDFont* font = NULL;
int fontTracking = 0;
int textHeight = 0;
int textWidth = 0;

// The total number of divisions of an imaginary circle, one of which crank rotation will be mapped to as it is operated.
#define NUM_CRANK_DIV 8

float prevCrankAngle = 0.0f;
float crankAngle = 0.0f;
float crankChange = 0.0f;
int crankAmount = 0;
/** The arbitrary amount of accumulated crankAmount before any action is taken based on crank input. A way to debounce the crank input, the amount based on whatever you want. */
int crankThreshold = 15;
/** The raw amount of crank rotation by crank input before any mapping according to NUM_CRANK_DIV. */
int crankRotation = 0;
/** The current mapped crank division, mapped from crankRotation. Valid range of [0, NUM_CRANK_DIV) */
int crankRotationDiv = 0;
/** The lower (i.e. closer to -inf) angle of the current crank rotation division, that together with the high angle forms the div's circular arc. */
float crankRotationDivLowAngle = 0;
/** The higher (i.e. closer to +inf) arc angle of the current crank rotation division, that together with the low angle forms the div's circular arc. */
float crankRotationDivHighAngle = 0;

/** The accelerometer's current x-axis value. */
float acceleroX;
float acceleroY;
float acceleroZ;

/** The previous frame's button input state */
PDButtons btnsPrev;
/** The current frame's button input state */
PDButtons btnsCurr;
PDButtons btnsUpdateDown;
PDButtons btnsUpdateUp;

PDMenuItem* darkModeMenuItemCheckmark;
PDMenuItem* resetAcceleroMenuItemAction;
char* darkModeMenuItemLabel = NULL;
int darkMode = 1;
int fgColor = -1;
int bgColor = -1;

LCDBitmapDrawMode textDrawMode = 0;
LCDBitmapDrawMode tempDrawMode = 0;

/**
 * Callback for Playdate API system menu user interaction, invoked by system menu user interaction.
 */
static void systemMenuItemDarkModeCallback(void* userdata) {
	darkMode = pd->system->getMenuItemValue(darkModeMenuItemCheckmark);
}

static void systemMenuItemResetAcceleroCallback(void* userdata) {
	// TODO: accelero handling+
}

/**
 * Playdate API callback for handling PDSystemEvent events, invoking by Playdate on an event.
 *
 * "On Windows, your event handler must be exported, you do this by adding the attribute __declspec(dllexport) to the function."
 */
#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg) {
	(void)arg; // arg is currently only used for event = kEventKeyPressed

	if (event == kEventInit) {
		// game bootup tasks:
		
		const char* err;
		
		// load and init fonts
		font = pd->graphics->loadFont(fontPath, &err);
		if ( font == NULL ) {
			pd->system->error("%s:%i Error loading font, path=%s: %s", __FILE__, __LINE__, fontPath, err);
		}
		pd->graphics->setFont(font);
		fontTracking = pd->graphics->getTextTracking();
		textHeight = pd->graphics->getFontHeight(font);
		// textWidth = pd->graphics->getTextWidth(font, helloText, strlen(helloText), kASCIIEncoding, fontTracking);
		
		// init system menu
		darkModeMenuItemCheckmark = pd->system->addCheckmarkMenuItem(getDarkModeMenuItemLabel(), darkMode, systemMenuItemDarkModeCallback, NULL);
		// resetAcceleroMenuItemAction = pd->system->addMenuItem(getResetAcceleroMenuItemLabel(), systemMenuItemResetAcceleroCallback, NULL); // TODO: accelero handling+
		
		// init/enable accelerometer
		pd->system->setPeripheralsEnabled(kAccelerometer);

		// use C-only Playdate API:
		// Note: If you set an update callback in the kEventInit handler, the system assumes the game is pure C and doesn't run any Lua code in the game
		pd->system->setUpdateCallback(update, pd);
	}
	else if (event == kEventTerminate) {
		// game shutdown tasks:
		
		pd->system->setPeripheralsEnabled(kNone);
	}
	
	return 0;
}

/**
 * "Loop Update callback" for C-only Playdate API ("Replaces the default Lua run loop function with a custom update function"),
 * repeatedly invoked at system refresh rate.
 *
 * @return "a non-zero number to tell the system to update the display, or zero if update isn’t needed."
 */
static int update(void* userdata) {
	pd = userdata;
	
	// check dark mode state, adjust colors rendered as needed
	if (darkMode == 0) {
		fgColor = kColorBlack;
		bgColor = kColorWhite;
		textDrawMode = kDrawModeFillBlack;
	}
	else {
		fgColor = kColorWhite;
		bgColor = kColorBlack;
		textDrawMode = kDrawModeFillWhite;
	}
	
	// read button input, if any
	pd->system->getButtonState(&btnsCurr, &btnsUpdateDown, &btnsUpdateUp);
	
	// read any Crank input (if in use)
	if (pd->system->isCrankDocked() == 0) {
		prevCrankAngle = crankAngle;
		crankAngle = pd->system->getCrankAngle();
		crankChange = pd->system->getCrankChange();
		
		if (crankChange > 0) {
			// positive (forward/toward-screen) crank change!
			crankAmount += crankChange;
			
			if (crankAmount > crankThreshold) {
				crankAmount = 0;
				crankRotation += 15;
			}
		}
		else if (crankChange < 0) {
			// negative (backward/behind-screen) crank change!
			crankAmount += fabs(crankChange);
			
			if (fabs(crankAmount) > crankThreshold) {
				crankAmount = 0;
				crankRotation -= 15;
			}
		}
	}
	crankRotationDiv = (crankRotation % 360) / (360 / NUM_CRANK_DIV); // simplify rotation range down to 0 to 360, then map to the index range of [0, plus-minus NUM_CRANK_DIV)
	crankRotationDiv = (crankRotationDiv >= 0) ? crankRotationDiv : (NUM_CRANK_DIV + crankRotationDiv); // ensure/map within range of [0, NUM_CRANK_DIV)
	crankRotationDivHighAngle = crankRotationDiv * 45 + 22.5;
	crankRotationDivLowAngle = crankRotationDiv * 45 - 22.5;
	
	// read accelerometer
	pd->system->getAccelerometer(&acceleroX, &acceleroY, &acceleroZ);
	acceleroX = acceleroX > 0 ?
		(acceleroX * LCD_COLUMNS / 2 + LCD_COLUMNS / 2) :
		(LCD_COLUMNS / 2 - fabs(acceleroX) * LCD_COLUMNS / 2);
	acceleroY = acceleroY > 0 ?
		(acceleroY * LCD_COLUMNS / 2 + LCD_COLUMNS / 2) :
		(LCD_COLUMNS / 2 - fabs(acceleroY) * LCD_COLUMNS / 2);
	acceleroZ = acceleroZ > 0 ?
		(acceleroZ * LCD_COLUMNS / 2 + LCD_COLUMNS / 2) :
		(LCD_COLUMNS / 2 - fabs(acceleroZ) * LCD_COLUMNS / 2);
	
	
	// clear frame buffer before rendering anything this frame
	pd->graphics->clear(bgColor);
	
	// render labels
	tempDrawMode = pd->graphics->setDrawMode(textDrawMode);
	pd->graphics->drawText(getDpadLabelText(), strlen(getDpadLabelText()), kASCIIEncoding, 40, 5);
	pd->graphics->drawText(getAButtonLabelText(), strlen(getAButtonLabelText()), kASCIIEncoding, 220, 5);
	pd->graphics->drawText(getBButtonLabelText(), strlen(getBButtonLabelText()), kASCIIEncoding, 170, 5);
	pd->graphics->drawText(getCrankLabelText(), strlen(getCrankLabelText()), kASCIIEncoding, 325, 5);
	pd->graphics->drawText(getAcceleroLabelText(), strlen(getAcceleroLabelText()), kASCIIEncoding, 165, 150);
	pd->graphics->drawText(getAcceleroXLabelText(), strlen(getAcceleroXLabelText()), kASCIIEncoding, 195, 170);
	pd->graphics->drawText(getAcceleroYLabelText(), strlen(getAcceleroYLabelText()), kASCIIEncoding, 195, 190);
	pd->graphics->drawText(getAcceleroZLabelText(), strlen(getAcceleroZLabelText()), kASCIIEncoding, 195, 210);
	pd->graphics->setDrawMode(tempDrawMode); // restore prior draw mode
	
	// render d-pad
	if (kButtonLeft & btnsCurr) {
		pd->graphics->fillTriangle(
			30, 45,
			40, 35,
			40, 55,
			fgColor
		);
	}
	else {
		pd->graphics->drawLine(
			30, 45,
			40, 35,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			40, 35,
			40, 55,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			40, 55,
			30, 45,
			1,
			fgColor
		);
	}
	
	if (kButtonRight & btnsCurr) {
		pd->graphics->fillTriangle(
			80, 45,
			70, 35,
			70, 55,
			fgColor
		);
	}
	else {
		pd->graphics->drawLine(
			80, 45,
			70, 35,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			70, 35,
			70, 55,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			70, 55,
			80, 45,
			1,
			fgColor
		);
	}
	
	if (kButtonUp & btnsCurr) {
		pd->graphics->fillTriangle(
			55, 20,
			45, 30,
			65, 30,
			fgColor
		);
	}
	else {
		pd->graphics->drawLine(
			55, 20,
			45, 30,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			45, 30,
			65, 30,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			65, 30,
			55, 20,
			1,
			fgColor
		);
	}
	
	if (kButtonDown & btnsCurr) {
		pd->graphics->fillTriangle(
			55, 70,
			45, 60,
			65, 60,
			fgColor
		);
	}
	else {
		pd->graphics->drawLine(
			55, 70,
			45, 60,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			45, 60,
			65, 60,
			1,
			fgColor
		);
		pd->graphics->drawLine(
			65, 60,
			55, 70,
			1,
			fgColor
		);
	}
	
	// render a / b
	if (kButtonA & btnsCurr) {
		pd->graphics->fillEllipse(210, 30, 30, 30, 0, 0, fgColor);
	}
	else {
		pd->graphics->drawEllipse(210, 30, 30, 30, 1, 0, 0, fgColor);
	}
	
	if (kButtonB & btnsCurr) {
		pd->graphics->fillEllipse(160, 30, 30, 30, 0, 0, fgColor);
	}
	else {
		pd->graphics->drawEllipse(160, 30, 30, 30, 1, 0, 0, fgColor);
	}
	
	// render crank
	pd->graphics->drawEllipse(320, 25, 40, 40, 1, 0, 0, fgColor);
	pd->graphics->fillEllipse(320, 25, 40, 40, crankRotationDivLowAngle, crankRotationDivHighAngle, fgColor);
	
	// render accelero values
	pd->graphics->drawLine(0, 184, LCD_COLUMNS, 184, 1, fgColor);
	pd->graphics->fillEllipse((acceleroX > 0 ? (acceleroX < LCD_COLUMNS - 8 ? acceleroX : LCD_COLUMNS - 8) : 0), 180, 8, 8, 0, 0, fgColor);
	pd->graphics->drawLine(0, 204, LCD_COLUMNS, 204, 1, fgColor);
	pd->graphics->fillEllipse((acceleroY > 0 ? (acceleroY < LCD_COLUMNS - 8 ? acceleroY : LCD_COLUMNS - 8) : 0), 200, 8, 8, 0, 0, fgColor);
	pd->graphics->drawLine(0, 224, LCD_COLUMNS, 224, 1, fgColor);
	pd->graphics->fillEllipse((acceleroZ > 0 ? (acceleroZ < LCD_COLUMNS - 8 ? acceleroZ : LCD_COLUMNS - 8) : 0), 220, 8, 8, 0, 0, fgColor);
    
	// render FPS text (debugging only)
	pd->system->drawFPS(0,0);
	
	// store this frame's button state for next frame's reference (only at end of this frame)
	btnsPrev = btnsCurr;

	return 1;
}
