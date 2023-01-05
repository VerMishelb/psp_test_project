#ifndef _Common_h_
#define _Common_h_

typedef float f32;

/// @param val 
/// @return The closest power of two >= val
unsigned int pow2(const unsigned int val) {
    unsigned int powerOfTwo = 1;
    while (powerOfTwo < val) {
        powerOfTwo <<= 1;
    }
    return powerOfTwo;
}

#endif
