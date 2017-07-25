/**
 * First pull in the build configuration macros like WORDSIZE, ARCH etc.
 */
#include <config.h>

/**
 * Now pull in the integer types, NULL, __shared etc.
 */
#include <types.h>

#include <abi/abi.h>

#include <arch/arch_interface.h>
#include <plat/platform_interface.h>

#include <debug.h>
