/**
 * @file memory.c
 * @brief Circular-buffer-based sliding window conversation memory
 *        with binary file persistence.
 */
#include "memory.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* File magic + version for the binary format */
#define MEM_FILE_MAGIC   0xA1B2C3D4u
#define MEM_FILE_VERSION 1u

typedef struct {
    unsigned int magic;
    unsigned int version;
    int          count;
} FileHeader;

struct ConvMemory {
    MemEntry entries[MEM_WINDOW_SIZE];
    int      head;          /* next write position (circular) */
    int      count;         /* number of valid entries */
    int      window;        /* configured window size */
    char    *persist_path;  /* NULL → no persistence */
};

ConvMemory *mem_create(int window_size, const char *persist_path) {
    ConvMemory *m = calloc(1, sizeof(ConvMemory));
    if (!m) return NULL;

    m->window = (window_size < 1 || window_size > MEM_WINDOW_SIZE)
                    ? MEM_WINDOW_SIZE : window_size;
    m->head  = 0;
    m->count = 0;

    if (persist_path) {
        m->persist_path = str_dup(persist_path);
        mem_load(m);  /* load existing history if present */
    }
    return m;
}

void mem_destroy(ConvMemory *mem) {
    if (!mem) return;
    free(mem->persist_path);
    free(mem);
}

void mem_push(ConvMemory *mem, const char *role, const char *content) {
    if (!mem || !role || !content) return;

    MemEntry *e = &mem->entries[mem->head];
    str_safe_copy(e->role,    role,    sizeof(e->role));
    str_safe_copy(e->content, content, sizeof(e->content));
    e->timestamp = time(NULL);

    mem->head = (mem->head + 1) % mem->window;
    if (mem->count < mem->window) ++mem->count;
}

int mem_count(const ConvMemory *mem) {
    return mem ? mem->count : 0;
}

const MemEntry *mem_get(const ConvMemory *mem, int idx) {
    if (!mem || idx < 0 || idx >= mem->count) return NULL;
    /* Map logical idx (0=oldest) to physical ring position */
    int physical;
    if (mem->count < mem->window) {
        physical = idx;
    } else {
        physical = (mem->head + idx) % mem->window;
    }
    return &mem->entries[physical];
}

void mem_format_context(const ConvMemory *mem, char *out, size_t out_sz) {
    if (!mem || !out || out_sz == 0) return;
    out[0] = '\0';
    size_t pos = 0;
    for (int i = 0; i < mem->count; ++i) {
        const MemEntry *e = mem_get(mem, i);
        if (!e) continue;
        int written = snprintf(out + pos, out_sz - pos,
                               "[%s]: %s\n", e->role, e->content);
        if (written < 0 || (size_t)written >= out_sz - pos) break;
        pos += (size_t)written;
    }
}

void mem_clear(ConvMemory *mem) {
    if (!mem) return;
    memset(mem->entries, 0, sizeof(mem->entries));
    mem->head  = 0;
    mem->count = 0;
}

bool mem_save(ConvMemory *mem) {
    if (!mem || !mem->persist_path) return false;

    FILE *f = fopen(mem->persist_path, "wb");
    if (!f) {
        log_warn("mem_save: cannot open '%s' for writing.", mem->persist_path);
        return false;
    }

    FileHeader hdr = {
        .magic   = MEM_FILE_MAGIC,
        .version = MEM_FILE_VERSION,
        .count   = mem->count,
    };
    fwrite(&hdr, sizeof(hdr), 1, f);

    for (int i = 0; i < mem->count; ++i) {
        const MemEntry *e = mem_get(mem, i);
        if (e) fwrite(e, sizeof(MemEntry), 1, f);
    }
    fclose(f);
    log_debug("mem_save: saved %d entries to '%s'.", mem->count, mem->persist_path);
    return true;
}

bool mem_load(ConvMemory *mem) {
    if (!mem || !mem->persist_path) return false;

    FILE *f = fopen(mem->persist_path, "rb");
    if (!f) return false;  /* no file yet — that's fine */

    FileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 ||
        hdr.magic != MEM_FILE_MAGIC || hdr.version != MEM_FILE_VERSION) {
        log_warn("mem_load: invalid or corrupt history file.");
        fclose(f);
        return false;
    }

    mem_clear(mem);
    int to_load = hdr.count < mem->window ? hdr.count : mem->window;
    for (int i = 0; i < to_load; ++i) {
        MemEntry e;
        if (fread(&e, sizeof(MemEntry), 1, f) != 1) break;
        mem_push(mem, e.role, e.content);
    }
    fclose(f);
    log_info("mem_load: restored %d entries from '%s'.", mem->count, mem->persist_path);
    return true;
}
