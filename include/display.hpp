#pragma once

#include <stdint.h>

// Driver minimal ST7735 128x160 pe hardware SPI AVR (registre directe).
//
// Pini (hardcodat conform schemei proiect):
//   SCK  = PB5
//   MOSI = PB3
//   CS   = PB2
//   DC   = PD7
//   RST  = PD6

namespace display {

// Culori RGB565 folosite in joc
constexpr uint16_t BLACK   = 0x0000;
constexpr uint16_t WHITE   = 0xFFFF;
constexpr uint16_t RED     = 0xF800;
constexpr uint16_t GREEN   = 0x07E0;
constexpr uint16_t BLUE    = 0x001F;
constexpr uint16_t YELLOW  = 0xFFE0;
constexpr uint16_t CYAN    = 0x07FF;
constexpr uint16_t MAGENTA = 0xF81F;
constexpr uint16_t GRAY    = 0x7BEF;
constexpr uint16_t DGRAY   = 0x39E7;
constexpr uint16_t LGRAY   = 0xC618;
constexpr uint16_t ORANGE  = 0xFD20;

// Initializeaza SPI si display-ul.
void init();

// Operatii de baza.
void fillScreen(uint16_t color);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawPixel(int16_t x, int16_t y, uint16_t color);
void drawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void drawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void fillTriangle(int16_t x0, int16_t y0,
                  int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2,
                  uint16_t color);
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

// Text — font 5x7, scaling intreg.
void setCursor(int16_t x, int16_t y);
void setTextColor(uint16_t color);
void setTextSize(uint8_t size);
void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t size);
void print(const char *str);
void printChar(char c);

// Dimensiuni ecran
constexpr int16_t WIDTH  = 128;
constexpr int16_t HEIGHT = 160;

} // namespace display
