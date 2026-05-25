#pragma once

#include <stdint.h>

// Logica jocului Minesweeper.
// Doua moduri: EASY (8x8, 10 mine) si HARD (10x12, 20 mine).
// Celula: 1 byte cu biti:
//   bit7 = mina
//   bit6 = descoperita
//   bit5 = flagged
//   bit3:0 = numar mine vecine (0-8)

namespace game {

// --- Dimensiuni moduri ---
constexpr uint8_t kEasyCols  = 8;
constexpr uint8_t kEasyRows  = 8;
constexpr uint8_t kEasyMines = 10;
constexpr uint8_t kEasyCellSize = 16; // px

constexpr uint8_t kHardCols  = 10;
constexpr uint8_t kHardRows  = 12;
constexpr uint8_t kHardMines = 20;
constexpr uint8_t kHardCellSize = 12; // px

// Offset Y pentru grilaL centrata vertical pe ecran 128x160
// EASY:  8 * 16 = 128px grid, offset = (160 - 128) / 2 = 16
// HARD: 12 * 12 = 144px grid, offset = (160 - 144) / 2 = 8
constexpr uint8_t kEasyOffsetY = 16;
constexpr uint8_t kHardOffsetY = 8;

// Offset X centrat orizontal
// EASY:  8 * 16 = 128px = exact latimea ecranului, offset = 0
// HARD: 10 * 12 = 120px, offset = (128 - 120) / 2 = 4
constexpr uint8_t kEasyOffsetX = 0;
constexpr uint8_t kHardOffsetX = 4;

constexpr uint8_t kMaskMine      = 0x80;
constexpr uint8_t kMaskRevealed  = 0x40;
constexpr uint8_t kMaskFlagged   = 0x20;
constexpr uint8_t kMaskNeighbors = 0x0F;

enum class Mode  : uint8_t { Easy, Hard };
enum class State : uint8_t { Menu, Playing, GameOver };
enum class Result: uint8_t { None, Win, Lose };

// Grid maxim pentru modul HARD (10x12)
extern uint8_t gGrid[kHardRows][kHardCols];
extern uint8_t gCols;
extern uint8_t gRows;
extern uint8_t gMines;
extern uint8_t gCellSize;
extern uint8_t gOffsetX;
extern uint8_t gOffsetY;
extern int8_t  gCursorX;
extern int8_t  gCursorY;
extern Mode    gMode;
extern State   gState;
extern Result  gResult;

// Initializeaza jocul (plasare mine + calcul vecini).
void init(Mode mode);

// Descopera celula la (r, c). Returneaza rezultatul.
Result reveal(int8_t r, int8_t c);

// Toggle flag pe celula curenta.
void toggleFlag(int8_t r, int8_t c);

// Verifica conditia de victorie.
Result checkWin();

// Muta cursorul (clamp la margini).
void moveCursor(int8_t dr, int8_t dc);

// Calculeaza coordonate ecran pentru celula (r, c).
int16_t cellX(int8_t c);
int16_t cellY(int8_t r);

} // namespace game
