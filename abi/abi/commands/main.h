/**
 * This file defines the Phidias Hypercall ABI.
 *
 * Guests can communicate with Phidias to benefit from enhanced paravirtu-
 * alization support by executing the architecture- and virtualization-
 * specific hypercall instruction (see below) with the first argument register
 * set to one of the following values.
 *
 * Hypercall Interface		Instruction		Registers
 * --------------------------------------------------------------------------
 * - ARMv7 Paravirt:		svc #0			{r0-r3}
 * - ARMv7+VE:			hvc #0			{r0-r3}
 * - x86 Paravirt:		int $0xa0
 * - x86 AMD SVM:		vmmcall
 * - x86 VT-x:			vmcall
 */

// --------------------------------------------------------------------------

#define	HYPERCALLMASK_TYPE		0xc0000000

#define	HYPERCALLMASK_OPERATION		0x3f000000

#define	HYPERCALLMASK_ARGUMENT_SPACE	0x00fffffc

#define	HYPERCALLMASK_DIRECTION		0x00000003

// --------------------------------------------------------------------------

#define	HYPERCALLTYPE_ARCHITECTURE	0xc0000000
#define	HYPERCALLTYPE_EMULATION		0x80000000

#define	HYPERCALLDIRECTION_READ		0x00000001
#define	HYPERCALLDIRECTION_WRITE	0x00000002
