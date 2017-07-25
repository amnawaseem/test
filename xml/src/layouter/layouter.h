int common_phys_fixed_to_rm(void *RM, xmlNodePtr memory);

int layout_memory_mmu(xmlNodePtr memory, uint32_t *aligns);
int layout_memory_mpu(struct scenario_context *sctx, xmlNodePtr memory);
int layout_memory_flat(struct scenario_context *sctx, xmlNodePtr memory);

int layout_map_mmu(struct scenario_context *sctx, xmlNodePtr address_space);
int layout_map_mpu(struct scenario_context *sctx, xmlNodePtr address_space);

uint32_t *mmu_gather_alignments(struct scenario_context *sctx, xmlNodePtr format);
