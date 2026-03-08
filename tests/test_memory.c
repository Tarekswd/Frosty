/**
 * @file test_memory.c
 */
#include <stdio.h>
#include <string.h>
#include "../src/memory.h"
#define TEST(name) printf("  [ TEST ] %s ... ", #name); fflush(stdout)
#define PASS()     printf("PASS\n")
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); g_failures++; } while(0)
static int g_failures = 0;

static void test_push_count(void) {
    TEST(m_push_count);
    ConvMemory *m = mem_create(5, NULL);
    mem_push(m, MEM_ROLE_USER, "Hi");
    mem_push(m, MEM_ROLE_BOT, "Hello");
    if (mem_count(m) != 2) FAIL("expected 2"); else PASS();
    mem_destroy(m);
}

static void test_overflow(void) {
    TEST(m_window_overflow);
    ConvMemory *m = mem_create(2, NULL);
    mem_push(m, MEM_ROLE_USER, "1");
    mem_push(m, MEM_ROLE_BOT, "2");
    mem_push(m, MEM_ROLE_USER, "3");
    if (mem_count(m) != 2) FAIL("expected 2 (window max)");
    else {
        const MemEntry *t = mem_get(m, 0);
        if (strcmp(t->content, "2") != 0) FAIL("expected '2' at oldest pos");
        else PASS();
    }
    mem_destroy(m);
}

int main(void) {
    printf("\n=== Episodic Memory Tests ===\n");
    test_push_count();
    test_overflow();
    printf("\nEpisodic Memory: %s (%d failure(s))\n", g_failures ? "FAILED" : "ALL PASS", g_failures);
    return g_failures ? 1 : 0;
}
