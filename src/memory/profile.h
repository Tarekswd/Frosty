/**
 * @file profile.h
 * @brief User profile key-value store with binary persistence.
 *
 * Stores arbitrary string key→value pairs scoped to a user_id.
 * Entries are kept in a flat in-memory list and flushed to a binary
 * file on save.
 *
 * Special well-known keys:
 *   "bot_name"    — Name the user has assigned to the bot.
 *   "tone"        — Preferred response tone ("formal", "casual", …).
 *   "interests"   — Comma-separated list of topics the user likes.
 *
 * File format (little-endian):
 *   [ProfileFileHeader]
 *   [ProfileEntry] × count
 */
#ifndef PROFILE_H
#define PROFILE_H

#include <stdbool.h>
#include <stddef.h>

#define PROF_USER_LEN   32    /**< Max user_id length. */
#define PROF_KEY_LEN    64    /**< Max key length. */
#define PROF_VAL_LEN   256    /**< Max value length. */
#define PROF_MAX_ENTRIES 512  /**< Max total entries across all users. */

/** Default bot name before the user assigns one. */
#define PROF_DEFAULT_BOT_NAME "Aura"

/** Opaque user profile store. */
typedef struct ProfileStore ProfileStore;

/**
 * @brief Create or load a profile store from a file.
 * @param path Binary file path.  Created if it does not exist.
 * @return New ProfileStore, or NULL on failure.
 */
ProfileStore *profile_create(const char *path);

/**
 * @brief Free the store (saves automatically).
 * @param store The store to destroy.
 */
void profile_destroy(ProfileStore *store);

/**
 * @brief Set a key-value pair for a user.  Creates or updates the entry.
 * @param store   The profile store.
 * @param user_id The user identifier.
 * @param key     The key name.
 * @param value   The value string.
 * @return true on success.
 */
bool profile_set(ProfileStore *store,
                 const char *user_id,
                 const char *key,
                 const char *value);

/**
 * @brief Retrieve the value for a key.
 * @param store   The profile store.
 * @param user_id The user identifier.
 * @param key     The key name.
 * @param out     Output buffer.
 * @param out_sz  Size of output buffer.
 * @return true if the key exists and was written to out.
 */
bool profile_get(ProfileStore *store,
                 const char *user_id,
                 const char *key,
                 char *out,
                 size_t out_sz);

/**
 * @brief Convenience: get the bot's name for a user.
 *        Returns PROF_DEFAULT_BOT_NAME if not set.
 * @param store   The profile store.
 * @param user_id The user identifier.
 * @param out     Output buffer (>= 64 bytes recommended).
 * @param out_sz  Size of output buffer.
 */
void profile_get_bot_name(ProfileStore *store,
                           const char *user_id,
                           char *out,
                           size_t out_sz);

/**
 * @brief Persist the store to disk.
 * @param store The profile store.
 * @return true on success.
 */
bool profile_save(ProfileStore *store);

#endif /* PROFILE_H */
