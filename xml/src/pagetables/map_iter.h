struct map_iterator {
  struct scenario_context *sctx;
  xmlNodePtr addr_space;
  uint32_t cpu;

  xmlNodePtr map;
  xmlNodePtr xref;
  uint64_t pa, size;
};

extern void init_map_iterator(struct map_iterator *iter, struct scenario_context *sctx, xmlNodePtr address_space, uint32_t cpu);
extern int iterate_maps(struct map_iterator *iter);
