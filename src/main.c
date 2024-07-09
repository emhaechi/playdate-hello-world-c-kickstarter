//
//  main.c
//  Extension
//
//  Created by Dave Hayden on 7/30/14.
//  Copyright (c) 2014 Panic, Inc. All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  playdate-hello-world-c-kickstarter/main.c - emhaechi - 2024-07-08
//
//  Modified and extended to demonstrate:
//
//  - handling inputs: D-pad, A / B buttons, Crank, System menu
//  - loading and using a custom font
//  - loading and rendering images
//  - loading and playing music/sound
//  - including additional C source/header files
//
//  This project renders stills of our Blue Planet that revolves based on input while 
//  Apollo beeps (quindar tones, also based on input) and a simple melody play.
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
/** in so many words: "Hello World!" */
char* helloText = NULL;

const char* musicFilePath = "assets/music/arp+surf=earth.mp3";
FilePlayer* filePlayer = NULL;

#define NUM_SOUND_PATHS 3
const char* soundPaths[NUM_SOUND_PATHS] = {
	"assets/sfx/nasa_apollo-11-moonwalk-restored_YT-S9HdPi9Ikhk_beep-1",
	"assets/sfx/nasa_apollo-11-moonwalk-restored_YT-S9HdPi9Ikhk_beep-2",
	"assets/sfx/nasa_apollo-11-moonwalk-restored_YT-S9HdPi9Ikhk_beep-3"
};
typedef struct {
	int id;
	AudioSample* sample;
} SoundInfo;
SoundInfo soundInfos[NUM_SOUND_PATHS] = {
	{ 1, NULL },
	{ 2, NULL },
	{ 3, NULL }
};
SamplePlayer* samplePlayer = NULL;

/** The current index into soundInfos. Valid range of [0, NUM_SOUND_PATHS) */
int soundRoundRobinIndex = 0;

#define NUM_BITMAP_PATHS 8
const char* bitmapPaths[NUM_BITMAP_PATHS] = {
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-01_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-02_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-03_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-04_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-05_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-06_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-07_1-bit",
	"assets/textures/nasa_the-blue-marble_ls-oc-sic_20020208_frame-08_1-bit"
};
typedef struct {
	int id;
	PDRect rect;
	LCDBitmap* bitmap;
} SpriteInfo;
SpriteInfo spriteInfos[NUM_BITMAP_PATHS] = {
	{ 1, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 2, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 3, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 4, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 5, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 6, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 7, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL },
	{ 8, { 0.0f, 0.0f, 400.0f, 240.0f }, NULL }
};
SpriteInfo* spriteInfoCurr;
SpriteInfo* spriteInfoPrev;
SpriteInfo* spriteInfoTemp;
/** The rendered sprite */
LCDSprite* sprite = NULL;

/**
 * The screen position of helloText's origin.
 *
 * (values take after css margin value arg indices; e.g. 'top' arg is first in css so here is 0):
 *
 * - top: 0,
 * - right: 1,
 * - bottom: 2,
 * - left: 3
 */
int textPosition = 2;
/** helloText's origin's x coordinate on screen */
int textX = 0;
/** helloText's origin's y coordinate on screen */
int textY = 0;

float prevCrankAngle = 0.0f;
float crankAngle = 0.0f;
float crankChange = 0.0f;
int crankAmount = 0;
/** The arbitrary amount of accumulated crankAmount before any action is taken based on crank input. A way to debounce the crank input, the amount based on whatever you want. */
int crankThreshold = 15;
/** The raw amount of image rotation by crank input before any mapping to a sprite. */
int imageRotation = 0;
/** The current index into spriteInfos, mapped from imageRotation to a sprite. Valid range of [0, NUM_BITMAP_PATHS) */
int spriteIndexImageRotation = 0;

/** The previous frame's button input state */
PDButtons btnsPrev;
/** The current frame's button input state */
PDButtons btnsCurr;
PDButtons btnsUpdateDown;
PDButtons btnsUpdateUp;

/** If 1, helloText will be rendered; otherwise, if 0 then will not be. */
int textShows = 1;
PDMenuItem* showTextMenuItemCheckmark;
char* showTextMenuItemLabel = NULL;


/**
 * Callback for Playdate API system menu user interaction, invoked by system menu user interaction.
 */
static void systemMenuItemCallback(void* userdata) {
	textShows = pd->system->getMenuItemValue(showTextMenuItemCheckmark);
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
		
		// init text
		helloText = getHelloText();
		showTextMenuItemLabel = getShowTextMenuItemLabel();
		
		const char* err;
		
		// load and init fonts
		font = pd->graphics->loadFont(fontPath, &err);
		if ( font == NULL ) {
			pd->system->error("%s:%i Error loading font, path=%s: %s", __FILE__, __LINE__, fontPath, err);
		}
		pd->graphics->setFont(font);
		fontTracking = pd->graphics->getTextTracking();
		textHeight = pd->graphics->getFontHeight(font);
		textWidth = pd->graphics->getTextWidth(font, helloText, strlen(helloText), kASCIIEncoding, fontTracking);
		
		// load and init music
		filePlayer = pd->sound->fileplayer->newPlayer();
		int musicFound = pd->sound->fileplayer->loadIntoPlayer(filePlayer, musicFilePath);
		if (musicFound == 0) {
			pd->system->error("%s:%i Error loading music, path=%s", __FILE__, __LINE__, musicFilePath);
		}
		pd->sound->fileplayer->setBufferLength(filePlayer, 0.5f);
		
		pd->sound->channel->addSource(
			pd->sound->getDefaultChannel(),
			(SoundSource*)filePlayer
		);
		
		// load and init sounds
		samplePlayer = pd->sound->sampleplayer->newPlayer();
		for (int i = 0; i < NUM_SOUND_PATHS; i++) {
			soundInfos[i].sample = pd->sound->sample->load(soundPaths[i]);
			if (soundInfos[i].sample == NULL) {
				pd->system->error("%s:%i Error loading sample[%i], path=%s", __FILE__, __LINE__, i, soundPaths[i]);
			}
		}
		
		pd->sound->channel->addSource(
			pd->sound->getDefaultChannel(),
			(SoundSource*)samplePlayer
		);
		
		// load and init sprites (and textures)
		for (int i = 0; i < NUM_BITMAP_PATHS; i++) {
			const char* outErr;
			spriteInfos[i].bitmap = pd->graphics->loadBitmap(bitmapPaths[i], &outErr);
			if (spriteInfos[i].bitmap == NULL) {
				pd->system->error("%s:%i Error loading bitmap[%i], path=%s, error=%s", __FILE__, __LINE__, i, bitmapPaths[i], outErr);
			}
		}
		spriteInfoCurr = &spriteInfos[0];
		
		sprite = pd->sprite->newSprite();
		pd->sprite->setCenter(sprite, 0.0f, 0.0f); // just as a preference, we'll use top-left as sprite origin (instead of playdate-default of center)
		pd->sprite->setSize(sprite, spriteInfoCurr->rect.width, spriteInfoCurr->rect.height);
		pd->sprite->moveTo(sprite, spriteInfoCurr->rect.x, spriteInfoCurr->rect.y);
		pd->sprite->setImage(sprite, spriteInfoCurr->bitmap, kBitmapUnflipped);
		pd->sprite->addSprite(sprite); // simply add to display list to simplify rendering
		
		// init system menu
		showTextMenuItemCheckmark = pd->system->addCheckmarkMenuItem(showTextMenuItemLabel, textShows, systemMenuItemCallback, NULL);

		// use C-only Playdate API:
		// Note: If you set an update callback in the kEventInit handler, the system assumes the game is pure C and doesn't run any Lua code in the game
		pd->system->setUpdateCallback(update, pd);
	}
	else if (event == kEventTerminate) {
		// game shutdown tasks:
		
		// clean up any allocated resources
		if (filePlayer != NULL) {
			pd->sound->fileplayer->stop(filePlayer);
			pd->sound->fileplayer->freePlayer(filePlayer);
		}
		
		if (samplePlayer != NULL) {
			pd->sound->sampleplayer->stop(samplePlayer);
			pd->sound->sampleplayer->freePlayer(samplePlayer);
		}
		for (int i = 0; i < NUM_SOUND_PATHS; i++) {
			if (soundInfos[i].sample != NULL) {
				pd->sound->sample->freeSample(soundInfos[i].sample);
			}
		}
		
		for (int i = 0; i < NUM_BITMAP_PATHS; i++) {
			if (spriteInfos[i].bitmap != NULL) {
				pd->graphics->freeBitmap(spriteInfos[i].bitmap);
			}
		}
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
	
	
	// read button input, if any
	pd->system->getButtonState(&btnsCurr, &btnsUpdateDown, &btnsUpdateUp);
	
	// play one of the sounds if A or B button pressed (only if button newly pressed/down, to avoid constant playing...)
	if (
		((kButtonA & btnsCurr) && !(kButtonA & btnsPrev)) || 
		((kButtonB & btnsCurr) && !(kButtonB & btnsPrev))
	) {
		pd->sound->sampleplayer->setSample(samplePlayer, soundInfos[soundRoundRobinIndex].sample);
		soundRoundRobinIndex = soundRoundRobinIndex < (NUM_SOUND_PATHS-1) ? soundRoundRobinIndex + 1 : 0;
		pd->sound->sampleplayer->play(samplePlayer, 1, 1.0f);
	}
	
	// update text position based on D-pad (if needed)
	if (
		((kButtonLeft & btnsCurr))
	) {
		textPosition = 3;
	}
	else if (
		((kButtonRight & btnsCurr))
	) {
		textPosition = 1;
	}
	
	if (
		((kButtonUp & btnsCurr))
	) {
		textPosition = 0;
	}
	else if (
		((kButtonDown & btnsCurr))
	) {
		textPosition = 2;
	}
	
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
				imageRotation += 15;
			}
		}
		else if (crankChange < 0) {
			// negative (backward/behind-screen) crank change!
			crankAmount += fabs(crankChange);
			
			if (fabs(crankAmount) > crankThreshold) {
				crankAmount = 0;
				imageRotation -= 15;
			}
		}
	}
	
	// update sprite based on updated rotation (if needed)
	spriteIndexImageRotation = (imageRotation % 360) / (360 / NUM_BITMAP_PATHS); // simplify rotation range down to 0 to 360, then map to the index range of [0, plus-minus NUM_BITMAP_PATHS)
	spriteIndexImageRotation = (spriteIndexImageRotation >= 0) ? spriteIndexImageRotation : (NUM_BITMAP_PATHS + spriteIndexImageRotation); // ensure/map within range of [0, NUM_BITMAP_PATHS)
	spriteInfoTemp = &spriteInfos[spriteIndexImageRotation];
	if (spriteInfoTemp != spriteInfoCurr) {
		spriteInfoPrev = spriteInfoCurr;
		spriteInfoCurr = spriteInfoTemp;
	}
	
	pd->sprite->setSize(sprite, spriteInfoCurr->rect.width, spriteInfoCurr->rect.height);
	pd->sprite->moveTo(sprite, spriteInfoCurr->rect.x, spriteInfoCurr->rect.y);
	pd->sprite->setImage(sprite, spriteInfoCurr->bitmap, kBitmapUnflipped);
	
	// loop music
	if (pd->sound->fileplayer->isPlaying(filePlayer) == 0) {
		pd->sound->fileplayer->play(filePlayer, 1);
	}
	
	
	// clear frame buffer before rendering anything this frame
	pd->graphics->clear(kColorWhite);
	
	// render sprite (simply all in display list)
	pd->sprite->drawSprites();
	
	// update text position (if needed)
	if (textShows) {
		if (textPosition == 0) {
			textX = LCD_COLUMNS / 2 - textWidth / 2;
			textY = 10;
			pd->graphics->fillRect(textX - 6, textY - 6, textWidth + 12, textHeight + 12, kColorBlack);
			pd->graphics->fillRect(textX - 4, textY - 4, textWidth + 8, textHeight + 8, kColorWhite);
			pd->graphics->drawText(helloText, strlen(helloText), kASCIIEncoding, textX, textY);
		}
		else if (textPosition == 1) {
			textX = LCD_COLUMNS - textWidth - 10;
			textY = LCD_ROWS / 2 - textHeight / 2;
			pd->graphics->fillRect(textX - 6, textY - 6, textWidth + 12, textHeight + 12, kColorBlack);
			pd->graphics->fillRect(textX - 4, textY - 4, textWidth + 8, textHeight + 8, kColorWhite);
			pd->graphics->drawText(helloText, strlen(helloText), kASCIIEncoding, textX, textY);
		}
		else if (textPosition == 2) {
			textX = LCD_COLUMNS / 2 - textWidth / 2;
			textY = LCD_ROWS - textHeight - 10;
			pd->graphics->fillRect(textX - 6, textY - 6, textWidth + 12, textHeight + 12, kColorBlack);
			pd->graphics->fillRect(textX - 4, textY - 4, textWidth + 8, textHeight + 8, kColorWhite);
			pd->graphics->drawText(helloText, strlen(helloText), kASCIIEncoding, textX, textY);
		}
		else if (textPosition == 3) {
			textX = 10;
			textY = LCD_ROWS / 2 - textHeight / 2;
			pd->graphics->fillRect(textX - 6, textY - 6, textWidth + 12, textHeight + 12, kColorBlack);
			pd->graphics->fillRect(textX - 4, textY - 4, textWidth + 8, textHeight + 8, kColorWhite);
			pd->graphics->drawText(helloText, strlen(helloText), kASCIIEncoding, textX, textY);
		}
	}
    
	// render FPS text (debugging only)
	pd->system->drawFPS(0,0);
	
	// store this frame's button state for next frame's reference (only at end of this frame)
	btnsPrev = btnsCurr;

	return 1;
}
