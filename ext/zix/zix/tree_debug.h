/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef ZIX_TREE_DEBUG_H
#define ZIX_TREE_DEBUG_H

#ifndef _MSC_VER
#    include <inttypes.h>
#endif

#ifdef ZIX_TREE_DUMP
static void
zix_tree_print(ZixTreeNode* node, int level)
{
	if (node) {
		if (!node->parent) {
			printf("{{{\n");
		}
		zix_tree_print(node->right, level + 1);
		for (int i = 0; i < level; i++) {
			printf("  ");
		}
		printf("%ld.%d\n", (intptr_t)node->data, node->balance);
		zix_tree_print(node->left, level + 1);
		if (!node->parent) {
			printf("}}}\n");
		}
	}
}
#endif

#ifdef ZIX_TREE_HYPER_VERIFY
static size_t
height(ZixTreeNode* n)
{
	if (!n) {
		return 0;
	} else {
		return 1 + MAX(height(n->left), height(n->right));
	}
}
#endif

#if defined(ZIX_TREE_VERIFY) || !defined(NDEBUG)
static bool
verify_balance(ZixTreeNode* n)
{
	if (!n) {
		return true;
	}

	if (n->balance < -2 || n->balance > 2) {
		fprintf(stderr, "Balance out of range : %ld (balance %d)\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

	if (n->balance < 0 && !n->left) {
		fprintf(stderr, "Bad balance : %ld (balance %d) has no left child\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

	if (n->balance > 0 && !n->right) {
		fprintf(stderr, "Bad balance : %ld (balance %d) has no right child\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

	if (n->balance != 0 && !n->left && !n->right) {
		fprintf(stderr, "Bad balance : %ld (balance %d) has no children\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

#ifdef ZIX_TREE_HYPER_VERIFY
	const intptr_t left_height  = (intptr_t)height(n->left);
	const intptr_t right_height = (intptr_t)height(n->right);
	if (n->balance != right_height - left_height) {
		fprintf(stderr, "Bad balance at %ld: h_r (%" PRIdPTR ")"
		        "- l_h (%" PRIdPTR ") != %d\n",
		        (intptr_t)n->data, right_height, left_height, n->balance);
		assert(false);
		return false;
	}
#endif

	return true;
}
#endif

#ifdef ZIX_TREE_VERIFY
static bool
verify(ZixTree* t, ZixTreeNode* n)
{
	if (!n) {
		return true;
	}

	if (n->parent) {
		if ((n->parent->left != n) && (n->parent->right != n)) {
			fprintf(stderr, "Corrupt child/parent pointers\n");
			return false;
		}
	}

	if (n->left) {
		if (t->cmp(n->left->data, n->data, t->cmp_data) > 0) {
			fprintf(stderr, "Bad order: %" PRIdPTR " with left child\n",
			        (intptr_t)n->data);
			fprintf(stderr, "%p ? %p\n", (void*)n, (void*)n->left);
			fprintf(stderr, "%" PRIdPTR " ? %" PRIdPTR "\n", (intptr_t)n->data,
			        (intptr_t)n->left->data);
			return false;
		}
		if (!verify(t, n->left)) {
			return false;
		}
	}

	if (n->right) {
		if (t->cmp(n->right->data, n->data, t->cmp_data) < 0) {
			fprintf(stderr, "Bad order: %" PRIdPTR " with right child\n",
			        (intptr_t)n->data);
			return false;
		}
		if (!verify(t, n->right)) {
			return false;
		}
	}

	if (!verify_balance(n)) {
		return false;
	}

	if (n->balance <= -2 || n->balance >= 2) {
		fprintf(stderr, "Imbalance: %p (balance %d)\n",
		        (void*)n, n->balance);
		return false;
	}

	return true;
}
#endif

#endif  // ZIX_TREE_DEBUG_H
