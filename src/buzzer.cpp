#include "buzzer.hpp"
#include "input.hpp"

#include <avr/io.h>
#include <stdint.h>

namespace buzzer {

namespace {

uint32_t gStopAtMs = 0;
bool     gActive   = false;

void timerStart(uint16_t freqHz) {
    if (freqHz == 0) {
        stop();
        return;
    }

    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;

    // Calculeaza cel mai bun prescaler dintre {1, 8, 64, 256, 1024}.
    // Frecventa reala = F_CPU / (2 * prescaler * (OCR1A + 1))
    const uint32_t fcpu = F_CPU;
    struct { uint16_t div; uint8_t cs; } prescalers[] = {
        {1,    (1 << CS10)},
        {8,    (1 << CS11)},
        {64,   (1 << CS11) | (1 << CS10)},
        {256,  (1 << CS12)},
        {1024, (1 << CS12) | (1 << CS10)},
    };

    uint8_t  bestCs  = (1 << CS12) | (1 << CS10); // fallback /1024
    uint16_t bestOcr = 0;

    for (uint8_t i = 0; i < 5; ++i) {
        const uint32_t ocr32 = fcpu / (2UL * prescalers[i].div * (uint32_t)freqHz);
        if (ocr32 == 0) continue;
        const uint32_t ocr = ocr32 - 1UL;
        if (ocr <= 0xFFFFUL) {
            bestCs  = prescalers[i].cs;
            bestOcr = static_cast<uint16_t>(ocr);
            break;
        }
    }

    // PB1 (OC1A) ca output
    DDRB  |= (1 << DDB1);

    // CTC mode (WGM12), toggle OC1A la compare match
    TCCR1A = (1 << COM1A0);
    TCCR1B = (1 << WGM12) | bestCs;
    OCR1A  = bestOcr;
    TCNT1  = 0;

    gActive = true;
}

} // namespace

void playTone(uint16_t freqHz, uint16_t durationMs) {
    timerStart(freqHz);
    gStopAtMs = (durationMs > 0) ? (input::millis() + durationMs) : 0;
}

void stop() {
    TCCR1A = 0;
    TCCR1B = 0;
    PORTB &= ~(1 << PORTB1);
    gActive   = false;
    gStopAtMs = 0;
}

void update() {
    if (!gActive || gStopAtMs == 0) {
        return;
    }
    if ((int32_t)(input::millis() - gStopAtMs) >= 0) {
        stop();
    }
}

void beepMove() {
    playTone(1200, 30);
}

void beepReveal() {
    playTone(880, 80);
}

void beepMine() {
    playTone(120, 600);
}

void beepWin() {
    playTone(1760, 400);
}

} // namespace buzzer