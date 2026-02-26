/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/runtime_hold_tap.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Use DT_DRV_COMPAT to enumerate hold-tap instances in device tree */
#define DT_DRV_COMPAT zmk_behavior_hold_tap

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define NUM_HT_INSTANCES DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
#else
#define NUM_HT_INSTANCES 0
#endif

#if NUM_HT_INSTANCES > 0

/* Names of hold-tap instances from device tree */
#define _HT_NAME(idx, _) DT_NODE_FULL_NAME(DT_DRV_INST(idx))
static const char *ht_names[] = {LISTIFY(NUM_HT_INSTANCES, _HT_NAME, (, ), 0)};

/* Compile-time defaults from device tree */
struct ht_defaults {
    int tapping_term_ms;
    int quick_tap_ms;
    int require_prior_idle_ms;
    int flavor;
};

#define _HT_DEFAULTS(idx, _)                                                                       \
    {                                                                                              \
        .tapping_term_ms = DT_INST_PROP(idx, tapping_term_ms),                                     \
        .quick_tap_ms = DT_INST_PROP(idx, quick_tap_ms),                                           \
        .require_prior_idle_ms = DT_INST_PROP_OR(idx, global_quick_tap, 0)                         \
                                     ? DT_INST_PROP(idx, quick_tap_ms)                             \
                                     : DT_INST_PROP(idx, require_prior_idle_ms),                   \
        .flavor = DT_ENUM_IDX(DT_DRV_INST(idx), flavor),                                           \
    }

static const struct ht_defaults defaults[] = {LISTIFY(NUM_HT_INSTANCES, _HT_DEFAULTS, (, ), 0)};

/* Runtime overrides */
static struct runtime_hold_tap_override overrides[NUM_HT_INSTANCES];

/* Settings storage */
#define SETTINGS_KEY "ht"

static int settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    int id;

    if (sscanf(name, "d%d", &id) != 1) {
        return -ENOENT;
    }

    if (id < 0 || id >= NUM_HT_INSTANCES) {
        LOG_WRN("Invalid hold-tap id in settings: %d", id);
        return -EINVAL;
    }

    if (len != sizeof(struct runtime_hold_tap_override)) {
        LOG_ERR("Invalid settings data size for d%d: %d vs %d", id, (int)len,
                (int)sizeof(struct runtime_hold_tap_override));
        return -EINVAL;
    }

    int rc = read_cb(cb_arg, &overrides[id], sizeof(struct runtime_hold_tap_override));
    if (rc < 0) {
        LOG_ERR("Failed to read settings for d%d: %d", id, rc);
        return rc;
    }

    LOG_DBG("Loaded hold-tap override for %s (id=%d): tapping_term=%d", ht_names[id], id,
            overrides[id].tapping_term_ms);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(runtime_hold_tap, SETTINGS_KEY, NULL, settings_set, NULL, NULL);

static int save_override(uint8_t id) {
    char key[16];
    snprintf(key, sizeof(key), SETTINGS_KEY "/d%d", id);
    int rc = settings_save_one(key, &overrides[id], sizeof(struct runtime_hold_tap_override));
    if (rc != 0) {
        LOG_ERR("Failed to save hold-tap override for id %d: %d", id, rc);
    }
    return rc;
}

/* Public API */

int zmk_runtime_hold_tap_count(void) { return NUM_HT_INSTANCES; }

int zmk_runtime_hold_tap_get_info(uint8_t id, struct runtime_hold_tap_info *info) {
    if (id >= NUM_HT_INSTANCES) {
        return -EINVAL;
    }

    const struct ht_defaults *def = &defaults[id];
    const struct runtime_hold_tap_override *ovr = &overrides[id];

    info->id = id;
    info->name = ht_names[id];

    /* Effective values: override if set, else default */
    info->tapping_term_ms = (ovr->tapping_term_ms > 0) ? ovr->tapping_term_ms : def->tapping_term_ms;
    info->quick_tap_ms = (ovr->quick_tap_ms >= 0) ? ovr->quick_tap_ms : def->quick_tap_ms;
    info->require_prior_idle_ms =
        (ovr->require_prior_idle_ms >= 0) ? ovr->require_prior_idle_ms : def->require_prior_idle_ms;
    info->flavor = (ovr->flavor >= 0) ? ovr->flavor : def->flavor;

    /* Compile-time defaults */
    info->default_tapping_term_ms = def->tapping_term_ms;
    info->default_quick_tap_ms = def->quick_tap_ms;
    info->default_require_prior_idle_ms = def->require_prior_idle_ms;
    info->default_flavor = def->flavor;

    return 0;
}

int zmk_runtime_hold_tap_set_tapping_term(uint8_t id, int value) {
    if (id >= NUM_HT_INSTANCES) {
        return -EINVAL;
    }
    overrides[id].tapping_term_ms = value;
    LOG_DBG("Set tapping_term_ms=%d for %s (id=%d)", value, ht_names[id], id);
    return save_override(id);
}

int zmk_runtime_hold_tap_set_quick_tap(uint8_t id, int value) {
    if (id >= NUM_HT_INSTANCES) {
        return -EINVAL;
    }
    overrides[id].quick_tap_ms = value;
    LOG_DBG("Set quick_tap_ms=%d for %s (id=%d)", value, ht_names[id], id);
    return save_override(id);
}

int zmk_runtime_hold_tap_set_require_prior_idle(uint8_t id, int value) {
    if (id >= NUM_HT_INSTANCES) {
        return -EINVAL;
    }
    overrides[id].require_prior_idle_ms = value;
    LOG_DBG("Set require_prior_idle_ms=%d for %s (id=%d)", value, ht_names[id], id);
    return save_override(id);
}

int zmk_runtime_hold_tap_set_flavor(uint8_t id, int value) {
    if (id >= NUM_HT_INSTANCES) {
        return -EINVAL;
    }
    if (value < 0 || value > 3) {
        return -EINVAL;
    }
    overrides[id].flavor = value;
    LOG_DBG("Set flavor=%d for %s (id=%d)", value, ht_names[id], id);
    return save_override(id);
}

int zmk_runtime_hold_tap_reset(uint8_t id) {
    if (id >= NUM_HT_INSTANCES) {
        return -EINVAL;
    }
    overrides[id].tapping_term_ms = 0;
    overrides[id].quick_tap_ms = -1;
    overrides[id].require_prior_idle_ms = -1;
    overrides[id].flavor = -1;
    LOG_DBG("Reset hold-tap overrides for %s (id=%d)", ht_names[id], id);
    return save_override(id);
}

/*
 * Query functions called by behavior_hold_tap.c in the ZMK fork.
 * These return override values; caller falls back to config defaults when
 * the override is in the "use default" state (0 for tapping_term, -1 for others).
 */
int zmk_runtime_hold_tap_get_tapping_term(uint8_t id) {
    if (id >= NUM_HT_INSTANCES) {
        return 0;
    }
    return overrides[id].tapping_term_ms;
}

int zmk_runtime_hold_tap_get_quick_tap(uint8_t id) {
    if (id >= NUM_HT_INSTANCES) {
        return -1;
    }
    return overrides[id].quick_tap_ms;
}

int zmk_runtime_hold_tap_get_require_prior_idle(uint8_t id) {
    if (id >= NUM_HT_INSTANCES) {
        return -1;
    }
    return overrides[id].require_prior_idle_ms;
}

int zmk_runtime_hold_tap_get_flavor(uint8_t id) {
    if (id >= NUM_HT_INSTANCES) {
        return -1;
    }
    return overrides[id].flavor;
}

/* Initialize overrides to "use default" values */
static int runtime_hold_tap_init(void) {
    for (int i = 0; i < NUM_HT_INSTANCES; i++) {
        overrides[i].tapping_term_ms = 0;
        overrides[i].quick_tap_ms = -1;
        overrides[i].require_prior_idle_ms = -1;
        overrides[i].flavor = -1;
    }
    return 0;
}

SYS_INIT(runtime_hold_tap_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#else /* NUM_HT_INSTANCES == 0 */

int zmk_runtime_hold_tap_count(void) { return 0; }
int zmk_runtime_hold_tap_get_info(uint8_t id, struct runtime_hold_tap_info *info) { return -EINVAL; }
int zmk_runtime_hold_tap_set_tapping_term(uint8_t id, int value) { return -EINVAL; }
int zmk_runtime_hold_tap_set_quick_tap(uint8_t id, int value) { return -EINVAL; }
int zmk_runtime_hold_tap_set_require_prior_idle(uint8_t id, int value) { return -EINVAL; }
int zmk_runtime_hold_tap_set_flavor(uint8_t id, int value) { return -EINVAL; }
int zmk_runtime_hold_tap_reset(uint8_t id) { return -EINVAL; }
int zmk_runtime_hold_tap_get_tapping_term(uint8_t id) { return 0; }
int zmk_runtime_hold_tap_get_quick_tap(uint8_t id) { return -1; }
int zmk_runtime_hold_tap_get_require_prior_idle(uint8_t id) { return -1; }
int zmk_runtime_hold_tap_get_flavor(uint8_t id) { return -1; }

#endif /* NUM_HT_INSTANCES */
