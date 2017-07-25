# =============================================================================
# Default target prelude

.PHONY: all clean hostclean

all:

TARGETS :=
CLEAN_CBI_PATTERN :=

# =============================================================================
# Default flags

CFLAGS ?= -Wall -Wextra -g -O0 -Iinclude -Llib -I/usr/include/libxml2

# =============================================================================
# Common (hypervisor-independent) targets

EASY_TARGETS := expand reparent measure combine
COMPLEX_TARGETS := layouter pagetables

TARGETS += $(EASY_TARGETS) $(COMPLEX_TARGETS) tools/xmlq

# =============================================================================
# LIBSCHISM

LIBSCHISM := lib/libschism.a
## libxml2 wrapper
SRCSCHISM := lib/fileio.c lib/tree.c lib/value_list.c lib/value_dict.c
HDRSCHISM := include/schism/xml42.h
## elf parser
SRCSCHISM += lib/elf.c
HDRSCHISM += include/schism/elf.h
## addendum management
SRCSCHISM += lib/gc_defs.c lib/gc_reflect.c
HDRSCHISM += include/schism/gc.h
## path lookup
SRCSCHISM += lib/path.c
HDRSCHISM += include/schism/path.h
## range manager
SRCSCHISM += lib/range.c
HDRSCHISM += include/schism/range.h
## scenario toolkit
SRCSCHISM += lib/toolkit.c
## builtin schemata
AS_SCHISM := lib/schemata.S

OBJSCHISM := $(patsubst %.c,%.o,$(SRCSCHISM)) $(patsubst %.S,%.o,$(AS_SCHISM))

# =============================================================================
# Check for prerequisite tools/xmlq

ifeq ($(shell test -x tools/xmlq && echo YES),)
ifneq ($(O),)
$(warning Building from a clean XML repository. Some tools are not built yet.)
$(warning Continuing anyway, but rerunning make may be necessary.)
endif
endif

# =============================================================================
# Check for output directory, determine CBI

ifneq ($(O),)
override O := $(shell realpath $(patsubst %/,%,$(O)))
override CBI := $(shell tools/xmlq -s0 -o$(O) /scenario:cbi || true 2>/dev/null)
endif

ifneq ($(CBI),)
include src/cbi/$(CBI)/Makefile.cbi
$(info Identified/Supplied CBI: $(CBI))
else
$(info No CBI.)
endif

# =============================================================================
# =============================================================================
# ^^^  Definitions  ^^^					vvv  Build Recipes  vvv
# =============================================================================
# =============================================================================

# =============================================================================
# XML Schema Sequence

define schema_seq
ssq_prev := $$(shell printf "%02d" `expr $(1) - 1`)

schema/$(1).xsd: schema/$$(ssq_prev).xsd schema/$(1).diff
	cp $$< $$@
	patch --no-backup-if-mismatch --no-backup $$@ $$(filter %.diff,$$^)
endef

$(foreach SSQ,01 02 03 04 05 06,$(eval $(call schema_seq,$(SSQ))))

# =============================================================================
# LIBSCHISM

$(LIBSCHISM): $(OBJSCHISM) $(HDRSCHISM) schema/06.xsd
	rm -f $@ && ar rs $@ $(filter %.o,$^)

$(patsubst %.c,%.o,$(SRCSCHISM)): %.o: %.c $(HDRSCHISM)
	gcc $< $(CFLAGS) -c -o $@

$(patsubst %.S,%.o,$(AS_SCHISM)): %.o: %.S $(HDRSCHISM) schema/06.xsd
	gcc $< $(CFLAGS) -c -o $@

tools/xmlq: tools/xmlq.c $(LIBSCHISM)
	gcc $< $(CFLAGS) $(LIBSCHISM) -lxml2 -o $@

# =============================================================================
# Common (hypervisor-independent) targets

$(EASY_TARGETS): %: src/%.c $(LIBSCHISM)
	gcc $< $(CFLAGS) -lschism -lxml2 -o $@

define forwarder
$(2): src/cbi_spec.c $(LIBSCHISM)
	gcc $$< $(CFLAGS) -DCBI_FORWARDER_NAME=\"$(2)\" -DCBI_FORWARDER_STAGE=$(1) -lschism -lxml2 -o $$@

TARGETS += $(2)
endef

define complex_target
$(1): $$(patsubst %.c,%.o,$$(wildcard src/$(1)/*.c)) $(LIBSCHISM)
	gcc $$(filter %.o,$$^) $(CFLAGS) -lschism -lxml2 -o $$@

$$(patsubst %.c,%.o,$$(wildcard src/$(1)/*.c)): %.o: %.c $$(wildcard src/$(1)/*.h) $(LIBSCHISM)
	gcc $$< $(CFLAGS) -c -o $$@
endef

$(eval $(call forwarder,0,precompilation))
$(eval $(call forwarder,2,generate_config))
$(foreach C,$(COMPLEX_TARGETS),$(eval $(call complex_target,$(C))))

# =============================================================================
# If we have "O=...", include output targets

ifneq ($(O),)
include Makefile.output
endif

# =============================================================================
# Phony targets

all: $(TARGETS)

hostclean:
	rm -f lib/*.o lib/*.a $(TARGETS) $(patsubst %,src/%/*.o,$(COMPLEX_TARGETS))
	rm -f schema/0[123456].xsd $(CLEAN_CBI_PATTERN)

ifeq ($(O),)
clean:
	:
else
clean:
	rm -f $(O)/Makeconf $(O)/.makeconf_checked
endif
