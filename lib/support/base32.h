#ifndef base32_h
#define base32_h

#include <Arduino.h>

int toBase32(byte* in, long length, byte* out, bool usePadding);

#endif
