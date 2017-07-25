#ifndef	_PHIDIAS_TYPES_H
#define	_PHIDIAS_TYPES_H

# define	NULL		((void *)0)

# define	__shared	__attribute__((section(".data_shared")))

# if	WORDSIZE == 32

typedef		signed char		int8_t;
typedef		unsigned char		uint8_t;

typedef		signed short		int16_t;
typedef		unsigned short		uint16_t;

typedef		signed int		int32_t;
typedef		unsigned int		uint32_t;

typedef		signed long long	int64_t;
typedef		unsigned long long	uint64_t;

typedef		uint32_t		uintptr_t;
typedef		uint32_t		wordsize_t;

# elif	WORDSIZE == 64

#  ifndef _LP64
#   error LP64 is the only supported 64-bit model.
#  endif

typedef		signed char		int8_t;
typedef		unsigned char		uint8_t;

typedef		signed short		int16_t;
typedef		unsigned short		uint16_t;

typedef		signed int		int32_t;
typedef		unsigned int		uint32_t;

typedef		signed long		int64_t;
typedef		unsigned long		uint64_t;

typedef		uint64_t		uintptr_t;
typedef		uint64_t		wordsize_t;

# else

#  error Unknown WORDSIZE.

# endif

# define	container(structname, membername, ptr)	(((void *)(ptr)) - offset(structname, membername))
# define	offset(structname, membername)		((uint8_t *)&(((struct structname##_ *)0)->membername) - (uint8_t *)((struct structname##_ *)0))

# define	BITS(var, start, width)			( ((var) >> start) & ((1U << width) - 1) )

#endif
