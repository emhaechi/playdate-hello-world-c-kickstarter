//
//  main.c
//  {}{{__KICKSTART_PLAYDATE_GAME_NAME__}}{}
//
//  {}{{__KICKSTART_AUTHOR__}}{}
//
//  Minimal, empty C project that can be built.
//

#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"


static int update(void* userdata);

/** Playdate API runtime */
PlaydateAPI* pd = NULL;


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

		// use C-only Playdate API:
		// Note: If you set an update callback in the kEventInit handler, the system assumes the game is pure C and doesn't run any Lua code in the game
		pd->system->setUpdateCallback(update, pd);
	}
	else if (event == kEventTerminate) {
		// game shutdown tasks:
		
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

	// clear frame buffer before rendering anything this frame
	pd->graphics->clear(kColorBlack);

	// render FPS text (debugging only)
	pd->system->drawFPS(0,0);

	return 1;
}
