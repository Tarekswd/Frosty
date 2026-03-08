/**
 * @file test_tokenizer.c
 */
#include <stdio.h>
#include <string.h>
#include "../src/tokenizer.h"
#define TEST(name) printf("  [ TEST ] %s ... ", #name); fflush(stdout)
#define PASS()     printf("PASS\n")
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); g_failures++; } while(0)
static int g_failures = 0;

static void test_special_tokens(void) {
    TEST(t_special_tokens);
    Tokenizer *t = tok_init(NULL);
    if (tok_str_to_id(t, "<unk>") != TOK_UNK ||
        tok_str_to_id(t, "<pad>") != TOK_PAD ||
        tok_str_to_id(t, "<sos>") != TOK_SOS ||
        tok_str_to_id(t, "<eos>") != TOK_EOS) FAIL("special tokens mismatch");
    else PASS();
    tok_destroy(t);
}

static void test_encode_decode(void) {
    TEST(t_encode_decode);
    Tokenizer *t = tok_init(NULL);
    int tokens[TOK_MAX_TOKENS];
    int n = tok_encode(t, "hello world hello", tokens);
    if (n != 3) FAIL("expected 3 tokens");
    else if (tokens[0] != tokens[2]) FAIL("hello != hello");
    else PASS();
    tok_destroy(t);
}

int main(void) {
    printf("\n=== Tokenizer Tests ===\n");
    test_special_tokens();
    test_encode_decode();
    printf("\nTokenizer: %s (%d failure(s))\n", g_failures ? "FAILED" : "ALL PASS", g_failures);
    return g_failures ? 1 : 0;
}
