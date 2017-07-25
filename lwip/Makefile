CC := aarch64-linux-gnu-gcc -g

MCHPORT := phidias

ARFLAGS := rs

TOPDIR := /home/amna/Thesis/phidias/lwip
TARGETDIR := $(TOPDIR)/build
LWIPDIR := $(TOPDIR)/src
PORTDIR := $(LWIPDIR)/ports/mch-$(MCHPORT)

CFLAGS := $(CPPFLAGS) -I$(LWIPDIR)/include -I.              \
	-I$(PORTDIR)/include -I$(PORTDIR)/include/arch    \
	-I$(LWIPDIR)/include/ipv4                           \
	-nostdinc -nostdlib -ffreestanding			\
	-mgeneral-regs-only

LDFLAGS := -Wl,-T$(PORTDIR)/lwip.lds -Wl,--build-id=none

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES := $(LWIPDIR)/core/mem.c          \
	$(LWIPDIR)/core/memp.c              \
	$(LWIPDIR)/core/netif.c             \
	$(LWIPDIR)/core/pbuf.c              \
	$(LWIPDIR)/core/raw.c               \
	$(LWIPDIR)/core/stats.c             \
	$(LWIPDIR)/core/ip.c                \
	$(LWIPDIR)/core/sys.c               \
	$(LWIPDIR)/core/timeouts.c          \
	$(LWIPDIR)/core/tcp.c               \
	$(LWIPDIR)/core/tcp_in.c            \
	$(LWIPDIR)/core/tcp_out.c           \
	$(LWIPDIR)/core/udp.c               \
	$(LWIPDIR)/core/init.c

CORE4FILES := $(LWIPDIR)/core/ipv4/icmp.c   \
	$(LWIPDIR)/core/ipv4/ip4.c          \
	$(LWIPDIR)/core/ipv4/ip4_addr.c     \
	$(LWIPDIR)/core/ipv4/ip4_frag.c     \
	$(LWIPDIR)/core/ipv4/etharp.c       \
	$(LWIPDIR)/core/inet_chksum.c

# NETIFFILES: Files implementing various generic network interface functions.'
NETIFFILES := $(LWIPDIR)/netif/ethernet.c

PORTFILES := $(PORTDIR)/drivers/netif_virt.c\
	$(PORTDIR)/main.c                   \
	$(PORTDIR)/arch_init.c              \
	$(PORTDIR)/startup_arm64.S          \
	$(PORTDIR)/misc.c                   \
	$(PORTDIR)/drivers/serial.c

# LWIPFILES: All of the above.
LWIPFILES := $(COREFILES) $(CORE4FILES) $(NETIFFILES) $(PORTFILES)

OBJS := $(patsubst %.c,%.o,$(filter %.c,$(LWIPFILES)))
ASMOBJS := $(patsubst %.S,%.o,$(filter %.S,$(LWIPFILES)))

LWIP_EXE := $(TARGETDIR)/lwip

.PHONY: all clean

$(LWIP_EXE): $(OBJS) $(ASMOBJS) $(PORTDIR)/lwip.lds
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.o,$^) -o $@

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ASMOBJS): %.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETDIR):
	mkdir -p $@

clean:
	rm -f $(OBJS) $(LWIP_EXE)
