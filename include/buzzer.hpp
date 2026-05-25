#pragma once

#include <Arduino.h>

// Non-blocking buzzer driver pe Timer1 / OC1A (PB1 = pin 9 Arduino)
// Foloseste toggle OC1A in CTC mode — nu blocheaza loop().
//
// Folosire:
//   buzzer::playTone(440, 200);   // 440 Hz, 200 ms
//   buzzer::beepMove();           // sunet scurt la miscare cursor
//   buzzer::beepReveal();         // sunet la descoperire celula libera
//   buzzer::beepMine();           // sunet exploziv la mina
//   buzzer::beepWin();            // sunet victorie
//
//   // In loop():
//   buzzer::update();

namespace buzzer {

constexpr uint8_t kPin = 9; // PB1 / OC1A

// Porneste un ton la frecventa (Hz) pentru durata (ms).
// Daca durationMs == 0, suna continuu pana la stop().
void playTone(uint16_t freqHz, uint16_t durationMs = 0);

// Opreste sunetul imediat.
void stop();

// Apelat din loop() — opreste automat sunetul dupa durata specificata.
void update();

// Tonuri predefinite pentru evenimentele din joc.
void beepMove();    // tick scurt la mutare cursor
void beepReveal();  // ton placut la celula libera descoperita
void beepMine();    // ton jos, lung — explozie
void beepWin();     // secventa victoriei (non-blocanta, prima nota)

} // namespace buzzer
