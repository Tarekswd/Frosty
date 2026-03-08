/**
 * @file model.h
 * @brief NLP Engine — V2: pattern-matching response generator with
 *        memory conditioning and persona (bot_name) support.
 */
#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>
#include "memory.h"

#define MODEL_MAX_PATTERNS  256
#define MODEL_MAX_RESPONSES  16
#define MODEL_RESP_LEN      512
#define MODEL_KEY_LEN        64

/** Default bot name when user has not set one. */
#define PROF_DEFAULT_BOT_NAME "Aura"

/** Opaque model handle. */
typedef struct NLPModel NLPModel;

/**
 * @brief Load the response pattern database from a file.
 * @param responses_path Path to `data/responses.txt`.
 * @return Newly allocated NLPModel, or NULL on failure.
 */
NLPModel *model_load(const char *responses_path);

/**
 * @brief Free all resources held by the model.
 * @param model The model to destroy.
 */
void model_destroy(NLPModel *model);

/**
 * @brief V1: Generate a response (no persona, no memories). Kept for compat.
 */
void model_respond(NLPModel *model,
                   const char *input,
                   int sentiment_score,
                   const ConvMemory *mem,
                   char *out,
                   size_t out_sz);

/**
 * @brief V2: Generate a response conditioned on bot persona and semantic memories.
 *
 * @param model           The loaded NLP model.
 * @param input           Raw user input (pre-lowercased).
 * @param sentiment_score Sentiment in [-10, 10].
 * @param bot_name        Bot's name for this user (from user profile).
 * @param memories        Array of memory text strings (may be NULL).
 * @param mem_count       Number of memory strings.
 * @param out             Output buffer.
 * @param out_sz          Size of output buffer.
 */
void model_respond_v2(NLPModel *model,
                      const char *input,
                      int sentiment_score,
                      const char *bot_name,
                      const char **memories,
                      int mem_count,
                      char *out,
                      size_t out_sz);

#endif /* MODEL_H */
