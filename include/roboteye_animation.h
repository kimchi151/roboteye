#ifndef ROBOTEYE_ANIMATION_H
#define ROBOTEYE_ANIMATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "roboteye_frames.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t *data;
    size_t length;
} RobotEyeFrameView;

typedef struct {
    const RobotEyeFrameView *frames;
    size_t frame_count;
    uint16_t frame_duration_ms;
    bool loop;
} RobotEyeAnimation;

typedef enum {
    ROBOTEYE_ANIMATION_IDLE = 0,
    ROBOTEYE_ANIMATION_BLINK,
    ROBOTEYE_ANIMATION_LOOK_LEFT,
    ROBOTEYE_ANIMATION_LOOK_RIGHT,
    ROBOTEYE_ANIMATION_COUNT
} RobotEyeAnimationId;

typedef struct RobotEyeAnimator {
    const RobotEyeAnimation *current;
    const RobotEyeAnimation *baseline;
    size_t frame_index;
    uint32_t frame_elapsed_ms;
    uint32_t blink_timer_ms;
    uint32_t blink_interval_ms;
    bool blinking_enabled;
    bool override_active;
    bool blink_in_progress;
    float horizontal_bias;
    bool movement_enabled;
    bool pending_start;
} RobotEyeAnimator;

void roboteye_animator_init(RobotEyeAnimator *animator, RobotEyeAnimationId default_animation);

void roboteye_animator_enable_blinking(RobotEyeAnimator *animator, bool enabled, uint32_t interval_ms);

void roboteye_animator_set_horizontal_bias(RobotEyeAnimator *animator, float bias);

void roboteye_animator_enable_movement(RobotEyeAnimator *animator, bool enabled);

void roboteye_animator_trigger(RobotEyeAnimator *animator, RobotEyeAnimationId animation_id);

const RobotEyeFrameView *roboteye_animator_tick(RobotEyeAnimator *animator, uint32_t delta_ms, size_t *frame_index);

const RobotEyeAnimation *roboteye_animation_get(RobotEyeAnimationId animation_id);

#ifdef __cplusplus
}
#endif

#endif /* ROBOTEYE_ANIMATION_H */
