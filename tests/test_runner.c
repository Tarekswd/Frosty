/**
 * @file test_runner.c
 * @brief Unified Test Runner — executes all module test suites.
 *
 * Runs tests for:
 *   - Tokenizer
 *   - Episodic Memory (backward-compatible memory tests)
 *   - Sentiment
 *   - Antigravity
 *   - Semantic Memory (V2)
 *   - User Profile (V2)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  define EXE ".exe"
#else
#  define EXE ""
#endif

/* Execute a test binary and return true if exit code == 0 */
static int run_suite(const char *binary, int *total) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), ".\\%s%s", binary, EXE);
    int rc = system(cmd);
    (*total)++;
    return (rc == 0) ? 1 : 0;
}

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║      AntiGravity Chatbot — Test Runner       ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    int passed = 0;
    int total  = 0;

    passed += run_suite("test_tokenizer",   &total);
    passed += run_suite("test_memory",      &total);
    passed += run_suite("test_sentiment",   &total);
    passed += run_suite("test_antigravity", &total);
    passed += run_suite("test_semantic",    &total);
    passed += run_suite("test_profile",     &total);

    printf("══════════════════════════════════════════════\n");
    if (passed == total) {
        printf("  Result: %d / %d test suites passed \xE2\x9C\x85\n", passed, total);
        printf("══════════════════════════════════════════════\n");
        return 0;
    } else {
        printf("  Result: %d / %d test suites passed \xE2\x9D\x8C\n", passed, total);
        printf("══════════════════════════════════════════════\n");
        return 1;
    }
}
