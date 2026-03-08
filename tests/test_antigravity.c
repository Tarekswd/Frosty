/**
 * @file test_antigravity.c
 */
#include <stdio.h>
#include <string.h>
#include "../src/antigravity.h"
#define TEST(name) printf("  [ TEST ] %s ... ", #name); fflush(stdout)
#define PASS()     printf("PASS\n")
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); g_failures++; } while(0)
static int g_failures = 0;

static void test_detect(void) {
    TEST(a_detect);
    ag_init(true);
    if (!ag_detect("turn on antigravity")) FAIL("should detect");
    else if (ag_detect("hello there")) FAIL("should not detect");
    else PASS();
}

static void test_lift(void) {
    TEST(a_lift);
    ag_init(true);
    float kg = 70.0f;
    float l0 = ag_lift_value(0);
    float lp = ag_lift_value(5);
    float ln = ag_lift_value(-5);
    if (lp <= l0 || ln >= l0) FAIL("lift mood scaling wrong");
    else PASS();
}

int main(void) {
    printf("\n=== Antigravity Tests ===\n");
    test_detect(); test_lift();
    printf("\nAntigravity: %s (%d failure(s))\n", g_failures ? "FAILED" : "ALL PASS", g_failures);
    return g_failures ? 1 : 0;
}
