# RobotEye animation demo

This repository contains a small C demo that organises converted OLED frame data, drives a simple animation state machine, and offers an ASCII based test harness for tuning.

## Build

```sh
make
```

## Run the OLED simulation

The default target builds `roboteye_demo`. Running it will execute the scripted animation phases, showing the rendered frames as ASCII art that mimics the OLED output.

```sh
./roboteye_demo
```

Use `make clean` to remove the build outputs.
