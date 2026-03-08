/**
 * @file model.c
 * @brief NLP Engine — V2: memory-conditioned and persona-aware response generator.
 *
 * V2 changes vs V1:
 *   - model_respond_v2() accepts retrieved semantic memories + bot_name
 *   - Bot self-identifies when asked "what is your name" etc.
 *   - Responses prefixed with bot_name when a name has been set
 *   - Pattern priority: name question > correction > pattern table > fallback
 */
#include "model.h"
#include "utils.h"
#include "sentiment.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ──────────────────────────────── Pattern table ─── */

typedef struct {
    char  keyword[MODEL_KEY_LEN];
    char  responses[MODEL_MAX_RESPONSES][MODEL_RESP_LEN];
    int   response_count;
    int   next_response;
} Pattern;

struct NLPModel {
    Pattern patterns[MODEL_MAX_PATTERNS];
    int     count;
};

/* ──────────────────────────────── Fallbacks (tiered by sentiment) ─── */

static const char *FALLBACKS_POSITIVE[] = {
    "That's a great point! Tell me more about it.",
    "I love your energy! What else is on your mind?",
    "Interesting! You seem to be in a wonderful mood today.",
    "Absolutely! Keep that positive spirit going.",
    "You always bring such a refreshing perspective!",
};
static const char *FALLBACKS_NEUTRAL[] = {
    "Hmm, let me think about that... Could you elaborate?",
    "I see. That's something worth exploring further.",
    "Interesting perspective. What made you think about that?",
    "Tell me more — I'm all ears (well, all bytes).",
    "Could you give me a bit more context?",
};
static const char *FALLBACKS_NEGATIVE[] = {
    "I hear you. Sometimes things can feel really tough.",
    "That sounds difficult. Remember: this too shall pass.",
    "I understand. Would talking about it help?",
    "You're not alone in feeling that way. Keep going.",
};

static const char *pick_fallback(int sentiment_score) {
    srand((unsigned int)time(NULL));
    if (sentiment_score > 1) {
        int n = (int)(sizeof(FALLBACKS_POSITIVE) / sizeof(FALLBACKS_POSITIVE[0]));
        return FALLBACKS_POSITIVE[rand() % n];
    } else if (sentiment_score < -1) {
        int n = (int)(sizeof(FALLBACKS_NEGATIVE) / sizeof(FALLBACKS_NEGATIVE[0]));
        return FALLBACKS_NEGATIVE[rand() % n];
    } else {
        int n = (int)(sizeof(FALLBACKS_NEUTRAL) / sizeof(FALLBACKS_NEUTRAL[0]));
        return FALLBACKS_NEUTRAL[rand() % n];
    }
}

/* ──────────────────────────────── File loader ─── */

static bool parse_line(const char *line, Pattern *pat) {
    char buf[2048];
    str_safe_copy(buf, line, sizeof(buf));
    char *save = NULL;
    char *tok  = strtok_r(buf, "|", &save);
    if (!tok) return false;
    str_trim(tok);
    if (tok[0] == '\0' || tok[0] == '#') return false;
    str_safe_copy(pat->keyword, tok, MODEL_KEY_LEN);
    str_lower(pat->keyword);
    pat->response_count = 0;
    pat->next_response  = 0;
    while ((tok = strtok_r(NULL, "|", &save)) != NULL &&
           pat->response_count < MODEL_MAX_RESPONSES) {
        str_trim(tok);
        if (tok[0] == '\0') continue;
        str_safe_copy(pat->responses[pat->response_count], tok, MODEL_RESP_LEN);
        ++pat->response_count;
    }
    return pat->response_count > 0;
}

NLPModel *model_load(const char *responses_path) {
    NLPModel *m = calloc(1, sizeof(NLPModel));
    if (!m) return NULL;
    FILE *f = fopen(responses_path, "r");
    if (!f) {
        log_warn("model_load: '%s' not found; using fallback responses only.", responses_path);
        return m;
    }
    char line[2048];
    while (fgets(line, sizeof(line), f) && m->count < MODEL_MAX_PATTERNS) {
        str_trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        if (parse_line(line, &m->patterns[m->count]))
            ++m->count;
    }
    fclose(f);
    log_info("model_load: loaded %d response patterns.", m->count);
    return m;
}

void model_destroy(NLPModel *model) { free(model); }

/* ──────────────────────────────── Pattern matching ─── */

static int score_pattern(const Pattern *pat, const char *input) {
    return str_contains_ci(input, pat->keyword) ? 10 : 0;
}

/* ──────────────────────────────── Self-identification check ─── */

static bool is_name_question(const char *input) {
    static const char *kws[] = {
        "your name", "what are you called", "who are you",
        "what should i call you", "introduce yourself",
        "what do i call you", NULL
    };
    for (int i = 0; kws[i]; ++i)
        if (str_contains_ci(input, kws[i])) return true;
    return false;
}

/* ──────────────────────────────── V1 API (kept for backward compat) ─── */

void model_respond(NLPModel *model,
                   const char *input,
                   int sentiment_score,
                   const ConvMemory *mem,
                   char *out,
                   size_t out_sz)
{
    (void)mem;
    /* Delegate to V2 with empty memories and default bot name */
    model_respond_v2(model, input, sentiment_score, PROF_DEFAULT_BOT_NAME,
                     NULL, 0, out, out_sz);
}

/* ──────────────────────────────── V2 API ─── */

void model_respond_v2(NLPModel *model,
                      const char *input,
                      int sentiment_score,
                      const char *bot_name,
                      const char **memories,
                      int mem_count,
                      char *out,
                      size_t out_sz)
{
    if (!model || !out || out_sz == 0) return;

    const char *name = (bot_name && bot_name[0]) ? bot_name : PROF_DEFAULT_BOT_NAME;

    /* 1. Self-identification */
    if (is_name_question(input)) {
        snprintf(out, out_sz,
            "[%s]: My name is %s! It's wonderful to chat with you.", name, name);
        return;
    }

    /* 2. Pattern match */
    int best_score = 0;
    Pattern *best  = NULL;
    for (int i = 0; i < model->count; ++i) {
        int s = score_pattern(&model->patterns[i], input);
        if (s > best_score) { best_score = s; best = &model->patterns[i]; }
    }

    const char *response = NULL;
    if (best && best->response_count > 0) {
        response = best->responses[best->next_response % best->response_count];
        best->next_response = (best->next_response + 1) % best->response_count;
    } else {
        response = pick_fallback(sentiment_score);
    }

    /* 3. Sentiment prefix */
    const char *prefix = "";
    if (sentiment_score <= -4)      prefix = "I'm sorry to hear that. ";
    else if (sentiment_score >= 4)  prefix = "Wonderful! ";

    /* 4. Memory recall prefix (top 2 memories, if any) */
    char mem_prefix[512] = {0};
    if (memories && mem_count > 0) {
        int show = mem_count < 2 ? mem_count : 2;
        char tmp[256] = {0};
        for (int i = 0; i < show; ++i) {
            if (i == 0) snprintf(tmp, sizeof(tmp), "(I remember: %s", memories[0]);
            else        strncat(tmp, memories[i], sizeof(tmp) - strlen(tmp) - 1);
            if (i < show - 1) strncat(tmp, "; ", sizeof(tmp) - strlen(tmp) - 1);
        }
        strncat(tmp, ") ", sizeof(tmp) - strlen(tmp) - 1);
        str_safe_copy(mem_prefix, tmp, sizeof(mem_prefix));
    }

    /* 5. Assemble final response */
    snprintf(out, out_sz, "[%s]: %s%s%s", name, mem_prefix, prefix, response);
}
