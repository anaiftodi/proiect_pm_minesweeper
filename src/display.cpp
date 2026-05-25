#include "display.hpp"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Pini
// ---------------------------------------------------------------------------
// SCK  = PB5, MOSI = PB3, CS = PB2  (SPI hardware)
// DC   = PD7
// RST  = PD6

#define CS_LOW()   (PORTB &= ~(1 << PB2))
#define CS_HIGH()  (PORTB |=  (1 << PB2))
#define DC_LOW()   (PORTD &= ~(1 << PD7))   // Command
#define DC_HIGH()  (PORTD |=  (1 << PD7))   // Data
#define RST_LOW()  (PORTD &= ~(1 << PD6))
#define RST_HIGH() (PORTD |=  (1 << PD6))

// ---------------------------------------------------------------------------
// Comenzi ST7735
// ---------------------------------------------------------------------------
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_INVOFF  0x20
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// ---------------------------------------------------------------------------
// Font 5x7 minimal (ASCII 32-126) stocat in flash
// ---------------------------------------------------------------------------
static const uint8_t PROGMEM gFont[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x08,0x2A,0x1C,0x2A,0x08}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x00,0x08,0x14,0x22,0x41}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x41,0x22,0x14,0x08,0x00}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x40,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x08,0x08,0x2A,0x1C,0x08}, // '~'
};

// ---------------------------------------------------------------------------
// SPI helpers (hardware SPI)
// ---------------------------------------------------------------------------
namespace {

static int16_t gCursorX = 0;
static int16_t gCursorY = 0;
static uint16_t gTextColor = 0xFFFF;
static uint8_t  gTextSize  = 1;

inline void spiWrite(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
}

void writeCmd(uint8_t cmd) {
    DC_LOW();
    CS_LOW();
    spiWrite(cmd);
    CS_HIGH();
}

void writeData(uint8_t data) {
    DC_HIGH();
    CS_LOW();
    spiWrite(data);
    CS_HIGH();
}

void writeData16(uint16_t data) {
    DC_HIGH();
    CS_LOW();
    spiWrite(data >> 8);
    spiWrite(data & 0xFF);
    CS_HIGH();
}

void setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    writeCmd(ST7735_CASET);
    DC_HIGH(); CS_LOW();
    spiWrite(0x00); spiWrite(x0);
    spiWrite(0x00); spiWrite(x1);
    CS_HIGH();

    writeCmd(ST7735_RASET);
    DC_HIGH(); CS_LOW();
    spiWrite(0x00); spiWrite(y0);
    spiWrite(0x00); spiWrite(y1);
    CS_HIGH();

    writeCmd(ST7735_RAMWR);
}

} // namespace

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void display::init() {
    // GPIO setup
    DDRB |= (1 << DDB5) | (1 << DDB3) | (1 << DDB2); // SCK, MOSI, CS output
    DDRD |= (1 << DDD7) | (1 << DDD6);                // DC, RST output

    CS_HIGH();
    RST_HIGH();

    // Hardware SPI: Master, Fosc/2 (8MHz), Mode 0
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR = (1 << SPI2X);

    // Reset hardware
    RST_LOW();
    _delay_ms(10);
    RST_HIGH();
    _delay_ms(120);

    // Secventa initializare ST7735 (tab negru / verde)
    writeCmd(ST7735_SWRESET); _delay_ms(150);
    writeCmd(ST7735_SLPOUT);  _delay_ms(500);

    writeCmd(ST7735_FRMCTR1);
    writeData(0x01); writeData(0x2C); writeData(0x2D);

    writeCmd(ST7735_FRMCTR2);
    writeData(0x01); writeData(0x2C); writeData(0x2D);

    writeCmd(ST7735_FRMCTR3);
    writeData(0x01); writeData(0x2C); writeData(0x2D);
    writeData(0x01); writeData(0x2C); writeData(0x2D);

    writeCmd(ST7735_INVCTR);  writeData(0x07);

    writeCmd(ST7735_PWCTR1);
    writeData(0xA2); writeData(0x02); writeData(0x84);

    writeCmd(ST7735_PWCTR2);  writeData(0xC5);

    writeCmd(ST7735_PWCTR3);
    writeData(0x0A); writeData(0x00);

    writeCmd(ST7735_PWCTR4);
    writeData(0x8A); writeData(0x2A);

    writeCmd(ST7735_PWCTR5);
    writeData(0x8A); writeData(0xEE);

    writeCmd(ST7735_VMCTR1);  writeData(0x0E);
    writeCmd(ST7735_INVOFF);

    writeCmd(ST7735_MADCTL);  writeData(0xC0); // Rotatie 180 grade (MY + MX)

    writeCmd(ST7735_COLMOD);  writeData(0x05); // 16-bit color

    writeCmd(ST7735_GMCTRP1);
    writeData(0x0F); writeData(0x1A); writeData(0x0F); writeData(0x18);
    writeData(0x2F); writeData(0x28); writeData(0x20); writeData(0x22);
    writeData(0x1F); writeData(0x1B); writeData(0x23); writeData(0x37);
    writeData(0x00); writeData(0x07); writeData(0x02); writeData(0x10);

    writeCmd(ST7735_GMCTRN1);
    writeData(0x0F); writeData(0x1B); writeData(0x0F); writeData(0x17);
    writeData(0x33); writeData(0x2C); writeData(0x29); writeData(0x2E);
    writeData(0x30); writeData(0x30); writeData(0x39); writeData(0x3F);
    writeData(0x00); writeData(0x07); writeData(0x03); writeData(0x10);

    writeCmd(ST7735_NORON);  _delay_ms(10);
    writeCmd(ST7735_DISPON); _delay_ms(100);
}

// ---------------------------------------------------------------------------
// Primitive grafice
// ---------------------------------------------------------------------------
void display::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    setAddrWindow(x, y, x, y);
    writeData16(color);
}

void display::fillScreen(uint16_t color) {
    fillRect(0, 0, WIDTH, HEIGHT, color);
}

void display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= WIDTH || y >= HEIGHT || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > WIDTH)  w = WIDTH  - x;
    if (y + h > HEIGHT) h = HEIGHT - y;

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    DC_HIGH(); CS_LOW();
    for (int32_t i = (int32_t)w * h; i > 0; --i) {
        spiWrite(hi);
        spiWrite(lo);
    }
    CS_HIGH();
}

void display::drawHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    fillRect(x, y, w, 1, color);
}

void display::drawVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    fillRect(x, y, 1, h, color);
}

void display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawHLine(x,         y,         w, color);
    drawHLine(x,         y + h - 1, w, color);
    drawVLine(x,         y,         h, color);
    drawVLine(x + w - 1, y,         h, color);
}

void display::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t x = r, y = 0, err = 1 - r;
    while (x >= y) {
        drawHLine(x0 - x, y0 + y, 2 * x + 1, color);
        drawHLine(x0 - x, y0 - y, 2 * x + 1, color);
        drawHLine(x0 - y, y0 + x, 2 * y + 1, color);
        drawHLine(x0 - y, y0 - x, 2 * y + 1, color);
        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            --x;
            err += 2 * (y - x + 1);
        }
    }
}

void display::fillTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           uint16_t color) {
    // Sort vertices by Y
    if (y0 > y1) { int16_t t; t=y0;y0=y1;y1=t; t=x0;x0=x1;x1=t; }
    if (y1 > y2) { int16_t t; t=y1;y1=y2;y2=t; t=x1;x1=x2;x2=t; }
    if (y0 > y1) { int16_t t; t=y0;y0=y1;y1=t; t=x0;x0=x1;x1=t; }

    if (y0 == y2) return;

    int16_t dx01 = x1 - x0, dy01 = y1 - y0;
    int16_t dx02 = x2 - x0, dy02 = y2 - y0;
    int16_t dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    int16_t last = (y1 == y2) ? y1 : y1 - 1;

    for (int16_t y = y0; y <= last; ++y) {
        int16_t a = x0 + sa / dy01;
        int16_t b = x0 + sb / dy02;
        sa += dx01; sb += dx02;
        if (a > b) { int16_t t = a; a = b; b = t; }
        drawHLine(a, y, b - a + 1, color);
    }
    sa = (int32_t)dx12 * (y0 - y1);  // reset
    sb = (int32_t)dx02 * (y0 - y1);  // reset (continua din dy02)
    // Recalculeaza sa/sb de la y1
    sa = 0; sb = (int32_t)dx02 * (y1 - y0);
    for (int16_t y = y1; y <= y2; ++y) {
        int16_t a = x1 + sa / (dy12 == 0 ? 1 : dy12);
        int16_t b = x0 + sb / (dy02 == 0 ? 1 : dy02);
        sa += dx12; sb += dx02;
        if (a > b) { int16_t t = a; a = b; b = t; }
        drawHLine(a, y, b - a + 1, color);
    }
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------
void display::setCursor(int16_t x, int16_t y) { gCursorX = x; gCursorY = y; }
void display::setTextColor(uint16_t color)     { gTextColor = color; }
void display::setTextSize(uint8_t size)        { gTextSize = size ? size : 1; }

void display::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t size) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *bitmap = gFont[c - 32];
    for (uint8_t col = 0; col < 5; ++col) {
        uint8_t line = pgm_read_byte(bitmap + col);
        for (uint8_t row = 0; row < 7; ++row) {
            if (line & (1 << row)) {
                if (size == 1) {
                    drawPixel(x + col, y + row, color);
                } else {
                    fillRect(x + col * size, y + row * size, size, size, color);
                }
            }
        }
    }
}

void display::printChar(char c) {
    if (c == '\n') {
        gCursorX  = 0;
        gCursorY += 8 * gTextSize;
        return;
    }
    drawChar(gCursorX, gCursorY, c, gTextColor, gTextSize);
    gCursorX += 6 * gTextSize;
}

void display::print(const char *str) {
    while (*str) {
        printChar(*str++);
    }
}