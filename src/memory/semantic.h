/**
 * @file semantic.h
 * @brief Long-term semantic memory: vector store with cosine-similarity retrieval.
 *
 * Each memory entry stores:
 *   - A short text (fact, summary, or user preference)
 *   - Its 128-dim embedding vector (pre-computed by embed.c)
 *   - An importance score (0.0–1.0) that decays over time
 *   - A user_id tag for multi-user isolation
 *   - A Unix timestamp
 *
 * Retrieval is linear-scan cosine similarity: O(n·d).  For n ≤ 10 000
 * and d = 128 this takes < 10 ms on any modern CPU — fast enough for
 * real-time conversation.
 *
 * Persistence uses a compact binary flat file with a versioned header.
 */
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include "../embed.h"

#define SEM_TEXT_LEN    512   /**< Max chars per memory text. */
#define SEM_USER_LEN     32   /**< Max chars in a user_id. */
#define SEM_MAX_ENTRIES 4096  /**< Max memories in store. */
#define SEM_TOP_K          5  /**< Default top-k for search results. */

/** A single semantic memory entry. */
typedef struct {
    char   user_id[SEM_USER_LEN];
    char   text[SEM_TEXT_LEN];
    float  vec[EMBED_DIM];        /**< Pre-computed L2-unit embedding. */
    float  importance;             /**< 0.0–1.0; higher = more relevant. */
    time_t timestamp;
    bool   active;                 /**< false = logically deleted. */
} SemanticEntry;

/** A search result pairing a score with an entry pointer. */
typedef struct {
    float               score;
    const SemanticEntry *entry;
} SemResult;

/** Opaque semantic memory store. */
typedef struct SemanticStore SemanticStore;

/**
 * @brief Create (or load) a semantic store from the given file.
 * @param path Binary file path.  Created if it does not exist.
 * @return New SemanticStore, or NULL on failure.
 */
SemanticStore *sem_create(const char *path);

/**
 * @brief Free and optionally save the store.
 * @param store The store to destroy (saved automatically).
 */
void sem_destroy(SemanticStore *store);

/**
 * @brief Add a new memory to the store.
 *
 * @param store      The semantic store.
 * @param user_id    Owner of this memory.
 * @param text       Human-readable text of the memory.
 * @param vec        Pre-computed EMBED_DIM embedding, or NULL to
 *                   compute it internally.
 * @param importance Initial importance in [0.0, 1.0].
 * @return true on success.
 */
bool sem_add(SemanticStore *store,
             const char *user_id,
             const char *text,
             const float vec[EMBED_DIM],
             float importance);

/**
 * @brief Search for the top_k most similar memories for a given user.
 *
 * Results are sorted descending by cosine similarity.
 *
 * @param store    The semantic store.
 * @param user_id  Restrict search to this user ("*" = all users).
 * @param query    Query vector (EMBED_DIM, unit-normalised).
 * @param top_k    Number of results to return.
 * @param results  Caller-allocated array of at least top_k SemResult.
 * @return Actual number of results returned (<= top_k).
 */
int sem_search(SemanticStore *store,
               const char *user_id,
               const float query[EMBED_DIM],
               int top_k,
               SemResult results[]);

/**
 * @brief Remove all entries below the given importance threshold.
 * @param store          The semantic store.
 * @param user_id        Target user (or NULL for all users).
 * @param min_importance Entries with importance < this are deleted.
 * @return Number of entries removed.
 */
int sem_forget(SemanticStore *store,
               const char *user_id,
               float min_importance);

/**
 * @brief Apply time-based importance decay.
 *
 * Importance halves every `half_life_days` days.
 *
 * @param store          The semantic store.
 * @param half_life_days Decay half-life in days (e.g., 7.0).
 */
void sem_decay(SemanticStore *store, double half_life_days);

/**
 * @brief Persist the store to its file.
 * @param store The store to save.
 * @return true on success.
 */
bool sem_save(SemanticStore *store);

/**
 * @brief Return the total number of active entries.
 * @param store The semantic store.
 * @return Active entry count.
 */
int sem_count(const SemanticStore *store);

#endif /* SEMANTIC_H */
