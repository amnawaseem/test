#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>
#include <schism/path.h>
#include <schism/gc.h>

static struct def_buffer makeconf_buffer;

// ---------------------------------------------------------------------------

static char *string_toupper(const char *input_string) {
  char *output_string;
  uint32_t len, i;

  len = strlen(input_string);

  output_string = malloc(len + 1);

  for (i = 0; i <= len; i++) {
    if ((input_string[i] >= 0x61) && (input_string[i] <= 0x7a)) {
      output_string[i] = input_string[i] ^ 0x20;
    } else {
      output_string[i] = input_string[i];
    }
  }

  return output_string;
}

// ---------------------------------------------------------------------------

static int rewrite_makeconf(const char *makeconf_filename) {
  int fd;

  fd = open(makeconf_filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if (fd < 0) {
    return 1;
  }

  if (write(fd, makeconf_buffer.buf, makeconf_buffer.buf_len) < makeconf_buffer.buf_len) {
    return 1;
  }

  close(fd);

  return 0;
}

// ---------------------------------------------------------------------------

static int compare_makeconf(const char *makeconf_filename) {
  void *mem;
  int fd;
  struct stat stt;
  int equal;

  fd = open(makeconf_filename, O_RDONLY);
  if (fd < 0) {
    return 1;
  }
  if (fstat(fd, &stt)) {
    close(fd);
    return 1;
  }

  if (makeconf_buffer.buf_len != stt.st_size) {
    close(fd);
    return 1;
  }

  mem = mmap(NULL, stt.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
    close(fd);
    return 1;
  }

  equal = memcmp(mem, makeconf_buffer.buf, makeconf_buffer.buf_len);

  munmap(mem, stt.st_size);
  close(fd);

  return equal;
}

// ---------------------------------------------------------------------------

static int generate_makeconf(struct scenario_context *sctx) {
  const char *cross_prefix, *cross_prefix_conf;
  const char *tmp;
  xmlNodePtr feature, guest, vdev;
  const char *feature_name, *feature_value;
  const char *vdevtype, *vdevfrontend;

  def_buffer_printf(&makeconf_buffer, "ARCH := %s\n", query_attribute(sctx->doc, "/scenario/platform", "arch"));
  def_buffer_printf(&makeconf_buffer, "PLATFORM := %s\n", query_attribute(sctx->doc, "/scenario/platform", "board"));

  cross_prefix_conf = query_attribute(sctx->doc, "/scenario/hypervisor", "ccprefix");
  cross_prefix = determine_cross_prefix(sctx);

  if ((cross_prefix == NULL) && (cross_prefix_conf != NULL)) {
    fprintf(stderr, "ERROR: cross compiler from ccprefix attribute not found.\n");
    return 1;
  } else if (cross_prefix == NULL) {
    fprintf(stderr, "ERROR: no cross-compiler found.\n");
    return 1;
  }
  if (cross_prefix_conf == NULL) {
    fprintf(stderr, "INFO: chosen cross-compiler prefix: \"%s\"\n", cross_prefix);
  }
  def_buffer_printf(&makeconf_buffer, "TARGET_PREFIX := %s\n", cross_prefix);

  tmp = query_attribute(sctx->doc, "/scenario/hypervisor/address_space", "type");

  if (strcmp(tmp, "mmu") == 0) {
    def_buffer_printf(&makeconf_buffer, "FEATURE_AS_MMU := yes\n");
  } else if (strcmp(tmp, "mpu") == 0) {
    def_buffer_printf(&makeconf_buffer, "FEATURE_AS_MPU := yes\n");
  }

  iterate_over_children(sctx->doc, "/scenario/hypervisor", "feature", feature) //{
    feature_name = get_attribute(feature, "name");
    feature_value = get_attribute(feature, "value");

    if (strcmp(feature_name, "debugger") == 0) {
      def_buffer_printf(&makeconf_buffer, "FEATURE_DEBUG := %s\n", feature_value);
    } else if (strcmp(feature_name, "tracer") == 0) {
      def_buffer_printf(&makeconf_buffer, "FEATURE_TRACE := %s\n", feature_value);
    } else if (strcmp(feature_name, "multiplexer") == 0) {
      def_buffer_printf(&makeconf_buffer,"FEATURE_MULTIPLEXER := %s\n", feature_value);
    } else if (strcmp(feature_name, "arch_width") == 0) {
      def_buffer_printf(&makeconf_buffer, "FEATURE_ARCH_WIDTH := %d\n", atoi(feature_value));
    } else if (strcmp(feature_name, "virt_type") == 0) {
      def_buffer_printf(&makeconf_buffer, "FEATURE_VIRT_TYPE_%s := yes\n", string_toupper(feature_value));

    } else if (strcmp(feature_name, "driver:uart") == 0) {
      def_buffer_printf(&makeconf_buffer, "UART_DRIVER_%s := yes\n", string_toupper(feature_value));
    } else if (strcmp(feature_name, "driver:timer") == 0) {
      def_buffer_printf(&makeconf_buffer, "TIMER_DRIVER_%s := yes\n", string_toupper(feature_value));
    } else if (strcmp(feature_name, "driver:clock") == 0) {
      def_buffer_printf(&makeconf_buffer, "CLOCK_DRIVER_%s := yes\n", string_toupper(feature_value));
    } else if (strcmp(feature_name, "driver:irq") == 0) {
      def_buffer_printf(&makeconf_buffer, "IRQ_DRIVER_%s := yes\n", string_toupper(feature_value));

    } else if (strcmp(feature_name, "tweak") == 0) {
      def_buffer_printf(&makeconf_buffer, "CONFIG_TWEAK_%s := yes\n", string_toupper(feature_value));
    }
  }

  iterate_over_children(sctx->doc, "/scenario", "guest", guest) //{
    iterate_over_children_N(guest, "vdev", vdev) //{
      vdevtype = get_attribute(vdev, "type");
      vdevfrontend = get_attribute(vdev, "frontend");

      if (strcmp(vdevtype, "memory32") == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_MEMORY32 := yes\n");
      } else if (strcmp(vdevtype, "vtlb") == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_VTLB := yes\n");
      } else if ((strcmp(vdevtype, "timer") | strcmp(vdevfrontend, "sp804")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_TIMER_SP804 := yes\n");
      } else if ((strcmp(vdevtype, "timer") | strcmp(vdevfrontend, "mpcore")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_TIMER_MPCORE := yes\n");
      } else if ((strcmp(vdevtype, "timer") | strcmp(vdevfrontend, "armcp14")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_TIMER_ARMCP14 := yes\n");
      } else if ((strcmp(vdevtype, "clock") | strcmp(vdevfrontend, "mpcore")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_CLOCK_MPCORE := yes\n");
      } else if ((strcmp(vdevtype, "serial") | strcmp(vdevfrontend, "pl011")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_UART_PL011 := yes\n");
      } else if ((strcmp(vdevtype, "serial") | strcmp(vdevfrontend, "pic32")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_UART_PIC32 := yes\n");
      } else if ((strcmp(vdevtype, "serial") | strcmp(vdevfrontend, "16550")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_UART_16550 := yes\n");
      } else if ((strcmp(vdevtype, "irq_controller") | strcmp(vdevfrontend, "arm_gic")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_IRQ_GIC := yes\n");
      } else if ((strcmp(vdevtype, "irq_controller") | strcmp(vdevfrontend, "arm_gic_virtext")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_IRQ_GIC := yes\n");
      } else if ((strcmp(vdevtype, "irq_controller") | strcmp(vdevfrontend, "lapic")) == 0) {
        def_buffer_printf(&makeconf_buffer, "EMULATE_IRQ_LAPIC := yes\n");
      } else {
        fprintf(stderr, "WARNING: unknown vdev %s/%s - cannot propagate to $(O)/Makeconf.\n", vdevtype, vdevfrontend);
      }
    }
  }

  return 0;
}

// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  char fn[256];

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_PRISTINE;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  init_def_buffer(&makeconf_buffer, 2048);

  if (generate_makeconf(&sctx)) {
    fprintf(stderr, "ERROR: could not generate $(O)/Makeconf contents.\n");
    return 1;
  }

  snprintf(fn, 256, "%s/Makeconf", ac[1]);
  if (compare_makeconf(fn)) {
    if (rewrite_makeconf(fn)) {
      fprintf(stderr, "ERROR: could not rewrite $(O)/Makeconf.\n");
      return 1;
    }
    fprintf(stderr, "INFO: Makeconf updated.\n");
  }

  return 0;
}
