# roboteye

A minimal toolchain for turning animated GIFs into Arduino-friendly bitmap
animations. The repository contains the conversion script, generated data
artifacts, and a reference `.ino` sketch that replays the converted frames.

## GIF → bitmap conversion

Use `tools/gif_to_animation.py` to convert an animated GIF into the data format
consumed by the firmware. The script requires [Pillow](https://python-pillow.org)
(`pip install pillow`).

```bash
python tools/gif_to_animation.py path/to/animation.gif \
  --json-out animations/animation.json \
  --header-out include/animations/animation.h
```

Key features:

- Generates a JSON manifest containing dimensions, frame timings, and per-frame
  byte arrays.
- Creates a header file with packed `PROGMEM` data that matches the
  `AnimationDescriptor` layout in `include/animation_format.h`.
- Supports manual thresholding (`--threshold`), dithering control
  (`--no-dither`), and bit inversion (`--invert`).
- Allows custom C++ namespaces through `--namespace`.

## Data format

Each animation uses the following structures:

```cpp
struct AnimationFrame {
  uint16_t duration_ms;      // Frame duration in milliseconds
  uint32_t bitmap_offset;    // Offset into the combined bitmap array
};

struct AnimationDescriptor {
  uint16_t width;            // Frame width in pixels
  uint16_t height;           // Frame height in pixels
  uint16_t frame_count;      // Total number of frames
  uint16_t bytes_per_frame;  // Packed bytes per frame ((width + 7) / 8 * height)
  const AnimationFrame* frames;  // Metadata table stored in PROGMEM
  const uint8_t* bitmaps;        // Packed bitmap data stored in PROGMEM
};
```

Refer to `animations/sample_eye.json` and `include/animations/sample_eye.h`
for a complete example produced with the above layout.

## Firmware example

`firmware/roboteye/roboteye.ino` demonstrates how to load the generated data,
copy it out of `PROGMEM`, and render each frame. The example renders the bitmap
to the serial console, but the `renderFrame` function is the only part that
needs to be adapted for a real LED matrix or display driver.

The sketch keeps a small RAM buffer (`kMaxFrameBytes`) for the currently active
frame, copies each frame into it using `memcpy_P`, and advances through the
animation based on the embedded frame durations.
