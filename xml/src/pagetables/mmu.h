struct pagetable_dir {
  int level;
  int is_placed;
  struct pagetable_entry *entries;
  uint64_t phys_addr;
  uint64_t head_padding;
};

#define		ENTRY_LEAF		1
#define		ENTRY_DIR		2

struct pagetable_entry {
  int entry_type;
  struct pagetable_dir *dir;
  uint64_t page_addr;
};

struct paging_level {
  int shift, width, bits_per_entry, alignment_shift;

  int can_dir, can_leaf;
  uint64_t dir_entry, leaf_entry;

  uint64_t permission_flags[7 * 2];

  uint32_t num_flaggroups;
  struct flaggroup *flaggroups;
};

struct pagetable_dir *pagetable_tree;

struct pagetable_dir **unplaced_dir_list;
uint32_t unplaced_dirs;

uint32_t paging_num_levels;
struct paging_level *paging_levels;

// ---------------------------------------------------------------------------

extern int create_pagetable(struct scenario_context *sctx, xmlNodePtr mmu, int mmu_cpu, int host_cpu, int fd);
