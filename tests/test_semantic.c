/**
 * @file test_semantic.c
 * @brief Unit tests for the semantic memory module.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../src/memory/semantic.h"
#include "../src/embed.h"
#include "../src/utils.h"

#define TEST(name) printf("  [ TEST ] %s ... ", #name); fflush(stdout)
#define PASS()     printf("PASS\n")
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); g_failures++; } while(0)

static int g_failures = 0;

static void test_add_and_count(void) {
    TEST(sem_add_and_count);
    SemanticStore *s = sem_create(NULL);
    sem_add(s, "user1", "I love space exploration.", NULL, 0.8f);
    sem_add(s, "user1", "My favourite colour is blue.", NULL, 0.7f);
    if (sem_count(s) != 2) FAIL("expected 2 entries"); else PASS();
    sem_destroy(s);
}

static void test_search_top_k(void) {
    TEST(sem_search_returns_top_k);
    SemanticStore *s = sem_create(NULL);
    sem_add(s, "u", "cats are wonderful pets",    NULL, 1.0f);
    sem_add(s, "u", "I enjoy hiking in mountains",NULL, 1.0f);
    sem_add(s, "u", "cats make great companions", NULL, 1.0f);

    float qvec[EMBED_DIM];
    embed_text("I love cats", qvec);
    SemResult res[3];
    int found = sem_search(s, "u", qvec, 3, res);
    if (found < 1) FAIL("expected at least 1 result");
    /* top result should be about cats */
    else if (!str_contains_ci(res[0].entry->text, "cats")) FAIL("top result should be about cats");
    else PASS();
    sem_destroy(s);
}

static void test_cosine_identical(void) {
    TEST(identical_texts_yield_high_cosine);
    float a[EMBED_DIM], b[EMBED_DIM];
    embed_text("space exploration is wonderful", a);
    embed_text("space exploration is wonderful", b);
    float sim = embed_cosine(a, b);
    if (sim < 0.99f) FAIL("identical texts should have cosine ~1.0");
    else PASS();
}

static void test_user_isolation(void) {
    TEST(search_respects_user_id);
    SemanticStore *s = sem_create(NULL);
    sem_add(s, "alice", "Alice likes jazz music.", NULL, 1.0f);
    sem_add(s, "bob",   "Bob prefers rock music.", NULL, 1.0f);

    float qvec[EMBED_DIM];
    embed_text("music preferences", qvec);
    SemResult res[5];
    int found = sem_search(s, "alice", qvec, 5, res);
    for (int i = 0; i < found; ++i) {
        if (strcmp(res[i].entry->user_id, "alice") != 0) {
            FAIL("search leaked across user IDs"); sem_destroy(s); return;
        }
    }
    PASS();
    sem_destroy(s);
}

static void test_forget(void) {
    TEST(sem_forget_removes_low_importance);
    SemanticStore *s = sem_create(NULL);
    sem_add(s, "u", "important memory", NULL, 0.9f);
    sem_add(s, "u", "trivial memory",   NULL, 0.02f);
    int removed = sem_forget(s, "u", 0.05f);
    if (removed != 1) FAIL("expected 1 removed"); else
    if (sem_count(s) != 1) FAIL("expected 1 remaining"); else PASS();
    sem_destroy(s);
}

static void test_persistence(void) {
    TEST(sem_save_and_reload);
    const char *path = "data/test_sem_tmp.bin";
    SemanticStore *s1 = sem_create(path);
    sem_add(s1, "u", "persisted fact", NULL, 0.7f);
    if (!sem_save(s1)) { FAIL("save failed"); sem_destroy(s1); return; }
    sem_destroy(s1);

    SemanticStore *s2 = sem_create(path);
    if (sem_count(s2) != 1) FAIL("expected 1 entry after reload"); else {
        SemResult res[1];
        float qvec[EMBED_DIM];
        embed_text("persisted fact", qvec);
        int found = sem_search(s2, "u", qvec, 1, res);
        if (found != 1 || !str_contains_ci(res[0].entry->text, "persisted"))
            FAIL("reloaded entry mismatch");
        else PASS();
    }
    sem_destroy(s2);
    remove(path);
}

int main(void) {
    printf("\n=== Semantic Memory Tests ===\n");
    test_add_and_count();
    test_search_top_k();
    test_cosine_identical();
    test_user_isolation();
    test_forget();
    test_persistence();
    printf("\nSemantic: %s (%d failure(s))\n\n",
           g_failures ? "FAILED" : "ALL PASS", g_failures);
    return g_failures ? 1 : 0;
}
