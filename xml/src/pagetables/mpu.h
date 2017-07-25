#define VERDICT_UNSURE          0
#define VERDICT_ON              1
#define VERDICT_OFF             2
#define VERDICT_CONFLICT        (VERDICT_ON | VERDICT_OFF)

struct protection_format {
  uint32_t bpe, width;

  uint32_t min_log2, max_log2;
  int aligned;
  uint32_t divisible, div_shift;

  uint64_t flagbits_base;
  uint64_t flagbits[14];

  uint32_t num_flaggroups;
  struct flaggroup *flaggroups;
};

extern int create_protections(struct scenario_context *sctx, xmlNodePtr mpu, int mpu_cpu, int host_cpu, int fd);

extern int create_flat(struct scenario_context *sctx);
