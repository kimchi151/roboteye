#pragma once

#include <stdint.h>
#include "animation_format.h"

#ifndef PROGMEM
#define PROGMEM
#endif

namespace animations {
namespace generated {

static const uint8_t sample_eye_bitmaps[] PROGMEM = {
    0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0xDB, 0x7E, 0x3C,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00
};

static const AnimationFrame sample_eye_frames[] PROGMEM = {
    {120, 0},
    {120, 8}
};

static const AnimationDescriptor sample_eye_animation PROGMEM = {
    8,   // width
    8,   // height
    2,   // frame count
    8,   // bytes per frame
    sample_eye_frames,
    sample_eye_bitmaps
};

}  // namespace generated
}  // namespace animations

