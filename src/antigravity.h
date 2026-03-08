/**
 * @file antigravity.h
 * @brief The Antigravity Module — a whimsical easter egg that defies gravity.
 *
 * When the NLP engine detects antigravity-related keywords this module
 * generates uplifting responses, ASCII floating animations, and imaginary
 * "lift force" calculations.
 *
 * The module is entirely self-contained and can be disabled by setting
 * the `enabled` flag in the configuration file.
 */
#ifndef ANTIGRAVITY_H
#define ANTIGRAVITY_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialise the antigravity module.
 * @param enabled Pass false to completely disable the module.
 */
void ag_init(bool enabled);

/**
 * @brief Check whether the input triggers the antigravity feature.
 *
 * Scans for keywords: "antigravity", "anti-gravity", "defy gravity",
 * "levitate", "float", "lift off", "zero gravity", "weightless", "soar".
 *
 * @param input Raw user input string (may be unmodified case).
 * @return true if the input should invoke the module.
 */
bool ag_detect(const char *input);

/**
 * @brief Generate an antigravity response string.
 *
 * Selects a creative response from the built-in response bank, injects
 * a calculated "lift value" based on the user's sentiment score, and
 * optionally prints an ASCII floating animation to stdout.
 *
 * @param sentiment_score Sentiment score in [-10, 10] from sentiment.c.
 * @param out             Output buffer for the response.
 * @param out_sz          Size of output buffer.
 */
void ag_respond(int sentiment_score, char *out, size_t out_sz);

/**
 * @brief Calculate an imaginary lift force based on sentiment.
 *
 * Formula (fictitious):  F_lift = base_mass * g * (1 + mood_factor)
 * where g = 9.81 m/s², base_mass = 70 kg (average human), and
 * mood_factor = sentiment_score / 10.0.
 *
 * @param sentiment_score Sentiment score in [-10, 10].
 * @return Imaginary lift force in Newtons (can be negative when very sad).
 */
double ag_lift_value(int sentiment_score);

/**
 * @brief Print a multi-line ASCII floating animation to stdout.
 *
 * Displays a small object "floating" upward across several frames.
 * This is a synchronous call; each frame has a short delay.
 *
 * @param frames Number of animation frames (recommended: 4–6).
 */
void ag_print_animation(int frames);

#endif /* ANTIGRAVITY_H */
