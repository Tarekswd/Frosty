/**
 * @file utils.h
 * @brief Utility functions: string helpers, memory pool, and logger.
 *
 * Provides a lightweight memory pool allocator, string manipulation
 * helpers, and a configurable logger used across the entire chatbot.
 */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>

/* ──────────────────────────────────────── Logger ─── */

typedef enum { LOG_DEBUG = 0, LOG_INFO, LOG_WARN, LOG_ERROR } LogLevel;

/**
 * @brief Set the minimum log level printed to stderr.
 * @param level One of LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR.
 */
void log_set_level(LogLevel level);

/**
 * @brief Log an informational message.
 * @param fmt printf-style format string.
 */
void log_info(const char *fmt, ...);

/**
 * @brief Log a warning message.
 * @param fmt printf-style format string.
 */
void log_warn(const char *fmt, ...);

/**
 * @brief Log an error message.
 * @param fmt printf-style format string.
 */
void log_error(const char *fmt, ...);

/**
 * @brief Log a debug message (only shown when level <= LOG_DEBUG).
 * @param fmt printf-style format string.
 */
void log_debug(const char *fmt, ...);

/* ──────────────────────────────────── String Helpers ─── */

/**
 * @brief Convert string to lower-case in-place.
 * @param s The string to convert.
 */
void str_lower(char *s);

/**
 * @brief Trim leading and trailing whitespace in-place.
 * @param s The string to trim.
 */
void str_trim(char *s);

/**
 * @brief Check whether haystack contains the substring needle
 *        (case-insensitive).
 * @param haystack Source string.
 * @param needle   Substring to search for.
 * @return true if found, false otherwise.
 */
bool str_contains_ci(const char *haystack, const char *needle);

/**
 * @brief Safe strncpy that always NUL-terminates dst.
 * @param dst  Destination buffer.
 * @param src  Source string.
 * @param size Size of dst buffer (including NUL).
 */
void str_safe_copy(char *dst, const char *src, size_t size);

/**
 * @brief Duplicate a string using malloc.  Caller owns the result.
 * @param s Source string.
 * @return Heap-allocated copy, or NULL on allocation failure.
 */
char *str_dup(const char *s);

/* ──────────────────────────────────── Memory Pool ─── */

/** Opaque memory pool handle. */
typedef struct MemPool MemPool;

/**
 * @brief Create a new memory pool with the given initial capacity.
 * @param capacity Initial byte capacity.  Doubles on overflow.
 * @return Newly allocated pool, or NULL on failure.
 */
MemPool *pool_create(size_t capacity);

/**
 * @brief Allocate n bytes from the pool (always aligned to 8 bytes).
 * @param pool The pool to allocate from.
 * @param n    Number of bytes required.
 * @return Pointer to allocated region, or NULL on failure.
 */
void *pool_alloc(MemPool *pool, size_t n);

/**
 * @brief Reset the pool's offset to 0 (reuse memory without freeing).
 * @param pool The pool to reset.
 */
void pool_reset(MemPool *pool);

/**
 * @brief Free the pool and all its memory.
 * @param pool The pool to destroy.
 */
void pool_destroy(MemPool *pool);

#endif /* UTILS_H */
