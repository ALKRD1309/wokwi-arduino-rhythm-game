// ------------------------------------------------------
// DECLARATIONS
// ------------------------------------------------------

// ==================== LIBRARIES ====================
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "pitches.h"
#define REST 0
// ==================== HARDWARE CONFIGURATION ====================
// --- LED MATRIX CONFIGURATION ---
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1
#define DATA_PIN 11
#define CLK_PIN 13
#define CS_PIN 10

// --- BUTTON CONFIGURATION ---
#define BTN_MAIN 7        // Main menu/restart button
#define BTN1 5            // Song 1 / Lane 1 button
#define BTN2 4            // Song 2 / Lane 2 button
#define BTN3 3            // Song 3 / Lane 3 button
#define BTN4 2            // Song 4 / Lane 4 button
const uint8_t buttonPins[4] = {BTN1, BTN2, BTN3, BTN4};

// --- BUZZER CONFIGURATION ---
#define BUZZER_SFX 6      // Sound effects buzzer
#define BUZZER_MUSIC 9    // Music playback buzzer

// ==================== GAME CONSTANTS ====================
// --- NOTE TIMING CONSTANTS ---
constexpr uint8_t HIT_START = 6;          // Top of hit zone (row index)
constexpr uint8_t HIT_END = 7;            // Bottom of hit zone (row index)
constexpr uint16_t NOTE_SPEED = 200;      // Initial note fall speed (ms)
constexpr uint16_t SPAWN_INTERVAL = 800;  // Time between new note spawns (ms)

// --- GAME LIMIT CONSTANTS ---
constexpr uint8_t MAX_NOTES = 6;          // Maximum concurrent notes on screen
constexpr uint8_t MAX_MISSES = 5;         // Misses before game over
constexpr uint8_t SCORE_TO_WIN = 99;      // Score needed to win

// --- SPEED INCREASE SYSTEM ---
uint32_t lastFall = 0;                    // Last time notes moved down
uint32_t lastSpawn = 0;                   // Last time a note was spawned
uint16_t currentNoteSpeed = NOTE_SPEED;   // Current note fall speed
uint32_t lastSpeedIncrease = 0;           // Last time speed increased
constexpr uint32_t SPEED_INCREASE_INTERVAL = 5000UL;  // Speed up every 5 seconds
constexpr uint16_t SPEED_STEP = 35;       // Speed increase amount (ms reduction)
constexpr uint16_t MIN_NOTE_SPEED = 70;   // Maximum possible speed (minimum time)

// --- BUTTON DEBOUNCE SYSTEM ---
uint32_t lastDebounce[4] = {0,0,0,0};     // Last button press time per lane
constexpr uint32_t DEBOUNCE_DELAY = 0;    // Debounce delay in ms (0 = minimal)

// ==================== GAME STATE MANAGEMENT ====================
// --- GAME STATE ENUMERATION ---
enum GameState : uint8_t { 
  MENU,           // Main menu/song selection screen
  PLAYING,        // Active gameplay
  GAME_OVER,      // Game over screen
  GAME_WIN        // Victory screen
};
GameState state = MENU;  // Current game state

// --- DISPLAY OBJECT ---
MD_MAX72XX display = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// ==================== NOTE MANAGEMENT SYSTEM ====================
// --- NOTE STRUCTURE (BIT-PACKED FOR MEMORY EFFICIENCY) ---
struct Note {
  uint8_t lane;   // Which lane (0-3) the note is in
  uint8_t row;    // Current row position (0-7)
  uint8_t flags;  // Bit 0: active, Bit 1: missed
};

Note notes[MAX_NOTES];  // Array of active/available notes

// --- INLINE FLAG MANIPULATION FUNCTIONS ---
inline bool noteActive(const Note &n) { return n.flags & 0x01; }
inline void setNoteActive(Note &n, bool v) { 
  if(v) n.flags |= 0x01; else n.flags &= ~0x01; 
}
inline bool noteMissed(const Note &n) { return n.flags & 0x02; }
inline void setNoteMissed(Note &n, bool v) { 
  if(v) n.flags |= 0x02; else n.flags &= ~0x02; 
}

// ==================== SCORE & COMBO SYSTEM ====================
int16_t score = 0;                     // Player's current score
int16_t combo = 0;                     // Current hit streak
uint8_t missedNotes = 0;               // Total missed notes
uint32_t lastComboTime = 0;            // Last time a note was hit
constexpr uint32_t COMBO_TIMEOUT = 2000UL;  // Time before combo resets (ms)

// ---------------- BITMAPS ----------------
const uint8_t PROGMEM playSymbol[]={0B00000000,0B01000010,0B01000010,0B01000010,0B00000000,0B01011010,0B00100100,0B00000000};
const uint8_t PROGMEM checkmarkSymbol[]={0B00000000,0B01000010,0B10100101,0B10100101,0B00000000,0B01011010,0B00100100,0B00000000};
const uint8_t PROGMEM skullSymbol[]={0B00000000,0B10100101,0B01000010,0B10100101,0B00000000,0B00011000,0B00100100,0B00000000};
const uint8_t PROGMEM circleSymbol[]={0B00111100,0B01000010,0B10011001,0B10100101,0B10100101,0B10011001,0B01000010,0B00111100};
const uint8_t PROGMEM xSymbol[]={0B01000010,0B10100101,0B01011010,0B00100100,0B00100100,0B01011010,0B10100101,0B01000010};
const uint8_t PROGMEM smallNum[10][6]={{B0110,B1001,B1001,B1001,B1001,B0110},{B0010,B0110,B0010,B0010,B0010,B0111},{B0110,B1001,B0001,B0010,B0100,B1111},{B1110,B0001,B0110,B0001,B1001,B0110},{B1001,B1001,B1111,B0001,B0001,B0001},{B1111,B1000,B1110,B0001,B1001,B0110},{B0110,B1000,B1110,B1001,B1001,B0110},{B1111,B0001,B0010,B0010,B0100,B0100},{B0110,B1001,B0110,B1001,B1001,B0110},{B0110,B1001,B1001,B0111,B0001,B0110}};

// ------------------------------------------------------
// FOUR SONGS
// ------------------------------------------------------
const int tempo1 = 180; // CANON
const int rawMelody1[] PROGMEM = {
 NOTE_D4,4, NOTE_FS4,8, NOTE_G4,8, NOTE_A4,4, NOTE_FS4,8, NOTE_G4,8, 
  NOTE_A4,4, NOTE_B3,8, NOTE_CS4,8, NOTE_D4,8, NOTE_E4,8, NOTE_FS4,8, NOTE_G4,8, 
  NOTE_FS4,4, NOTE_D4,8, NOTE_E4,8, NOTE_FS4,4, NOTE_FS3,8, NOTE_G3,8,
  NOTE_A3,8, NOTE_G3,8, NOTE_FS3,8, NOTE_G3,8, NOTE_A3,2,
  NOTE_G3,4, NOTE_B3,8, NOTE_A3,8, NOTE_G3,4, NOTE_FS3,8, NOTE_E3,8, 
  NOTE_FS3,4, NOTE_D3,8, NOTE_E3,8, NOTE_FS3,8, NOTE_G3,8, NOTE_A3,8, NOTE_B3,8,

  NOTE_G3,4, NOTE_B3,8, NOTE_A3,8, NOTE_B3,4, NOTE_CS4,8, NOTE_D4,8,
  NOTE_A3,8, NOTE_B3,8, NOTE_CS4,8, NOTE_D4,8, NOTE_E4,8, NOTE_FS4,8, NOTE_G4,8, NOTE_A4,2,
  NOTE_A4,4, NOTE_FS4,8, NOTE_G4,8, NOTE_A4,4,
  NOTE_FS4,8, NOTE_G4,8, NOTE_A4,8, NOTE_A3,8, NOTE_B3,8, NOTE_CS4,8,
  NOTE_D4,8, NOTE_E4,8, NOTE_FS4,8, NOTE_G4,8, NOTE_FS4,4, NOTE_D4,8, NOTE_E4,8,
  NOTE_FS4,8, NOTE_CS4,8, NOTE_A3,8, NOTE_A3,8,

  NOTE_CS4,4, NOTE_B3,4, NOTE_D4,8, NOTE_CS4,8, NOTE_B3,4,
  NOTE_A3,8, NOTE_G3,8, NOTE_A3,4, NOTE_D3,8, NOTE_E3,8, NOTE_FS3,8, NOTE_G3,8,
  NOTE_A3,8, NOTE_B3,4, NOTE_G3,4, NOTE_B3,8, NOTE_A3,8, NOTE_B3,4,
  NOTE_CS4,8, NOTE_D4,8, NOTE_A3,8, NOTE_B3,8, NOTE_CS4,8, NOTE_D4,8, NOTE_E4,8,
  NOTE_FS4,8, NOTE_G4,8, NOTE_A4,2,  
};
const uint16_t melodyLength1 = sizeof(rawMelody1)/sizeof(rawMelody1[0])/2;

const int tempo2 = 144; // christmascarol
const int rawMelody2[] PROGMEM = {
  
    NOTE_D5,4, NOTE_G4,8, NOTE_A4,8, NOTE_B4,8, NOTE_C5,8, //1
  NOTE_D5,4, NOTE_G4,4, NOTE_G4,4,
  NOTE_E5,4, NOTE_C5,8, NOTE_D5,8, NOTE_E5,8, NOTE_FS5,8,
  NOTE_G5,4, NOTE_G4,4, NOTE_G4,4,
  NOTE_C5,4, NOTE_D5,8, NOTE_C5,8, NOTE_B4,8, NOTE_A4,8,
  
  NOTE_B4,4, NOTE_C5,8, NOTE_B4,8, NOTE_A4,8, NOTE_G4,8,//6
  NOTE_FS4,4, NOTE_G4,8, NOTE_A4,8, NOTE_B4,8, NOTE_G4,8,
  NOTE_A4,-2,
  NOTE_D5,4, NOTE_G4,8, NOTE_A4,8, NOTE_B4,8, NOTE_C5,8, 
  NOTE_D5,4, NOTE_G4,4, NOTE_G4,4,
  NOTE_E5,4, NOTE_C5,8, NOTE_D5,8, NOTE_E5,8, NOTE_FS5,8,
  
  NOTE_G5,4, NOTE_G4,4, NOTE_G4,4,
  NOTE_C5,4, NOTE_D5,8, NOTE_C5,8, NOTE_B4,8, NOTE_A4,8, //12
  NOTE_B4,4, NOTE_C5,8, NOTE_B4,8, NOTE_A4,8, NOTE_G4,8,
  NOTE_A4,4, NOTE_B4,8, NOTE_A4,8, NOTE_G4,8, NOTE_FS4,8,
  NOTE_G4,-2,

  
};
const uint16_t melodyLength2 = sizeof(rawMelody2)/sizeof(rawMelody2[0])/2;

const int tempo3 = 160; // greensleves
const int rawMelody3[] PROGMEM = {
  NOTE_G4,8,//1
  NOTE_AS4,4, NOTE_C5,8, NOTE_D5,-8, NOTE_DS5,16, NOTE_D5,8,
  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,
  NOTE_AS4,4, NOTE_G4,8, NOTE_G4,-8, NOTE_FS4,16, NOTE_G4,8,
  NOTE_A4,4, NOTE_FS4,8, NOTE_D4,4, NOTE_G4,8,
  
  NOTE_AS4,4, NOTE_C5,8, NOTE_D5,-8, NOTE_DS5,16, NOTE_D5,8,//6
  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,
  NOTE_AS4,-8, NOTE_A4,16, NOTE_G4,8, NOTE_FS4,-8, NOTE_E4,16, NOTE_FS4,8, 
  NOTE_G4,-2,
  NOTE_F5,2, NOTE_E5,16, NOTE_D5,8,

  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,//11
  NOTE_AS4,4, NOTE_G4,8, NOTE_G4,-8, NOTE_FS4,16, NOTE_G4,8,
  NOTE_A4,4, NOTE_FS4,8, NOTE_D4,04,
  NOTE_F5,2, NOTE_E5,16, NOTE_D5,8,
  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,

  NOTE_AS4,-8, NOTE_A4,16, NOTE_G4,8, NOTE_FS4,-8, NOTE_E4,16, NOTE_FS4,8,//16
  NOTE_G4,-2,

  //repeats from the beginning

  NOTE_G4,8,//1
  NOTE_AS4,4, NOTE_C5,8, NOTE_D5,-8, NOTE_DS5,16, NOTE_D5,8,
  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,
  NOTE_AS4,4, NOTE_G4,8, NOTE_G4,-8, NOTE_FS4,16, NOTE_G4,8,
  NOTE_A4,4, NOTE_FS4,8, NOTE_D4,4, NOTE_G4,8,
  
  NOTE_AS4,4, NOTE_C5,8, NOTE_D5,-8, NOTE_DS5,16, NOTE_D5,8,//6
  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,
  NOTE_AS4,-8, NOTE_A4,16, NOTE_G4,8, NOTE_FS4,-8, NOTE_E4,16, NOTE_FS4,8, 
  NOTE_G4,-2,
  NOTE_F5,2, NOTE_E5,16, NOTE_D5,8,

  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,//11
  NOTE_AS4,4, NOTE_G4,8, NOTE_G4,-8, NOTE_FS4,16, NOTE_G4,8,
  NOTE_A4,4, NOTE_FS4,8, NOTE_D4,04,
  NOTE_F5,2, NOTE_E5,16, NOTE_D5,8,
  NOTE_C5,4, NOTE_A4,8, NOTE_F4,-8, NOTE_G4,16, NOTE_A4,8,

  NOTE_AS4,-8, NOTE_A4,16, NOTE_G4,8, NOTE_FS4,-8, NOTE_E4,16, NOTE_FS4,8,//16
  NOTE_G4,-2
  
  
};
const uint16_t melodyLength3 = sizeof(rawMelody3)/sizeof(rawMelody3[0])/2;

const int tempo4 = 240; // sans
const int rawMelody4[] PROGMEM = {

NOTE_D4,8, NOTE_D4,8, NOTE_D5,4, NOTE_A4,-4,
NOTE_GS4,4, NOTE_G4,4, NOTE_F4,4, NOTE_D4,8,
NOTE_F4,8, NOTE_G4,8, NOTE_C4,16, NOTE_C4,16,

NOTE_C4,16, NOTE_C4,16, NOTE_D5,4, NOTE_A4,-4,
NOTE_GS4,4, NOTE_G4,4, NOTE_F4,4, NOTE_D4,8,
NOTE_F4,8, NOTE_G4,8,

NOTE_B3,8, NOTE_B3,8,NOTE_D5,4, NOTE_A4,-4,
NOTE_GS4,4, NOTE_G4,4,NOTE_F4,4, NOTE_D4,8, NOTE_F4,8, NOTE_G4,8,
NOTE_AS3,16, NOTE_AS3,16, 

NOTE_AS3,16, NOTE_AS3,16,NOTE_D5,4, NOTE_A4,-4,
NOTE_GS4,4, NOTE_G4,4,NOTE_F4,4, NOTE_D4,8, NOTE_F4,8, NOTE_G4,8,

NOTE_D5,8, NOTE_D5,8, NOTE_D6,4, NOTE_A5,-4,
NOTE_GS5,4, NOTE_G5,4, NOTE_F5,4, NOTE_D5,8,
NOTE_F5,8, NOTE_G5,8, NOTE_C5,8, NOTE_C5,8,
NOTE_D6,4, NOTE_A5,-4, NOTE_GS5,4, NOTE_G5,4,
NOTE_F5,4, NOTE_D5,8, NOTE_F5,8, NOTE_G5,8,
NOTE_B4,8, NOTE_B4,8, NOTE_D6,4, NOTE_A5,-4,
NOTE_GS5,4, NOTE_G5,4, NOTE_F5,4, NOTE_D5,8,
NOTE_F5,8, NOTE_G5,8, NOTE_AS4,8, NOTE_AS4,8,
NOTE_D6,4, NOTE_A5,-4, NOTE_GS5,4, NOTE_G5,4,
NOTE_F5,4, NOTE_D5,8, NOTE_F5,8, NOTE_G5,8,

};
const uint16_t melodyLength4 = sizeof(rawMelody4)/sizeof(rawMelody4[0])/2;

// ==================== MUSIC PLAYBACK SYSTEM ====================
// --- MUSIC PLAYBACK VARIABLES ---
uint16_t melodyNotes[128];        // Buffer for note frequencies
uint16_t melodyDurations[128];    // Buffer for note durations
uint16_t currentNoteIndex = 0;    // Current position in melody
uint16_t currentMelodyLength = 0; // Length of current melody
uint32_t lastNoteTime = 0;        // Last time a note changed
bool musicPlaying = false;        // Music playback state flag
bool canRestart = false;          // Anti-instant-restart flag




// ------------------------------------------------------
// CODES
// ------------------------------------------------------

// ==================== MELODY PREPARATION FUNCTION ====================
// Converts raw melody data (notes + durations) into playable format
void prepareMelody(uint8_t songIndex) {
  // Local variables to store song data
  const int* m;        // Pointer to raw melody array in PROGMEM
  uint16_t len;        // Length of melody (number of notes)
  int tempoUsed;       // Tempo (beats per minute) for this song
  
  // Song selection switch statement
  switch(songIndex) {
    case 0: 
      m = rawMelody1;          // Pointer to SONG1 melody data
      len = melodyLength1;     // Number of notes in Canon
      tempoUsed = tempo1;      // Canon's tempo (180 BPM)
      break;
    case 1: 
      m = rawMelody2;          // Pointer to SONG2 melody
      len = melodyLength2;     // Number of notes
      tempoUsed = tempo2;      // Tempo (144 BPM)
      break;
    case 2: 
      m = rawMelody3;          // Pointer to SONG3 melody
      len = melodyLength3;     // Number of notes
      tempoUsed = tempo3;      // Tempo
      break;
    case 3: 
      m = rawMelody4;          // Pointer to SONG4 melody
      len = melodyLength4;     // Number of notes
      tempoUsed = tempo4;      // Tempo (240 BPM)
      break;
  }
  
  // Store the melody length globally for playback reference
  currentMelodyLength = len;
  // Calculate duration of a whole note in milliseconds
  // Formula: (60000 ms/minute * 4 beats) / tempo (beats/minute)
  // This gives ms per whole note (4 beats)
  long wholenote = (60000L * 4) / tempoUsed;
  
  // Process each note in the melody
  for(uint16_t i = 0; i < len; i++) {
    // Read the duration divisor from PROGMEM
    // rawMelody format: [note1, duration1, note2, duration2, ...]
    // i*2+1 gives duration index (odd positions)
    int div = pgm_read_word(&m[i * 2 + 1]);
    
    // Calculate actual duration in milliseconds
    uint16_t dur;
    if(div > 0) {
      // Positive divisor: normal note
      // Example: div=4 means quarter note = wholenote/4
      dur = wholenote / div;
    } else {
      // Negative divisor: dotted note (1.5 times longer)
      // Example: div=-8 means dotted eighth note
      // abs(div) gets the base duration, *1.5 extends it
      dur = (wholenote / abs(div)) * 1.5;
    }
    
    // Safety limit: cap maximum duration at 2000ms (2 seconds)
    if(dur > 2000) dur = 2000;
    
    // Store the note frequency in playback buffer
    // i*2 gives note index (even positions)
    melodyNotes[i] = pgm_read_word(&m[i * 2]);
    
    // Store the calculated duration in playback buffer
    melodyDurations[i] = dur;
  }
  
  // Reset playback position to start of melody
  currentNoteIndex = 0;
}

// ==================== PWM MUSIC INITIALIZATION ====================
// Sets up Timer1 for PWM (Pulse Width Modulation) audio output
void initMusicPWM() {
  // Configure buzzer pin as output
  pinMode(BUZZER_MUSIC, OUTPUT);
  
  // TCCR1A: Timer1 Control Register A
  // _BV(COM1A1): Set bit COM1A1 (Compare Output Mode for channel A)
  // This enables PWM output on OC1A pin (Arduino pin 9)
  TCCR1A = _BV(COM1A1);
  
  // TCCR1B: Timer1 Control Register B
  // _BV(WGM13): Set bit WGM13 (Waveform Generation Mode)
  // _BV(CS11):  Set bit CS11 (Clock Select, prescaler = 8)
  // Mode 8: Phase and Frequency Correct PWM, TOP value in ICR1
  TCCR1B = _BV(WGM13) | _BV(CS11);
}

// ==================== START MUSIC PLAYBACK ====================
void startMusic() {
  musicPlaying = true;        // Set global flag to enable playback
  currentNoteIndex = 0;       // Start from first note
  lastNoteTime = millis();    // Record start time for first note
  
  // Ensure PWM output is enabled (might already be set in initMusicPWM)
  TCCR1A |= _BV(COM1A1);
  
  // Start playing the first note frequency
  setMusicFrequency(melodyNotes[0]);
}

// ==================== SET FREQUENCY FUNCTION ====================
// Changes PWM frequency to produce different musical notes
void setMusicFrequency(uint16_t f) {
  if (f == 0) {
    // Frequency 0 = silence (rest note)
    // Set compare register to 0 - no PWM output
    // This is better than digitalWrite(LOW) which would create clicks
    OCR1A = 0; 
    return;
  }
  
  // Calculate TOP value for Timer1 to generate desired frequency
  // Formula for Phase and Frequency Correct PWM mode:
  // f_pwm = f_cpu / (2 * N * TOP)
  // Where:
  //   f_cpu = Arduino clock frequency (16,000,000 Hz)
  //   N = prescaler (8, set by CS11 bit)
  //   TOP = value in ICR1 register
  // Rearranged: TOP = f_cpu / (2 * N * f)
  uint32_t top = (F_CPU / (2UL * 8UL * f));
  
  // Set the TOP value (determines frequency)
  ICR1 = top;
  
  // Set duty cycle to 50% (square wave)
  // OCR1A = TOP / 2 gives 50% high, 50% low
  OCR1A = top / 2;
}

// ==================== MUSIC PLAYBACK UPDATE FUNCTION ====================
// Non-blocking music playback - must be called regularly in main loop
void playMusic() {
  // Exit if music playback is not enabled
  if (!musicPlaying) return;

  // Get current time
  uint32_t now = millis();
  
  // Check if current note's duration has elapsed
  if (now - lastNoteTime >= melodyDurations[currentNoteIndex]) {
    // Move to next note in melody
    currentNoteIndex++;
    
    // Loop back to beginning if we reached the end
    if (currentNoteIndex >= currentMelodyLength)
      currentNoteIndex = 0;

    // Update note change time
    lastNoteTime = now;

    // Get frequency of next note
    uint16_t freq = melodyNotes[currentNoteIndex];

    // Change PWM frequency to play new note
    // No blocking delay() or digitalWrite() - runs in background
    setMusicFrequency(freq);
  }
}

// ==================== STOP MUSIC FUNCTION ====================
void stopMusic() {
  musicPlaying = false;    // Disable playback updates
  setMusicFrequency(0);    // Set frequency to 0 (silence)
}



// -------------------------------------------------------
// SONG SELECTION FUNCTION 
// -------------------------------------------------------
void checkSongSelectDuringEnd() {
  // Check if button for Song 1 (Lane 0) is pressed with debounce
  if (debouncedRead(0)) {
    prepareMelody(0);     // Load Song 1 (Canon) into playback buffers
    startMusic();         // Start playing the loaded melody
    
    // Reset all game state variables for fresh start
    state = PLAYING;               // Change game state to active gameplay
    score = combo = missedNotes = 0;  // Reset score, combo, and miss counter
    
    // Reset difficulty to initial values
    currentNoteSpeed = NOTE_SPEED;   // Reset note falling speed to default
    lastSpeedIncrease = millis();    // Reset speed increase timer
    
    // Clear all existing notes from the playfield
    for (uint8_t i = 0; i < MAX_NOTES; i++) notes[i].flags = 0;
    
    // Play confirmation sound (different pitch for each song)
    tone(BUZZER_SFX, 800, 150);   // 800Hz tone for 150ms
    delay(150);                   // Wait for tone to finish
    
    canRestart = false;           // Prevent immediate restart after game end
  }
  
  // Similar structure for Song 2 (Lane 1)
  if (debouncedRead(1)) {
    prepareMelody(1);     // Load Song 2 
    startMusic();         // Start playing
    state = PLAYING;      // Switch to gameplay state
    score = combo = missedNotes = 0;  // Reset stats
    currentNoteSpeed = NOTE_SPEED;    // Reset speed
    lastSpeedIncrease = millis();     // Reset timer
    for (uint8_t i = 0; i < MAX_NOTES; i++) notes[i].flags = 0;  // Clear notes
    tone(BUZZER_SFX, 950, 150);   // Different pitch: 950Hz
    delay(150);                   // Wait for tone
    canRestart = false;           // Reset restart flag
  }
  
  // Song 3 (Lane 2) selection
  if (debouncedRead(2)) {
    prepareMelody(2);     // Load Song 3 
    startMusic();         // Start playing
    state = PLAYING;      // Switch to gameplay
    score = combo = missedNotes = 0;  // Reset counters
    currentNoteSpeed = NOTE_SPEED;    // Reset speed
    lastSpeedIncrease = millis();     // Reset timer
    for (uint8_t i = 0; i < MAX_NOTES; i++) notes[i].flags = 0;  // Clear notes
    tone(BUZZER_SFX, 900, 150);   // 900Hz confirmation tone
    delay(150);                   // Wait for tone
    canRestart = false;           // Reset restart flag
  }
  
  // Song 4 (Lane 3) selection
  if (debouncedRead(3)) {
    prepareMelody(3);     // Load Song 4 
    startMusic();         // Start playing
    state = PLAYING;      // Switch to gameplay
    score = combo = missedNotes = 0;  // Reset all counters
    currentNoteSpeed = NOTE_SPEED;    // Reset note speed
    lastSpeedIncrease = millis();     // Reset speed increase timer
    for (uint8_t i = 0; i < MAX_NOTES; i++) notes[i].flags = 0;  // Clear all notes
    tone(BUZZER_SFX, 880, 150);   // 880Hz confirmation tone
    delay(150);                   // Wait for tone completion
    canRestart = false;           // Prevent instant restart
  }
}

// -------------------------------------------------------
// DISPLAY ROTATION FUNCTIONS (Converts vertical gameplay to horizontal display)
// -------------------------------------------------------
void setRotatedPixel(uint8_t r, uint8_t c, bool s) {
  // Swaps row and column coordinates to rotate the display 90 degrees
  // Original: (row, column) -> Display: (column, row)
  // This makes vertical gameplay (notes falling down) appear correctly
  display.setPoint(c, r, s);  // Set pixel at (column, row) to state s
}

void drawBitmap(const uint8_t bmp[]) {
  display.clear();  // Clear the entire LED matrix display
  
  // Loop through all 8 rows of the bitmap
  for(uint8_t r = 0; r < 8; r++) {
    // Read one row (8 bits) from program memory (PROGMEM)
    uint8_t row = pgm_read_byte(&bmp[r]);
    
    // Loop through all 8 columns in this row
    for(uint8_t c = 0; c < 8; c++)
      // Check if bit (7-c) is set (bit 7 is leftmost, bit 0 is rightmost)
      // The (1 << (7-c)) shifts a 1 to the appropriate position
      if(row & (1 << (7-c)))  // If pixel should be ON
        setRotatedPixel(r, c, true);  // Draw pixel with rotation
  }
  display.update();  // Send the buffer to the actual LED matrix
}

// -------------------------------------------------------
// DEBOUNCED BUTTON READING FUNCTION
// -------------------------------------------------------
bool debouncedRead(uint8_t lane) {
  // Check if button for given lane is pressed (LOW = pressed due to INPUT_PULLUP)
  if(digitalRead(buttonPins[lane]) == LOW) {
    uint32_t now = millis();  // Get current time in milliseconds
    
    // Check if enough time has passed since last valid press
    if(now - lastDebounce[lane] > DEBOUNCE_DELAY) {
      lastDebounce[lane] = now;  // Update last press time
      return true;               // Return true for valid button press
    }
  }
  return false;  // Button not pressed or still in debounce period
}

// -------------------------------------------------------
// NOTE SPAWNING FUNCTION
// -------------------------------------------------------
void spawnNote(uint8_t lane) {
  // Search for an inactive note slot in the notes array
  for(uint8_t i = 0; i < MAX_NOTES; i++)
    if(!noteActive(notes[i])) {  // Found an available note slot
      notes[i].lane = lane;   // Set which lane (0-3) this note belongs to
      notes[i].row = 0;       // Start at top row (row 0)
      notes[i].flags = 1;     // Set bit 0 (active flag) to 1, others to 0
      return;                 // Exit after spawning one note
    }
  // If loop completes, no available slots (shouldn't happen with proper MAX_NOTES)
}

// -------------------------------------------------------
// GAME RENDERING FUNCTION
// -------------------------------------------------------
void drawGame() {
  display.clear();  // Clear the display buffer
  
  // Draw all active notes
  for(uint8_t i = 0; i < MAX_NOTES; i++)
    if(noteActive(notes[i])) {  // Only draw if note is active
      uint8_t c = notes[i].lane * 2;  // Calculate column: each lane is 2 pixels wide
      setRotatedPixel(notes[i].row, c, true);      // Draw left pixel of note
      setRotatedPixel(notes[i].row, c + 1, true);  // Draw right pixel of note
    }
  
  // Draw hit zone markers (bottom row where notes should be hit)
  for(uint8_t l = 0; l < 4; l++) {
    uint8_t c = l * 2;  // Calculate column for each lane
    setRotatedPixel(HIT_END, c, true);      // Left marker pixel
    setRotatedPixel(HIT_END, c + 1, true);  // Right marker pixel
  }
  
  display.update();  // Send the complete frame to the display
}

// -------------------------------------------------------
// NOTE MOVEMENT FUNCTION
// -------------------------------------------------------
void moveNotes() {
  // Process all notes in the array
  for(uint8_t i = 0; i < MAX_NOTES; i++) {
    if(!noteActive(notes[i])) continue;  // Skip inactive notes
    
    notes[i].row++;  // Move note down by one row
    
    // Check if note passed the hit zone without being hit
    if(notes[i].row > HIT_END && !noteMissed(notes[i])) {
      setNoteMissed(notes[i], true);  // Mark note as missed
      missedNotes++;                  // Increment miss counter
      combo = 0;                      // Reset combo streak
      tone(BUZZER_SFX, 300, 100);     // Play miss sound (low pitch)
    }
    
    // Check if note moved off screen (below row 7)
    if(notes[i].row > 7)
      setNoteActive(notes[i], false);  // Deactivate the note
  }
}

// -------------------------------------------------------
// HIT PROCESSING FUNCTION
// -------------------------------------------------------
void hitNote(bool hit, Note &n) {
  if(!hit) return;  // Only process if actually a hit
  
  combo++;                    // Increase combo counter
  lastComboTime = millis();   // Record time of last successful hit
  
  // Calculate score based on combo level (tiered scoring system)
  int add = (combo >= 10) ? 5 :    // 10+ combo = 5 points per hit
            (combo >= 5) ? 2 :     // 5-9 combo = 2 points per hit
            1;                     // 0-4 combo = 1 point per hit
  score += add;                   // Add to total score
  
  // Determine pitch based on combo level (higher combo = higher pitch)
  int f = (combo >= 10) ? 1400 :   // 10+ combo = 1400Hz
          (combo >= 5) ? 1000 :    // 5-9 combo = 1000Hz
          600;                     // 0-4 combo = 600Hz
  
  tone(BUZZER_SFX, f, 80);        // Play hit sound with determined pitch
  setNoteActive(n, false);        // Remove the note from playfield
}

// -------------------------------------------------------
// HIT DETECTION FUNCTION
// -------------------------------------------------------
void checkHits() {
  // Check all 4 lanes for button presses
  for(uint8_t lane = 0; lane < 4; lane++) {
    if(!debouncedRead(lane)) continue;  // Skip if no button press in this lane
    
    int best = -1;   // Index of best note to hit (-1 = none found)
    int row = -1;    // Row position of best note (closest to bottom)
    
    // Search through all notes for hittable notes in this lane
    for(uint8_t i = 0; i < MAX_NOTES; i++)
      // Check if note is: active, in correct lane, within hit zone, and lower than current best
      if(noteActive(notes[i]) && 
         notes[i].lane == lane && 
         notes[i].row >= HIT_START && 
         notes[i].row <= HIT_END && 
         notes[i].row > row) {
        row = notes[i].row;  // Update best row position
        best = i;            // Update best note index
      }
    
    // If found a hittable note, process the hit
    if(best != -1)
      hitNote(true, notes[best]);  // Process successful hit
  }
  
  // Check for combo timeout (reset combo if too long between hits)
  if(combo > 0 && (millis() - lastComboTime) > COMBO_TIMEOUT)
    combo = 0;  // Reset combo streak
}

// -------------------------------------------------------
// VISUAL FEEDBACK FUNCTIONS (Flashing animations)
// -------------------------------------------------------

// ==================== SCORE FLASHER SYSTEM ====================
void flashScore(int s) {
  // Extract tens and ones digits from score
  uint8_t d1 = (s / 10) % 10;  // Tens digit
  uint8_t d2 = s % 10;         // Ones digit
  
  // Flash the score 4 times (on/off cycles)
  for(uint8_t f = 0; f < 4; f++) {
    display.clear();  // Clear display buffer
    
    // Draw both digits (6 rows tall, 4 columns wide each)
    for(uint8_t r = 0; r < 6; r++) {
      // Read bitmap rows for both digits from PROGMEM
      uint8_t L = pgm_read_byte(&smallNum[d1][r]);  // Left digit
      uint8_t R = pgm_read_byte(&smallNum[d2][r]);  // Right digit
      
      // Draw left digit (columns 0-3)
      for(uint8_t c = 0; c < 4; c++)
        if(L & (1 << (3 - c)))  // Check if pixel should be ON (bit 3 is leftmost)
          setRotatedPixel(r + 1, c, true);  // Draw with 1-row vertical offset
      
      // Draw right digit (columns 4-7)
      for(uint8_t c = 0; c < 4; c++)
        if(R & (1 << (3 - c)))  // Check if pixel should be ON
          setRotatedPixel(r + 1, c + 4, true);  // Offset by 4 columns
    }
    
    display.update();  // Show score
    delay(300);        // Keep score visible for 300ms
    
    display.clear();   // Clear screen
    display.update();  // Show blank screen
    delay(200);        // Pause between flashes
  }
}

// ==================== BITMAP FLASHER SYSTEM ====================
void flashBitmap(const uint8_t bmp[], int t) {
  // Flash a bitmap t times
  for(int i = 0; i < t; i++) {
    drawBitmap(bmp);   // Draw the bitmap
    delay(300);        // Keep visible
    display.clear();   // Clear screen
    display.update();  // Update display
    delay(200);        // Pause between flashes
  }
}

// ==================== SCORE FLASHER ANIMATION SYSTEM ====================
void flashNumber(int n, int t) {
  // Extract tens and ones digits
  uint8_t d1 = (n / 10) % 10;  // Tens digit
  uint8_t d2 = n % 10;         // Ones digit
  
  // Flash the number t times
  for(int f = 0; f < t; f++) {
    display.clear();  // Clear display buffer
    
    // Draw both digits
    for(uint8_t r = 0; r < 6; r++) {
      uint8_t L = pgm_read_byte(&smallNum[d1][r]);  // Left digit
      uint8_t R = pgm_read_byte(&smallNum[d2][r]);  // Right digit
      
      // Draw left digit
      for(uint8_t c = 0; c < 4; c++)
        if(L & (1 << (3 - c)))
          setRotatedPixel(r + 1, c, true);
      
      // Draw right digit
      for(uint8_t c = 0; c < 4; c++)
        if(R & (1 << (3 - c)))
          setRotatedPixel(r + 1, c + 4, true);
    }
    
    display.update();  // Show number
    delay(300);        // Keep visible
    display.clear();   // Clear screen
    display.update();  // Update display
    delay(200);        // Pause between flashes
  }
}

// -------------------------------------------------------
// MAIN MENU STATE HANDLER
// -------------------------------------------------------
void handleMenu() {
  drawBitmap(playSymbol);   // Display play button symbol
  stopMusic();              // Ensure no music is playing
  checkSongSelectDuringEnd();  // Check for song selection via buttons
}

// -------------------------------------------------------
// VICTORY STATE HANDLER
// -------------------------------------------------------
void handleWin() {
  stopMusic();  // Stop any background music
  
  // Static variables maintain their values between function calls
  static uint8_t step = 0;        // Current animation step (0-7)
  static uint32_t lastT = 0;      // Last animation update time
  
  uint32_t now = millis();        // Current time
  
  // Check for main menu button press (return to menu)
  if (digitalRead(BTN_MAIN) == LOW) {
    step = 0;                     // Reset animation step
    state = MENU;                 // Change to menu state
    score = combo = missedNotes = 0;  // Reset all game stats
    delay(200);                   // Debounce delay
    return;                       // Exit victory handler
  }
  
  // Animation timer: update every 600ms
  if (now - lastT > 600) {
    lastT = now;                  // Update last animation time
    display.clear();              // Clear display buffer
    
    // 8-step animation cycle (modulo 8 creates looping sequence)
    switch(step % 8) {
      case 0: drawBitmap(checkmarkSymbol); break;       // Show checkmark
      case 1: flashScore(score); break;                 // Flash final score
      case 2: flashBitmap(circleSymbol, 1); break;      // Flash circle (score symbol)
      case 3: flashNumber(score, 1); break;             // Flash score number
      case 4: flashBitmap(xSymbol, 1); break;           // Flash X (miss symbol)
      case 5: flashNumber(missedNotes, 1); break;       // Flash miss count
      case 6: drawBitmap(checkmarkSymbol); break;       // Show checkmark again
      case 7: display.clear(); display.update(); break; // Brief blank screen
    }
    step++;  // Advance to next animation step
  }
}

// -------------------------------------------------------
// GAME OVER STATE HANDLER
// -------------------------------------------------------
void handleGameOver() {
  stopMusic();  // Stop any background music
  
  // Animation state variables
  static uint8_t step = 0;        // Current animation step
  static uint32_t lastT = 0;      // Last animation update time
  
  uint32_t now = millis();        // Current time
  
  // Check for main menu button press
  if (digitalRead(BTN_MAIN) == LOW) {
    step = 0;                     // Reset animation
    state = MENU;                 // Return to menu
    score = combo = missedNotes = 0;  // Reset stats
    delay(200);                   // Debounce delay
    return;                       // Exit game over handler
  }
  
  // Animation timer: update every 600ms
  if (now - lastT > 600) {
    lastT = now;                  // Update last animation time
    display.clear();              // Clear display buffer
    
    // 8-step animation cycle (similar to victory but with skull)
    switch(step % 8) {
      case 0: drawBitmap(skullSymbol); break;           // Show skull symbol
      case 1: flashScore(score); break;                 // Flash final score
      case 2: flashBitmap(circleSymbol, 1); break;      // Flash circle (score)
      case 3: flashNumber(score, 1); break;             // Flash score number
      case 4: flashBitmap(xSymbol, 1); break;           // Flash X (misses)
      case 5: flashNumber(missedNotes, 1); break;       // Flash miss count
      case 6: drawBitmap(skullSymbol); break;           // Show skull again
      case 7: display.clear(); display.update(); break; // Brief blank screen
    }
    step++;  // Advance animation step
  }
}

// -------------------------------------------------------
// MAIN GAMEPLAY LOOP HANDLER
// -------------------------------------------------------
void handleGame() {
  uint32_t now = millis();  // Get current time
  
  playMusic();  // Update music playback (non-blocking)
  
  // Dynamic difficulty: increase speed every SPEED_INCREASE_INTERVAL
  if(now - lastSpeedIncrease > SPEED_INCREASE_INTERVAL) {
    lastSpeedIncrease = now;  // Reset speed increase timer
    
    // Only increase speed if not already at minimum
    if(currentNoteSpeed > MIN_NOTE_SPEED)
      // Reduce note movement time by SPEED_STEP, but not below MIN_NOTE_SPEED
      currentNoteSpeed = (currentNoteSpeed > SPEED_STEP) ? 
                        currentNoteSpeed - SPEED_STEP : 
                        MIN_NOTE_SPEED;
  }
  
  // Note movement timer: move notes based on current speed
  if(now - lastFall > currentNoteSpeed) {
    lastFall = now;        // Reset movement timer
    moveNotes();           // Move all notes down
    checkHits();           // Check for button hits
    
    // Win condition check
    if(score >= SCORE_TO_WIN) {
      state = GAME_WIN;                     // Change to victory state
      tone(BUZZER_SFX, 1200, 150);          // Victory sound part 1
      delay(150);                           // Wait for first tone
      tone(BUZZER_SFX, 1500, 200);          // Victory sound part 2
      return;                               // Exit gameplay loop
    }
    
    // Lose condition check
    if(missedNotes >= MAX_MISSES) {
      state = GAME_OVER;                    // Change to game over state
      tone(BUZZER_SFX, 300, 400);           // Game over sound (low pitch)
      return;                               // Exit gameplay loop
    }
  }
  
  // Note spawning timer: create new notes periodically
  if(now - lastSpawn > SPAWN_INTERVAL) {
    lastSpawn = now;        // Reset spawn timer
    
    int chance = random(0, 100);  // Generate random number 0-99
    
    // 80% chance: spawn single note in random lane
    if(chance < 80)
      spawnNote(random(0, 4));  // Random lane 0-3
    
    // 20% chance: spawn two notes in different lanes
    else {
      uint8_t l1 = random(0, 4);  // First random lane
      uint8_t l2 = random(0, 4);  // Second random lane
      
      // Ensure lanes are different
      while(l2 == l1)
        l2 = random(0, 4);  // Keep trying until different
      
      spawnNote(l1);  // Spawn first note
      spawnNote(l2);  // Spawn second note
    }
  }
  
  drawGame();  // Draw the current game state to display
}

// -------------------------------------------------------
// ARDUINO SETUP FUNCTION (Runs once at startup)
// -------------------------------------------------------
void setup() {
  display.begin();                     // Initialize LED matrix library
  display.control(MD_MAX72XX::INTENSITY, 5);  // Set brightness (0-15)
  
  // Configure button pins with internal pull-up resistors
  for(uint8_t i = 0; i < 4; i++)
    pinMode(buttonPins[i], INPUT_PULLUP);
  
  pinMode(BUZZER_SFX, OUTPUT);  // Set sound effects buzzer as output
  initMusicPWM();               // Initialize PWM for music playback
  
  // Initialize all notes as inactive
  for(uint8_t i = 0; i < MAX_NOTES; i++)
    notes[i].flags = 0;
  
  randomSeed(analogRead(0));    // Seed random number generator with analog noise
  
  pinMode(BTN_MAIN, INPUT_PULLUP);  // Configure main menu button
}

// -------------------------------------------------------
// ARDUINO MAIN LOOP (Runs continuously)
// -------------------------------------------------------
void loop() {
  // State machine: call appropriate handler based on current game state
  switch(state) {
    case MENU: handleMenu(); break;        // Main menu/song selection
    case PLAYING: handleGame(); break;     // Active gameplay
    case GAME_OVER: handleGameOver(); break;  // Game over screen
    case GAME_WIN: handleWin(); break;     // Victory screen
  }
}