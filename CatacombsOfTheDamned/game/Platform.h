#pragma once

#include <furi.h>
#include <stdint.h>

#include "game/Defines.h"

typedef struct Gui Gui;
typedef struct ViewPort ViewPort;

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)
#endif

typedef struct {
    uint8_t back_buffer[BUFFER_SIZE];
    uint8_t front_buffer[BUFFER_SIZE];

    Gui* gui;
    ViewPort* view_port;
    FuriMutex* fb_mutex;
    FuriMessageQueue* input_queue;
    FuriMutex* input_mutex;

    uint8_t input_state;
    bool back_pressed;
    bool exit_requested;
    bool audio_enabled;
    bool input_active;
} FlipperState;

extern FlipperState* g_state;

class Platform
{
public:
	static uint8_t GetInput(void);
	static uint8_t* GetScreenBuffer(); 

	static void PlaySound(const uint16_t* audioPattern);
	static bool IsAudioEnabled();
	static void SetAudioEnabled(bool isEnabled);
	
	static void FillScreen(uint8_t col);
	static void PutPixel(uint8_t x, uint8_t y, uint8_t colour);
	static void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask, uint8_t frame, uint8_t mask_frame);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame);	

	static void DrawVLine(uint8_t x, int8_t y1, int8_t y2, uint8_t pattern);
	static void DrawBackground();
};
