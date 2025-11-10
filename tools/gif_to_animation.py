#!/usr/bin/env python3
"""Convert an animated GIF into Arduino-friendly bitmap data.

The script produces two artifacts:
  * A JSON file with metadata that can be consumed by tooling.
  * A header file with packed PROGMEM data that matches AnimationDescriptor.

Example:
    python tools/gif_to_animation.py assets/eye.gif \
        --json-out animations/eye.json \
        --header-out include/animations/eye.h
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

try:
    from PIL import Image, ImageSequence
except ImportError as exc:  # pragma: no cover - dependency error
    raise SystemExit(
        "Pillow is required. Install it with `pip install pillow`.\n" f"Original error: {exc}"
    )


@dataclass
class FrameData:
    """Container for a single frame's bitmap payload."""

    duration_ms: int
    bitmap: bytes
    offset: int


@dataclass
class AnimationExport:
    """Exported animation information."""

    name: str
    width: int
    height: int
    bytes_per_frame: int
    frames: List[FrameData]
    loop_count: int
    source_path: str


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gif", type=Path, help="Path to an animated GIF")
    parser.add_argument(
        "--name",
        help="Symbol name to use for the generated data (defaults to GIF stem)",
    )
    parser.add_argument(
        "--json-out",
        type=Path,
        help="Destination JSON file (defaults to animations/<name>.json)",
    )
    parser.add_argument(
        "--header-out",
        type=Path,
        help="Destination header file (defaults to include/animations/<name>.h)",
    )
    parser.add_argument(
        "--namespace",
        default="animations::generated",
        help="C++ namespace for the generated symbols",
    )
    parser.add_argument(
        "--threshold",
        type=int,
        default=None,
        help=(
            "Optional manual threshold for binarising the image (0-255). "
            "When omitted Pillow's built-in conversion is used."
        ),
    )
    parser.add_argument(
        "--no-dither",
        dest="dither",
        action="store_false",
        help="Disable Floyd-Steinberg dithering when converting to 1-bit",
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert bits so 1 represents an 'off' pixel",
    )
    parser.set_defaults(dither=True)
    return parser.parse_args(argv)


def sanitize_name(raw: str) -> str:
    """Create a C identifier friendly version of *raw*."""

    cleaned = re.sub(r"[^0-9a-zA-Z_]", "_", raw)
    if cleaned and cleaned[0].isdigit():
        cleaned = f"_{cleaned}"
    return cleaned or "animation"


def ensure_paths(args: argparse.Namespace, name: str) -> Tuple[Path, Path]:
    json_out = args.json_out or Path("animations") / f"{name}.json"
    header_out = args.header_out or Path("include/animations") / f"{name}.h"
    for path in (json_out, header_out):
        path.parent.mkdir(parents=True, exist_ok=True)
    return json_out, header_out


def frame_duration(frame: Image.Image) -> int:
    """Return the duration for *frame* in milliseconds."""

    duration = frame.info.get("duration", 0)
    return int(duration) if duration else 100


def convert_frame(
    frame: Image.Image,
    width: int,
    height: int,
    threshold: int | None,
    dither: bool,
    invert: bool,
) -> bytes:
    """Convert the supplied frame into packed 1-bit bitmap bytes."""

    grayscale = frame.convert("L")
    if threshold is not None:
        threshold = max(0, min(255, threshold))
        bw = grayscale.point(lambda px: 255 if px >= threshold else 0, mode="1")
    else:
        dither_mode = Image.FLOYDSTEINBERG if dither else Image.NONE
        bw = grayscale.convert("1", dither=dither_mode)

    bytes_per_row = (width + 7) // 8
    expected_length = bytes_per_row * height
    payload = bw.tobytes()
    if len(payload) != expected_length:
        raise ValueError(
            f"Unexpected frame payload size: got {len(payload)}, expected {expected_length}"
        )
    if invert:
        payload = bytes(b ^ 0xFF for b in payload)
    return payload


def extract_animation(args: argparse.Namespace) -> AnimationExport:
    image_path = args.gif
    if not image_path.exists():
        raise FileNotFoundError(f"GIF not found: {image_path}")

    with Image.open(image_path) as img:
        if not getattr(img, "is_animated", False):
            raise ValueError("Input GIF must be animated")
        width, height = img.size
        bytes_per_row = (width + 7) // 8
        bytes_per_frame = bytes_per_row * height
        name = sanitize_name(args.name or image_path.stem)

        frames: List[FrameData] = []
        offset = 0
        for frame in ImageSequence.Iterator(img):
            bitmap = convert_frame(
                frame,
                width,
                height,
                threshold=args.threshold,
                dither=args.dither,
                invert=args.invert,
            )
            duration_ms = frame_duration(frame)
            frames.append(FrameData(duration_ms=duration_ms, bitmap=bitmap, offset=offset))
            offset += len(bitmap)

        loop_count = int(img.info.get("loop", 0))

    return AnimationExport(
        name=name,
        width=width,
        height=height,
        bytes_per_frame=bytes_per_frame,
        frames=frames,
        loop_count=loop_count,
        source_path=str(image_path),
    )


def format_hex_bytes(data: Iterable[int], indent: str = "    ", per_line: int = 12) -> str:
    hex_values = [f"0x{byte:02X}" for byte in data]
    lines = [
        indent + ", ".join(hex_values[i : i + per_line])
        for i in range(0, len(hex_values), per_line)
    ]
    return ",\n".join(lines)


def format_frames_array(frames: Sequence[FrameData], indent: str = "    ") -> str:
    lines = []
    for frame in frames:
        lines.append(f"{indent}{{{frame.duration_ms}, {frame.offset}}}")
    return ",\n".join(lines)


def build_json_payload(animation: AnimationExport) -> str:
    frames_json = [
        {
            "index": index,
            "duration_ms": frame.duration_ms,
            "bitmap_offset": frame.offset,
            "hex": [f"0x{byte:02X}" for byte in frame.bitmap],
        }
        for index, frame in enumerate(animation.frames)
    ]
    payload = {
        "name": animation.name,
        "source": animation.source_path,
        "width": animation.width,
        "height": animation.height,
        "bytes_per_frame": animation.bytes_per_frame,
        "frame_count": len(animation.frames),
        "loop_count": animation.loop_count,
        "frames": frames_json,
    }
    return json.dumps(payload, indent=2) + "\n"


def build_header_payload(animation: AnimationExport, namespace: str) -> str:
    ns_parts = [part for part in namespace.split("::") if part]
    safe_name = animation.name
    header_lines: List[str] = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "#include \"animation_format.h\"",
        "",
        "#ifndef PROGMEM",
        "#define PROGMEM",
        "#endif",
        "",
        "// Auto-generated by tools/gif_to_animation.py",
    ]

    for part in ns_parts:
        header_lines.append(f"namespace {part} {{")
    header_lines.append("")

    header_lines.append(
        f"static const uint8_t {safe_name}_bitmaps[] PROGMEM = {{"
    )
    header_lines.append(
        format_hex_bytes(
            (byte for frame in animation.frames for byte in frame.bitmap),
            indent="    ",
        )
    )
    header_lines.append("};\n")

    header_lines.append(
        f"static const AnimationFrame {safe_name}_frames[] PROGMEM = {{"
    )
    header_lines.append(format_frames_array(animation.frames))
    header_lines.append("};\n")

    header_lines.append(
        f"static const AnimationDescriptor {safe_name}_animation PROGMEM = {{"
    )
    header_lines.append(f"    {animation.width},")
    header_lines.append(f"    {animation.height},")
    header_lines.append(f"    {len(animation.frames)},")
    header_lines.append(f"    {animation.bytes_per_frame},")
    header_lines.append(f"    {safe_name}_frames,")
    header_lines.append(f"    {safe_name}_bitmaps")
    header_lines.append("};\n")

    for part in reversed(ns_parts):
        header_lines.append(f"}}  // namespace {part}")
    header_lines.append("")

    return "\n".join(header_lines)


def write_file(path: Path, payload: str) -> None:
    path.write_text(payload, encoding="utf-8")
    print(f"Wrote {path}")


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    animation = extract_animation(args)
    json_out, header_out = ensure_paths(args, animation.name)

    write_file(json_out, build_json_payload(animation))
    write_file(header_out, build_header_payload(animation, args.namespace))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
