/**
 * @file main.c
 * @brief Entry point — V2: full persistent memory & personalization pipeline.
 *
 * New in V2:
 *   - Bot naming detection ("call you X", "your name is X", …)
 *   - Semantic memory: embed input → search top-k → condition response
 *   - After each turn, new facts are stored in semantic memory
 *   - User profile (bot_name, tone) persists across sessions
 *   - Per-user memory partitioning (user_id = "default" in CLI mode)
 *
 * Run modes:
 *   --cli      Interactive CLI (default)
 *   --server   HTTP REST server
 */
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "utils.h"
#include "tokenizer.h"
#include "model.h"
#include "memory.h"
#include "sentiment.h"
#include "antigravity.h"
#include "plugins.h"
#include "server.h"
#include "embed.h"
#include "memory/semantic.h"
#include "memory/profile.h"

/* ──────────────────────────────── Config ─── */

typedef struct {
    int  server_port;
    int  server_threads;
    int  memory_window;
    char memory_path[256];
    char semantic_path[256];
    char profile_path[256];
    char responses_path[256];
    char vocab_path[256];
    bool ag_enabled;
    char log_level[16];
    double mem_half_life_days;
} Config;

static Config g_cfg = {
    .server_port        = 8080,
    .server_threads     = 4,
    .memory_window      = 20,
    .memory_path        = "data/history.bin",
    .semantic_path      = "data/semantic.bin",
    .profile_path       = "data/profile.bin",
    .responses_path     = "data/responses.txt",
    .vocab_path         = "data/vocab.txt",
    .ag_enabled         = true,
    .log_level          = "info",
    .mem_half_life_days = 30.0,
};

static void config_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { log_warn("Config '%s' not found; using defaults.", path); return; }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        str_trim(line);
        if (line[0] == '#' || line[0] == '[' || line[0] == '\0') continue;
        char key[128], val[256];
        if (sscanf(line, "%127[^=]=%255s", key, val) != 2) continue;
        str_trim(key); str_trim(val);
        if      (!strcmp(key, "port"))              g_cfg.server_port        = atoi(val);
        else if (!strcmp(key, "threads"))           g_cfg.server_threads     = atoi(val);
        else if (!strcmp(key, "window_size"))       g_cfg.memory_window      = atoi(val);
        else if (!strcmp(key, "persist_path"))      str_safe_copy(g_cfg.memory_path,    val, sizeof(g_cfg.memory_path));
        else if (!strcmp(key, "semantic_path"))     str_safe_copy(g_cfg.semantic_path,  val, sizeof(g_cfg.semantic_path));
        else if (!strcmp(key, "profile_path"))      str_safe_copy(g_cfg.profile_path,   val, sizeof(g_cfg.profile_path));
        else if (!strcmp(key, "responses_path"))    str_safe_copy(g_cfg.responses_path, val, sizeof(g_cfg.responses_path));
        else if (!strcmp(key, "vocab_path"))        str_safe_copy(g_cfg.vocab_path,     val, sizeof(g_cfg.vocab_path));
        else if (!strcmp(key, "enabled"))           g_cfg.ag_enabled         = (strcmp(val, "false") != 0);
        else if (!strcmp(key, "level"))             str_safe_copy(g_cfg.log_level,      val, sizeof(g_cfg.log_level));
        else if (!strcmp(key, "half_life_days"))    g_cfg.mem_half_life_days = atof(val);
    }
    fclose(f);
}

/* ──────────────────────────────── Naming detection ─── */

/**
 * @brief Detect bot-naming intent and extract the proposed name.
 *
 * Patterns (case-insensitive):
 *   "call you X"        "your name is X"    "i'll call you X"
 *   "i will call you X" "name you X"        "you are called X"
 *   "you'll be called X"
 *
 * @param input       Raw user input.
 * @param name_out    Buffer receiving extracted name (>= 64 bytes).
 * @return true if a name was detected and written to name_out.
 */
static bool detect_naming(const char *input, char *name_out) {
    static const char *PATTERNS[] = {
        "call you ",
        "your name is ",
        "i'll call you ",
        "i will call you ",
        "name you ",
        "you are called ",
        "you'll be called ",
        "you will be called ",
        NULL
    };

    char lower[1024];
    str_safe_copy(lower, input, sizeof(lower));
    str_lower(lower);

    for (int i = 0; PATTERNS[i]; ++i) {
        const char *pos = strstr(lower, PATTERNS[i]);
        if (!pos) continue;

        /* Advance past the pattern keyword */
        const char *name_start_lower = pos + strlen(PATTERNS[i]);
        /* Find the same position in the original (preserve capitalisation) */
        size_t offset = (size_t)(name_start_lower - lower);
        const char *name_start = input + offset;

        /* Extract until punctuation / end of string */
        char name[64];
        int ni = 0;
        const char *p = name_start;
        /* Skip leading spaces */
        while (*p == ' ') ++p;
        while (*p && !strchr(".,!?;:\n\r", *p) && ni < 63)
            name[ni++] = *p++;
        name[ni] = '\0';
        str_trim(name);

        /* Validate: must be 1–30 chars, starts with letter */
        if (ni < 1 || ni > 30 || !isalpha((unsigned char)name[0])) continue;

        /* Capitalise first letter */
        name[0] = (char)toupper((unsigned char)name[0]);
        str_safe_copy(name_out, name, 64);
        return true;
    }
    return false;
}

/* ──────────────────────────────── Correction detection ─── */

static bool detect_correction(const char *input) {
    static const char *kws[] = {
        "that's wrong", "thats wrong", "you're wrong", "youre wrong",
        "actually,", "actually ", "no,", "incorrect", "not quite",
        "i meant", "i prefer", "i don't like", "i do not like",
        NULL
    };
    char lower[512];
    str_safe_copy(lower, input, sizeof(lower));
    str_lower(lower);
    for (int i = 0; kws[i]; ++i)
        if (str_contains_ci(lower, kws[i])) return true;
    return false;
}

/* ──────────────────────────────── CLI banner ─── */

static void print_banner(const char *bot_name) {
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║   ★  AntiGravity Chatbot  v2.0  ★               ║\n");
    printf("  ║   Your AI companion: %-26s  ║\n", bot_name);
    printf("  ║   Persistent memory | Personalization | Antigravity ║\n");
    printf("  ║   Type /help for commands, 'quit' to exit.       ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n");
    printf("\n");
}

/* ──────────────────────────────── V2 Pipeline ─── */

typedef struct {
    NLPModel       *model;
    ConvMemory     *episodic;
    SemanticStore  *semantic;
    ProfileStore   *profile;
    PluginRegistry *plugins;
    const char     *user_id;
} PipelineCtx;

/**
 * @brief Full V2 pipeline: plugins → naming → antigravity → memory retrieval → NLP.
 */
static void run_pipeline_v2(const char *input, PipelineCtx *ctx,
                             char *response_out, size_t resp_sz,
                             bool animate_cli)
{
    char input_lower[1024];
    str_safe_copy(input_lower, input, sizeof(input_lower));
    str_lower(input_lower);
    str_trim(input_lower);

    int  score = sentiment_score(input_lower);
    bool handled = false;

    /* ── 1. Plugin commands ── */
    char plugin_resp[1024] = {0};
    if (plugin_dispatch(ctx->plugins, input_lower, plugin_resp, sizeof(plugin_resp))) {
        if (strcmp(plugin_resp, "[HISTORY_REQUESTED]") == 0) {
            char ctx_buf[4096] = {0};
            mem_format_context(ctx->episodic, ctx_buf, sizeof(ctx_buf));
            snprintf(response_out, resp_sz, "[History]\n%s",
                     ctx_buf[0] ? ctx_buf : "(no history yet)");
        } else if (str_contains_ci(plugin_resp, "CLEAR")) {
            mem_clear(ctx->episodic);
            str_safe_copy(response_out, plugin_resp, resp_sz);
        } else {
            str_safe_copy(response_out, plugin_resp, resp_sz);
        }
        handled = true;
    }

    /* ── 2. Naming detection ── */
    char new_name[64] = {0};
    if (!handled && detect_naming(input, new_name)) {
        profile_set(ctx->profile, ctx->user_id, "bot_name", new_name);
        profile_save(ctx->profile);

        /* Store as a semantic memory too */
        char fact[128];
        snprintf(fact, sizeof(fact), "User named me %s.", new_name);
        sem_add(ctx->semantic, ctx->user_id, fact, NULL, 0.9f);

        snprintf(response_out, resp_sz,
            "Understood! I'll go by %s from now on. "
            "It's a pleasure to meet you — I'll remember that! %s",
            new_name, sentiment_emoji(score));
        handled = true;
    }

    /* ── 3. Antigravity easter egg ── */
    if (!handled && ag_detect(input_lower)) {
        if (animate_cli) ag_print_animation(5);
        ag_respond(score, response_out, resp_sz);
        handled = true;
    }

    /* ── 4. Memory-conditioned NLP ── */
    if (!handled) {
        /* Embed query */
        float query_vec[EMBED_DIM];
        embed_text(input_lower, query_vec);

        /* Retrieve top-k semantic memories */
        SemResult results[SEM_TOP_K];
        int found = sem_search(ctx->semantic, ctx->user_id,
                               query_vec, SEM_TOP_K, results);

        const char *mem_texts[SEM_TOP_K];
        int         mem_count = 0;
        for (int i = 0; i < found; ++i) {
            if (results[i].score > 0.15f) /* similarity threshold */
                mem_texts[mem_count++] = results[i].entry->text;
        }

        /* Get bot name */
        char bot_name[64];
        profile_get_bot_name(ctx->profile, ctx->user_id, bot_name, sizeof(bot_name));

        model_respond_v2(ctx->model, input_lower, score,
                         bot_name, mem_texts, mem_count,
                         response_out, resp_sz);
    }

    /* ── 5. Store this turn in episodic memory ── */
    mem_push(ctx->episodic, MEM_ROLE_USER, input);
    mem_push(ctx->episodic, MEM_ROLE_BOT,  response_out);

    /* ── 6. Extract and store facts in semantic memory ── */
    /* Store every user utterance with moderate importance */
    char fact_buf[SEM_TEXT_LEN];
    snprintf(fact_buf, sizeof(fact_buf), "User said: %s", input);
    if (!detect_correction(input_lower))
        sem_add(ctx->semantic, ctx->user_id, fact_buf, NULL, 0.5f);
    else {
        /* Correction → store with higher importance */
        snprintf(fact_buf, sizeof(fact_buf), "User corrected: %s", input);
        sem_add(ctx->semantic, ctx->user_id, fact_buf, NULL, 0.8f);
    }
}

/* ──────────────────────────────── CLI loop ─── */

static void run_cli(PipelineCtx *ctx) {
    char bot_name[64];
    profile_get_bot_name(ctx->profile, ctx->user_id, bot_name, sizeof(bot_name));
    print_banner(bot_name);

    /* Apply importance decay on startup */
    sem_decay(ctx->semantic, g_cfg.mem_half_life_days);
    /* Prune very-low-importance entries */
    int pruned = sem_forget(ctx->semantic, ctx->user_id, 0.05f);
    if (pruned) log_info("Pruned %d stale semantic memories.", pruned);

    char input[1024];
    char response[1400];
    while (1) {
        /* Refresh bot name in case it was just changed */
        profile_get_bot_name(ctx->profile, ctx->user_id, bot_name, sizeof(bot_name));
        printf("  You: ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        str_trim(input);
        if (input[0] == '\0') continue;
        if (!strcmp(input,"quit") || !strcmp(input,"exit") ||
            !strcmp(input,"bye")  || !strcmp(input,"q"))    break;

        run_pipeline_v2(input, ctx, response, sizeof(response), /*animate=*/true);
        printf("\n  %s %s\n\n", response, sentiment_emoji(sentiment_score(input)));
    }

    printf("\n  [%s]: Goodbye! Until next time — I'll remember you! (^o^)/\n\n",
           bot_name);

    /* Persist everything */
    mem_save(ctx->episodic);
    sem_save(ctx->semantic);
    profile_save(ctx->profile);
}

/* ──────────────────────────────── Signal ─── */

static volatile sig_atomic_t g_interrupted = 0;
static void on_signal(int sig) { (void)sig; g_interrupted = 1; }

/* ──────────────────────────────── main ─── */

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    bool server_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--server")) server_mode = true;
        if (!strcmp(argv[i], "--cli"))    server_mode = false;
    }

    config_load("data/config.ini");

    if      (!strcmp(g_cfg.log_level, "debug")) log_set_level(LOG_DEBUG);
    else if (!strcmp(g_cfg.log_level, "warn"))  log_set_level(LOG_WARN);
    else if (!strcmp(g_cfg.log_level, "error")) log_set_level(LOG_ERROR);
    else                                         log_set_level(LOG_INFO);

    ag_init(g_cfg.ag_enabled);

    /* Initialise all modules */
    NLPModel       *model    = model_load(g_cfg.responses_path);
    ConvMemory     *episodic = mem_create(g_cfg.memory_window, g_cfg.memory_path);
    SemanticStore  *semantic = sem_create(g_cfg.semantic_path);
    ProfileStore   *profile  = profile_create(g_cfg.profile_path);
    Tokenizer      *tok      = tok_init(g_cfg.vocab_path);
    PluginRegistry *plugins  = plugin_create();

    if (!model || !episodic || !semantic || !profile || !tok || !plugins) {
        log_error("Failed to initialise one or more modules. Exiting.");
        return EXIT_FAILURE;
    }

    PipelineCtx ctx = {
        .model    = model,
        .episodic = episodic,
        .semantic = semantic,
        .profile  = profile,
        .plugins  = plugins,
        .user_id  = "default",
    };

    if (server_mode) {
        signal(SIGINT, on_signal); signal(SIGTERM, on_signal);
        ChatServer *srv = server_start(g_cfg.server_port, g_cfg.server_threads);
        if (!srv) { log_error("Failed to start server."); return EXIT_FAILURE; }
        printf("  Server at http://localhost:%d | POST /chat\n", g_cfg.server_port);
        printf("  Press Ctrl+C to stop.\n\n");
        while (!g_interrupted) {
#ifdef _WIN32
            Sleep(500);
#else
            usleep(500000);
#endif
        }
        server_stop(srv); server_destroy(srv);
    } else {
        run_cli(&ctx);
    }

    tok_destroy(tok);
    model_destroy(model);
    mem_save(episodic);  mem_destroy(episodic);
    sem_save(semantic);  sem_destroy(semantic);
    profile_save(profile); profile_destroy(profile);
    plugin_destroy(plugins);

    return EXIT_SUCCESS;
}
