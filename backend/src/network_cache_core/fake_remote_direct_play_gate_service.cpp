#include "kivo/cinema_engine/network_cache_core/fake_remote_direct_play_gate_service.hpp"

namespace kivo::cinema_engine {

// --- Request builders ---

RemoteDirectPlayGateRequest FakeRemoteDirectPlayGateService::build_direct_play_request() {
    RemoteDirectPlayGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-001";
    r.source_url = "https://emby.example.com/Videos/42/stream?static=true";
    r.is_direct_play = true;
    r.has_credential = false;
    r.redirect_chain = {};
    r.cache_identity_id = "cache-identity-001";
    return r;
}

RemoteDirectPlayGateRequest FakeRemoteDirectPlayGateService::build_transcode_request() {
    RemoteDirectPlayGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-002";
    r.source_url = "https://emby.example.com/Videos/42/master.m3u8";
    r.is_direct_play = false;
    r.has_credential = false;
    r.redirect_chain = {};
    r.cache_identity_id = "cache-identity-002";
    return r;
}

RemoteDirectPlayGateRequest FakeRemoteDirectPlayGateService::build_credential_request() {
    RemoteDirectPlayGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-003";
    r.source_url = "https://emby.example.com/Videos/42/stream?token=secret123";
    r.is_direct_play = true;
    r.has_credential = true;
    r.redirect_chain = {};
    r.cache_identity_id = "cache-identity-003";
    return r;
}

RemoteDirectPlayGateRequest FakeRemoteDirectPlayGateService::build_redirect_leak_request() {
    RemoteDirectPlayGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-004";
    r.source_url = "https://emby.example.com/Videos/42/stream";
    r.is_direct_play = true;
    r.has_credential = false;
    r.redirect_chain = {"https://cdn.example.com/redirect?auth=leaked-token"};
    r.cache_identity_id = "cache-identity-004";
    return r;
}

// --- Result builders ---

RemoteDirectPlayGateResult FakeRemoteDirectPlayGateService::build_passed_result() {
    RemoteDirectPlayGateResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-001";
    r.gate_passed = true;
    r.transcode_requires_approval = false;
    r.credentials_redacted = false;
    r.redirect_leak_detected = false;
    r.cache_binding_valid = true;
    r.failure_reason = "";
    return r;
}

RemoteDirectPlayGateResult FakeRemoteDirectPlayGateService::build_transcode_blocked_result() {
    RemoteDirectPlayGateResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-002";
    r.gate_passed = false;
    r.transcode_requires_approval = true;
    r.credentials_redacted = false;
    r.redirect_leak_detected = false;
    r.cache_binding_valid = true;
    r.failure_reason = "silent transcode blocked, user approval required";
    return r;
}

RemoteDirectPlayGateResult FakeRemoteDirectPlayGateService::build_credential_leak_result() {
    RemoteDirectPlayGateResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "gate-req-004";
    r.gate_passed = false;
    r.transcode_requires_approval = false;
    r.credentials_redacted = false;
    r.redirect_leak_detected = true;
    r.cache_binding_valid = true;
    r.failure_reason = "redirect URL contains credentials";
    return r;
}

// --- Policy builders ---

RemoteDirectPlayGatePolicy FakeRemoteDirectPlayGateService::build_strict_policy() {
    RemoteDirectPlayGatePolicy p;
    p.metadata.schema_version = "1.1";
    p.policy_id = "gate-policy-strict-001";
    p.block_silent_transcode = true;
    p.redact_credentials_in_logs = true;
    p.block_credential_in_redirect = true;
    p.require_cache_identity_binding = true;
    return p;
}

RemoteDirectPlayGatePolicy FakeRemoteDirectPlayGateService::build_permissive_policy() {
    RemoteDirectPlayGatePolicy p;
    p.metadata.schema_version = "1.1";
    p.policy_id = "gate-policy-permissive-001";
    p.block_silent_transcode = false;
    p.redact_credentials_in_logs = false;
    p.block_credential_in_redirect = false;
    p.require_cache_identity_binding = false;
    return p;
}

}  // namespace kivo::cinema_engine
