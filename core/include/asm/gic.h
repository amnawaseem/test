#ifndef	_PHIDIAS_ASM_GIC_H_
#define	_PHIDIAS_ASM_GIC_H_

# define		GICV2_AREAOFFSET_DIST		0x1000
# define		GICV2_AREAOFFSET_CPU		0x2000
# define		GICV2_AREAOFFSET_HYP		0x4000
# define		GICV2_AREAOFFSET_VCPU		0x6000

# define		GIC_DIST_CTRL			0x000
# define		GIC_DIST_TYPE			0x004
# define		GIC_DIST_GROUP_BASE		0x080
# define		GIC_DIST_ENABLE_BASE		0x100
# define		GIC_DIST_DISABLE_BASE		0x180
# define		GIC_DIST_SETPENDING_BASE	0x200
# define		GIC_DIST_CLEARPENDING_BASE	0x280
# define		GIC_DIST_SETACTIVE_BASE		0x300
# define		GIC_DIST_CLEARACTIVE_BASE	0x380
# define		GIC_DIST_PRIORITY_BASE		0x400
# define		GIC_DIST_TARGETS_BASE		0x800
# define		GIC_DIST_CONFIG_BASE		0xc00
# define		GIC_DIST_NSACR_BASE		0xe00
# define		GIC_DIST_SGIR			0xf00

# define		GIC_DIST_CTRL_ENABLE0		0x001
# define		GIC_DIST_CTRL_ENABLE1		0x002
# define		GIC_DIST_TYPE_CPUNO(x)		((x) << 5)
# define		GIC_DIST_TYPE_NLINES(x)		(((x) >> 5) - 1)

# define		GIC_CPU_CTRL			0x000
# define		GIC_CPU_PMR			0x004
# define		GIC_CPU_BPR			0x008
# define		GIC_CPU_IAR			0x00c
# define		GIC_CPU_EOIR			0x010
# define		GIC_CPU_RPR			0x014
# define		GIC_CPU_HPPIR			0x018
# define		GIC_CPU_ABPR			0x01c
# define		GIC_CPU_AIAR			0x020
# define		GIC_CPU_AEOIR			0x024
# define		GIC_CPU_AHPPIR			0x028
# define		GIC_CPU_DIR			0x1000

# define		GIC_CPU_CTRL_ENABLE0		0x001
# define		GIC_CPU_CTRL_ENABLE1		0x002
# define		GIC_CPU_CTRL_EOIMODE		0x200

# define		GIC_HYP_CTRL			0x000
# define		GIC_HYP_VTYPE			0x004
# define		GIC_HYP_VMCR			0x008
# define		GIC_HYP_MISR			0x010
# define		GIC_HYP_EOISTATUS(x)		(0x020 + ((x) << 2))
# define		GIC_HYP_ELSR(x)			(0x030 + ((x) << 2))
# define		GIC_HYP_APR(x)			(0x0f0 + ((x) << 2))
# define		GIC_HYP_LIST(x)			(0x100 + ((x) << 2))

# define		GIC_HYP_CTRL_ENABLE		0x001
# define		GIC_HYP_CTRL_UNDERFLOW		0x002

# define		GIC_HYP_LR_HW			0x80000000
# define		GIC_HYP_LR_GRP1			0x40000000
# define		GIC_HYP_LR_STATE_ACTIVE		0x20000000
# define		GIC_HYP_LR_STATE_PENDING	0x10000000
# define		GIC_HYP_LR_PRIO__SHIFT		23
# define		GIC_HYP_LR_PHYSID__SHIFT	10
# define		GIC_HYP_LR_VIRTID__SHIFT	0

# define		GICIRQ_SGI_BASE			0x00
# define		GICIRQ_PPI_BASE			0x10
# define		GICIRQ_SPI_BASE			0x20
# define		GICIRQ_SPURIOUS			0x3ff

#endif
