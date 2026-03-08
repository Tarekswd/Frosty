/**
 * @file utils.c
 * @brief Implementation of utility functions: logger, string helpers,
 *        and a simple bump-pointer memory pool.
 */
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ──────────────────────────────────────── Logger ─── */

static LogLevel g_log_level = LOG_INFO;

void log_set_level(LogLevel level) { g_log_level = level; }

static void log_write(LogLevel level, const char *tag, const char *fmt, va_list ap) {
    if (level < g_log_level) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(stderr, "[%02d:%02d:%02d] [%s] ", t->tm_hour, t->tm_min, t->tm_sec, tag);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void log_info(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_write(LOG_INFO,  "INFO ", fmt, ap); va_end(ap);
}
void log_warn(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_write(LOG_WARN,  "WARN ", fmt, ap); va_end(ap);
}
void log_error(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_write(LOG_ERROR, "ERROR", fmt, ap); va_end(ap);
}
void log_debug(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_write(LOG_DEBUG, "DEBUG", fmt, ap); va_end(ap);
}

/* ──────────────────────────────────── String Helpers ─── */

void str_lower(char *s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

void str_trim(char *s) {
    /* leading */
    char *start = s;
    while (*start && isspace((unsigned char)*start)) ++start;
    if (start != s) memmove(s, start, strlen(start) + 1);
    /* trailing */
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

bool str_contains_ci(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    size_t hlen = strlen(haystack), nlen = strlen(needle);
    if (nlen > hlen) return false;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i+j]) ==
                            tolower((unsigned char)needle[j])) ++j;
        if (j == nlen) return true;
    }
    return false;
}

void str_safe_copy(char *dst, const char *src, size_t size) {
    if (!dst || size == 0) return;
    strncpy(dst, src ? src : "", size - 1);
    dst[size - 1] = '\0';
}

char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

/* ──────────────────────────────────── Memory Pool ─── */

struct MemPool {
    unsigned char *buf;
    size_t         cap;
    size_t         offset;
};

MemPool *pool_create(size_t capacity) {
    MemPool *p = malloc(sizeof(MemPool));
    if (!p) return NULL;
    p->buf = malloc(capacity);
    if (!p->buf) { free(p); return NULL; }
    p->cap    = capacity;
    p->offset = 0;
    return p;
}

void *pool_alloc(MemPool *pool, size_t n) {
    if (!pool) return NULL;
    /* align to 8 bytes */
    size_t aligned = (n + 7u) & ~7u;
    if (pool->offset + aligned > pool->cap) {
        /* grow */
        size_t new_cap = pool->cap * 2 + aligned;
        unsigned char *new_buf = realloc(pool->buf, new_cap);
        if (!new_buf) return NULL;
        pool->buf = new_buf;
        pool->cap = new_cap;
    }
    void *ptr = pool->buf + pool->offset;
    pool->offset += aligned;
    return ptr;
}

void pool_reset(MemPool *pool) {
    if (pool) pool->offset = 0;
}

void pool_destroy(MemPool *pool) {
    if (!pool) return;
    free(pool->buf);
    free(pool);
}
