/**
 * NDIPTYPE.H
 */

#ifndef _NDIPTYPE_H_
#define _NDIPTYPE_H_

#ifdef _WIN32
    #ifdef _WIN64
        #include <sys/types.h>
        #include <stdint.h>
    #else
        #include <sys/types.h>
        #include <stdint.h>
    #endif
#elif __APPLE__
    #include <sys/types.h>
    #include <stdint.h>
#elif __linux__
    #include <sys/types.h>
    #include <stdint.h>
#elif __unix__
    #include <sys/types.h>
    #include <stdint.h>
#elif defined(_POSIX_VERSION)    
    #include <sys/types.h>
    #include <stdint.h>
#else
// Assumes DOS 16-bit
    #include "TYPES.H"
#endif

// Reference: https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func

//! Byte swap unsigned short
inline uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

//! Byte swap short
inline int16_t swap_int16(int16_t val) {
    return (val << 8) | ((val >> 8) & 0xFF);
}

//! Byte swap unsigned int
inline uint32_t swap_uint32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF); 
    return (val << 16) | (val >> 16);
}

//! Byte swap int
inline int32_t swap_int32(int32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF); 
    return (val << 16) | ((val >> 16) & 0xFFFF);
}



#endif