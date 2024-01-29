
#include "base32.h"

int toBase32(byte* in, long length, byte* out, bool usePadding) {
  char base32StandardAlphabet[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"};
  char standardPaddingChar = '='; 

  int result = 0;
  int count = 0;
  int bufSize = 8;
  int index = 0;
  int size = 0; // size of temporary array
  //byte* temp = NULL;

  if (length < 0 || length > 268435456LL) 
  { 
    return 0;
  }

  //  size = 8 * ceil(length / 4.0); // Calculating size of temporary array. Not very precise.
  //temp = (byte*)malloc(size); // Allocating temporary array.

  if (length > 0)
  {
    int buffer = in[0];
    int next = 1;
    int bitsLeft = 8;
    
    while (count < bufSize && (bitsLeft > 0 || next < length))
    {
      if (bitsLeft < 5)
      {
        if (next < length)
        {
          buffer <<= 8;
          buffer |= in[next] & 0xFF;
          next++;
          bitsLeft += 8;
        }
        else
        {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      index = 0x1F & (buffer >> (bitsLeft -5));

      bitsLeft -= 5;
      out[result] = (byte)base32StandardAlphabet[index];
      result++;
    }
  }
  
  if (usePadding)
  {
    int pads = (result % 8);
    if (pads > 0)
    {
      pads = (8 - pads);
      for (int i = 0; i < pads; i++) 
      {
        out[result] = standardPaddingChar;
        result++;
      }
    }
  }

  return result;
}
