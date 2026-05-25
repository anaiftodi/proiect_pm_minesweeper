#include "game.hpp"

#include <avr/io.h>
#include <stdint.h>

// Seed pseudo-random din LCG
namespace {

uint16_t gSeed = 1;

void seedRandom() {
    // Citeste ADC6 (pin neconectat = zgomot termic)
    ADMUX  = (1 << REFS0) | 0x06;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    uint8_t lo = ADCL;
    uint8_t hi = ADCH;
    gSeed = ((uint16_t)hi << 8) | lo;
    if (gSeed == 0) gSeed = 42;
    // Reseteaza pe ADC0 pentru joystick
    ADMUX = (1 << REFS0) | 0x00;
}

uint16_t randNext() {
    gSeed = gSeed * 25173u + 13849u;
    return gSeed;
}

uint8_t randRange(uint8_t max) {
    return (uint8_t)((randNext() >> 8) % max);
}

} // namespace

// ---------------------------------------------------------------------------
// State global
// ---------------------------------------------------------------------------
namespace game {

uint8_t gGrid[kHardRows][kHardCols];
uint8_t gCols     = kEasyCols;
uint8_t gRows     = kEasyRows;
uint8_t gMines    = kEasyMines;
uint8_t gCellSize = kEasyCellSize;
uint8_t gOffsetX  = kEasyOffsetX;
uint8_t gOffsetY  = kEasyOffsetY;
int8_t  gCursorX  = 0;
int8_t  gCursorY  = 0;
Mode    gMode     = Mode::Easy;
State   gState    = State::Menu;
Result  gResult   = Result::None;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void init(Mode mode) {
    gMode = mode;

    if (mode == Mode::Easy) {
        gCols     = kEasyCols;
        gRows     = kEasyRows;
        gMines    = kEasyMines;
        gCellSize = kEasyCellSize;
        gOffsetX  = kEasyOffsetX;
        gOffsetY  = kEasyOffsetY;
    } else {
        gCols     = kHardCols;
        gRows     = kHardRows;
        gMines    = kHardMines;
        gCellSize = kHardCellSize;
        gOffsetX  = kHardOffsetX;
        gOffsetY  = kHardOffsetY;
    }

    for (uint8_t r = 0; r < gRows; ++r)
        for (uint8_t c = 0; c < gCols; ++c)
            gGrid[r][c] = 0;

    gCursorX = 0;
    gCursorY = 0;
    gResult  = Result::None;
    gState   = State::Playing;

    seedRandom();

    uint8_t placed = 0;
    while (placed < gMines) {
        uint8_t r = randRange(gRows);
        uint8_t c = randRange(gCols);
        if (!(gGrid[r][c] & kMaskMine)) {
            gGrid[r][c] |= kMaskMine;
            ++placed;
        }
    }

    // Calculeaza vecini
    for (uint8_t r = 0; r < gRows; ++r) {
        for (uint8_t c = 0; c < gCols; ++c) {
            if (gGrid[r][c] & kMaskMine) continue;
            uint8_t count = 0;
            for (int8_t dr = -1; dr <= 1; ++dr) {
                for (int8_t dc = -1; dc <= 1; ++dc) {
                    int8_t nr = (int8_t)r + dr;
                    int8_t nc = (int8_t)c + dc;
                    if (nr >= 0 && nr < (int8_t)gRows &&
                        nc >= 0 && nc < (int8_t)gCols &&
                        (gGrid[nr][nc] & kMaskMine)) ++count;
                }
            }
            gGrid[r][c] |= (count & kMaskNeighbors);
        }
    }
}

// ---------------------------------------------------------------------------
// Flood fill ITERATIV cu coada explicita pe SRAM
// Coada circulara de max kHardRows*kHardCols = 120 elemente
// Fiecare element = r*16 + c (packed in uint8_t, max cols=16)
// ---------------------------------------------------------------------------
static void floodFill(int8_t startR, int8_t startC) {
    // Coada statica — evita stack overflow
    // Max celule = 10*12 = 120, folosim uint8_t packed r<<4|c
    static uint8_t queue[120];
    uint8_t head = 0, tail = 0;

    auto enqueue = [&](int8_t r, int8_t c) {
        if (r < 0 || r >= (int8_t)gRows || c < 0 || c >= (int8_t)gCols) return;
        if (gGrid[r][c] & kMaskRevealed) return;
        if (gGrid[r][c] & kMaskFlagged)  return;
        if (gGrid[r][c] & kMaskMine)     return;
        gGrid[r][c] |= kMaskRevealed;
        queue[tail] = ((uint8_t)r << 4) | (uint8_t)c;
        tail = (tail + 1) % 120;
    };

    enqueue(startR, startC);

    while (head != tail) {
        uint8_t packed = queue[head];
        head = (head + 1) % 120;

        int8_t r = (int8_t)(packed >> 4);
        int8_t c = (int8_t)(packed & 0x0F);

        // Daca are vecini cu mine, ne oprim (nu propagam)
        if ((gGrid[r][c] & kMaskNeighbors) > 0) continue;

        for (int8_t dr = -1; dr <= 1; ++dr)
            for (int8_t dc = -1; dc <= 1; ++dc)
                if (dr != 0 || dc != 0)
                    enqueue(r + dr, c + dc);
    }
}

// ---------------------------------------------------------------------------
// Reveal
// ---------------------------------------------------------------------------
Result reveal(int8_t r, int8_t c) {
    uint8_t cell = gGrid[r][c];
    if (cell & kMaskRevealed) return Result::None;
    if (cell & kMaskFlagged)  return Result::None;

    if (cell & kMaskMine) {
        gGrid[r][c] |= kMaskRevealed;
        gResult = Result::Lose;
        gState  = State::GameOver;
        return Result::Lose;
    }

    floodFill(r, c);
    return checkWin();
}

// ---------------------------------------------------------------------------
// Toggle flag
// ---------------------------------------------------------------------------
void toggleFlag(int8_t r, int8_t c) {
    if (gGrid[r][c] & kMaskRevealed) return;
    gGrid[r][c] ^= kMaskFlagged;
}

// ---------------------------------------------------------------------------
// Verifica victorie
// ---------------------------------------------------------------------------
Result checkWin() {
    uint16_t total    = (uint16_t)gRows * gCols;
    uint16_t revealed = 0;
    for (uint8_t r = 0; r < gRows; ++r)
        for (uint8_t c = 0; c < gCols; ++c)
            if (gGrid[r][c] & kMaskRevealed) ++revealed;

    if (revealed == (total - gMines)) {
        gResult = Result::Win;
        gState  = State::GameOver;
        return Result::Win;
    }
    return Result::None;
}

// ---------------------------------------------------------------------------
// Miscare cursor
// ---------------------------------------------------------------------------
void moveCursor(int8_t dr, int8_t dc) {
    int8_t nx = gCursorX + dc;
    int8_t ny = gCursorY + dr;
    if (nx >= 0 && nx < (int8_t)gCols) gCursorX = nx;
    if (ny >= 0 && ny < (int8_t)gRows) gCursorY = ny;
}

// ---------------------------------------------------------------------------
// Coordonate ecran
// ---------------------------------------------------------------------------
int16_t cellX(int8_t c) {
    return (int16_t)gOffsetX + (int16_t)c * gCellSize;
}

int16_t cellY(int8_t r) {
    return (int16_t)gOffsetY + (int16_t)r * gCellSize;
}

} // namespace game