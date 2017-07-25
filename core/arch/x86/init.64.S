#include <asm/assembler.h>
#include <asm/offsets.h>
#include <asm/memarea.h>

.pushsection    .text.startup, "ax"

.global	__boot

.code32

.macro	ll_uart	ch
99:
	mov	$0x3fd, %edx
	in	(%dx), %al
	test	$0x40, %al
	jz	99b
	mov	$\ch, %eax
	mov	$0x3f8, %edx
	out	%al, (%dx)
	//mov	(%esp), %eax

	mov	$0x10000, %ecx
	loop	.
.endm

ASM_FUNCTION __boot
__boot:
	mov	$0x83, %eax	// 8n1, DLAB
	mov	$0x3fb, %edx
	out	%al, (%dx)

	mov	$0x01, %eax
	mov	$0x3f8, %edx
	out	%al, (%dx)
	dec	%eax
	inc	%edx
	out	%al, (%dx)	// DL := 0001 (115200 baud)

	mov	$0x03, %eax	// 8n1
	mov	$0x3fb, %edx
	out	%al, (%dx)

	mov	$0x00, %eax	// clear FIFOCR
	mov	$0x3fa, %edx
	out	%al, (%dx)
	mov	$0x0f, %eax	// set MCR to funny values
	mov	$0x3fc, %edx
	out	%al, (%dx)

.LXYXY:
	mov	$0x65ffc, %esp
	call	1f
1:
	pop	%eax
	sub	$1b - __boot, %eax
	push	%eax
	lea	_specification - __boot(%eax), %ebx

	mov	(%esp), %eax

	// read core no. from LAPIC
	mov	0xfee00020, %edi
	shr	$24, %edi

	test	%edi, %edi
	jnz	2f

	// copy real mode boot code into place
	mov	$0x66000, %edi
	lea	mpboot_realmode - __boot(%eax), %esi
	mov	$mpboot_end - mpboot_realmode, %ecx
	rep movsb
	mov	%eax, (%edi)

	mov	$0, %edi

2:
	mov	$0x20b0, %esi
	mov	%esi, %cr4

	mov	$0xc0000080, %ecx
	mov	$0x900, %eax
	xor	%edx, %edx
	wrmsr

	mov	(%esp), %ecx
	add	$.Lgdt_longmode - __boot, %ecx
	lea	.Ldescriptors - .Lgdt_longmode(%ecx), %edx
	mov	%ecx, 2(%edx)
	lgdt	(%edx)

	mov	(%esp), %ecx
	add	$__boot_long - __boot, %ecx
	mov	%ecx, 2+4(%edx)
	mov	%edx, %esi

	mov	offset(specification, boot_pagetable_address)(%ebx), %eax
	mov	%eax, %cr3
	mov	$0x80010031, %eax
	mov	%eax, %cr0

	ljmp	*6(%esi)
ASM_FUNCTION_END __boot

.align	8
.Lgdt_longmode:
	.long	0x00000000	// null selector
	.long	0x00000000
	.long	0x00000000	// CS64
	.long	0x00a09b00
	.long	0x00000000	// DS
	.long	0x00a09300

.Ldescriptors:
	// GDT descriptor: length first, then address
	.short	0x17
	.long	0
	// indirect far jump: address first, then segment (crazy x86 devs)
	.long	0
	.short	0x08

.code64

.macro	ll_uart64 ch
	push	%rcx
	push	%rdx
	push	%rax
99:
	mov	$0x3fd, %edx
	in	(%dx), %al
	test	$0x40, %al
	jz	99b
	mov	$\ch, %eax
	mov	$0x3f8, %edx
	out	%al, (%dx)
	//mov	(%esp), %eax

	mov	$0x10000, %ecx
	loop	.

	pop	%rax
	pop	%rdx
	pop	%rcx
.endm

// 64 bit code segment, but still at physical address; time to relocate
// note: %rsp is at 0x65ff8 now; aligned, but not covered by pagetable
ASM_FUNCTION __boot_long
__boot_long:
	movabs	$__boot_long_paged, %rax
	jmp	*%rax
ASM_FUNCTION_END __boot_long

ASM_FUNCTION	__boot_long_paged
__boot_long_paged:
	mov	$0x10, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

	lea	_specification(%rip), %rbx
	mov	offset(specification, cpus)(%rbx), %rcx

	mov	%edi, %eax
	mov	$sizeof(specification_cpu), %esi
	mul	%rsi
	add	%rax, %rcx

	mov	offset(specification_cpu, pagetable_address)(%rcx), %rdx
	mov	%rdx, %cr3

	// now we run with the CPU-private pagetable

	mov	offset(specification_cpu, memareas)(%rcx), %rdx
	add	$MEMAREA_STACK * sizeof(memarea), %rdx
	mov	offset(memarea, vaddr)(%rdx), %rsp
	add	offset(memarea, size)(%rdx), %rsp

	// stack pointer is usable again

	push	%rdi
	push	%rbx
	call	prepare_areas
	pop	%rbx
	pop	%rdi

	mov	%edi, cpu_number(%rip)
	mov	%rsp, top_of_stack(%rip)

	mov	offset(specification, arch_specification)(%rbx), %rcx

	mov	offset(specification_arch, gdt)(%rcx), %rdx
	push	%rdx
	mov	$0x37, %edx
	shl	$48, %rdx
	push	%rdx
	lgdt	6(%rsp)
	add	$16, %rsp

	lea	.Lidt(%rip), %rdx
	push	%rdx
	mov	$0xfff, %rdx
	shl	$48, %rdx
	push	%rdx
	lidt	6(%rsp)
	add	$16, %rsp

	mov	$0x18, %eax
	ltr	%ax

	mov	offset(specification_arch, tss)(%rcx), %rdx
	mov	%rsp, 4(%rdx)

	call	x86_virt_setup

	jmp	main
ASM_FUNCTION_END	__boot_long_paged

/**
 * Native exceptions (vectors 0--31) can only happen in paravirt mode,
 * and nobody wants that for x86. So just make sure we can report what
 * went wrong and then settle into a panic().
 *
 * Interrupts are a valid thing to happen, but always come with a clean
 * slate. Either we were in idle mode before (then we don't care about
 * the register state), or we exited from a guest, but then the virtual
 * state has already been saved before RFLAGS.IF was activated and we
 * end up here.
 */
.align	0x200
.Lidt:
	// 256 16B Interrupt Gates targeting the .Lirqhandlers below
	.rep	256
	.long	0x00081200 + 9 * ((. - .Lidt) >> 4)
	.long	0xf4008e00
	.long	0xffffffff
	.long	0x0
	.endr

	// handler macros: make sure each is exactly 9 bytes long,
	// we assume this in the IDT above
.macro	handle_exc_no_ec	code
	pushq	$0
	pushq	$\code
	jmp.d32	prep_exception_handler
.endm
.macro	handle_exc_ec		code
	pushq	$\code
	jmp.d32	prep_exception_handler
	nop
	nop
.endm
.macro	handle_irq	nr
#if 1
.if	\nr < 128
	pushq	$\nr
.else
	pushq	$\nr - 256
.endif
#else
	nop
	nop
#endif
	jmp.d32	prep_irq_handler
	nop
	nop
.endm

.Lirqhandlers:
	handle_exc_no_ec	0
	handle_exc_no_ec	1
	handle_exc_no_ec	2
	handle_exc_no_ec	3
	handle_exc_no_ec	4
	handle_exc_no_ec	5
	handle_exc_no_ec	6
	handle_exc_no_ec	7
	handle_exc_ec		8
	handle_exc_no_ec	9
	handle_exc_ec		10
	handle_exc_ec		11
	handle_exc_ec		12
	handle_exc_ec		13
	handle_exc_ec		14
	handle_exc_no_ec	15
	handle_exc_no_ec	16
	handle_exc_ec		17
	handle_exc_no_ec	18
	handle_exc_no_ec	19
	handle_exc_no_ec	20
$gateno = 21
	.rep	11
	handle_exc_no_ec	$gateno
$gateno = $gateno + 1
	.endr
	.rep	256 - 32
	handle_irq		$gateno
$gateno = $gateno + 1
	.endr

prep_exception_handler:
	push	%rdi
	lea	8(%rsp), %rdi
	call	exception_handler
	// we're dead anyway...
	jmp	panic

prep_irq_handler:
	pop	%rdi
	and	$0xff, %edi
	mov	%rdi, _incoming_idt_vector
	// that call doesn't return; if it does, panic()
	call	interrupt_handler
	jmp	panic

.code16

mpboot_realmode:
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %ss
	mov	$.Lgdtdesc_realmode - mpboot_realmode, %bx
	lgdt	%ds:(%bx)
	mov	$0x1, %ax
	mov	%eax, %cr0
	dword ljmp	$8, $0x66000 + mpboot_protectedmode - mpboot_realmode

.Lgdt_realmode:
	.long	0x00000000
	.long	0x00000000
	.long	0x0000ffff
	.long	0x00cf9b00
	.long	0x0000ffff
	.long	0x00cf9300

.Lgdtdesc_realmode:
	.short	0x17
	.long	0x66000 + .Lgdt_realmode - mpboot_realmode

.code32

mpboot_protectedmode:
	mov	$0x10, %edx
	mov	%dx, %ds
	mov	%dx, %ss
	mov	0x66000 + mpboot_end - mpboot_realmode, %eax
	jmp	*%eax

.align	4
mpboot_end:
	.long	0

.popsection
