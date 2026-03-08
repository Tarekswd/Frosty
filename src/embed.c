/**
 * @file embed.c
 * @brief 128-dim bag-of-words text embedding via the hashing trick.
 *
 * The hashing trick maps each unique word to one of EMBED_DIM buckets
 * using djb2 and accumulates term frequencies.  The resulting vector
 * is L2-normalised so that cosine similarity == dot product.
 */
#include "embed.h"

#include <string.h>
#include <ctype.h>
#include <math.h>

/* djb2 hash */
static unsigned int djb2(const char *s) {
    unsigned int h = 5381;
    for (; *s; ++s) h = ((h << 5) + h) + (unsigned char)tolower((unsigned char)*s);
    return h;
}

void embed_text(const char *text, float vec[EMBED_DIM]) {
    /* Zero-init */
    memset(vec, 0, EMBED_DIM * sizeof(float));
    if (!text || !*text) return;

    char buf[4096];
    size_t len = strlen(text);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, text, len);
    buf[len] = '\0';

    /* Tokenise on non-alphanumeric boundaries */
    char *p = buf;
    while (*p) {
        /* Skip delimiters */
        while (*p && !isalnum((unsigned char)*p)) ++p;
        if (!*p) break;
        /* Collect word */
        char word[64];
        int wi = 0;
        while (*p && isalnum((unsigned char)*p) && wi < 63)
            word[wi++] = (char)tolower((unsigned char)*p++);
        word[wi] = '\0';
        if (wi == 0) continue;
        /* Accumulate into bucket */
        unsigned int bucket = djb2(word) % EMBED_DIM;
        vec[bucket] += 1.0f;
    }

    /* L2 normalise */
    float norm = 0.0f;
    for (int i = 0; i < EMBED_DIM; ++i) norm += vec[i] * vec[i];
    if (norm > 1e-9f) {
        norm = sqrtf(norm);
        for (int i = 0; i < EMBED_DIM; ++i) vec[i] /= norm;
    }
}

float embed_cosine(const float a[EMBED_DIM], const float b[EMBED_DIM]) {
    float dot = 0.0f;
    for (int i = 0; i < EMBED_DIM; ++i) dot += a[i] * b[i];
    return dot;
}
