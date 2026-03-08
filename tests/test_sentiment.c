/**
 * @file test_sentiment.c
 */
#include <stdio.h>
#include <string.h>
#include "../src/sentiment.h"
#define TEST(name) printf("  [ TEST ] %s ... ", #name); fflush(stdout)
#define PASS()     printf("PASS\n")
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); g_failures++; } while(0)
static int g_failures = 0;

static void test_pos(void) {
    TEST(s_positive);
    if (sentiment_score("i am very happy and glad") <= 2) FAIL("expected > 2"); else PASS();
}
static void test_neg(void) {
    TEST(s_negative);
    if (sentiment_score("this is terrible and sad") >= 0) FAIL("expected < 0"); else PASS();
}

int main(void) {
    printf("\n=== Sentiment Tests ===\n");
    test_pos(); test_neg();
    printf("\nSentiment: %s (%d failure(s))\n", g_failures ? "FAILED" : "ALL PASS", g_failures);
    return g_failures ? 1 : 0;
}
