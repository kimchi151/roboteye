#ifndef ROBOTEYE_OLED_H
#define ROBOTEYE_OLED_H

#include <stdio.h>

#include "roboteye_animation.h"

#ifdef __cplusplus
extern "C" {
#endif

void roboteye_oled_draw_frame_ascii(const RobotEyeFrameView *frame, FILE *stream);

void roboteye_oled_run_test_sequence(void);

#ifdef __cplusplus
}
#endif

#endif /* ROBOTEYE_OLED_H */
