#include "roboteye_animation.h"

#include <stddef.h>

static const RobotEyeFrameView ROBOTEYE_IDLE_FRAMES[] = {
    {ROBOTEYE_FRAME_IDLE_OPEN, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_IDLE_FOCUS, ROBOTEYE_FRAME_BYTES},
};

static const RobotEyeFrameView ROBOTEYE_BLINK_FRAMES[] = {
    {ROBOTEYE_FRAME_IDLE_OPEN, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_BLINK_HALF, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_BLINK_CLOSED, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_BLINK_HALF, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_IDLE_OPEN, ROBOTEYE_FRAME_BYTES},
};

static const RobotEyeFrameView ROBOTEYE_LOOK_LEFT_FRAMES[] = {
    {ROBOTEYE_FRAME_LOOK_LEFT, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_IDLE_FOCUS, ROBOTEYE_FRAME_BYTES},
};

static const RobotEyeFrameView ROBOTEYE_LOOK_RIGHT_FRAMES[] = {
    {ROBOTEYE_FRAME_LOOK_RIGHT, ROBOTEYE_FRAME_BYTES},
    {ROBOTEYE_FRAME_IDLE_FOCUS, ROBOTEYE_FRAME_BYTES},
};

static const RobotEyeAnimation ROBOTEYE_ANIMATIONS[ROBOTEYE_ANIMATION_COUNT] = {
    [ROBOTEYE_ANIMATION_IDLE] = {
        .frames = ROBOTEYE_IDLE_FRAMES,
        .frame_count = sizeof(ROBOTEYE_IDLE_FRAMES) / sizeof(ROBOTEYE_IDLE_FRAMES[0]),
        .frame_duration_ms = 140,
        .loop = true,
    },
    [ROBOTEYE_ANIMATION_BLINK] = {
        .frames = ROBOTEYE_BLINK_FRAMES,
        .frame_count = sizeof(ROBOTEYE_BLINK_FRAMES) / sizeof(ROBOTEYE_BLINK_FRAMES[0]),
        .frame_duration_ms = 45,
        .loop = false,
    },
    [ROBOTEYE_ANIMATION_LOOK_LEFT] = {
        .frames = ROBOTEYE_LOOK_LEFT_FRAMES,
        .frame_count = sizeof(ROBOTEYE_LOOK_LEFT_FRAMES) / sizeof(ROBOTEYE_LOOK_LEFT_FRAMES[0]),
        .frame_duration_ms = 120,
        .loop = true,
    },
    [ROBOTEYE_ANIMATION_LOOK_RIGHT] = {
        .frames = ROBOTEYE_LOOK_RIGHT_FRAMES,
        .frame_count = sizeof(ROBOTEYE_LOOK_RIGHT_FRAMES) / sizeof(ROBOTEYE_LOOK_RIGHT_FRAMES[0]),
        .frame_duration_ms = 120,
        .loop = true,
    },
};

static const RobotEyeAnimation *roboteye_choose_baseline(const RobotEyeAnimator *animator) {
    if (!animator) {
        return NULL;
    }

    if (!animator->movement_enabled) {
        return &ROBOTEYE_ANIMATIONS[ROBOTEYE_ANIMATION_IDLE];
    }

    const float bias = animator->horizontal_bias;
    if (bias <= -0.35f) {
        return &ROBOTEYE_ANIMATIONS[ROBOTEYE_ANIMATION_LOOK_LEFT];
    }

    if (bias >= 0.35f) {
        return &ROBOTEYE_ANIMATIONS[ROBOTEYE_ANIMATION_LOOK_RIGHT];
    }

    return &ROBOTEYE_ANIMATIONS[ROBOTEYE_ANIMATION_IDLE];
}

static void roboteye_reset_to_baseline(RobotEyeAnimator *animator, bool reset_elapsed) {
    animator->current = animator->baseline;
    animator->frame_index = 0;
    if (reset_elapsed) {
        animator->frame_elapsed_ms = 0;
    }
    animator->pending_start = true;
}

void roboteye_animator_init(RobotEyeAnimator *animator, RobotEyeAnimationId default_animation) {
    if (!animator || default_animation >= ROBOTEYE_ANIMATION_COUNT) {
        return;
    }

    animator->baseline = &ROBOTEYE_ANIMATIONS[default_animation];
    animator->current = animator->baseline;
    animator->frame_index = 0;
    animator->frame_elapsed_ms = 0;
    animator->blink_timer_ms = 0;
    animator->blink_interval_ms = 1000;
    animator->blinking_enabled = false;
    animator->override_active = false;
    animator->blink_in_progress = false;
    animator->horizontal_bias = 0.0f;
    animator->movement_enabled = false;
    animator->pending_start = true;
}

void roboteye_animator_enable_blinking(RobotEyeAnimator *animator, bool enabled, uint32_t interval_ms) {
    if (!animator) {
        return;
    }

    animator->blinking_enabled = enabled;
    if (enabled) {
        animator->blink_interval_ms = interval_ms ? interval_ms : 1000;
        if (animator->blink_timer_ms > animator->blink_interval_ms) {
            animator->blink_timer_ms = animator->blink_interval_ms;
        }
    } else {
        animator->blink_timer_ms = 0;
        animator->blink_in_progress = false;
    }
}

void roboteye_animator_set_horizontal_bias(RobotEyeAnimator *animator, float bias) {
    if (!animator) {
        return;
    }

    if (bias > 1.0f) {
        bias = 1.0f;
    } else if (bias < -1.0f) {
        bias = -1.0f;
    }
    animator->horizontal_bias = bias;
}

void roboteye_animator_enable_movement(RobotEyeAnimator *animator, bool enabled) {
    if (!animator) {
        return;
    }

    animator->movement_enabled = enabled;
    if (!enabled) {
        animator->horizontal_bias = 0.0f;
    }
}

void roboteye_animator_trigger(RobotEyeAnimator *animator, RobotEyeAnimationId animation_id) {
    if (!animator || animation_id >= ROBOTEYE_ANIMATION_COUNT) {
        return;
    }

    const RobotEyeAnimation *animation = &ROBOTEYE_ANIMATIONS[animation_id];
    animator->current = animation;
    animator->frame_index = 0;
    animator->frame_elapsed_ms = 0;
    animator->override_active = !animation->loop;
    animator->blink_in_progress = (animation_id == ROBOTEYE_ANIMATION_BLINK);
    animator->pending_start = true;

    if (animation->loop) {
        animator->baseline = animation;
    } else {
        animator->blink_timer_ms = 0;
    }
}

const RobotEyeFrameView *roboteye_animator_tick(RobotEyeAnimator *animator, uint32_t delta_ms, size_t *frame_index) {
    if (!animator) {
        return NULL;
    }

    const RobotEyeAnimation *desired_baseline = roboteye_choose_baseline(animator);
    if (desired_baseline && desired_baseline != animator->baseline) {
        bool baseline_was_active = (!animator->override_active) && (animator->current == animator->baseline);
        animator->baseline = desired_baseline;
        if (baseline_was_active) {
            roboteye_reset_to_baseline(animator, true);
        }
    }

    if (!animator->current) {
        roboteye_reset_to_baseline(animator, true);
        if (!animator->current) {
            return NULL;
        }
    }

    bool started_new_animation = false;
    if (animator->pending_start) {
        started_new_animation = true;
        animator->pending_start = false;
    }

    if (animator->blinking_enabled && animator->blink_interval_ms > 0) {
        if (!animator->blink_in_progress && !animator->override_active) {
            if (animator->blink_timer_ms + delta_ms >= animator->blink_interval_ms) {
                roboteye_animator_trigger(animator, ROBOTEYE_ANIMATION_BLINK);
                animator->blink_timer_ms = 0;
                started_new_animation = true;
                animator->pending_start = false;
            } else {
                animator->blink_timer_ms += delta_ms;
            }
        } else if (animator->blink_in_progress) {
            animator->blink_timer_ms = 0;
        }
    }

    if (!started_new_animation) {
        animator->frame_elapsed_ms += delta_ms;

        while (animator->frame_elapsed_ms >= animator->current->frame_duration_ms) {
            animator->frame_elapsed_ms -= animator->current->frame_duration_ms;
            animator->frame_index++;

            if (animator->frame_index >= animator->current->frame_count) {
                if (animator->current->loop) {
                    animator->frame_index = 0;
                } else {
                    animator->override_active = false;
                    if (animator->blink_in_progress) {
                        animator->blink_in_progress = false;
                    }
                    roboteye_reset_to_baseline(animator, false);
                    if (!animator->current) {
                        return NULL;
                    }
                    animator->frame_index = 0;
                }
            }
        }
    }

    if (frame_index) {
        *frame_index = animator->frame_index;
    }

    return &animator->current->frames[animator->frame_index];
}

const RobotEyeAnimation *roboteye_animation_get(RobotEyeAnimationId animation_id) {
    if (animation_id >= ROBOTEYE_ANIMATION_COUNT) {
        return NULL;
    }
    return &ROBOTEYE_ANIMATIONS[animation_id];
}
