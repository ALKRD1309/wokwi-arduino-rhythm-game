# Installation and Upload Guide

This guide covers running the game in Wokwi and uploading it to a physical Arduino Uno.

## Requirements

- Arduino Uno or compatible ATmega328P board
- USB data cable
- Arduino IDE 2.x (or Wokwi)
- MAX7219 LED matrix module
- Five push buttons
- Two passive buzzers / piezo speakers
- Jumper wires and optional breadboard

> **Matrix configuration warning:** The supplied Wokwi diagram shows four chained MAX7219 modules, but `sketch.ino` currently sets `MAX_DEVICES` to `1` and draws an 8×8 layout. For the current code, configure/use one 8×8 module. To use all four modules, update the code for a 32×8 display and test the coordinate mapping. The current sketch should not be assumed to drive all four modules correctly.

## Option A — Run in Wokwi

1. Visit [Wokwi](https://wokwi.com/).
2. Create a new Arduino Uno project.
3. Add the files from this repository:
   - `sketch.ino`
   - `pitches.h`
   - `diagram.json`
   - `libraries.txt`
4. Confirm that `pitches.h` is in the same sketch folder as `sketch.ino`.
5. Check the matrix chain count. Use 1 for the current 8×8 sketch, unless you have modified the code for a four-module chain.
6. Start the simulation.
7. Use the keyboard shortcuts:

| Key | Button |
|---|---|
| `D` | Green lane |
| `F` | Blue lane |
| `J` | Yellow lane |
| `K` | Black lane |
| `R` | Red menu/restart |

## Option B — Upload to a physical Arduino Uno

### 1. Install the MD_MAX72XX library

1. Open Arduino IDE.
2. Select **Sketch → Include Library → Manage Libraries**.
3. Search for **MD_MAX72XX**.
4. Install the library by MajicDesigns.

`SPI` is included with the Arduino AVR core. The project supplies `pitches.h`; do not delete it.

### 2. Prepare the sketch folder

1. Create a folder named `RhythmGame` in your Arduino sketchbook.
2. Copy `sketch.ino` and `pitches.h` into that folder.
3. Rename `sketch.ino` to `RhythmGame.ino` if you want the folder and primary sketch names to match (Arduino IDE convention).
4. Open the `.ino` file in Arduino IDE.

Keep `pitches.h` beside the `.ino` file so the `#include "pitches.h"` directive can find it.

### 3. Connect the components

#### MAX7219 matrix

| Matrix pin | Arduino Uno |
|---|---|
| V+ | 5V |
| GND | GND |
| DIN | D11 |
| CLK | D13 |
| CS | D10 |

#### Lane buttons

Connect one terminal of each push button to its assigned input pin and the other terminal to GND.

| Button | Arduino Uno |
|---|---:|
| Green | D2 |
| Blue | D3 |
| Yellow | D4 |
| Black | D5 |
| Red menu/restart | D7 |

The code uses `INPUT_PULLUP`, so external pull-up resistors are not required. A pressed button connects the input to GND.

#### Buzzers

| Buzzer | Signal terminal | Other terminal |
|---|---|---|
| Music | D9 | GND |
| Sound effects | D6 | GND |

The music output uses Timer1 / OC1A on pin D9. The sketch is written for the ATmega328P-based Arduino Uno and may need changes for other Arduino families.

### 4. Select the board and port

1. Connect the Uno to your computer using a USB data cable.
2. In Arduino IDE, choose **Tools → Board → Arduino AVR Boards → Arduino Uno**.
3. Choose the correct device under **Tools → Port**.

### 5. Verify and upload

1. Click **Verify** (checkmark) to compile.
2. Resolve any missing-library or file-location errors.
3. Click **Upload** (right arrow).
4. When upload completes, the game should initialize and show its menu symbol on the matrix.

### 6. Play

Use the lane buttons to select a song and hit notes. Press the red button after the win/game-over display to return to the menu. See [USER_GUIDE.md](USER_GUIDE.md) for full gameplay instructions.

