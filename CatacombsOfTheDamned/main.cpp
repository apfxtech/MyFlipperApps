#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/canvas_i.h>
#include <input/input.h>

#include <stdlib.h>
#include <string.h>

#include "game/Game.h"
#include "game/Platform.h"
#include "lib/EEPROM.h"

#define TARGET_FRAMERATE 30
#define HOLD_TIME_MS     300
#define INPUT_QUEUE_SIZE 32

FlipperState* g_state = nullptr;

static inline bool audio_enable() {
    return !furi_hal_rtc_is_flag_set(FuriHalRtcFlagStealthMode);
}

static inline uint8_t input_key_to_bit(InputKey key) {
    switch(key) {
    case InputKeyUp:
        return INPUT_UP;
    case InputKeyDown:
        return INPUT_DOWN;
    case InputKeyLeft:
        return INPUT_LEFT;
    case InputKeyRight:
        return INPUT_RIGHT;
    case InputKeyOk:
        return INPUT_B;
    default:
        return 0;
    }
}

static inline bool input_is_pressed_type(InputType type) {
    return type == InputTypePress;
}

static inline bool input_is_release_type(InputType type) {
    return type == InputTypeRelease;
}

static void input_view_port_callback(InputEvent* event, void* ctx) {
    if(!event || !ctx) return;
    if(!input_is_pressed_type(event->type) && !input_is_release_type(event->type)) return;

    FlipperState* state = static_cast<FlipperState*>(ctx);
    if(!state->input_mutex) return;

    if(furi_mutex_acquire(state->input_mutex, 0) != FuriStatusOk) return;

    if(state->input_active && state->input_queue) {
        (void)furi_message_queue_put(state->input_queue, event, FuriWaitForever);
    }

    furi_mutex_release(state->input_mutex);
}

static void game_view_port_draw_callback(Canvas* canvas, void* context) {
    FlipperState* state = static_cast<FlipperState*>(context);
    if(!state || !canvas || !state->fb_mutex) return;

    if(furi_mutex_acquire(state->fb_mutex, FuriWaitForever) != FuriStatusOk) return;

    uint8_t* dst = u8g2_GetBufferPtr(&canvas->fb);
    if(dst) {
        for(size_t i = 0; i < BUFFER_SIZE; i++) {
            dst[i] = (uint8_t)(state->front_buffer[i] ^ 0xFFu);
        }
    }

    furi_mutex_release(state->fb_mutex);
}

static void apply_input_event(FlipperState* state, const InputEvent* event) {
    if(!state || !event) return;

    if(event->key == InputKeyBack) {
        if(input_is_pressed_type(event->type)) {
            state->back_pressed = true;
        } else if(input_is_release_type(event->type)) {
            state->back_pressed = false;
        }
        return;
    }

    const uint8_t bit = input_key_to_bit(event->key);
    if(!bit) return;

    if(input_is_pressed_type(event->type)) {
        state->input_state |= bit;
    } else if(input_is_release_type(event->type)) {
        state->input_state &= (uint8_t)~bit;
    }
}

static void drain_input_queue(FlipperState* state) {
    if(!state || !state->input_queue) return;

    InputEvent event;
    while(furi_message_queue_get(state->input_queue, &event, 0) == FuriStatusOk) {
        apply_input_event(state, &event);
    }
}

extern "C" int32_t arduboy3d_app(void* p) {
    UNUSED(p);

    int32_t result = 0;
    bool save_ready = false;

    FlipperState* st = static_cast<FlipperState*>(malloc(sizeof(FlipperState)));
    if(!st) return -1;
    memset(st, 0, sizeof(FlipperState));
    g_state = st;

    do {
        st->fb_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
        if(!st->fb_mutex) {
            result = -1;
            break;
        }

        st->input_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
        if(!st->input_mutex) {
            result = -1;
            break;
        }

        st->input_queue = furi_message_queue_alloc(INPUT_QUEUE_SIZE, sizeof(InputEvent));
        if(!st->input_queue) {
            result = -1;
            break;
        }
        st->input_active = true;

        memset(st->back_buffer, 0x00, BUFFER_SIZE);
        memset(st->front_buffer, 0x00, BUFFER_SIZE);

        EEPROM.begin();
        furi_delay_ms(50);
        Platform::SetAudioEnabled(audio_enable());
        Game::menu.ReadSave();
        save_ready = true;

        st->gui = static_cast<Gui*>(furi_record_open(RECORD_GUI));
        if(!st->gui) {
            result = -1;
            break;
        }

        st->view_port = view_port_alloc();
        if(!st->view_port) {
            result = -1;
            break;
        }
        view_port_draw_callback_set(st->view_port, game_view_port_draw_callback, st);
        view_port_input_callback_set(st->view_port, input_view_port_callback, st);
        gui_add_view_port(st->gui, st->view_port, GuiLayerFullscreen);

        const uint32_t tick_hz = furi_kernel_get_tick_frequency();
        uint32_t frame_ticks = (tick_hz + (TARGET_FRAMERATE / 2u)) / TARGET_FRAMERATE;
        if(frame_ticks == 0) frame_ticks = 1;
        const uint32_t hold_ticks = (uint32_t)((HOLD_TIME_MS * tick_hz + 999u) / 1000u);

        bool back_was_pressed = false;
        bool back_hold_fired = false;
        uint32_t back_press_tick = 0;
        uint32_t next_tick = furi_get_tick();

        while(!st->exit_requested) {
            const uint32_t now = furi_get_tick();

            if((int32_t)(now - next_tick) < 0) {
                uint32_t dt_ticks = next_tick - now;
                uint32_t dt_ms = (dt_ticks * 1000u) / tick_hz;
                furi_delay_ms(dt_ms ? dt_ms : 1);
                continue;
            }

            if((int32_t)(now - next_tick) > (int32_t)(frame_ticks * 2u)) {
                next_tick = now;
            }
            next_tick += frame_ticks;

            drain_input_queue(st);

            if(!st->back_pressed) {
                back_was_pressed = false;
                back_hold_fired = false;
            } else {
                if(!back_was_pressed) {
                    back_was_pressed = true;
                    back_hold_fired = false;
                    back_press_tick = now;
                }

                if(!back_hold_fired && ((uint32_t)(now - back_press_tick) >= hold_ticks)) {
                    back_hold_fired = true;
                    if(Game::InMenu())
                        st->exit_requested = true;
                    else
                        Game::GoToMenu();
                }
            }

            if(st->exit_requested) break;

            Game::Tick();
            Game::Draw();
            if(furi_mutex_acquire(st->fb_mutex, FuriWaitForever) == FuriStatusOk) {
                memcpy(st->front_buffer, st->back_buffer, BUFFER_SIZE);
                furi_mutex_release(st->fb_mutex);
            }
            view_port_update(st->view_port);
        }
    } while(false);

    Platform::SetAudioEnabled(false);

    if(st->input_mutex) {
        if(furi_mutex_acquire(st->input_mutex, FuriWaitForever) == FuriStatusOk) {
            st->input_active = false;
            furi_mutex_release(st->input_mutex);
        }
    }

    FuriMessageQueue* input_queue = nullptr;
    if(st->input_mutex) {
        if(furi_mutex_acquire(st->input_mutex, FuriWaitForever) == FuriStatusOk) {
            input_queue = st->input_queue;
            st->input_queue = nullptr;
            st->input_state = 0;
            st->back_pressed = false;
            furi_mutex_release(st->input_mutex);
        }
    } else {
        input_queue = st->input_queue;
        st->input_queue = nullptr;
    }

    if(input_queue) {
        furi_message_queue_free(input_queue);
    }

    if(st->gui) {
        if(st->view_port) {
            view_port_enabled_set(st->view_port, false);
            gui_remove_view_port(st->gui, st->view_port);
            view_port_free(st->view_port);
            st->view_port = nullptr;
        }
        furi_record_close(RECORD_GUI);
        st->gui = nullptr;
    }

    FuriMutex* input_mutex = st->input_mutex;
    st->input_mutex = nullptr;
    if(input_mutex) {
        furi_mutex_free(input_mutex);
    }

    FuriMutex* fb_mutex = st->fb_mutex;
    st->fb_mutex = nullptr;
    if(fb_mutex) {
        furi_mutex_free(fb_mutex);
    }

    g_state = nullptr;
    free(st);

    if(save_ready) {
        Game::menu.WriteSave();
    }

    return result;
}
