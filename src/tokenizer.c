/**
 * @file tokenizer.c
 * @brief Word-based tokenizer with open-addressing hash map for vocabulary.
 */
#include "tokenizer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ──────────────────────────────── Hash-map internals ─── */

#define HASH_SIZE 4096   /* must be power of 2 */
#define HASH_MASK (HASH_SIZE - 1)

typedef struct HashEntry {
    char  word[TOK_MAX_WORD];
    int   id;
    bool  used;
} HashEntry;

struct Tokenizer {
    HashEntry  table[HASH_SIZE];   /* word → id */
    char     **id_to_word;         /* id  → word (dynamic array) */
    int        vocab_size;
    int        capacity;
};

/* Simple djb2 hash */
static unsigned int hash_word(const char *w) {
    unsigned int h = 5381;
    for (; *w; ++w) h = ((h << 5) + h) + (unsigned char)tolower((unsigned char)*w);
    return h & HASH_MASK;
}

static int vocab_add(Tokenizer *tok, const char *word) {
    unsigned int h = hash_word(word);
    unsigned int probe = h;
    for (unsigned int i = 0; i < HASH_SIZE; ++i) {
        probe = (h + i) & HASH_MASK;
        HashEntry *e = &tok->table[probe];
        if (!e->used) {
            str_safe_copy(e->word, word, TOK_MAX_WORD);
            e->id   = tok->vocab_size;
            e->used = true;
            /* grow id_to_word array */
            if (tok->vocab_size >= tok->capacity) {
                int new_cap = tok->capacity * 2;
                char **tmp = realloc(tok->id_to_word, (size_t)new_cap * sizeof(char *));
                if (!tmp) return -1;
                tok->id_to_word = tmp;
                tok->capacity   = new_cap;
            }
            tok->id_to_word[tok->vocab_size] = str_dup(word);
            return tok->vocab_size++;
        }
        if (e->used && strcasecmp(e->word, word) == 0)
            return e->id;
    }
    return TOK_UNK;
}

static int vocab_lookup(const Tokenizer *tok, const char *word) {
    unsigned int h = hash_word(word);
    for (unsigned int i = 0; i < HASH_SIZE; ++i) {
        unsigned int probe = (h + i) & HASH_MASK;
        const HashEntry *e = &tok->table[probe];
        if (!e->used) return TOK_UNK;
        if (strcasecmp(e->word, word) == 0) return e->id;
    }
    return TOK_UNK;
}

/* ──────────────────────────────── Public API ─── */

Tokenizer *tok_init(const char *vocab_path) {
    Tokenizer *tok = calloc(1, sizeof(Tokenizer));
    if (!tok) return NULL;

    tok->capacity   = 256;
    tok->id_to_word = malloc((size_t)tok->capacity * sizeof(char *));
    if (!tok->id_to_word) { free(tok); return NULL; }

    /* Insert special tokens first */
    vocab_add(tok, "<unk>");
    vocab_add(tok, "<pad>");
    vocab_add(tok, "<sos>");
    vocab_add(tok, "<eos>");

    /* Load vocab file */
    FILE *f = fopen(vocab_path, "r");
    if (!f) {
        log_warn("Vocab file '%s' not found; using built-in vocab only.", vocab_path);
    } else {
        char line[TOK_MAX_WORD];
        while (fgets(line, sizeof(line), f)) {
            str_trim(line);
            if (line[0] == '\0' || line[0] == '#') continue;
            str_lower(line);
            vocab_add(tok, line);
        }
        fclose(f);
    }

    log_info("Tokenizer initialised: %d tokens in vocabulary.", tok->vocab_size);
    return tok;
}

void tok_destroy(Tokenizer *tok) {
    if (!tok) return;
    for (int i = 0; i < tok->vocab_size; ++i) free(tok->id_to_word[i]);
    free(tok->id_to_word);
    free(tok);
}

int tok_encode(Tokenizer *tok, const char *input, int out[TOK_MAX_TOKENS]) {
    if (!tok || !input || !out) return 0;

    char buf[4096];
    str_safe_copy(buf, input, sizeof(buf));
    str_lower(buf);

    int count = 0;
    char *ptr = buf;

    while (*ptr && count < TOK_MAX_TOKENS - 1) {
        /* Skip non-alpha-numeric characters (use as delimiters) */
        while (*ptr && !isalnum((unsigned char)*ptr) && *ptr != '\'') ++ptr;
        if (!*ptr) break;

        /* Collect a word */
        char word[TOK_MAX_WORD];
        int  wlen = 0;
        while (*ptr && (isalnum((unsigned char)*ptr) || *ptr == '\'') && wlen < TOK_MAX_WORD - 1)
            word[wlen++] = *ptr++;
        word[wlen] = '\0';
        if (wlen == 0) continue;

        int id = vocab_lookup(tok, word);
        if (id == TOK_UNK && wlen > 0) {
            /* dynamically add unknown words to session vocab */
            id = vocab_add(tok, word);
            if (id < 0) id = TOK_UNK;
        }
        out[count++] = id;
    }
    out[count] = TOK_EOS;
    return count;
}

void tok_decode(Tokenizer *tok, const int *ids, int n, char *out, size_t out_sz) {
    if (!tok || !ids || !out || out_sz == 0) return;
    out[0] = '\0';
    size_t pos = 0;
    for (int i = 0; i < n; ++i) {
        const char *w = tok_id_to_str(tok, ids[i]);
        if (ids[i] == TOK_EOS) break;
        if (i > 0 && pos < out_sz - 1) out[pos++] = ' ';
        size_t wlen = strlen(w);
        if (pos + wlen >= out_sz - 1) wlen = out_sz - 1 - pos;
        memcpy(out + pos, w, wlen);
        pos += wlen;
    }
    out[pos] = '\0';
}

const char *tok_id_to_str(Tokenizer *tok, int id) {
    if (!tok || id < 0 || id >= tok->vocab_size) return "<unk>";
    return tok->id_to_word[id];
}

int tok_str_to_id(Tokenizer *tok, const char *word) {
    if (!tok || !word) return TOK_UNK;
    return vocab_lookup(tok, word);
}

int tok_vocab_size(Tokenizer *tok) {
    return tok ? tok->vocab_size : 0;
}
