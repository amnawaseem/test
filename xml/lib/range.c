#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <schism/range.h>

#define		NO_NEIGHBOURS		0
#define		LOWER_NEIGHBOUR		1
#define		HIGHER_NEIGHBOUR	2

struct range {
	uint64_t base;
	uint64_t size;
	uint32_t neighbours;
	struct range *prev;
	struct range *next;
};

struct range_manager {
	uint32_t count;
	struct range **ranges;
};

static struct range *claim_range_one_(struct range *head, uint64_t base, uint64_t size);
static int claimable_one_(struct range *head, uint64_t base, uint64_t size);

static int find_range_one__(struct range *head, uint64_t size, uint32_t align,
				uint64_t pad, uint64_t *window, uint64_t *base_ret);

// ---------------------------------------------------------------------------
// Initializing manager: public interface

void *init_range_manager(uint64_t base, uint64_t size, uint32_t count) {
	uint32_t i;
	struct range_manager *RM;

	RM = malloc(sizeof(struct range_manager));
	RM->count = count;
	RM->ranges = malloc(count * sizeof(struct range *));

	for (i = 0; i < count; i++) {
		RM->ranges[i] = malloc(sizeof(struct range));
		RM->ranges[i]->base = base;
		RM->ranges[i]->size = size;
		RM->ranges[i]->neighbours = NO_NEIGHBOURS;
		RM->ranges[i]->prev = NULL;
		RM->ranges[i]->next = NULL;
	}

	return RM;
}

// ---------------------------------------------------------------------------
// Claiming a range of memory: public interface

int claim_range_all(void *rm, uint64_t base, uint64_t size) {
	uint32_t i;
	struct range_manager *RM = rm;
	struct range *R;

	for (i = 0; i < RM->count; i++) {
		if (claimable_one_(RM->ranges[i], base, size) != 0)
			return 1;
	}
	for (i = 0; i < RM->count; i++) {
		R = claim_range_one_(RM->ranges[i], base, size);
		RM->ranges[i] = R;
	}

	return 0;
}

int claim_range_one(void *rm, uint32_t slot, uint64_t base, uint64_t size) {
	struct range_manager *RM = rm;
	struct range *R;

	R = claim_range_one_(RM->ranges[slot], base, size);

	if (R != NULL) {
		RM->ranges[slot] = R;
		return 0;
	} else {
		return 1;
	}
}

// ---------------------------------------------------------------------------
// Claiming a range of memory: internal worker

static int claimable_one_(struct range *head, uint64_t base, uint64_t size) {
	struct range *R;

	// look for the first range at or above base
	for (R = head; R && (R->base + R->size <= base); R = R->next)
		;

	// Sanity Check 1: there is no such range (base is out of bounds)
	if (R == NULL) {
		return 1;
	}

	// Sanity Check 2: range is above base (i.e. the range covering base
	// is already taken) or is not large enough
	if ((R->base > base) || (R->base + R->size < base + size)) {
		return 1;
	}

	return 0;
}

static struct range *claim_range_one_(struct range *head, uint64_t base, uint64_t size) {
	struct range *R, *Q;

	// look for the first range at or above base
	for (R = head; R && (R->base + R->size <= base); R = R->next)
		;

	// Sanity Check 1: there is no such range (base is out of bounds)
	if (R == NULL) {
		return NULL;
	}

	// Sanity Check 2: range is above base (i.e. the range covering base
	// is already taken) or is not large enough
	if ((R->base > base) || (R->base + R->size < base + size)) {
		return NULL;
	}

	// Ok, now we're sure we can satisfy the allocation.

	// Case 1: request fully covers node -> remove node
	if ((R->base == base) && (R->size == size)) {
		if (R->prev != NULL) {
			R->prev->next = R->next;
		}
		if (R->next != NULL) {
			R->next->prev = R->prev;
		}
		if (head != R) {
			return head;
		} else if (R->next == NULL) {
			R->size = 0;
			return R;
		} else {
			return R->next;
		}
	}

	// Case 2: request is at bottom of node
	if (R->base == base) {
		R->base += size;
		R->size -= size;
		R->neighbours |= LOWER_NEIGHBOUR;
		return head;
	}

	// Case 3: request is at ceiling of node
	if (R->base + R->size == base + size) {
		R->size -= size;
		R->neighbours |= HIGHER_NEIGHBOUR;
		return head;
	}

	// Case 4: request is inside -> split node
	Q = malloc(sizeof(struct range));
	Q->base = base + size;
	Q->size = (R->base + R->size) - Q->base;
	Q->neighbours = LOWER_NEIGHBOUR | (R->neighbours & HIGHER_NEIGHBOUR);
	Q->prev = R;
	Q->next = R->next;

	R->size = (base - R->base);
	R->neighbours |= HIGHER_NEIGHBOUR;
	R->next = Q;

	return head;
}

// ---------------------------------------------------------------------------
// Finding a range of memory (single slot): public interface

int find_range_one(void *rm, uint32_t slot, uint64_t size, uint64_t *base_ret) {
	struct range_manager *RM = rm;

	return find_range_one__(RM->ranges[slot], size, 0, 0, NULL, base_ret);
}

int find_range_one_aligned(void *rm, uint32_t slot, uint64_t size, uint32_t align, uint64_t *base_ret) {
	struct range_manager *RM = rm;

	return find_range_one__(RM->ranges[slot], size, align, 0, NULL, base_ret);
}

int find_range_one_aligned_padded(void *rm, uint32_t slot, uint64_t size, uint32_t align,
				uint64_t pad, uint64_t *base_ret) {
	struct range_manager *RM = rm;

	return find_range_one__(RM->ranges[slot], size, align, pad, NULL, base_ret);
}

int find_range_one_aligned_padded_window(void *rm, uint32_t slot, uint64_t size, uint32_t align,
				uint64_t pad, uint64_t *window, uint64_t *base_ret) {
	struct range_manager *RM = rm;

	return find_range_one__(RM->ranges[slot], size, align, pad, window, base_ret);
}

// ---------------------------------------------------------------------------
// Finding a range of memory (single slot): internal worker

static int find_range_one__(struct range *head, uint64_t size, uint32_t align,
				uint64_t pad, uint64_t *window, uint64_t *base_ret) {
	struct range *R;
	uint64_t base, basereq, sizereq;
	uint64_t usable_base, usable_ceil;

	for (R = head; R; R = R->next) {
		usable_base = R->base + ((R->neighbours & LOWER_NEIGHBOUR) ? pad : 0);
		usable_ceil = R->base + R->size - ((R->neighbours & HIGHER_NEIGHBOUR) ? pad : 0);;

		if (window) {
			usable_base = _DO_CLAMP(usable_base, window[0], window[1]);
			usable_ceil = _DO_CLAMP(usable_ceil, window[0], window[1]);
		}

		basereq = base = _DO_ALIGN_UP(usable_base, align);
		sizereq = size;

		if (basereq + sizereq > usable_ceil) {
			continue;
		}

		// ok, we can service the request from this range

		if (basereq == R->base + pad) {
			basereq -= pad;
			sizereq += pad;
		}
		if (basereq + sizereq == R->base + R->size - pad) {
			sizereq += pad;
		}

		R = claim_range_one_(head, basereq, sizereq);
		if (R && base_ret) {
			*base_ret = base;
		}
		if (R != head)
			abort();
		return 0;
	}

	return 1;
}

// ---------------------------------------------------------------------------
// Finding a range of memory (all slots): public interface

int find_range_all(void *rm, uint64_t size, uint64_t *base_ret) {
	return find_range_all_aligned_padded_window(rm, size, 0, 0, NULL, base_ret);
}

int find_range_all_aligned(void *rm, uint64_t size, uint32_t align, uint64_t *base_ret) {
	return find_range_all_aligned_padded_window(rm, size, align, 0, NULL, base_ret);
}

int find_range_all_aligned_padded(void *rm, uint64_t size, uint32_t align,
				uint64_t pad, uint64_t *base_ret) {
	return find_range_all_aligned_padded_window(rm, size, align, pad, NULL, base_ret);
}

int find_range_all_aligned_padded_window(void *rm, uint64_t size, uint32_t align,
				uint64_t pad, uint64_t *window, uint64_t *base_ret) {
	uint64_t base_tmp;
	int ret;
	struct range_manager *RM = rm;
	struct range *R;
	uint32_t i;

	ret = find_range_one__(RM->ranges[0], size, align, pad, window, &base_tmp);

	if (ret)
		return ret;

	for (i = 1; i < RM->count; i++) {
		if (claimable_one_(RM->ranges[i], base_tmp, size) != 0)
			return 1;
	}
	for (i = 1; i < RM->count; i++) {
		R = claim_range_one_(RM->ranges[i], base_tmp, size);
		RM->ranges[i] = R;
	}

	*base_ret = base_tmp;

	return 0;
}

// ---------------------------------------------------------------------------
// Debug Output

void dump_range_manager(void *rm) {
	struct range_manager *RM = rm;
	struct range *R;
	uint32_t i;
	const char neighbour_chars[] = " LH";

	fprintf(stderr, "Range Manager @%p\n", RM);

	for (i = 0; i < RM->count; i++) {
		fprintf(stderr, "\t[%d]\n", i);
		for (R = RM->ranges[i]; R; R = R->next) {
			fprintf(stderr, "[0x%016lx -- 0x%016lx] %c%c\n",
				R->base, R->base + R->size,
				neighbour_chars[R->neighbours & LOWER_NEIGHBOUR],
				neighbour_chars[R->neighbours & HIGHER_NEIGHBOUR]);
		}
	}
}
