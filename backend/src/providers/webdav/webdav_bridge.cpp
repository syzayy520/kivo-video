#include "kivo/video/providers/webdav/webdav_bridge.hpp"
#include "kivo/video/providers/http/http_url.hpp"
namespace wd = kivo::video::providers::webdav;
namespace sc = kivo::video::source_core;
namespace ph = kivo::video::providers::http;

wd::WebDavInfo wd::probe_resource_type(const std::string& http_url, ph::HttpTransportFn transport) {
    WebDavInfo info;
    ph::HttpTransportRequest req; req.method="PROPFIND"; req.url=http_url;
    req.headers["Depth"]="0"; req.headers["Content-Type"]="application/xml; charset=utf-8";
    req.body_cap=65536;
    auto resp=transport(req);
    if (resp.status==207) info.probe=WebDavProbeResult::file_like;
    else if (resp.status==405||resp.status==501||resp.status==403) info.probe=WebDavProbeResult::propfind_failed;
    else info.probe=WebDavProbeResult::resource_type_unknown;
    info.safe_label="webdav:"+std::to_string(resp.status);
    return info;
}

sc::SourceOpenResult wd::WebDavRangeBridge::open(const sc::SourceOpenRequest& req, ph::HttpTransportFn transport, ph::HttpRangeSessionStore& store) {
    std::string raw = req.uri.untrusted_input_for_open_boundary();
    // Probe resource type
    auto info = probe_resource_type(raw, transport);
    if (info.probe == WebDavProbeResult::collection)
        return sc::SourceOpenResult::failure({sc::SourceErrorCode::invalid_request, "collection rejected"});
    if (info.probe == WebDavProbeResult::malformed)
        return sc::SourceOpenResult::failure({sc::SourceErrorCode::invalid_request, "propfind malformed"});

    // Bridge to HTTP Range
    return ph::HttpRangeProvider::open(req, transport, store);
}
