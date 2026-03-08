/**
 * @file antigravity.c
 * @brief Implementation of the Antigravity Easter Egg Module.
 *
 * Detects gravity-defying keywords, produces uplifting replies,
 * calculates an imaginary lift force, and renders ASCII animations.
 */
#include "antigravity.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#  include <windows.h>
#  define sleep_ms(ms) Sleep(ms)
#else
#  include <unistd.h>
#  define sleep_ms(ms) usleep((ms) * 1000)
#endif

/* ──────────────────────────────── State ─── */

static bool g_enabled = true;
static unsigned int g_seed = 0;

/* ──────────────────────────────── Trigger keywords ─── */

static const char *AG_KEYWORDS[] = {
    "antigravity", "anti-gravity", "anti gravity",
    "defy gravity", "levitate", "weightless",
    "zero gravity", "zero-gravity", "lift off",
    "liftoff", "soar into", "float away",
    "defying gravity", "gravity off",
    NULL
};

/* ──────────────────────────────── Response bank ─── */

static const char *AG_RESPONSES_NEUTRAL[] = {
    "Initiating antigravity sequence... Lift vector acquired! "
        "Your current lift force is %.1f N — that's enough to raise your spirits by %d%%!",
    "Antigravity engaged! The laws of physics have filed a formal complaint, "
        "but we ignored it. Your imaginary lift: %.1f N.",
    "WARPING GRAVITY FIELD... Done! You are now %.0f%% lighter than a feather "
        "and twice as free. Lift force: %.1f N.",
    "Newton is spinning in his grave — but that just generates more energy! "
        "Your lift coefficient today: %.1f N (mood-adjusted).",
    "Gravity? Never heard of her. You've achieved %.1f N of pure, "
        "unadulterated levitation.",
};

static const char *AG_RESPONSES_POSITIVE[] = {
    "Your positivity alone generates %.1f N of lift — "
        "you're practically already flying! ✈",
    "Wow, such great energy! Combined with antigravity, "
        "your lift force hits a staggering %.1f N. The sky isn't even the limit!",
    "Your mood is so uplifting it defies gravity all by itself. "
        "Calculated supplementary lift: only %.1f N — you barely needed us!",
};

static const char *AG_RESPONSES_NEGATIVE[] = {
    "Even on a heavy day, antigravity has your back. "
        "Applying %.1f N of compensatory lift — hang in there!",
    "Gravity can be overwhelming sometimes. We're countering with %.1f N of lift. "
        "Remember: every heavy heart can still learn to float.",
    "Detected low-gravity mood. Injecting %.1f N of imaginary lift and a reminder: "
        "you've defied gravity before, and you will again.",
};

/* ──────────────────────────────── ASCII animation frames ─── */

static const char *FRAMES[][6] = {
    {
        "           ",
        "           ",
        "           ",
        "    (o_o)  ",
        "   /|   |\\  ",
        "   / \\ / \\ ",
    },
    {
        "           ",
        "           ",
        "    (o_o)  ",
        "   /|   |\\  ",
        "   / \\ / \\ ",
        "  ~~~~~~~~~~",
    },
    {
        "           ",
        "    (o_o)  ",
        "   /|   |\\  ",
        "   / \\ / \\ ",
        "  ~~~~~~~~~~",
        "            ",
    },
    {
        "    (^_^)  ",
        "   /|   |\\  ",
        "   / \\ / \\ ",
        "  ~~~~~~~~~~",
        "            ",
        "            ",
    },
    {
        "  *  (^o^) *",
        " * /|   |\\ *",
        "*  / \\ / \\  *",
        "  ~~~~~~~~~~",
        "  ANTIGRAVITY",
        "  ACTIVATED! ",
    },
};
#define NUM_FRAMES 5
#define FRAME_LINES 6

/* ──────────────────────────────── API ─── */

void ag_init(bool enabled) {
    g_enabled = enabled;
    g_seed    = (unsigned int)time(NULL);
    srand(g_seed);
    if (enabled)
        log_info("Antigravity module ENABLED. Gravity is merely a suggestion.");
    else
        log_info("Antigravity module disabled.");
}

bool ag_detect(const char *input) {
    if (!g_enabled || !input) return false;
    for (int i = 0; AG_KEYWORDS[i]; ++i)
        if (str_contains_ci(input, AG_KEYWORDS[i])) return true;
    return false;
}

double ag_lift_value(int sentiment_score) {
    /* F_lift = m * g * (1 + mood_factor), m=70 kg, g=9.81 m/s^2 */
    double mood_factor = (double)sentiment_score / 10.0;
    double lift = 70.0 * 9.81 * (1.0 + mood_factor);
    return lift;
}

void ag_respond(int sentiment_score, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    if (!g_enabled) {
        str_safe_copy(out, "(Antigravity module is disabled.)", out_sz);
        return;
    }

    double lift = ag_lift_value(sentiment_score);
    int    pct  = (int)((lift / (70.0 * 9.81)) * 100.0);

    const char *tmpl = NULL;
    if (sentiment_score > 2) {
        int n = (int)(sizeof(AG_RESPONSES_POSITIVE) / sizeof(AG_RESPONSES_POSITIVE[0]));
        tmpl = AG_RESPONSES_POSITIVE[rand() % n];
        snprintf(out, out_sz, tmpl, lift);
    } else if (sentiment_score < -2) {
        int n = (int)(sizeof(AG_RESPONSES_NEGATIVE) / sizeof(AG_RESPONSES_NEGATIVE[0]));
        tmpl = AG_RESPONSES_NEGATIVE[rand() % n];
        snprintf(out, out_sz, tmpl, lift);
    } else {
        int n = (int)(sizeof(AG_RESPONSES_NEUTRAL) / sizeof(AG_RESPONSES_NEUTRAL[0]));
        tmpl = AG_RESPONSES_NEUTRAL[rand() % n];
        snprintf(out, out_sz, tmpl, lift, pct, lift);
    }
}

void ag_print_animation(int frames) {
    if (!g_enabled) return;
    int max = frames < NUM_FRAMES ? frames : NUM_FRAMES;

    printf("\n");
    for (int f = 0; f < max; ++f) {
        /* Clear previous frame (ANSI: move cursor up FRAME_LINES lines) */
        if (f > 0) {
            for (int i = 0; i < FRAME_LINES; ++i) printf("\033[A\033[2K");
        }
        for (int line = 0; line < FRAME_LINES; ++line)
            printf("  %s\n", FRAMES[f][line]);
        fflush(stdout);
        sleep_ms(300);
    }
    printf("\n");
}
