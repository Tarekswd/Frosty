/**
 * @file semantic.c
 * @brief Long-term semantic memory implementation.
 *
 * Flat binary file format:
 *   [FileHeader]  (magic + version + count)
 *   [SemanticEntry] × count
 *
 * Linear cosine-similarity scan for top-k retrieval (insertion sort).
 */
#include "semantic.h"
#include "../utils.h"
#include "../embed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ──────────────────────────────── File format ─── */

#define SEM_MAGIC   0xSE4A1B2Cu
#define SEM_VERSION 2u

typedef struct {
    unsigned int magic;
    unsigned int version;
    int          count;
} SemFileHeader;

#undef SEM_MAGIC
#define SEM_MAGIC 0x5E4A1B2Cu

/* ──────────────────────────────── Store ─── */

struct SemanticStore {
    SemanticEntry entries[SEM_MAX_ENTRIES];
    int           count;        /* total slots used (including inactive) */
    int           active_count;
    char         *path;
};

/* ──────────────────────────────── Helpers ─── */

/* Insert a (score, entry*) pair into a sorted top-k array (descending). */
static void topk_insert(SemResult *res, int *found, int top_k,
                         float score, const SemanticEntry *entry)
{
    if (*found < top_k || score > res[*found - 1].score) {
        int pos = (*found < top_k) ? *found : top_k - 1;
        if (*found < top_k) ++(*found);
        /* Shift down */
        for (int i = pos; i > 0 && res[i-1].score < score; --i) {
            res[i] = res[i-1];
            pos = i - 1;
        }
        res[pos].score = score;
        res[pos].entry = entry;
    }
}

/* ──────────────────────────────── API ─── */

SemanticStore *sem_create(const char *path) {
    SemanticStore *s = calloc(1, sizeof(SemanticStore));
    if (!s) return NULL;
    s->path = str_dup(path);

    FILE *f = path ? fopen(path, "rb") : NULL;
    if (!f) {
        log_info("sem_create: no existing store at '%s'; starting fresh.", path ? path : "(null)");
        return s;
    }

    SemFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 ||
        hdr.magic != SEM_MAGIC || hdr.version != SEM_VERSION) {
        log_warn("sem_create: invalid store file '%s'; starting fresh.", path);
        fclose(f);
        return s;
    }

    int to_load = hdr.count < SEM_MAX_ENTRIES ? hdr.count : SEM_MAX_ENTRIES;
    s->count = (int)fread(s->entries, sizeof(SemanticEntry), (size_t)to_load, f);
    fclose(f);

    for (int i = 0; i < s->count; ++i)
        if (s->entries[i].active) ++s->active_count;

    log_info("sem_create: loaded %d entries (%d active) from '%s'.",
             s->count, s->active_count, path);
    return s;
}

void sem_destroy(SemanticStore *store) {
    if (!store) return;
    sem_save(store);
    free(store->path);
    free(store);
}

bool sem_add(SemanticStore *store,
             const char *user_id,
             const char *text,
             const float vec[EMBED_DIM],
             float importance)
{
    if (!store || !user_id || !text) return false;
    if (store->count >= SEM_MAX_ENTRIES) {
        /* Try to reclaim a deleted slot */
        for (int i = 0; i < store->count; ++i) {
            if (!store->entries[i].active) {
                /* Reuse this slot */
                SemanticEntry *e = &store->entries[i];
                str_safe_copy(e->user_id,  user_id,    SEM_USER_LEN);
                str_safe_copy(e->text,     text,        SEM_TEXT_LEN);
                if (vec) memcpy(e->vec, vec, EMBED_DIM * sizeof(float));
                else     embed_text(text, e->vec);
                e->importance = importance < 0.0f ? 0.0f :
                                importance > 1.0f ? 1.0f : importance;
                e->timestamp  = time(NULL);
                e->active     = true;
                ++store->active_count;
                return true;
            }
        }
        log_warn("sem_add: store is full (%d entries).", SEM_MAX_ENTRIES);
        return false;
    }

    SemanticEntry *e = &store->entries[store->count++];
    str_safe_copy(e->user_id,  user_id,    SEM_USER_LEN);
    str_safe_copy(e->text,     text,        SEM_TEXT_LEN);
    if (vec) memcpy(e->vec, vec, EMBED_DIM * sizeof(float));
    else     embed_text(text, e->vec);
    e->importance = importance < 0.0f ? 0.0f :
                    importance > 1.0f ? 1.0f : importance;
    e->timestamp  = time(NULL);
    e->active     = true;
    ++store->active_count;
    return true;
}

int sem_search(SemanticStore *store,
               const char *user_id,
               const float query[EMBED_DIM],
               int top_k,
               SemResult results[])
{
    if (!store || !query || !results || top_k <= 0) return 0;
    if (top_k > SEM_MAX_ENTRIES) top_k = SEM_MAX_ENTRIES;

    int found = 0;
    bool all_users = (user_id == NULL || strcmp(user_id, "*") == 0);

    for (int i = 0; i < store->count; ++i) {
        SemanticEntry *e = &store->entries[i];
        if (!e->active) continue;
        if (!all_users && strcmp(e->user_id, user_id) != 0) continue;

        /* Importance gate: below 0.05 is practically irrelevant */
        if (e->importance < 0.05f) continue;

        float score = embed_cosine(query, e->vec) * e->importance;
        topk_insert(results, &found, top_k, score, e);
    }
    return found;
}

int sem_forget(SemanticStore *store, const char *user_id, float min_importance) {
    if (!store) return 0;
    int removed = 0;
    bool all_users = (user_id == NULL || strcmp(user_id, "*") == 0);
    for (int i = 0; i < store->count; ++i) {
        SemanticEntry *e = &store->entries[i];
        if (!e->active) continue;
        if (!all_users && strcmp(e->user_id, user_id) != 0) continue;
        if (e->importance < min_importance) {
            e->active = false;
            --store->active_count;
            ++removed;
        }
    }
    if (removed) log_info("sem_forget: removed %d entries.", removed);
    return removed;
}

void sem_decay(SemanticStore *store, double half_life_days) {
    if (!store || half_life_days <= 0.0) return;
    time_t now = time(NULL);
    double decay_constant = log(2.0) / (half_life_days * 86400.0);
    for (int i = 0; i < store->count; ++i) {
        SemanticEntry *e = &store->entries[i];
        if (!e->active) continue;
        double age_secs = difftime(now, e->timestamp);
        if (age_secs < 0) age_secs = 0;
        e->importance *= (float)exp(-decay_constant * age_secs);
    }
}

bool sem_save(SemanticStore *store) {
    if (!store || !store->path) return false;
    FILE *f = fopen(store->path, "wb");
    if (!f) { log_warn("sem_save: cannot write to '%s'.", store->path); return false; }
    SemFileHeader hdr = { SEM_MAGIC, SEM_VERSION, store->count };
    fwrite(&hdr, sizeof(hdr), 1, f);
    fwrite(store->entries, sizeof(SemanticEntry), (size_t)store->count, f);
    fclose(f);
    log_debug("sem_save: wrote %d entries to '%s'.", store->count, store->path);
    return true;
}

int sem_count(const SemanticStore *store) {
    return store ? store->active_count : 0;
}
