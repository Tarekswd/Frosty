/**
 * @file episodic.h
 * @brief Short-term episodic memory: per-session sliding window of dialogue turns.
 *
 * This is a renamed, header-compatible wrapper around the original memory.h
 * API.  The underlying circular-buffer ring + binary persistence are unchanged.
 * The type alias EpisodicMemory == ConvMemory ensures backward compatibility.
 */
#ifndef EPISODIC_H
#define EPISODIC_H

/*
 * Re-export the existing memory module under the EpisodicMemory alias.
 * All functions prefixed with mem_ continue to work unchanged.
 */
#include "../memory.h"

/** Alias bringing the V2 naming into line with the architecture docs. */
typedef ConvMemory EpisodicMemory;

/* Convenience macros so callers can use either naming convention. */
#define episodic_create   mem_create
#define episodic_push     mem_push
#define episodic_get      mem_get
#define episodic_count    mem_count
#define episodic_format   mem_format_context
#define episodic_clear    mem_clear
#define episodic_save     mem_save
#define episodic_load     mem_load
#define episodic_destroy  mem_destroy

#endif /* EPISODIC_H */
