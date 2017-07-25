#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#define BYTE_ORDER  LITTLE_ENDIAN

typedef unsigned char     u8_t;
typedef signed char      s8_t;
typedef unsigned short    u16_t;
typedef signed short     s16_t;
typedef unsigned int    u32_t;
typedef signed int     s32_t;
typedef unsigned long    u64_t;
typedef signed long     s64_t;

typedef void * sys_prot_t;
typedef void * sys_thread_t;

typedef u64_t	size_t;
typedef u64_t	mem_ptr_t;

#include "mch.h"

#define LWIP_ERR_T  int

/* Define (sn)printf formatters for these lwIP types */
#define	U8_F  "hhu"
#define	D8_F  "hhd"
#define	X8_F  "hhx"
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

/* Compiler hints for packing structures */
#define PACK_STRUCT_FIELD(x)    x
#define PACK_STRUCT_STRUCT  __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)   do {                \
        printf x;                   \
    } while (0)

#define LWIP_PLATFORM_ASSERT(x) do {                \
        printf("Assert \"%s\" failed at line %d in %s\n",   \
                x, __LINE__, __FILE__);             \
        mch_abort();                        \
    } while (0)

#define SYS_MBOX_NULL   NULL
#define SYS_SEM_NULL    NULL

#define	LWIP_NO_STDINT_H	    1
#define	LWIP_NO_INTTYPES_H	    1

#define LWIP_RAND()		sys_rand()

#endif /* __ARCH_CC_H__ */
