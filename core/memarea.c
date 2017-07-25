#include <phidias.h>
#include <memarea.h>

const memarea *find_memarea_by_va(const tree_memarea *tree, uintptr_t vaddr) {
	while (tree) {
		if ((tree->item->vaddr <= vaddr) &&
			(tree->item->vaddr + tree->item->size > vaddr)) {
			return tree->item;
		} else if (vaddr > tree->item->vaddr) {
			tree = tree->right;
		} else {
			tree = tree->left;
		}
	}

	return NULL;
}
