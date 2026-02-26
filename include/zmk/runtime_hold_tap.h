/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

/**
 * Runtime hold-tap override values.
 * A value of 0 for tapping_term_ms or -1 for others means "use compile-time default".
 */
struct runtime_hold_tap_override {
    int tapping_term_ms;         /* 0 = use default */
    int quick_tap_ms;            /* -1 = use default */
    int require_prior_idle_ms;   /* -1 = use default */
    int flavor;                  /* -1 = use default */
};

struct runtime_hold_tap_info {
    uint8_t id;
    const char *name;
    /* Current effective values */
    int tapping_term_ms;
    int quick_tap_ms;
    int require_prior_idle_ms;
    int flavor;
    /* Compile-time defaults */
    int default_tapping_term_ms;
    int default_quick_tap_ms;
    int default_require_prior_idle_ms;
    int default_flavor;
};

/**
 * Get count of hold-tap instances.
 */
int zmk_runtime_hold_tap_count(void);

/**
 * Get info about a hold-tap instance by index.
 */
int zmk_runtime_hold_tap_get_info(uint8_t id, struct runtime_hold_tap_info *info);

/**
 * Set the tapping term for a hold-tap instance.
 */
int zmk_runtime_hold_tap_set_tapping_term(uint8_t id, int value);

/**
 * Set the quick-tap for a hold-tap instance.
 */
int zmk_runtime_hold_tap_set_quick_tap(uint8_t id, int value);

/**
 * Set the require-prior-idle for a hold-tap instance.
 */
int zmk_runtime_hold_tap_set_require_prior_idle(uint8_t id, int value);

/**
 * Set the flavor for a hold-tap instance.
 */
int zmk_runtime_hold_tap_set_flavor(uint8_t id, int value);

/**
 * Reset a hold-tap instance to compile-time defaults.
 */
int zmk_runtime_hold_tap_reset(uint8_t id);

/**
 * Query functions called by behavior_hold_tap.c.
 * Return the override value, or "use default" sentinel (0 for tapping_term, -1 for others).
 */
int zmk_runtime_hold_tap_get_tapping_term(uint8_t id);
int zmk_runtime_hold_tap_get_quick_tap(uint8_t id);
int zmk_runtime_hold_tap_get_require_prior_idle(uint8_t id);
int zmk_runtime_hold_tap_get_flavor(uint8_t id);
