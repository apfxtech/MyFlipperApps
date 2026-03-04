#include "lib/ArduboyTones.h"
#include "lib/runtime.h"

ArduboyTones sound(false);

static constexpr uint8_t DrivinBtnUp = 0x80u;
static constexpr uint8_t DrivinBtnDown = 0x10u;
static constexpr uint8_t DrivinBtnLeft = 0x20u;
static constexpr uint8_t DrivinBtnRight = 0x40u;
static constexpr uint8_t DrivinBtnA = 0x08u;
static constexpr uint8_t DrivinBtnB = 0x04u;

uint8_t arduboy_runtime_map_buttons(uint8_t buttons) {
    uint8_t out = 0;
    if(buttons & UP_BUTTON) out |= DrivinBtnUp;
    if(buttons & DOWN_BUTTON) out |= DrivinBtnDown;
    if(buttons & LEFT_BUTTON) out |= DrivinBtnLeft;
    if(buttons & RIGHT_BUTTON) out |= DrivinBtnRight;
    if(buttons & A_BUTTON) out |= DrivinBtnB;
    if(buttons & B_BUTTON) out |= DrivinBtnA;
    return out;
}

uint32_t arduboy_runtime_fps(void) {
    return 30u;
}

void arduboy_runtime_on_stop(void) {
    sound.noTone();
    arduboy_tone_sound_system_deinit();
}
