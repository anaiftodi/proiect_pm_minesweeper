#pragma once

#include <stdint.h>

// Modul input: joystick pe ADC0/ADC1, butoane pe PD3(SELECT), PD4(FLAG), PB7(SW200).
// Fara Arduino.h — registre AVR directe + debouncing software.
//
// Joystick:
//   VRX = PC0 (ADC0)
//   VRY = PC1 (ADC1)
//
// Butoane:
//   SELECT = PD3  (pull-up extern 10k, activ LOW)
//   FLAG   = PD4  (pull-up extern 10k, activ LOW)
//   SW200  = PB7  (pull-up intern, activ LOW) — buton onboard fallback

namespace input {

constexpr uint16_t kJoyCenter    = 512;
constexpr uint16_t kJoyThreshold = 200;  // deadzone fata de centru
constexpr uint8_t  kDebounceMs   = 20;

enum class Dir : uint8_t { None, Up, Down, Left, Right };

// Initializeaza ADC si pinii de butoane.
void init();

// Citeste directia joystick-ului (cu threshold).
Dir readJoystick();

// Citeste raw ADC pe canalul dat (0 = X, 1 = Y). 0..1023.
uint16_t readAdc(uint8_t channel);

// Returneaza true o singura data per apasare (edge detection + debounce).
bool selectPressed();   // PD2 joystick SW — meniu + game over
bool revealPressed();   // PD3 buton extern — descopera celula in joc
bool flagPressed();     // PD4 buton extern — flag

// millis() propriu bazat pe Timer0 overflow (incrementat din ISR).
uint32_t millis();

} // namespace input