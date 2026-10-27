#pragma once
#include <cstdint>
#include <cstddef>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
void pinMode(int, int);
void digitalWrite(int, int);
int digitalRead(int);
uint32_t millis();
void delay(uint32_t);
void delayMicroseconds(uint32_t);
inline void yield() {}
struct SerialStub {
  void println(const char *) {}
  template <typename... T> void printf(const char *, T...) {}
};
inline SerialStub Serial;
