/*
 * Hold-Tap Runtime Configuration - Custom Studio RPC Handler
 */

#include <pb_decode.h>
#include <pb_encode.h>
#include <zmk/hold_tap/custom.pb.h>
#include <zmk/studio/custom.h>
#include <zmk/runtime_hold_tap.h>

#include <zephyr/logging/log.h>
#include <string.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_rpc_custom_subsystem_meta hold_tap_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS("http://localhost:5173"),
    .security = ZMK_STUDIO_RPC_HANDLER_SECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(zmk__hold_tap, &hold_tap_meta, hold_tap_rpc_handle_request);
ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(zmk__hold_tap, zmk_hold_tap_Response);

static int handle_list_hold_taps(zmk_hold_tap_Response *resp);
static int handle_set_tapping_term(const zmk_hold_tap_SetTappingTermRequest *req,
                                    zmk_hold_tap_Response *resp);
static int handle_set_quick_tap(const zmk_hold_tap_SetQuickTapRequest *req,
                                 zmk_hold_tap_Response *resp);
static int handle_set_require_prior_idle(const zmk_hold_tap_SetRequirePriorIdleRequest *req,
                                          zmk_hold_tap_Response *resp);
static int handle_set_flavor(const zmk_hold_tap_SetFlavorRequest *req,
                              zmk_hold_tap_Response *resp);
static int handle_reset_hold_tap(const zmk_hold_tap_ResetHoldTapRequest *req,
                                  zmk_hold_tap_Response *resp);

static bool hold_tap_rpc_handle_request(const zmk_custom_CallRequest *raw_request,
                                         pb_callback_t *encode_response) {
    zmk_hold_tap_Response *resp =
        ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(zmk__hold_tap, encode_response);

    zmk_hold_tap_Request req = zmk_hold_tap_Request_init_zero;

    pb_istream_t req_stream =
        pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);
    if (!pb_decode(&req_stream, zmk_hold_tap_Request_fields, &req)) {
        LOG_WRN("Failed to decode hold-tap request: %s", PB_GET_ERROR(&req_stream));
        zmk_hold_tap_ErrorResponse err = zmk_hold_tap_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to decode request");
        resp->which_response_type = zmk_hold_tap_Response_error_tag;
        resp->response_type.error = err;
        return true;
    }

    int rc = 0;
    switch (req.which_request_type) {
    case zmk_hold_tap_Request_list_hold_taps_tag:
        rc = handle_list_hold_taps(resp);
        break;
    case zmk_hold_tap_Request_set_tapping_term_tag:
        rc = handle_set_tapping_term(&req.request_type.set_tapping_term, resp);
        break;
    case zmk_hold_tap_Request_set_quick_tap_tag:
        rc = handle_set_quick_tap(&req.request_type.set_quick_tap, resp);
        break;
    case zmk_hold_tap_Request_set_require_prior_idle_tag:
        rc = handle_set_require_prior_idle(&req.request_type.set_require_prior_idle, resp);
        break;
    case zmk_hold_tap_Request_set_flavor_tag:
        rc = handle_set_flavor(&req.request_type.set_flavor, resp);
        break;
    case zmk_hold_tap_Request_reset_hold_tap_tag:
        rc = handle_reset_hold_tap(&req.request_type.reset_hold_tap, resp);
        break;
    default:
        LOG_WRN("Unsupported hold-tap request type: %d", req.which_request_type);
        rc = -1;
    }

    if (rc != 0) {
        zmk_hold_tap_ErrorResponse err = zmk_hold_tap_ErrorResponse_init_zero;
        snprintf(err.message, sizeof(err.message), "Failed to process request");
        resp->which_response_type = zmk_hold_tap_Response_error_tag;
        resp->response_type.error = err;
    }
    return true;
}

static void fill_hold_tap_info(zmk_hold_tap_HoldTapInfo *dest,
                                const struct runtime_hold_tap_info *src) {
    dest->id = src->id;
    strncpy(dest->name, src->name, sizeof(dest->name) - 1);
    dest->tapping_term_ms = src->tapping_term_ms;
    dest->quick_tap_ms = src->quick_tap_ms;
    dest->require_prior_idle_ms = src->require_prior_idle_ms;
    dest->flavor = src->flavor;
    dest->default_tapping_term_ms = src->default_tapping_term_ms;
    dest->default_quick_tap_ms = src->default_quick_tap_ms;
    dest->default_require_prior_idle_ms = src->default_require_prior_idle_ms;
    dest->default_flavor = src->default_flavor;
}

static int handle_list_hold_taps(zmk_hold_tap_Response *resp) {
    int count = zmk_runtime_hold_tap_count();
    LOG_DBG("List hold-taps: count=%d", count);

    zmk_hold_tap_ListHoldTapsResponse result = zmk_hold_tap_ListHoldTapsResponse_init_zero;
    result.hold_taps_count = 0;

    for (int i = 0; i < count && i < ARRAY_SIZE(result.hold_taps); i++) {
        struct runtime_hold_tap_info info;
        if (zmk_runtime_hold_tap_get_info(i, &info) == 0) {
            fill_hold_tap_info(&result.hold_taps[result.hold_taps_count], &info);
            result.hold_taps_count++;
        }
    }

    resp->which_response_type = zmk_hold_tap_Response_list_hold_taps_tag;
    resp->response_type.list_hold_taps = result;
    return 0;
}

static int handle_set_tapping_term(const zmk_hold_tap_SetTappingTermRequest *req,
                                    zmk_hold_tap_Response *resp) {
    LOG_DBG("Set tapping_term: id=%d value=%d", req->id, req->value);

    int rc = zmk_runtime_hold_tap_set_tapping_term(req->id, req->value);

    zmk_hold_tap_SetResponse result = zmk_hold_tap_SetResponse_init_zero;
    result.success = (rc == 0);
    resp->which_response_type = zmk_hold_tap_Response_set_tapping_term_tag;
    resp->response_type.set_tapping_term = result;
    return rc;
}

static int handle_set_quick_tap(const zmk_hold_tap_SetQuickTapRequest *req,
                                 zmk_hold_tap_Response *resp) {
    LOG_DBG("Set quick_tap: id=%d value=%d", req->id, req->value);

    int rc = zmk_runtime_hold_tap_set_quick_tap(req->id, req->value);

    zmk_hold_tap_SetResponse result = zmk_hold_tap_SetResponse_init_zero;
    result.success = (rc == 0);
    resp->which_response_type = zmk_hold_tap_Response_set_quick_tap_tag;
    resp->response_type.set_quick_tap = result;
    return rc;
}

static int handle_set_require_prior_idle(const zmk_hold_tap_SetRequirePriorIdleRequest *req,
                                          zmk_hold_tap_Response *resp) {
    LOG_DBG("Set require_prior_idle: id=%d value=%d", req->id, req->value);

    int rc = zmk_runtime_hold_tap_set_require_prior_idle(req->id, req->value);

    zmk_hold_tap_SetResponse result = zmk_hold_tap_SetResponse_init_zero;
    result.success = (rc == 0);
    resp->which_response_type = zmk_hold_tap_Response_set_require_prior_idle_tag;
    resp->response_type.set_require_prior_idle = result;
    return rc;
}

static int handle_set_flavor(const zmk_hold_tap_SetFlavorRequest *req,
                              zmk_hold_tap_Response *resp) {
    LOG_DBG("Set flavor: id=%d value=%d", req->id, req->value);

    int rc = zmk_runtime_hold_tap_set_flavor(req->id, req->value);

    zmk_hold_tap_SetResponse result = zmk_hold_tap_SetResponse_init_zero;
    result.success = (rc == 0);
    resp->which_response_type = zmk_hold_tap_Response_set_flavor_tag;
    resp->response_type.set_flavor = result;
    return rc;
}

static int handle_reset_hold_tap(const zmk_hold_tap_ResetHoldTapRequest *req,
                                  zmk_hold_tap_Response *resp) {
    LOG_DBG("Reset hold-tap: id=%d", req->id);

    int rc = zmk_runtime_hold_tap_reset(req->id);

    zmk_hold_tap_SetResponse result = zmk_hold_tap_SetResponse_init_zero;
    result.success = (rc == 0);
    resp->which_response_type = zmk_hold_tap_Response_reset_hold_tap_tag;
    resp->response_type.reset_hold_tap = result;
    return rc;
}
