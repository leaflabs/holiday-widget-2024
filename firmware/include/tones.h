#ifndef __TONES_H
#define __TONES_H

/* Digitized Sine Wave used to synthesize output waveforms */
static const uint16_t sine_wave[32] = {
    0x800, 0x98F, 0xB0F, 0xC71, 0xDA7, 0xEA6, 0xF63, 0xFD8, 0xFFF, 0xFD8, 0xF63,
    0xEA6, 0xDA7, 0xC71, 0xB0F, 0x98F, 0x800, 0x670, 0x4F0, 0x38E, 0x258, 0x159,
    0x09C, 0x027, 0x000, 0x027, 0x09C, 0x159, 0x258, 0x38E, 0x4F0, 0x670};

/* Number of samples in the sine wave */
#define SINE_WAVE_SAMPLES sizeof(sine_wave) / sizeof(sine_wave[0])

/** Chromatic Scale Definitions **/

/* Octave 4 Chromatic Scale */
#define C4_NATURAL (int)(0x1F3 / 4)
#define C4_SHARP (int)(0x1D7 / 4)
#define D4_FLAT C4_SHARP
#define D4_NATURAL (int)(0x1BD / 4)
#define D4_SHARP (int)(0x1A4 / 4)
#define E4_FLAT D4_SHARP
#define E4_NATURAL (int)(0x18C / 4)
#define F4_NATURAL (int)(0x176 / 4)
#define F4_SHARP (int)(0x161 / 4)
#define G4_FLAT F4_SHARP
#define G4_NATURAL (int)(0x14D / 4)
#define G4_SHARP (int)(0x13A / 4)
#define A4_FLAT G4_SHARP
#define A4_NATURAL (int)(0x128 / 4)
#define A4_SHARP (int)(0x118 / 4)
#define B4_FLAT A4_SHARP
#define B4_NATURAL (int)(0x108 / 4)

/* Octave 3 Chromatic Scale */
#define C3_NATURAL (C4_NATURAL * 2)
#define C3_SHARP (C4_SHARP * 2)
#define D3_FLAT C3_SHARP
#define D3_NATURAL (D4_NATURAL * 2)
#define D3_SHARP (D4_SHARP * 2)
#define E3_FLAT D3_SHARP
#define E3_NATURAL (E4_NATURAL * 2)
#define F3_NATURAL (F4_NATURAL * 2)
#define F3_SHARP (F4_SHARP * 2)
#define G3_FLAT F3_SHARP
#define G3_NATURAL (G4_NATURAL * 2)
#define G3_SHARP (G4_SHARP * 2)
#define A3_FLAT G3_SHARP
#define A3_NATURAL (A4_NATURAL * 2)
#define A3_SHARP (A4_SHARP * 2)
#define B3_FLAT A3_SHARP
#define B3_NATURAL (B4_NATURAL * 2)

/* Octave 5 Chromatic Scale */
#define C5_NATURAL (C4_NATURAL / 2)
#define C5_SHARP (C4_SHARP / 2)
#define D5_FLAT C5_SHARP
#define D5_NATURAL (D4_NATURAL / 2)
#define D5_SHARP (D4_SHARP / 2)
#define E5_FLAT D5_SHARP
#define E5_NATURAL (E4_NATURAL / 2)
#define F5_NATURAL (F4_NATURAL / 2)
#define F5_SHARP (F4_SHARP / 2)
#define G5_FLAT F5_SHARP
#define G5_NATURAL (G4_NATURAL / 2)
#define G5_SHARP (G4_SHARP / 2)
#define A5_FLAT G5_SHARP
#define A5_NATURAL (A4_NATURAL / 2)
#define A5_SHARP (A4_SHARP / 2)
#define B5_FLAT A5_SHARP
#define B5_NATURAL (B4_NATURAL / 2)

/* Silence has MAX period value so that no audible tone is played */
#define SILENCE 0xFFFF

/* Tempo in BPM */
#define TEMPO 240

/* TIM3 Clock Cycles per Second */
#define CYCLES_PER_SECOND 2000.0 / 9.0

/* Duration of one Beat in TIM3 Clock Cycles */
#define BEAT_DURATION ((60.0 / TEMPO) * CYCLES_PER_SECOND)

/* Note Duration Definitions */
#define QUARTER_NOTE (int)(BEAT_DURATION)
#define HALF_NOTE (int)(BEAT_DURATION * 2.0)
#define WHOLE_NOTE (int)(BEAT_DURATION * 4.0)
#define EIGHTH_NOTE (int)(BEAT_DURATION / 2.0)

/* Song Enumeration */
enum Song {
    JINGLE_BELLS,
    WE_WISH_YOU_A_MERRY_CHRISTMAS,
    DECK_THE_HALLS,
    /* Additional songs go here */
    NO_SONG
};

/** Individual Song Look-Up Tables **/

/* Jingle Bells Look-Up Tables */
static const uint32_t JINGLE_BELLS_NOTES[] = {
    E4_NATURAL, SILENCE, E4_NATURAL, SILENCE, E4_NATURAL, SILENCE,
    E4_NATURAL, SILENCE, E4_NATURAL, SILENCE, E4_NATURAL, SILENCE,
    E4_NATURAL, SILENCE, G4_NATURAL, SILENCE, C4_NATURAL, SILENCE,
    D4_NATURAL, SILENCE, E4_NATURAL, SILENCE, F4_NATURAL, SILENCE,
    F4_NATURAL, SILENCE, F4_NATURAL, SILENCE, F4_NATURAL, SILENCE,
    F4_NATURAL, SILENCE, E4_NATURAL, SILENCE, E4_NATURAL, SILENCE,
    E4_NATURAL, SILENCE, E4_NATURAL, SILENCE, G4_NATURAL, SILENCE,
    G4_NATURAL, SILENCE, F4_NATURAL, SILENCE, D4_NATURAL, SILENCE,
    C4_NATURAL, SILENCE};

static const uint32_t JINGLE_BELLS_DURATIONS[] = {
    QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  HALF_NOTE,
    EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,
    HALF_NOTE,    EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  HALF_NOTE,    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    WHOLE_NOTE,   EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  HALF_NOTE,    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  HALF_NOTE,    EIGHTH_NOTE,  WHOLE_NOTE,   EIGHTH_NOTE};

/* We Wish You a Merry Christmas Look-Up Tables */
static const uint32_t WE_WISH_YOU_A_MERRY_CHRISTMAS_NOTES[] = {
    G4_NATURAL, SILENCE, C5_NATURAL, SILENCE, C5_NATURAL, SILENCE,
    D5_NATURAL, SILENCE, C5_NATURAL, SILENCE, B4_NATURAL, SILENCE,
    A4_NATURAL, SILENCE, A4_NATURAL, SILENCE, A4_NATURAL, SILENCE,
    D5_NATURAL, SILENCE, D5_NATURAL, SILENCE, E5_NATURAL, SILENCE,
    D5_NATURAL, SILENCE, C5_NATURAL, SILENCE, B4_NATURAL, SILENCE,
    G4_NATURAL, SILENCE, G4_NATURAL, SILENCE, E5_NATURAL, SILENCE,
    E5_NATURAL, SILENCE, F5_NATURAL, SILENCE, E5_NATURAL, SILENCE,
    D5_NATURAL, SILENCE, C5_NATURAL, SILENCE, A4_NATURAL, SILENCE,
    G4_NATURAL, SILENCE, G4_NATURAL, SILENCE, A4_NATURAL, SILENCE,
    D5_NATURAL, SILENCE, B4_NATURAL, SILENCE, C5_NATURAL, SILENCE};

static const uint32_t WE_WISH_YOU_A_MERRY_CHRISTMAS_DURATIONS[] = {
    QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,
    QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  HALF_NOTE,    EIGHTH_NOTE,
};

/* Deck the Halls Look-Up Tables */
static const uint32_t DECK_THE_HALLS_NOTES[] = {
    G4_NATURAL, SILENCE, F4_NATURAL, SILENCE, E4_NATURAL, SILENCE,
    D4_NATURAL, SILENCE, C4_NATURAL, SILENCE, D4_NATURAL, SILENCE,
    E4_NATURAL, SILENCE, C4_NATURAL, SILENCE, D4_NATURAL, SILENCE,
    E4_NATURAL, SILENCE, F4_NATURAL, SILENCE, D4_NATURAL, SILENCE,
    E4_NATURAL, SILENCE, D4_NATURAL, SILENCE, C4_NATURAL, SILENCE,
    B3_NATURAL, SILENCE, C4_NATURAL, SILENCE,
};

static const uint32_t DECK_THE_HALLS_DURATIONS[] = {
    QUARTER_NOTE, EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,
    QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE,
    EIGHTH_NOTE,  EIGHTH_NOTE,  EIGHTH_NOTE,  QUARTER_NOTE, EIGHTH_NOTE,
    QUARTER_NOTE, EIGHTH_NOTE,  HALF_NOTE,    EIGHTH_NOTE,
};

/** All Song Look-Up Table **/

/* Notes Look-Up Table */
static const uint32_t *NOTES[] = {
    JINGLE_BELLS_NOTES,
    WE_WISH_YOU_A_MERRY_CHRISTMAS_NOTES,
    DECK_THE_HALLS_NOTES,
};

/* Durations Look-Up Table */
static const uint32_t *DURATIONS[] = {
    JINGLE_BELLS_DURATIONS,
    WE_WISH_YOU_A_MERRY_CHRISTMAS_DURATIONS,
    DECK_THE_HALLS_DURATIONS,
};

/* Song Length Look-Up Table */
static const size_t SIZES[] = {
    sizeof(JINGLE_BELLS_NOTES) / sizeof(JINGLE_BELLS_NOTES[0]),
    sizeof(WE_WISH_YOU_A_MERRY_CHRISTMAS_NOTES) /
        sizeof(WE_WISH_YOU_A_MERRY_CHRISTMAS_NOTES[0]),
    sizeof(DECK_THE_HALLS_NOTES) / sizeof(DECK_THE_HALLS_NOTES[0]),
};

#endif