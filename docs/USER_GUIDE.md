# User Guide

## Controls

The game uses four lane buttons and one menu/restart button.

| Button color | Arduino pin | Wokwi key | Role |
|---|---:|---|---|
| Black | D5 | `K` | Lane 1 / Song 1 |
| Yellow | D4 | `J` | Lane 2 / Song 2 |
| Blue | D3 | `F` | Lane 3 / Song 3 |
| Green | D2 | `D` | Lane 4 / Song 4 |
| Red | D7 | `R` | Return to menu after a result |

## Select a song

At startup, the LED matrix shows the menu symbol. Press one of the four lane buttons to select the corresponding song. The selected song begins immediately.

| Button | Song | Tempo |
|---|---|---:|
| Black (D5) | Canon | 180 BPM |
| Yellow (D4) | Christmas Carol | 144 BPM |
| Blue (D3) | Greensleeves | 160 BPM |
| Green (D2) | Sans (source-code label) | 240 BPM |

## Play

1. Watch for notes appearing at the top of the display.
2. Follow each note as it moves downward.
3. Press the button for the note's lane when it reaches the bottom hit zone (the bottom two rows).
4. A successful hit increases your score and combo.
5. Missing a note increases the miss count and breaks the combo.

Press the lane button that matches the note's column/lane. A press in a different lane will not hit that note.

## Scoring and combo

- Combo 1–4: +1 point per successful hit.
- Combo 5–9: +2 points per successful hit.
- Combo 10 or higher: +5 points per successful hit.
- A missed note resets the combo.
- The combo also expires after 2 seconds without another successful hit.

## Win and game over

- **Win:** Score reaches 99.
- **Game over:** 5 notes have been missed.

After the win or game-over sequence, press the red button (D7 / `R` in Wokwi) to return to the menu and select another song.

## Difficulty

Notes initially move every 200 ms. The game reduces this interval by 35 ms every 5 seconds, down to a minimum of 70 ms. New notes are spawned about every 800 ms, with the game allowing up to six active notes.


