#ifndef _Debug_h_
#define _Debug_h_

#include <stdio.h>

#define DEBUG_LEVEL 1

#define VME_PRINT(fmt, args...) fprintf(stdout, "[VME] " fmt, ##args)
#define VME_PRINT(fmt) fprintf(stdout, "[VME] " fmt)

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL > 0
    #define DBG_PRINT(fmt, args...) fprintf(stdout, "[VME:%d Debug] %s:%d, %s: " fmt, DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##args)
    #define DBG_PRINT(fmt)          fprintf(stdout, "[VME:%d Debug] %s:%d, %s: " fmt, DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__)
    
    #define DBG_INFO(fmt, args...) fprintf(stdout, "[VME:%d Info] %s:%d, %s: " fmt, DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##args)
    #define DBG_INFO(fmt)          fprintf(stdout, "[VME:%d Info] %s:%d, %s: " fmt, DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__)
    
    #define DBG_ERROR(fmt, args...) fprintf(stdout, "[VME:%d Error] %s:%d, %s: " fmt, DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##args)
    #define DBG_ERROR(fmt)          fprintf(stdout, "[VME:%d Error] %s:%d, %s: " fmt, DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__)
    //Usage: DBG_PRINT_VAL("%i", vInt);
    #define DBG_PRINT_VAL(fmt, val) fprintf(stdout, "[VME:%d Value] %s:%d, %s: %s=" fmt "\n", DEBUG_LEVEL, __FILE__, __LINE__, __PRETTY_FUNCTION__, #val, val)

    #define DBG_PRINT_MEM(p) _dbg_print_mem((p), sizeof(*(p)))

#else
    #define DBG_PRINT(fmt, args...)
    #define DBG_PRINT(fmt)

    #define DBG_INFO(fmt, args...)
    #define DBG_INFO(fmt)

    #define DBG_ERROR(fmt, args...)
    #define DBG_ERROR(fmt)
#endif

#ifdef __cplusplus
extern "C" {
#endif

inline static void _dbg_print_mem(void const *vp, size_t n) {
    unsigned char const *p = (unsigned char const *) vp;
    for (size_t i = 0; i < n; ++i) {
        printf("%02x ", p[i]);
    }
    putchar('\n');
}

#ifdef __cplusplus
}
#endif

#endif
