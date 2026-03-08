/**
 * @file profile.c
 * @brief User profile store: flat in-memory list + binary file persistence.
 */
#include "profile.h"
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────── File format ─── */

#define PROF_MAGIC   0x50524F46u  /* "PROF" */
#define PROF_VERSION 1u

typedef struct {
    unsigned int magic;
    unsigned int version;
    int          count;
} ProfileFileHeader;

typedef struct {
    char user_id[PROF_USER_LEN];
    char key    [PROF_KEY_LEN];
    char value  [PROF_VAL_LEN];
} ProfileEntry;

/* ──────────────────────────────── Store ─── */

struct ProfileStore {
    ProfileEntry entries[PROF_MAX_ENTRIES];
    int          count;
    char        *path;
};

/* ──────────────────────────────── Internal lookup ─── */

static ProfileEntry *find_entry(ProfileStore *s,
                                 const char *user_id,
                                 const char *key)
{
    for (int i = 0; i < s->count; ++i) {
        if (strcmp(s->entries[i].user_id, user_id) == 0 &&
            strcmp(s->entries[i].key,     key)     == 0)
            return &s->entries[i];
    }
    return NULL;
}

/* ──────────────────────────────── API ─── */

ProfileStore *profile_create(const char *path) {
    ProfileStore *s = calloc(1, sizeof(ProfileStore));
    if (!s) return NULL;
    s->path = str_dup(path);

    FILE *f = path ? fopen(path, "rb") : NULL;
    if (!f) {
        log_info("profile_create: no existing profile at '%s'.", path ? path : "(null)");
        return s;
    }

    ProfileFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 ||
        hdr.magic != PROF_MAGIC || hdr.version != PROF_VERSION) {
        log_warn("profile_create: invalid profile file; starting fresh.");
        fclose(f); return s;
    }

    int to_load = hdr.count < PROF_MAX_ENTRIES ? hdr.count : PROF_MAX_ENTRIES;
    s->count = (int)fread(s->entries, sizeof(ProfileEntry), (size_t)to_load, f);
    fclose(f);
    log_info("profile_create: loaded %d profile entries from '%s'.",
             s->count, path);
    return s;
}

void profile_destroy(ProfileStore *store) {
    if (!store) return;
    profile_save(store);
    free(store->path);
    free(store);
}

bool profile_set(ProfileStore *store,
                 const char *user_id,
                 const char *key,
                 const char *value)
{
    if (!store || !user_id || !key || !value) return false;

    ProfileEntry *e = find_entry(store, user_id, key);
    if (e) {
        str_safe_copy(e->value, value, PROF_VAL_LEN);
        return true;
    }
    if (store->count >= PROF_MAX_ENTRIES) {
        log_warn("profile_set: store full."); return false;
    }
    e = &store->entries[store->count++];
    str_safe_copy(e->user_id, user_id, PROF_USER_LEN);
    str_safe_copy(e->key,     key,     PROF_KEY_LEN);
    str_safe_copy(e->value,   value,   PROF_VAL_LEN);
    return true;
}

bool profile_get(ProfileStore *store,
                 const char *user_id,
                 const char *key,
                 char *out,
                 size_t out_sz)
{
    if (!store || !user_id || !key || !out) return false;
    const ProfileEntry *e = find_entry(store, user_id, key);
    if (!e) return false;
    str_safe_copy(out, e->value, out_sz);
    return true;
}

void profile_get_bot_name(ProfileStore *store,
                           const char *user_id,
                           char *out,
                           size_t out_sz)
{
    if (!profile_get(store, user_id, "bot_name", out, out_sz))
        str_safe_copy(out, PROF_DEFAULT_BOT_NAME, out_sz);
}

bool profile_save(ProfileStore *store) {
    if (!store || !store->path) return false;
    FILE *f = fopen(store->path, "wb");
    if (!f) { log_warn("profile_save: cannot write '%s'.", store->path); return false; }
    ProfileFileHeader hdr = { PROF_MAGIC, PROF_VERSION, store->count };
    fwrite(&hdr, sizeof(hdr), 1, f);
    fwrite(store->entries, sizeof(ProfileEntry), (size_t)store->count, f);
    fclose(f);
    log_debug("profile_save: wrote %d entries.", store->count);
    return true;
}
