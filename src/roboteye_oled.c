#include "roboteye_oled.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifndef ROBOTEYE_FRAME_WIDTH
#error "roboteye_frames.h must define ROBOTEYE_FRAME_WIDTH"
#endif

void roboteye_oled_draw_frame_ascii(const RobotEyeFrameView *frame, FILE *stream) {
    if (!frame || !frame->data || !stream) {
        return;
    }

    const size_t bit_count = ROBOTEYE_FRAME_WIDTH * ROBOTEYE_FRAME_HEIGHT;
    const size_t max_bits = frame->length * 8;
    const size_t usable_bits = bit_count < max_bits ? bit_count : max_bits;

    for (size_t y = 0; y < ROBOTEYE_FRAME_HEIGHT; ++y) {
        for (size_t x = 0; x < ROBOTEYE_FRAME_WIDTH; ++x) {
            const size_t bit_index = y * ROBOTEYE_FRAME_WIDTH + x;
            if (bit_index >= usable_bits) {
                fputc('.', stream);
                continue;
            }

            const size_t byte_index = bit_index / 8;
            const size_t bit_offset = 7 - (bit_index % 8);
            const uint8_t byte = frame->data[byte_index];
            const bool on = (byte >> bit_offset) & 0x01u;
            fputc(on ? '#' : '.', stream);
        }
        fputc('\n', stream);
    }
    fputc('\n', stream);
}

void roboteye_oled_run_test_sequence(void) {
    RobotEyeAnimator animator;
    roboteye_animator_init(&animator, ROBOTEYE_ANIMATION_IDLE);
    roboteye_animator_enable_blinking(&animator, true, 900);
    roboteye_animator_enable_movement(&animator, true);

    struct Phase {
        const char *label;
        float bias;
        unsigned int steps;
    } phases[] = {
        {"Idle", 0.0f, 5},
        {"LookLeft", -0.8f, 6},
        {"Idle", 0.0f, 4},
        {"LookRight", 0.8f, 6},
        {"ManualBlink", 0.0f, 5},
    };

    const uint32_t step_ms = 40;

    for (size_t i = 0; i < sizeof(phases) / sizeof(phases[0]); ++i) {
        roboteye_animator_set_horizontal_bias(&animator, phases[i].bias);
        printf("\n== Phase %s (bias %.2f) ==\n", phases[i].label, phases[i].bias);

        if (phases[i].bias == 0.0f && phases[i].label && phases[i].label[0] == 'M') {
            roboteye_animator_trigger(&animator, ROBOTEYE_ANIMATION_BLINK);
        }

        for (unsigned int step = 0; step < phases[i].steps; ++step) {
            size_t frame_index = 0;
            const RobotEyeFrameView *frame = roboteye_animator_tick(&animator, step_ms, &frame_index);
            if (!frame) {
                continue;
            }

            printf("Frame %zu\n", frame_index + 1);
            roboteye_oled_draw_frame_ascii(frame, stdout);
        }
    }
}
