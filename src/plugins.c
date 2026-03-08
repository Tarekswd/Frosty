/**
 * @file plugins.c
 * @brief Plugin registry implementation and built-in plugin definitions.
 *
 * Built-in plugins (all triggered by "/" prefix commands):
 *   /help     — list available commands
 *   /clear    — clear conversation history (handled in main.c via flag)
 *   /history  — show last 5 dialogue turns
 *   /version  — show chatbot version info
 *   /lift     — show current antigravity lift value
 */
#include "plugins.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────── Registry ─── */

typedef struct {
    char          name[PLUGIN_NAME_LEN];
    char          trigger[PLUGIN_TRIGGER_LEN];
    PluginHandler handler;
} PluginEntry;

struct PluginRegistry {
    PluginEntry entries[PLUGIN_MAX];
    int         count;
};

/* ──────────────────────────────── Built-in handlers ─── */

static bool handler_help(const char *input, char *out, size_t out_sz) {
    (void)input;
    snprintf(out, out_sz,
        "=== Available Commands ===\n"
        "  /help      Show this help message\n"
        "  /clear     Clear conversation history\n"
        "  /history   Show recent conversation turns\n"
        "  /version   Show chatbot version\n"
        "  /lift      Calculate your current antigravity lift\n"
        "\nOr just chat naturally — try saying 'antigravity' for a surprise!"
    );
    return true;
}

static bool handler_version(const char *input, char *out, size_t out_sz) {
    (void)input;
    snprintf(out, out_sz,
        "AntiGravity Chatbot v1.0.0\n"
        "Built with C11 | Modular NLP Engine\n"
        "Defying gravity since 2026 (^o^)/"
    );
    return true;
}

static bool handler_clear(const char *input, char *out, size_t out_sz) {
    (void)input;
    snprintf(out, out_sz,
        "[CLEAR] Conversation history has been cleared. Fresh start!"
    );
    return true;
}

static bool handler_history(const char *input, char *out, size_t out_sz) {
    (void)input;
    /* Placeholder: main.c checks for PLUGIN_HISTORY_REQUESTED flag.
     * This handler writes a notice; actual history injection happens there. */
    snprintf(out, out_sz, "[HISTORY_REQUESTED]");
    return true;
}

static bool handler_lift(const char *input, char *out, size_t out_sz) {
    (void)input;
    snprintf(out, out_sz,
        "Your current base lift force: 686.7 N (neutral mood, 70 kg).\n"
        "Say 'antigravity' to activate full lift calculations!"
    );
    return true;
}

/* ──────────────────────────────── API ─── */

PluginRegistry *plugin_create(void) {
    PluginRegistry *reg = calloc(1, sizeof(PluginRegistry));
    if (!reg) return NULL;

    /* Register built-in plugins */
    plugin_register(reg, "Help",    "/help",    handler_help);
    plugin_register(reg, "Version", "/version", handler_version);
    plugin_register(reg, "Clear",   "/clear",   handler_clear);
    plugin_register(reg, "History", "/history", handler_history);
    plugin_register(reg, "Lift",    "/lift",    handler_lift);

    log_info("Plugin system initialised: %d built-in plugins registered.", reg->count);
    return reg;
}

void plugin_destroy(PluginRegistry *reg) {
    free(reg);
}

bool plugin_register(PluginRegistry *reg,
                     const char *name,
                     const char *trigger,
                     PluginHandler handler)
{
    if (!reg || !name || !trigger || !handler) return false;
    if (reg->count >= PLUGIN_MAX) {
        log_warn("plugin_register: registry full, cannot add '%s'.", name);
        return false;
    }
    PluginEntry *e = &reg->entries[reg->count++];
    str_safe_copy(e->name,    name,    PLUGIN_NAME_LEN);
    str_safe_copy(e->trigger, trigger, PLUGIN_TRIGGER_LEN);
    e->handler = handler;
    return true;
}

bool plugin_dispatch(PluginRegistry *reg,
                     const char *input,
                     char *out,
                     size_t out_sz)
{
    if (!reg || !input) return false;
    for (int i = 0; i < reg->count; ++i) {
        if (str_contains_ci(input, reg->entries[i].trigger))
            return reg->entries[i].handler(input, out, out_sz);
    }
    return false;
}
