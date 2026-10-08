#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

#define IRAM_ATTR
#define LOW 0
#define HIGH 1
#define INPUT 0x01
#define INPUT_PULLUP 0x05
#define FALLING 0x02
#define ADC_11db 3

typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) (void)(m)
#define portEXIT_CRITICAL(m) (void)(m)
#define portENTER_CRITICAL_ISR(m) (void)(m)
#define portEXIT_CRITICAL_ISR(m) (void)(m)

namespace sim {
extern uint64_t nowUs;
extern int pinLevel[40];
extern float pinMv[40];
extern void (*isr[40])();
float adcReadMv(uint8_t pin);
}

inline uint32_t micros() { return (uint32_t)sim::nowUs; }
inline uint32_t millis() { return (uint32_t)(sim::nowUs / 1000); }
inline void delay(uint32_t ms) { sim::nowUs += (uint64_t)ms * 1000; }
inline int digitalRead(uint8_t pin) { return sim::pinLevel[pin]; }
inline void pinMode(uint8_t, uint8_t) {}
inline uint32_t analogReadMilliVolts(uint8_t pin) { return (uint32_t)sim::adcReadMv(pin); }
inline void analogReadResolution(int) {}
inline void analogSetAttenuation(int) {}
inline uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }
inline void attachInterrupt(uint8_t pin, void (*fn)(), int) { sim::isr[pin] = fn; }

template <class A, class B> auto min(A a, B b) { return a < b ? a : b; }
template <class A, class B> auto max(A a, B b) { return a > b ? a : b; }
template <class T, class L, class H> T constrain(T x, L lo, H hi) { return x < lo ? lo : x > hi ? hi : x; }
