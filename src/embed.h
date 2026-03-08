/**
 * @file embed.h
 * @brief Lightweight 128-dimensional text embedding service.
 *
 * Uses the hashing trick to project word-frequency bag-of-words
 * representations into a fixed 128-dim float vector.  The result
 * is L2-normalised so cosine similarity == dot product.
 *
 * No external libraries required.  The embedding is deterministic,
 * cheap to compute (< 0.1 ms for typical sentences), and sufficient
 * for semantic similarity within a single domain / persona.
 */
#ifndef EMBED_H
#define EMBED_H

#include <stddef.h>

/** Dimensionality of the embedding vector. */
#define EMBED_DIM 128

/**
 * @brief Compute a normalised embedding vector for a text string.
 *
 * Algorithm:
 *  1. Tokenise text on whitespace / punctuation.
 *  2. For each token: bucket = djb2(token) % EMBED_DIM; vec[bucket] += 1.
 *  3. L2-normalise vec in-place (so ||vec|| == 1).
 *
 * @param text  Input text (NUL-terminated).  May be any length.
 * @param vec   Output array of EMBED_DIM floats.  Caller-owned.
 */
void embed_text(const char *text, float vec[EMBED_DIM]);

/**
 * @brief Compute cosine similarity between two EMBED_DIM unit vectors.
 *
 * Since embed_text always produces unit vectors, this is simply the
 * dot product and avoids an extra square-root.
 *
 * @param a  First unit vector (EMBED_DIM floats).
 * @param b  Second unit vector (EMBED_DIM floats).
 * @return   Cosine similarity in [-1.0, 1.0].
 */
float embed_cosine(const float a[EMBED_DIM], const float b[EMBED_DIM]);

#endif /* EMBED_H */
