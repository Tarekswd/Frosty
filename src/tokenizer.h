/**
 * @file tokenizer.h
 * @brief Word-based tokenizer with special tokens.
 *
 * Provides a vocabulary hash-map, tokenization (string → ID array),
 * and detokenization (ID array → string).  Special tokens:
 *   ID 0 → <unk>
 *   ID 1 → <pad>
 *   ID 2 → <sos>
 *   ID 3 → <eos>
 */
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

#define TOK_UNK 0
#define TOK_PAD 1
#define TOK_SOS 2
#define TOK_EOS 3
#define TOK_MAX_TOKENS 512  /**< Maximum tokens per input sequence. */
#define TOK_MAX_WORD   64   /**< Maximum characters per token. */

/** Opaque tokenizer state. */
typedef struct Tokenizer Tokenizer;

/**
 * @brief Initialise the tokenizer, loading vocabulary from a file.
 * @param vocab_path Path to vocabulary file (one word per line).
 * @return Newly allocated Tokenizer, or NULL on failure.
 */
Tokenizer *tok_init(const char *vocab_path);

/**
 * @brief Free all resources held by the tokenizer.
 * @param tok The tokenizer to destroy.
 */
void tok_destroy(Tokenizer *tok);

/**
 * @brief Encode a string into an array of token IDs.
 *
 * Splits on whitespace and punctuation, lower-cases each token,
 * and maps to vocabulary IDs (unknown → TOK_UNK).
 *
 * @param tok    The tokenizer.
 * @param input  NUL-terminated input string.
 * @param out    Output array of at least TOK_MAX_TOKENS ints.
 * @return Number of tokens written to out (excluding terminating EOS).
 */
int tok_encode(Tokenizer *tok, const char *input, int out[TOK_MAX_TOKENS]);

/**
 * @brief Decode an array of token IDs back to a space-separated string.
 * @param tok   The tokenizer.
 * @param ids   Array of token IDs.
 * @param n     Number of IDs.
 * @param out   Output buffer.
 * @param out_sz Size of output buffer.
 */
void tok_decode(Tokenizer *tok, const int *ids, int n, char *out, size_t out_sz);

/**
 * @brief Look up the string for a given token ID.
 * @param tok The tokenizer.
 * @param id  Token ID.
 * @return Pointer to the token string (owned by tokenizer), or "<unk>".
 */
const char *tok_id_to_str(Tokenizer *tok, int id);

/**
 * @brief Look up the ID for a given word.
 * @param tok  The tokenizer.
 * @param word The word to look up (case-insensitive).
 * @return Token ID, or TOK_UNK if not found.
 */
int tok_str_to_id(Tokenizer *tok, const char *word);

/**
 * @brief Return the total vocabulary size (including special tokens).
 * @param tok The tokenizer.
 * @return Vocabulary size.
 */
int tok_vocab_size(Tokenizer *tok);

#endif /* TOKENIZER_H */
