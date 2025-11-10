#pragma once

#include <stdint.h>

#ifndef PROGMEM
#define PROGMEM
#endif

struct AnimationFrame {
    uint16_t duration_ms;      // Duration of the frame in milliseconds
    uint32_t bitmap_offset;    // Offset into the bitmap array where this frame begins
};

struct AnimationDescriptor {
    uint16_t width;            // Width of the bitmap in pixels
    uint16_t height;           // Height of the bitmap in pixels
    uint16_t frame_count;      // Total number of frames in the animation
    uint16_t bytes_per_frame;  // Size of a single frame in bytes
    const AnimationFrame* frames;  // Pointer to frame metadata stored in PROGMEM
    const uint8_t* bitmaps;        // Pointer to packed bitmap data stored in PROGMEM
};
