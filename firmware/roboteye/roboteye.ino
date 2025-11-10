#include <Arduino.h>

#if defined(ARDUINO_ARCH_AVR)
#include <avr/pgmspace.h>
#elif defined(ARDUINO)
#include <pgmspace.h>
#endif

#include "animation_format.h"
#include "animations/sample_eye.h"

namespace {
constexpr size_t kMaxFrameBytes = 512;

AnimationDescriptor activeDescriptor{};
AnimationFrame currentFrameMeta{};
uint8_t frameBuffer[kMaxFrameBytes];
uint16_t currentFrameIndex = 0;
uint32_t lastFrameTick = 0;

AnimationDescriptor loadDescriptor(const AnimationDescriptor* descriptor) {
  AnimationDescriptor copy;
  memcpy_P(&copy, descriptor, sizeof(AnimationDescriptor));
  return copy;
}

AnimationFrame loadFrameMeta(const AnimationFrame* frames, uint16_t index) {
  AnimationFrame meta;
  memcpy_P(&meta, frames + index, sizeof(AnimationFrame));
  return meta;
}

void loadFrameBuffer(const AnimationDescriptor& descriptor,
                     const AnimationFrame& frame) {
  if (descriptor.bytes_per_frame > kMaxFrameBytes) {
    Serial.println(F("Frame buffer is too small for the selected animation"));
    while (true) {
      delay(1000);
    }
  }

  const uint8_t* source = descriptor.bitmaps + frame.bitmap_offset;
  memcpy_P(frameBuffer, source, descriptor.bytes_per_frame);
}

void renderFrame(const AnimationDescriptor& descriptor, const uint8_t* data) {
  const uint16_t bytesPerRow = (descriptor.width + 7) / 8;

  Serial.println(F("--- Frame Start ---"));
  for (uint16_t y = 0; y < descriptor.height; ++y) {
    const uint8_t* row = data + (y * bytesPerRow);
    for (uint16_t x = 0; x < descriptor.width; ++x) {
      const uint16_t byteIndex = x / 8;
      const uint8_t bitIndex = 7 - (x % 8);
      const bool pixelOn = row[byteIndex] & (1 << bitIndex);
      Serial.print(pixelOn ? F("#") : F("."));
    }
    Serial.println();
  }
  Serial.println(F("--- Frame End ---"));
}

void displayFrame(uint16_t index) {
  currentFrameMeta = loadFrameMeta(activeDescriptor.frames, index);
  loadFrameBuffer(activeDescriptor, currentFrameMeta);
  renderFrame(activeDescriptor, frameBuffer);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  activeDescriptor = loadDescriptor(&animations::generated::sample_eye_animation);
  displayFrame(currentFrameIndex);
  lastFrameTick = millis();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastFrameTick >= currentFrameMeta.duration_ms) {
    currentFrameIndex = (currentFrameIndex + 1) % activeDescriptor.frame_count;
    displayFrame(currentFrameIndex);
    lastFrameTick = now;
  }
}
