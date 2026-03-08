/**
 * @file memory.h
 * @brief Conversation memory manager.
 *
 * Maintains a sliding window of the most recent N dialogue turns in RAM,
 * and optionally persists the history to a binary file on disk.
 *
 * Each entry stores:
 *  - role:    "user" or "bot"
 *  - content: the message text (up to MEM_MAX_CONTENT chars)
 *  - timestamp: Unix epoch seconds
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#define MEM_WINDOW_SIZE  20       /**< Max dialogue turns kept in RAM. */
#define MEM_MAX_CONTENT  1024     /**< Max characters per message. */
#define MEM_ROLE_USER    "user"
#define MEM_ROLE_BOT     "bot"

/** A single dialogue turn. */
typedef struct {
    char   role[8];
    char   content[MEM_MAX_CONTENT];
    time_t timestamp;
} MemEntry;

/** Opaque memory manager handle. */
typedef struct ConvMemory ConvMemory;

/**
 * @brief Create a new conversation memory with given window size.
 * @param window_size Max turns to keep (clamped to MEM_WINDOW_SIZE).
 * @param persist_path Path to persistent binary store, or NULL to disable.
 * @return Newly allocated ConvMemory, or NULL on failure.
 */
ConvMemory *mem_create(int window_size, const char *persist_path);

/**
 * @brief Free all memory associated with the conversation memory.
 * @param mem The memory to destroy.
 */
void mem_destroy(ConvMemory *mem);

/**
 * @brief Push a new entry onto the sliding window.
 *
 * If the window is full the oldest entry is evicted.
 *
 * @param mem     The conversation memory.
 * @param role    Either MEM_ROLE_USER or MEM_ROLE_BOT.
 * @param content The message text.
 */
void mem_push(ConvMemory *mem, const char *role, const char *content);

/**
 * @brief Get the number of entries currently in the window.
 * @param mem The conversation memory.
 * @return Entry count.
 */
int mem_count(const ConvMemory *mem);

/**
 * @brief Access an entry by index (0 = oldest, count-1 = newest).
 * @param mem The conversation memory.
 * @param idx Index.
 * @return Pointer to the entry (owned by mem), or NULL if out of range.
 */
const MemEntry *mem_get(const ConvMemory *mem, int idx);

/**
 * @brief Build a formatted context string from recent turns.
 *
 * Produces output like:
 *   [user]: hello
 *   [bot]:  hi there!
 *
 * @param mem    The conversation memory.
 * @param out    Output buffer.
 * @param out_sz Size of output buffer.
 */
void mem_format_context(const ConvMemory *mem, char *out, size_t out_sz);

/**
 * @brief Clear the in-memory sliding window (does not affect disk).
 * @param mem The conversation memory.
 */
void mem_clear(ConvMemory *mem);

/**
 * @brief Save the current window to disk (if persist_path was set).
 * @param mem The conversation memory.
 * @return true on success.
 */
bool mem_save(ConvMemory *mem);

/**
 * @brief Load history from disk into the sliding window.
 * @param mem The conversation memory.
 * @return true on success.
 */
bool mem_load(ConvMemory *mem);

#endif /* MEMORY_H */
