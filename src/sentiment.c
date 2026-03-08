/**
 * @file sentiment.c
 * @brief Keyword-based sentiment analyser with intensifier support.
 */
#include "sentiment.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>

/* ──────────────────────────────── Word lists ─── */

static const char *POSITIVE_WORDS[] = {
    "good","great","awesome","excellent","wonderful","fantastic","love","happy",
    "joy","excited","brilliant","perfect","amazing","beautiful","outstanding",
    "superb","delightful","cheerful","glad","pleased","thankful","grateful",
    "nice","fun","cool","sweet","marvelous","splendid","enjoy","like",
    "best","helpful","kind","optimistic","hopeful","smile","laugh","win",
    "success","achieve","thrive","rise","up","elevate","lift","soar","fly",
    NULL
};

static const char *NEGATIVE_WORDS[] = {
    "bad","terrible","horrible","awful","hate","sad","angry","upset","miserable",
    "depressed","worst","dreadful","dislike","boring","disappointed","tired",
    "stressed","anxious","worried","fear","nervous","unhappy","gloomy","fail",
    "lose","problem","issue","broken","wrong","pain","hurt","alone","lost",
    "dark","heavy","stuck","down","fall","crash","error","bug","struggle",
    NULL
};

static const char *INTENSIFIERS[] = {
    "very","extremely","really","super","absolutely","totally","incredibly",
    "deeply","utterly","completely","awfully","terribly",
    NULL
};

/* ──────────────────────────────── Helpers ─── */

static bool list_contains(const char **list, const char *word) {
    for (int i = 0; list[i]; ++i)
        if (strcmp(list[i], word) == 0) return true;
    return false;
}

static bool is_intensifier(const char *word) {
    return list_contains(INTENSIFIERS, word);
}

/* ──────────────────────────────── API ─── */

int sentiment_score(const char *input) {
    if (!input) return 0;

    char buf[4096];
    str_safe_copy(buf, input, sizeof(buf));
    str_lower(buf);

    int score   = 0;
    int weight  = 1;  /* doubles after an intensifier */
    char *token = strtok(buf, " \t\n\r,.:;!?\"'()[]{}");
    while (token) {
        if (is_intensifier(token)) {
            weight = 2;
        } else if (list_contains(POSITIVE_WORDS, token)) {
            score += weight;
            weight = 1;
        } else if (list_contains(NEGATIVE_WORDS, token)) {
            score -= weight;
            weight = 1;
        } else {
            weight = 1;
        }
        token = strtok(NULL, " \t\n\r,.:;!?\"'()[]{}");
    }

    /* Clamp to range */
    if (score >  SENTIMENT_MAX) score =  SENTIMENT_MAX;
    if (score <  SENTIMENT_MIN) score =  SENTIMENT_MIN;
    return score;
}

const char *sentiment_label(int score) {
    if (score >= 4)  return "very positive";
    if (score >= 1)  return "positive";
    if (score == 0)  return "neutral";
    if (score >= -3) return "negative";
    return "very negative";
}

const char *sentiment_emoji(int score) {
    if (score >= 4)  return "(^o^)";
    if (score >= 1)  return "(^_^)";
    if (score == 0)  return "(-_-)";
    if (score >= -3) return "(>_<)";
    return "(T_T)";
}
