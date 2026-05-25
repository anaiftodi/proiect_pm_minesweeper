#include "display.hpp"
#include "input.hpp"
#include "game.hpp"
#include "buzzer.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Utilitare text numeric
// ---------------------------------------------------------------------------
namespace {

// Converteste uint8 la sir de caractere in buf (minim 4 bytes)
void u8toa(uint8_t val, char *buf) {
    if (val >= 100) { buf[0] = '0' + val / 100; buf[1] = '0' + (val % 100) / 10; buf[2] = '0' + val % 10; buf[3] = '\0'; }
    else if (val >= 10) { buf[0] = '0' + val / 10; buf[1] = '0' + val % 10; buf[2] = '\0'; }
    else { buf[0] = '0' + val; buf[1] = '\0'; }
}

// ---------------------------------------------------------------------------
// Randare celula individuala
// ---------------------------------------------------------------------------
void drawCell(int8_t r, int8_t c, bool cursor) {
    int16_t x = game::cellX(c);
    int16_t y = game::cellY(r);
    uint8_t sz = game::gCellSize;
    uint8_t cell = game::gGrid[r][c];

    if (cell & game::kMaskRevealed) {
        display::fillRect(x, y, sz, sz, display::BLACK);
        display::drawRect(x, y, sz, sz, display::DGRAY);

        if (cell & game::kMaskMine) {
            display::fillRect(x, y, sz, sz, display::RED);
            display::drawRect(x, y, sz, sz, display::DGRAY);
            display::fillCircle(x + sz/2, y + sz/2, sz/4, display::BLACK);
        } else {
            uint8_t n = cell & game::kMaskNeighbors;
            if (n > 0) {
                // Culori standard Minesweeper
                uint16_t col = display::WHITE;
                if      (n == 1) col = display::BLUE;
                else if (n == 2) col = display::GREEN;
                else if (n == 3) col = display::RED;
                else if (n == 4) col = display::MAGENTA;
                else if (n == 5) col = display::ORANGE;
                else             col = display::CYAN;

                char buf[2] = { (char)('0' + n), '\0' };
                display::setTextColor(col);
                display::setTextSize(1);
                // Centreaza numarul in celula (font 5x7, char width 6)
                display::setCursor(x + (sz - 5) / 2, y + (sz - 7) / 2);
                display::print(buf);
            }
        }
    } else {
        // Celula acoperita: efect 3D simplu
        display::fillRect(x, y, sz, sz, display::LGRAY);
        display::drawHLine(x,        y,        sz, display::WHITE); // top highlight
        display::drawVLine(x,        y,        sz, display::WHITE); // left highlight
        display::drawHLine(x,        y + sz-1, sz, display::DGRAY); // bottom shadow
        display::drawVLine(x + sz-1, y,        sz, display::DGRAY); // right shadow

        if (cell & game::kMaskFlagged) {
            // Flag: triunghi rosu mic
            display::fillTriangle(
                x + sz/4,     y + sz/4,
                x + sz/4,     y + 3*sz/4,
                x + 3*sz/4,   y + sz/2,
                display::RED
            );
        }
    }

    // Cursor galben (doua dreptunghiuri concentrice)
    if (cursor) {
        display::drawRect(x,   y,   sz,   sz,   display::YELLOW);
        if (sz >= 4) {
            display::drawRect(x+1, y+1, sz-2, sz-2, display::YELLOW);
        }
    }
}

void drawBoard() {
    for (int8_t r = 0; r < (int8_t)game::gRows; ++r) {
        for (int8_t c = 0; c < (int8_t)game::gCols; ++c) {
            drawCell(r, c, (r == game::gCursorY && c == game::gCursorX));
        }
    }
}

// ---------------------------------------------------------------------------
// Ecran meniu
// ---------------------------------------------------------------------------
void drawMenu(uint8_t sel) {
    display::fillScreen(display::BLACK);

    display::setTextSize(2);
    display::setTextColor(display::YELLOW);
    display::setCursor(0, 20);
    display::print("MINESWEEPER");

    display::setTextSize(1);
    display::setTextColor(display::WHITE);
    display::setCursor(28, 72);
    display::print("Select Mode:");

    // EASY
    display::setCursor(30, 92);
    if (sel == 0) {
        display::setTextColor(display::GREEN);
        display::print("> EASY <");
    } else {
        display::setTextColor(display::GRAY);
        display::print("  EASY  ");
    }

    // HARD
    display::setCursor(30, 110);
    if (sel == 1) {
        display::setTextColor(display::RED);
        display::print("> HARD <");
    } else {
        display::setTextColor(display::GRAY);
        display::print("  HARD  ");
    }

    // Prompt
    display::setTextColor(display::DGRAY);
    display::setCursor(8, 146);
    display::print("Press SW to start");
}

// ---------------------------------------------------------------------------
// Ecran game over / victorie
// ---------------------------------------------------------------------------
void drawGameOver(bool win) {
    // Fundalul jocului ramane, suprapunem un banner central
    // Stergem zona banner (centrat vertical pe ecran 160px)
    int16_t bx = 8, by = 58, bw = 112, bh = 44;
    display::fillRect(bx, by, bw, bh, display::BLACK);

    if (win) {
        display::drawRect(bx, by, bw, bh, display::GREEN);
        display::setTextSize(2);
        display::setTextColor(display::GREEN);
        display::setCursor(18, 72);
        display::print("VICTORY!");
    } else {
        display::drawRect(bx, by, bw, bh, display::RED);
        display::setTextSize(2);
        display::setTextColor(display::RED);
        display::setCursor(11, 72);
        display::print("GAME OVER");
    }

    display::setTextSize(1);
    display::setTextColor(display::DGRAY);
    display::setCursor(8, 146);
    display::print("Press SW to continue");
}

// Melodie mica la start joc (non-blocanta — doar prima nota, restul in loop)
// Folosim o secventa simpla cu timing
struct TuneNote { uint16_t freq; uint16_t ms; };
static const TuneNote kStartTune[] = {
    {880, 120}, {1108, 120}, {1319, 200}
};
static uint8_t  gTuneIdx = 0;
static uint32_t gTuneNext = 0;
static bool     gTunePlaying = false;

void startTune() {
    gTuneIdx     = 0;
    gTunePlaying = true;
    gTuneNext    = input::millis();
}

void updateTune() {
    if (!gTunePlaying) return;
    uint32_t now = input::millis();
    if ((int32_t)(now - gTuneNext) < 0) return;
    if (gTuneIdx >= 3) {
        gTunePlaying = false;
        buzzer::stop();
        return;
    }
    buzzer::playTone(kStartTune[gTuneIdx].freq, kStartTune[gTuneIdx].ms);
    gTuneNext = now + kStartTune[gTuneIdx].ms + 30; // 30ms pauza intre note
    ++gTuneIdx;
}

} // namespace

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------
static uint8_t gMenuSel = 0;
static uint32_t gLastJoyMs = 0;
static input::Dir gLastDir = input::Dir::None;

static void enterMenu() {
    game::gState = game::State::Menu;
    drawMenu(gMenuSel);
}

static void enterGame(game::Mode mode) {
    // Asteptam release buton — altfel selectPressed() triggereaza din nou
    while (!((PIND >> PD2) & 1)) {
        _delay_ms(1);
    }
    _delay_ms(50); // debounce

    display::fillScreen(display::BLACK);
    display::setTextSize(1);
    display::setTextColor(display::CYAN);
    display::setCursor(20, 72);
    display::print("Generating map...");

    game::init(mode);

    // Reseteaza ADC pe canalul 0 dupa seedRandom() care l-a lasat pe ADC6
    ADMUX = (1 << REFS0) | 0x00;

    drawBoard();
    startTune();
}

static void handleMenu() {
    input::Dir dir = input::readJoystick();

    if (dir != gLastDir) {
        if (dir == input::Dir::Down && gMenuSel == 0) {
            gMenuSel = 1;
            drawMenu(gMenuSel);
            buzzer::beepMove();
        } else if (dir == input::Dir::Up && gMenuSel == 1) {
            gMenuSel = 0;
            drawMenu(gMenuSel);
            buzzer::beepMove();
        }
        gLastDir = dir;
    }

    if (input::selectPressed()) {
        game::Mode mode = (gMenuSel == 0) ? game::Mode::Easy : game::Mode::Hard;
        enterGame(mode);
    }
}

static void handlePlaying() {
    uint32_t now = input::millis();
    input::Dir dir = input::readJoystick();

    // Miscare cursor cu repetare dupa 200ms
    bool move = false;
    if (dir != input::Dir::None) {
        if (dir != gLastDir) {
            gLastDir   = dir;
            gLastJoyMs = now;
            move       = true;
        } else if ((uint32_t)(now - gLastJoyMs) >= 200) {
            gLastJoyMs = now;
            move       = true;
        }
    } else {
        gLastDir = input::Dir::None;
    }

    if (move) {
        int8_t oldX = game::gCursorX;
        int8_t oldY = game::gCursorY;

        int8_t dr = 0, dc = 0;
        if      (dir == input::Dir::Up)    dr = -1;
        else if (dir == input::Dir::Down)  dr =  1;
        else if (dir == input::Dir::Left)  dc = -1;
        else if (dir == input::Dir::Right) dc =  1;

        game::moveCursor(dr, dc);

        if (game::gCursorX != oldX || game::gCursorY != oldY) {
            drawCell(oldY, oldX, false);
            drawCell(game::gCursorY, game::gCursorX, true);
            buzzer::beepMove();
        }
    }

    // REVEAL — descopera celula (buton extern PD3)
    if (input::revealPressed()) {
        int8_t r = game::gCursorY;
        int8_t c = game::gCursorX;
        uint8_t cell = game::gGrid[r][c];

        if (!(cell & game::kMaskFlagged) && !(cell & game::kMaskRevealed)) {
            if (cell & game::kMaskMine) {
                // Explozie — descopera toate minele
                game::gGrid[r][c] |= game::kMaskRevealed;
                buzzer::beepMine();
                // Arata toate minele
                for (int8_t rr = 0; rr < (int8_t)game::gRows; ++rr) {
                    for (int8_t cc = 0; cc < (int8_t)game::gCols; ++cc) {
                        if (game::gGrid[rr][cc] & game::kMaskMine) {
                            game::gGrid[rr][cc] |= game::kMaskRevealed;
                        }
                    }
                }
                drawBoard();
                game::gResult = game::Result::Lose;
                game::gState  = game::State::GameOver;
                drawGameOver(false);
            } else {
                buzzer::beepReveal();
                game::reveal(r, c);
                // Redeseneaza celulele modificate de flood fill
                for (int8_t rr = 0; rr < (int8_t)game::gRows; ++rr) {
                    for (int8_t cc = 0; cc < (int8_t)game::gCols; ++cc) {
                        if (game::gGrid[rr][cc] & game::kMaskRevealed) {
                            drawCell(rr, cc, (rr == game::gCursorY && cc == game::gCursorX));
                        }
                    }
                }
                if (game::gState == game::State::GameOver) {
                    buzzer::beepWin();
                    drawGameOver(true);
                }
            }
        }
    }

    // FLAG — marcheaza/demarcheaza mina
    if (input::flagPressed()) {
        game::toggleFlag(game::gCursorY, game::gCursorX);
        drawCell(game::gCursorY, game::gCursorX, true);
    }
}

static void handleGameOver() {
    if (input::selectPressed()) {
        // Asteptam release
        while (!((PIND >> PD2) & 1)) {
            _delay_ms(1);
        }
        _delay_ms(50);
        enterMenu();
    }
}

// ---------------------------------------------------------------------------
// Setup & Loop 
// ---------------------------------------------------------------------------
extern "C" void setup() {
    display::init();
    input::init();

    enterMenu();
}

extern "C" void loop() {
    updateTune();
    buzzer::update();

    switch (game::gState) {
        case game::State::Menu:
            handleMenu();
            break;
        case game::State::Playing:
            handlePlaying();
            break;
        case game::State::GameOver:
            handleGameOver();
            break;
    }
}


