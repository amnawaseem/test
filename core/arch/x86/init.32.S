#include <asm/assembler.h>
#include <asm/offsets.h>
#include <asm/memarea.h>

.pushsection    .text.startup, "ax"

.global	__boot

__boot:
	mov	$0x65ffc, %esp
	call	1f
1:
	pop	%eax
	sub	$1b - __boot, %eax
	lea	_specification - __boot(%eax), %ebx

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
	mov	$0x90, %esi
	mov	%esi, %cr4

	mov	offset(specification, boot_pagetable_address)(%ebx), %ecx
	mov	%ecx, %cr3
	mov	$0x80010011, %eax
	mov	%eax, %cr0

	mov	$__boot_paged, %edx
	jmp	*%edx

	// now we run with paging enabled, 1:1 init pagetable

__boot_paged:
	mov	$_specification, %ebx
	mov	offset(specification, cpus)(%ebx), %ecx

	mov	%edi, %eax
	mov	$sizeof(specification_cpu), %esi
	mul	%esi
	add	%eax, %ecx

	mov	offset(specification_cpu, pagetable_address)(%ecx), %edx
	mov	%edx, %cr3

	// now we run with the CPU-private pagetable

	mov	offset(specification_cpu, memareas)(%ecx), %edx
	add	$MEMAREA_STACK * sizeof(memarea), %edx
	mov	offset(memarea, vaddr)(%edx), %esp
	add	offset(memarea, size)(%edx), %esp
	sub	$4, %esp

	mov	%edi, %eax
	call	prepare_areas

	mov	%edi, cpu_number
	mov	%esp, top_of_stack

	mov	offset(specification, arch_specification)(%ebx), %ecx

	mov	offset(specification_arch, gdt)(%ecx), %edx
	push	%edx
	push	$0x2f << 16
	lgdt	2(%esp)
	add	$8, %esp

	push	$.Lidt
	push	$0x7ff << 16
	lidt	2(%esp)
	add	$8, %esp

	mov	$0x18, %eax
	ltr	%ax

	mov	offset(specification_arch, tss)(%ecx), %edx
	mov	%esp, 4(%edx)

	mov	$0x10, %ebx
	mov	%bx, %ds
	mov	%bx, %es
	mov	%bx, %fs
	mov	%bx, %gs
	mov	%bx, %ss

	call	x86_virt_setup

	ljmp	$8, $main

.align	0x200
.Lirq_catchall:
	cli
	hlt
	jmp	.Lirq_catchall

.align	8
.Lidt:
	.rep	256
	.long	0x00080200
	.long	0xf4008e00
	.endr

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
