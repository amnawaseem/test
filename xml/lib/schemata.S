#if defined(__x86_64__)
# define	PTR		.quad
#else
# define	PTR		.long
#endif

.pushsection	".rodata", "a"

.global		schemata
.global		schemata_lengths

schemata:
	PTR	schema_00
	PTR	schema_01
	PTR	schema_02
	PTR	schema_03
	PTR	schema_04
	PTR	schema_05
	PTR	schema_06
	PTR	0

schemata_lengths:
	.long	schema_01 - schema_00
	.long	schema_02 - schema_01
	.long	schema_03 - schema_02
	.long	schema_04 - schema_03
	.long	schema_05 - schema_04
	.long	schema_06 - schema_05
	.long	schema_EE - schema_06
	.long	0

schema_00:
	.incbin	"../schema/00.xsd"

schema_01:
	.incbin	"../schema/01.xsd"

schema_02:
	.incbin	"../schema/02.xsd"

schema_03:
	.incbin	"../schema/03.xsd"

schema_04:
	.incbin	"../schema/04.xsd"

schema_05:
	.incbin	"../schema/05.xsd"

schema_06:
	.incbin	"../schema/06.xsd"

schema_EE:

.popsection
