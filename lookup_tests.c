/* Focused lookup tests; no external test framework required. */
#include "rb_tree.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

struct stored_key {
    int tag;
    int value;
};

static unsigned comparisons;
static unsigned allocations;

static int
stored_compare(const void* a, const void* b)
{
    const struct stored_key* ka = a;
    const struct stored_key* kb = b;
    CHECK(ka->tag == 123 && kb->tag == 123);
    return (ka->value > kb->value) - (ka->value < kb->value);
}

static int
lookup_compare(const void* query, const void* stored)
{
    const int value = *(const int*)query;
    const struct stored_key* key = stored;
    CHECK(key->tag == 123);
    ++comparisons;
    return (value > key->value) - (value < key->value);
}

static void*
reject_allocation(size_t size)
{
    (void)size;
    ++allocations;
    return NULL;
}

int
main(void)
{
    struct stored_key keys[] = {
        {123, 40}, {123, 20}, {123, 60}, {123, 10},
        {123, 30}, {123, 50}, {123, 70}
    };
    const int missing[] = {0, 25, 80};
    rb_tree* tree = rb_tree_new(stored_compare);
    CHECK(tree != NULL);
    int query = 40;
    CHECK(rb_tree_search_cmp(tree, &query, lookup_compare) == NULL);
    CHECK(comparisons == 0);

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        dict_insert_result result = rb_tree_insert(tree, &keys[i]);
        CHECK(result.inserted && result.datum_ptr != NULL);
        *result.datum_ptr = &keys[i];
    }

    dict_malloc_func = reject_allocation;
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        query = keys[i].value;
        void** slot = rb_tree_search_cmp(tree, &query, lookup_compare);
        CHECK(slot != NULL && *slot == &keys[i]);
        CHECK(slot == rb_tree_search(tree, &keys[i]));
    }
    for (size_t i = 0; i < sizeof(missing) / sizeof(missing[0]); ++i)
        CHECK(rb_tree_search_cmp(tree, &missing[i], lookup_compare) == NULL);
    CHECK(comparisons > 0);
    CHECK(allocations == 0);
    dict_malloc_func = malloc;

    /* A custom lookup must not replace the tree's stored-key comparator. */
    CHECK(!rb_tree_insert(tree, &keys[0]).inserted);
    CHECK(rb_tree_verify(tree));
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        dict_remove_result result = rb_tree_remove(tree, &keys[i]);
        CHECK(result.removed && result.key == &keys[i]);
        query = keys[i].value;
        CHECK(rb_tree_search_cmp(tree, &query, lookup_compare) == NULL);
        CHECK(rb_tree_verify(tree));
    }
    CHECK(rb_tree_count(tree) == 0);
    CHECK(rb_tree_free(tree, NULL) == 0);
    return EXIT_SUCCESS;
}
