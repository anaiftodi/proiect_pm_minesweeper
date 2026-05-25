#include "input.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

namespace {

volatile uint32_t gMillis = 0;

uint8_t gPrevJoySw  = 1; // PD2 — meniu + game over
uint8_t gPrevReveal = 1; // PD3 — descopera celula in joc
uint8_t gPrevFlag   = 1; // PD4 — flag

uint32_t gLastJoySwMs  = 0;
uint32_t gLastRevealMs = 0;
uint32_t gLastFlagMs   = 0;

} // namespace

ISR(TIMER0_COMPA_vect) {
    ++gMillis;
}

uint32_t input::millis() {
    uint32_t m;
    uint8_t sreg = SREG;
    asm volatile("cli");
    m = gMillis;
    SREG = sreg;
    return m;
}

void input::init() {
    // Timer0: CTC, 1ms tick
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);
    sei();

    // ADC
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // PD2 — joystick SW, pull-up intern
    DDRD  &= ~(1 << DDD2);
    PORTD |=  (1 << PORTD2);

    // PD3 — buton reveal, pull-up extern
    DDRD  &= ~(1 << DDD3);
    PORTD &= ~(1 << PORTD3);

    // PD4 — buton flag, pull-up extern
    DDRD  &= ~(1 << DDD4);
    PORTD &= ~(1 << PORTD4);

    _delay_ms(50);

    gPrevJoySw  = (PIND >> PD2) & 1;
    gPrevReveal = (PIND >> PD3) & 1;
    gPrevFlag   = (PIND >> PD4) & 1;
}

uint16_t input::readAdc(uint8_t channel) {
    ADMUX = (1 << REFS0) | (channel & 0x07);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    uint8_t lo = ADCL;
    uint8_t hi = ADCH;
    return ((uint16_t)hi << 8) | lo;
}

input::Dir input::readJoystick() {
    uint16_t x = readAdc(0);
    uint16_t y = readAdc(1);

    if (x > kJoyCenter + kJoyThreshold) return Dir::Right;
    if (x < kJoyCenter - kJoyThreshold) return Dir::Left;
    if (y > kJoyCenter + kJoyThreshold) return Dir::Down;
    if (y < kJoyCenter - kJoyThreshold) return Dir::Up;
    return Dir::None;
}

// Joystick SW (PD2) — folosit in meniu si game over
bool input::selectPressed() {
    uint32_t now = millis();
    uint8_t curr = (PIND >> PD2) & 1;

    bool pressed = false;
    if (curr == 0 && gPrevJoySw == 1) {
        if ((now - gLastJoySwMs) >= kDebounceMs) {
            pressed = true;
            gLastJoySwMs = now;
        }
    }
    gPrevJoySw = curr;
    return pressed;
}

// Buton PD3 — descopera celula in joc
bool input::revealPressed() {
    uint32_t now = millis();
    uint8_t curr = (PIND >> PD3) & 1;

    bool pressed = false;
    if (curr == 0 && gPrevReveal == 1) {
        if ((now - gLastRevealMs) >= kDebounceMs) {
            pressed = true;
            gLastRevealMs = now;
        }
    }
    gPrevReveal = curr;
    return pressed;
}

// Buton PD4 — flag
bool input::flagPressed() {
    uint32_t now = millis();
    uint8_t curr = (PIND >> PD4) & 1;

    bool pressed = false;
    if (curr == 0 && gPrevFlag == 1) {
        if ((now - gLastFlagMs) >= kDebounceMs) {
            pressed = true;
            gLastFlagMs = now;
        }
    }
    gPrevFlag = curr;
    return pressed;
}