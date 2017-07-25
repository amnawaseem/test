#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <elf.h>
#include <schism/xml42.h>
#include <schism/range.h>
#include "pagetables.h"

// ---------------------------------------------------------------------------

static struct protection_format prot_format;

static int parse_protection_format(xmlNodePtr format) {
  uint32_t div;
  uint32_t i;
  xmlNodePtr flag, flaggroup;

  prot_format.min_log2 = get_attribute_number(format, "min_size_log2");
  prot_format.max_log2 = get_attribute_number(format, "max_size_log2");

  if (get_attribute(format, "natural_align") != NULL)
    prot_format.aligned = 1;

  if (get_attribute(format, "divisible") != NULL) {
    prot_format.divisible = get_attribute_number(format, "divisible");
    prot_format.div_shift = 0;

    for (div = prot_format.divisible; div > 1; div >>= 1) {
      prot_format.div_shift++;
    }
  } else {
    prot_format.divisible = prot_format.div_shift = 0;
  }

  prot_format.bpe = get_attribute_number(format, "bpe");
  prot_format.width = get_attribute_number(format, "addr_width");

  prot_format.flagbits_base = get_attribute_number(format, "flags_base");

  iterate_over_children_N(format, "flag", flag) //{
    char flag_str = get_attribute(flag, "name")[0];
    enum permission_index index;
    switch (flag_str) {
    case 'r': index = PERM_R; break;
    case 'w': index = PERM_W; break;
    case 'x': index = PERM_X; break;
    case 'g': index = PERM_G; break;
    case 'd': index = PERM_D; break;
    case 'u': index = PERM_U; break;
    case 's': index = PERM_S; break;
    default:
      fprintf(stderr, "ERROR: unknown protection flag.\n");
      return 1;
    }

    prot_format.flagbits[index] = get_attribute_number(flag, "value_set");
    prot_format.flagbits[index+1] = get_attribute_number(flag, "value_clear");
  }

  prot_format.num_flaggroups = count_children(format, "flaggroup");
  prot_format.flaggroups = malloc(sizeof(struct flaggroup) * prot_format.num_flaggroups);

  i = 0;
  iterate_over_children_N(format, "flaggroup", flaggroup) //{
    prot_format.flaggroups[i].flaggroup = get_attribute(flaggroup, "group");
    prot_format.flaggroups[i].bits_set = get_attribute_number(flaggroup, "bits_set");
    prot_format.flaggroups[i].bits_clear = get_attribute_number(flaggroup, "bits_clear");
    ++i;
  }

  return 0;
}

static uint64_t protection_flags(const char *flags) {
  uint64_t flag_bits = prot_format.flagbits_base;
  uint32_t i;
  int negate;

  flag_bits |= prot_format.flagbits[PERM_R + (strchr(flags, 'r') ? 0 : 1)];
  flag_bits |= prot_format.flagbits[PERM_W + (strchr(flags, 'w') ? 0 : 1)];
  flag_bits |= prot_format.flagbits[PERM_X + (strchr(flags, 'x') ? 0 : 1)];
  flag_bits |= prot_format.flagbits[PERM_G + (strchr(flags, 'g') ? 0 : 1)];
  flag_bits |= prot_format.flagbits[PERM_D + (strchr(flags, 'd') ? 0 : 1)];
  flag_bits |= prot_format.flagbits[PERM_U + (strchr(flags, 'u') ? 0 : 1)];
  flag_bits |= prot_format.flagbits[PERM_S + (strchr(flags, 's') ? 0 : 1)];

  for (i = 0; i < prot_format.num_flaggroups; i++) {
    int found;
    uint32_t j;

    negate = 0;
    for (j = 0; prot_format.flaggroups[i].flaggroup[j]; j++) {
      if (prot_format.flaggroups[i].flaggroup[j] == '!') {
        negate = 1; continue;
      }
      found = strchr(flags, prot_format.flaggroups[i].flaggroup[j]) ? 1 : 0;
      if (found ^ !negate) break;
    }
    if (prot_format.flaggroups[i].flaggroup[j] == 0) {
      fprintf(stderr, "FLAGGROUP HIT! (%s matches %s)\n", flags, prot_format.flaggroups[i].flaggroup);
      flag_bits = (flag_bits | prot_format.flaggroups[i].bits_set) & ~prot_format.flaggroups[i].bits_clear;
    }
  }

  return flag_bits;
}

static uint64_t division_flags(const int *verdicts) {
  uint64_t div_flags = 0;
  uint32_t i;

  for (i = 0; i < prot_format.divisible; i++) {
    if (verdicts[i] == VERDICT_ON) {
      ;
    } else {
      div_flags |= (1UL << (8 + i));
    }
  }

  return div_flags;
}

// ---------------------------------------------------------------------------

struct bucket_map {
  uint64_t pa, size;
};

struct flag_bucket {
  const char *flags;
  uint32_t num_maps;
  struct bucket_map *maps;

  int *division_verdicts;
  uint64_t region_base;
  uint64_t region_size;
  uint64_t region_flags;
  uint64_t region_number;
};

static uint64_t region_counter;

static struct flag_bucket *bucketlist;
static uint32_t num_buckets;
static struct flag_bucket white_bucket;

// ---------------------------------------------------------------------------

static int try_fit_bucket(struct flag_bucket *bucket);

static void initialize_buckets() {
  uint64_t i;

  if (bucketlist) {
    for (i = 0; i < num_buckets; i++) {
      if (bucketlist[i].num_maps) {
        free(bucketlist[i].maps);
      }
    }
    free(bucketlist);
  }

  if (white_bucket.num_maps) {
    free(white_bucket.maps);
  }

  bucketlist = malloc(sizeof(struct flag_bucket));
  num_buckets = 0;

  white_bucket.num_maps = 0;
}

static void push_into_buckets(const char *flags, uint64_t pa, uint64_t size) {
  uint32_t i, j;

  for (i = 0; i < num_buckets; i++) {
    if (strcmp(bucketlist[i].flags, flags) == 0) {
      break;
    }
  }

  if (i == num_buckets) {
    bucketlist = realloc(bucketlist, (num_buckets+1) * sizeof(struct flag_bucket));
    bucketlist[i].flags = flags;
    bucketlist[i].maps = NULL;
    bucketlist[i].num_maps = 0;
    if (prot_format.divisible) {
      bucketlist[i].division_verdicts = malloc(sizeof(int) * prot_format.divisible);
    }
    num_buckets++;
  }

  j = bucketlist[i].num_maps;
  bucketlist[i].maps = realloc(bucketlist[i].maps, (j+1)*sizeof(struct bucket_map));
  bucketlist[i].maps[j].pa = pa;
  bucketlist[i].maps[j].size = size;
  bucketlist[i].num_maps++;

  printf("ProtMap[%d] %lx-%lx (%s)\n", i, pa, size, flags);
}

static int serve_and_disable_bucket(const char *flag_match) {
  struct flag_bucket *bucket = NULL;
  uint64_t min_addr = ~0UL;
  uint64_t max_addr = 0UL;
  uint32_t i;

  for (i = 0; i < num_buckets; i++) {
    if (bucketlist[i].flags == NULL)
      continue;

    if (strcmp(bucketlist[i].flags, flag_match) == 0) {
      bucket = bucketlist+i;
      break;
    }
  }

  if (bucket == NULL) { 
    fprintf(stderr, "ERROR: no bucket for \"%s\".\n", flag_match);
    return 1;
  }

  for (i = 0; i < bucket->num_maps; i++) {
    if (min_addr > bucket->maps[i].pa)
      min_addr = bucket->maps[i].pa;
    if (max_addr < bucket->maps[i].pa + bucket->maps[i].size)
      max_addr = bucket->maps[i].pa + bucket->maps[i].size;
  }

  bucket->region_size = 1UL << LOG2_CEIL(max_addr - min_addr);
  bucket->region_base = min_addr;
  while (prot_format.aligned) {
    bucket->region_size = 1UL << LOG2_CEIL(max_addr - bucket->region_base);
    if ((bucket->region_base % bucket->region_size) == 0)
      break;
    bucket->region_base -= (bucket->region_base % bucket->region_size);
  }

  if (bucket->region_size < (1UL << prot_format.min_log2)) {
    fprintf(stderr, "ERROR: bucket %s too small, not representable\n", flag_match);
    return 1;
  }
  if (bucket->region_size > (1UL << prot_format.max_log2)) {
    fprintf(stderr, "ERROR: bucket %s too big, not representable\n", flag_match);
    return 1;
  }

  printf("Bucket %s:\n  min/max %lx-%lx\n  bounds %lx+%lx\n", flag_match,
		min_addr, max_addr,
		bucket->region_base, bucket->region_size);
    
  if (try_fit_bucket(bucket) == 1) {
    return 1;
  }

  printf("Bucket %s succeeded", flag_match);
  if (prot_format.divisible) {
    printf(" [");
    for (i = 0; i < prot_format.divisible; i++) {
      printf("%c", bucket->division_verdicts[i] == VERDICT_ON ? '*' : '-');
    } 
    printf("]");
  }
  printf("\n");
  bucket->region_flags = protection_flags(flag_match);
  bucket->region_size = ((LOG2_CEIL(bucket->region_size) - 1) << 1) | 1;
  if (prot_format.divisible) {
    bucket->region_size |= division_flags(bucket->division_verdicts);
  }
  bucket->region_number = (region_counter--);

  bucket->flags = NULL;

  white_bucket.maps = realloc(white_bucket.maps, sizeof(struct bucket_map) * (white_bucket.num_maps + bucket->num_maps));
  memcpy(white_bucket.maps + sizeof(struct bucket_map) * white_bucket.num_maps,
	bucket->maps, sizeof(struct bucket_map) * bucket->num_maps);

  return 0;
}

static int serve_other_buckets(void) {
  uint32_t i;
  int r;
  int progress = 1;

  while (progress) {
    progress = 0;
    for (i = 0; i < num_buckets; i++) {
      if (bucketlist[i].flags != NULL) {
        r = serve_and_disable_bucket(bucketlist[i].flags);
        if (r == 0) {
          progress = 1;
        }
      }
    }
    if (progress) printf("Retrying buckets.\n");
  }

  if (r) {
    fprintf(stderr, "ERROR: Irreconcilable regions: [");
    for (i = 0; i < num_buckets; i++) {
      if (bucketlist[i].flags != NULL) fprintf(stderr, " %s", bucketlist[i].flags);
    }
    fprintf(stderr, " ]\nInserting clever padding may improve things...\n");
  }

  return r;
}

// search this bucket for maps at bb_base: yes -> VERDICT_ON, advance over
// search white bucket for maps at bb_base: yes -> advance over
// search all other buckets for maps at bb_base: yes -> VERDICT_OFF, advance over
// search whole system for next map closest to bb_base: advance until
static int bucketfeed(struct flag_bucket *bucket, uint64_t *base, uint64_t *size) {
  int verdict = VERDICT_UNSURE;
  uint32_t i, j;
  uint64_t skip_size;
  struct bucket_map *closest = NULL;

  /**
   * Check the whitelist, i.e. the ranges that have already been covered
   * by higher prioritized regions, first.
   * If there is a region [x,y] with x <= BASE < y, skip until y.
   */
  for (i = 0; i < white_bucket.num_maps; i++) {
    if ((white_bucket.maps[i].pa <= *base) &&
	(white_bucket.maps[i].pa + white_bucket.maps[i].size > *base)) {
      verdict = VERDICT_UNSURE;
      skip_size = white_bucket.maps[i].size;
      goto bucketfeed_skip;
    }
  }

  /**
   * Search all the active buckets for a region with those parameters.
   * If it's in the current bucket, the verdict is ON - if it's in a
   * different, it's OFF.
   *
   * While we're running through the whole world of mappings, also find
   * the region [x,y] with BASE < x and min(x - BASE).
   */
  for (i = 0; i < num_buckets; i++) {
    if (bucketlist[i].flags == NULL)
      continue;

    for (j = 0; j < bucketlist[i].num_maps; j++) {
      if ((bucketlist[i].maps[j].pa <= *base) &&
	(bucketlist[i].maps[j].pa + bucketlist[i].maps[j].size > *base)) {
        verdict = (bucketlist+i == bucket) ? VERDICT_ON : VERDICT_OFF;
        skip_size = bucketlist[i].maps[j].size;
        goto bucketfeed_skip;
      } else if ((bucketlist[i].maps[j].pa > *base) &&
		(!closest || (bucketlist[i].maps[j].pa < closest->pa))) {
        closest = bucketlist[i].maps + j;
      }
    }
  }

  /**
   * Ok, no match at all for this BASE. Skip until the closest region
   * we do have.
   */
  if (closest)
    skip_size = closest->pa - *base;
  else
    skip_size = *size;

bucketfeed_skip:
  if (skip_size > *size) {
    skip_size = *size;
  }

  *size -= skip_size;
  *base += skip_size;

  return verdict;
}

static int try_fit_bucket(struct flag_bucket *bucket) {
  uint64_t bb_base, bb_size;

  if (prot_format.divisible) {
    uint32_t div;

    for (div = 0; div < prot_format.divisible; div++) {
      bucket->division_verdicts[div] = VERDICT_UNSURE;
      bb_size = bucket->region_size >> prot_format.div_shift;
      bb_base = bucket->region_base + div * bb_size;

      // check if division #div should be ON or OFF
      while (bb_size) {
        bucket->division_verdicts[div] |= bucketfeed(bucket, &bb_base, &bb_size);
        // printf("%s:%d / %d / %lx+%lx\n", bucket->flags, div, bucket->division_verdicts[div], bb_base, bb_size);
      }

      if (bucket->division_verdicts[div] == VERDICT_CONFLICT) {
        return 1;
      }
    }
  } else return 1;

  printf("Bucket %s conflict-free!\n", bucket->flags);
  return 0;
}

// ---------------------------------------------------------------------------

static char zeroes[32];

int create_protections(struct scenario_context *sctx, xmlNodePtr mpu, int mpu_cpu, int host_cpu, int fd) {
  xmlNodePtr protection_format;
  struct map_iterator map_iter;
  int r = 0, ctr;
  uint32_t i;

  protection_format = resolve(sctx->doc, get_attribute(mpu, "format"));
  if (protection_format == NULL) {
    fprintf(stderr, "ERROR: unknown protection format.\n");
    return 1;
  }
  if (parse_protection_format(protection_format)) {
    return 1;
  }

  region_counter = 11;

  initialize_buckets();
  init_map_iterator(&map_iter, sctx, mpu, host_cpu);

  while (iterate_maps(&map_iter) == 0) {
    if ((get_attribute(map_iter.map, "cpumap") != NULL) &&
		(!list_contains(map_iter.map, "cpumap", mpu_cpu)))
      continue;

    push_into_buckets(get_attribute(map_iter.map, "flags"), map_iter.pa, map_iter.size);
  }

  // ok, serve the buckets in a very specific order
/*
  r = serve_and_disable_bucket("rxg");
  if (r) return 1;

  r = serve_and_disable_bucket("rwgs");
  if (r) return 1;

  r = serve_and_disable_bucket("rwg");
  if (r) return 1;

  r = serve_and_disable_bucket("rg");
  if (r) return 1;
*/
  r = serve_other_buckets();

  if (r) return 1;

  dict_append_hexnumber(mpu, "base", mpu_cpu, pagetable_base);

  for (ctr = 11; ctr >= 0; ctr--) {
    for (i = 0; i < num_buckets; i++) {
      if (bucketlist[i].region_number == (uint64_t)ctr) {
        if (prot_format.bpe == 8) {
          write(fd, &bucketlist[i].region_number, 8);
          write(fd, &bucketlist[i].region_base, 8);
          write(fd, &bucketlist[i].region_size, 8);
          write(fd, &bucketlist[i].region_flags, 8);
        } else if (prot_format.bpe == 4) {
          uint32_t V[4] = { bucketlist[i].region_number, bucketlist[i].region_base, bucketlist[i].region_size, bucketlist[i].region_flags };
          write(fd, V, 4*4);
        } else return 1;
      }
    }
  }
  write(fd, zeroes, sizeof(zeroes));

  return r;
}
