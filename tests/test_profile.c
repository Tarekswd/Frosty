/**
 * @file test_profile.c
 * @brief Unit tests for the user profile store.
 */
#include <stdio.h>
#include <string.h>
#include "../src/memory/profile.h"
#include "../src/utils.h"

#define TEST(name) printf("  [ TEST ] %s ... ", #name); fflush(stdout)
#define PASS()     printf("PASS\n")
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); g_failures++; } while(0)

static int g_failures = 0;

static void test_set_get(void) {
    TEST(profile_set_and_get);
    ProfileStore *s = profile_create(NULL);
    profile_set(s, "alice", "tone", "casual");
    char val[64];
    if (!profile_get(s, "alice", "tone", val, sizeof(val))) FAIL("get returned false");
    else if (strcmp(val, "casual") != 0) FAIL("value mismatch");
    else PASS();
    profile_destroy(s);
}

static void test_update(void) {
    TEST(profile_update_existing_key);
    ProfileStore *s = profile_create(NULL);
    profile_set(s, "u", "bot_name", "HAL");
    profile_set(s, "u", "bot_name", "Aura");  /* update */
    char val[64];
    profile_get(s, "u", "bot_name", val, sizeof(val));
    if (strcmp(val, "Aura") != 0) FAIL("update did not take effect"); else PASS();
    profile_destroy(s);
}

static void test_missing_key(void) {
    TEST(profile_get_missing_key_returns_false);
    ProfileStore *s = profile_create(NULL);
    char val[64];
    if (profile_get(s, "nobody", "missing", val, sizeof(val)))
        FAIL("expected false for missing key");
    else PASS();
    profile_destroy(s);
}

static void test_user_isolation(void) {
    TEST(profile_user_isolation);
    ProfileStore *s = profile_create(NULL);
    profile_set(s, "alice", "bot_name", "Alice-Bot");
    profile_set(s, "bob",   "bot_name", "Bob-Bot");
    char val[64];
    profile_get(s, "alice", "bot_name", val, sizeof(val));
    if (strcmp(val, "Alice-Bot") != 0) FAIL("alice's bot_name wrong"); else PASS();
    profile_destroy(s);
}

static void test_default_bot_name(void) {
    TEST(profile_get_bot_name_default);
    ProfileStore *s = profile_create(NULL);
    char val[64];
    profile_get_bot_name(s, "nobody", val, sizeof(val));
    if (strcmp(val, PROF_DEFAULT_BOT_NAME) != 0) FAIL("default name wrong"); else PASS();
    profile_destroy(s);
}

static void test_persistence(void) {
    TEST(profile_save_and_reload);
    const char *path = "data/test_profile_tmp.bin";
    ProfileStore *s1 = profile_create(path);
    profile_set(s1, "u", "bot_name", "Cassidy");
    if (!profile_save(s1)) { FAIL("save failed"); profile_destroy(s1); return; }
    profile_destroy(s1);

    ProfileStore *s2 = profile_create(path);
    char val[64];
    if (!profile_get(s2, "u", "bot_name", val, sizeof(val)))
        FAIL("key missing after reload");
    else if (strcmp(val, "Cassidy") != 0) FAIL("value mismatch after reload");
    else PASS();
    profile_destroy(s2);
    remove(path);
}

int main(void) {
    printf("\n=== Profile Tests ===\n");
    test_set_get();
    test_update();
    test_missing_key();
    test_user_isolation();
    test_default_bot_name();
    test_persistence();
    printf("\nProfile: %s (%d failure(s))\n\n",
           g_failures ? "FAILED" : "ALL PASS", g_failures);
    return g_failures ? 1 : 0;
}
