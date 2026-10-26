/* Equivalent-key replacement tests; no external test framework required. */
#include "rb_tree.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static unsigned allocations, frees;

static void* reject_allocation(size_t size)
{
    (void)size;
    ++allocations;
    return NULL;
}

static void count_free(void* p)
{
    ++frees;
    free(p);
}

int main(void)
{
    int keys[] = {40, 20, 60, 10, 30, 50, 70};
    int replacements[] = {40, 20, 60, 10, 30, 50, 70};
    int values[] = {4, 2, 6, 1, 3, 5, 7};
    void** slots[7];
    int absent = 25;
    rb_tree* tree = rb_tree_new(dict_int_cmp);
    CHECK(tree != NULL);
    dict_replace_result result = rb_tree_replace(tree, &absent, &absent);
    CHECK(!result.replaced && result.key == NULL && result.datum == NULL);
    CHECK(rb_tree_count(tree) == 0);

    for (size_t i = 0; i < 7; ++i) {
        dict_insert_result inserted = rb_tree_insert(tree, &keys[i]);
        CHECK(inserted.inserted && inserted.datum_ptr != NULL);
        slots[i] = inserted.datum_ptr;
        *slots[i] = &keys[i];
    }
    size_t path_length = rb_tree_total_path_length(tree);
    dict_malloc_func = reject_allocation;
    dict_free_func = count_free;

    result = rb_tree_replace(tree, &absent, NULL);
    CHECK(!result.replaced && result.key == NULL && result.datum == NULL);
    CHECK(rb_tree_count(tree) == 7 && rb_tree_verify(tree));
    for (size_t i = 0; i < 7; ++i) {
        result = rb_tree_replace(tree, &replacements[i], &values[i]);
        CHECK(result.replaced && result.key == &keys[i] && result.datum == &keys[i]);
        CHECK(rb_tree_search(tree, &replacements[i]) == slots[i]);
        CHECK(*slots[i] == &values[i]);
        /* Neither the tree nor its comparator may retain the previous key. */
        keys[i] = -100;
        CHECK(rb_tree_search(tree, &replacements[i]) == slots[i]);
        CHECK(rb_tree_count(tree) == 7 && rb_tree_verify(tree));
        CHECK(rb_tree_total_path_length(tree) == path_length);
    }

    /* Replacing with the same key pointer and a NULL datum is valid. */
    result = rb_tree_replace(tree, &replacements[0], NULL);
    CHECK(result.replaced && result.key == &replacements[0] && result.datum == &values[0]);
    CHECK(rb_tree_search(tree, &replacements[0]) == slots[0]);
    CHECK(*slots[0] == NULL);
    CHECK(allocations == 0 && frees == 0);
    dict_malloc_func = malloc;
    dict_free_func = free;

    rb_itor* it = rb_itor_new(tree);
    CHECK(it != NULL && rb_itor_first(it));
    for (int expected = 10; expected <= 70; expected += 10) {
        CHECK(rb_itor_valid(it));
        CHECK(*(const int*)rb_itor_key(it) == expected);
        rb_itor_next(it);
    }
    CHECK(!rb_itor_valid(it));
    rb_itor_free(it);

    for (size_t i = 0; i < 7; ++i) {
        dict_remove_result removed = rb_tree_remove(tree, &replacements[i]);
        CHECK(removed.removed && removed.key == &replacements[i]);
        CHECK(removed.datum == (i ? &values[i] : NULL));
        CHECK(rb_tree_verify(tree));
    }
    CHECK(rb_tree_free(tree, NULL) == 0);
    return EXIT_SUCCESS;
}
