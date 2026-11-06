# Arduino Rhythm Game

An Arduino Uno rhythm game featuring four selectable songs, four note lanes, an LED matrix display, and two buzzers. Choose a song, follow the falling notes, and press the matching lane button when a note reaches the hit area.

The repository includes the Arduino sketch, pitch definitions, Wokwi circuit configuration, and setup instructions.

## Features

- Four built-in songs:
  1. **Canon** — tempo 180 BPM
  2. **Christmas Carol** — tempo 144 BPM
  3. **Greensleeves** — tempo 160 BPM
  4. **Sans** — tempo 240 BPM (the name used in the source code)
- Four lane buttons for song selection and note input.
- MAX7219 LED matrix game display.
- Separate buzzers for melody playback and sound effects.
- Combo-based scoring.
- Notes become faster as the game progresses.
- Win by reaching 99 points; game over after 5 missed notes.

## Hardware

| Component | Quantity | Purpose |
|---|---:|---|
| Arduino Uno (ATmega328P) | 1 | Game controller |
| MAX7219 8×8 LED matrix module(s) | 1 or more | Visual game display |
| Push buttons | 5 | Four lanes + menu/restart |
| Passive buzzers / piezo speakers | 2 | Music and sound effects |
| Jumper wires | As needed | Connections |
| Breadboard | Optional | Prototyping |


## Important: LED matrix configuration

There is one detail to check before building or simulating:

- The supplied `diagram.json` and wiring image show a **four-module MAX7219 chain** (`"chain": "4"`).
- The sketch currently defines `MAX_DEVICES` as **1** and draws an **8×8** game layout.

The code and circuit configuration are therefore not fully aligned for a four-module display. For the current game layout, use one 8×8 module and set the Wokwi chain to 1. If you intend to use all four chained modules, the sketch's `MAX_DEVICES` and display layout should be updated and tested for the 32×8 display before relying on all four modules. Do not assume the current sketch uses the full 32×8 area.

The sketch also selects `MD_MAX72XX::FC16_HW`; use a compatible FC-16-style MAX7219 module or adjust the hardware type for your module.

## How to play

1. Power the Arduino or start the Wokwi simulation.
2. The matrix displays the menu/play symbol.
3. Press one of the four lane buttons to select a song. The game starts immediately.
4. Watch the notes fall through the four lanes.
5. Press the matching lane button when its note reaches the bottom hit zone.
6. Keep hitting notes to build a combo and increase your score.
7. Reach 99 points to win. Missing **5 notes** causes game over.
8. Press the red menu/restart button to return to the menu after a result.

See [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for detailed controls and scoring, and [docs/INSTALLATION.md](docs/INSTALLATION.md) for setup and upload instructions.

## Wokwi simulation

1. Open [Wokwi](https://wokwi.com/).
2. Create an Arduino Uno project.
3. Add `sketch.ino`, `pitches.h`, `diagram.json`, and `libraries.txt`.
4. Confirm the matrix chain setting matches the display configuration you intend to use (see the warning above).
5. Start the simulation.
6. Use these keyboard shortcuts, which are configured in `diagram.json`:

| Key | Button |

| `D` | Green |
| `F` | Blue |
| `J` | Yellow |
| `K` | Black |
| `R` | Red menu/restart |

## Software and libraries

- Arduino IDE or Wokwi
- MD_MAX72XX library
- Arduino `SPI` library (included with the Arduino AVR core)
- `pitches.h` is included in this repository

The Wokwi `libraries.txt` also lists `max7219`, `LedController`, `LedControl`, and `toneAC`. The sketch directly includes `MD_MAX72xx.h` and `SPI.h`; install MD_MAX72XX if the compiler reports that header as missing.

