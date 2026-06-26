#pragma once
#include "kivo/video/providers/http/http_range_provider.hpp"
#include <functional>
#include <atomic>
namespace ph = kivo::video::providers::http;
enum class FixtureMode { range_ok, empty_416, no_range_200, unknown_length, one_byte_valid, one_byte_nonconformant, source_changed, body_cap, timeout_sim, redirect_ok, rate_limit_429, temp_unavailable_503, compressed_range, multipart_range, weak_etag, last_modified_only, length_only, auth_required_401, cancel_sim };
inline ph::HttpTransportFn make_fixture(FixtureMode mode, std::uint64_t total=6, const std::string& body="abcdef") {
    return [=, call=std::make_shared<int>(0)]() mutable -> ph::HttpTransportFn {
        return [=](const ph::HttpTransportRequest& req) mutable -> ph::HttpTransportResponse {
            ph::HttpTransportResponse r; (*call)++;
            switch(mode) {
            case FixtureMode::range_ok:
                if (req.range_start.has_value()) {
                    auto s=req.range_start.value(), e=req.range_end.value();
                    if (s==0&&e==0){r.status=206;r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total);r.body.push_back(static_cast<std::uint8_t>(body[0]));}
                    else if(s<total){auto len=std::min(e-s+1,total-s);r.status=206;r.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(s+len-1)+"/"+std::to_string(total);if(s<static_cast<std::uint64_t>(body.size())){auto end=std::min(static_cast<std::uint64_t>(body.size()),s+len);r.body.assign(body.begin()+s,body.begin()+end);}}
                }
                break;
            case FixtureMode::empty_416: r.status=416; r.headers["Content-Range"]="bytes */0"; break;
            case FixtureMode::no_range_200: r.status=200; r.body.assign(body.begin(),body.end()); break;
            case FixtureMode::unknown_length: if(req.range_start.has_value()&&req.range_start.value()<static_cast<std::uint64_t>(body.size())){r.status=206;r.headers["Content-Range"]="bytes "+std::to_string(req.range_start.value())+"-0/*";} break;
            case FixtureMode::one_byte_valid: r.status=206;r.headers["Content-Range"]="bytes 0-0/1";r.body.push_back(static_cast<std::uint8_t>(body[0])); break;
            case FixtureMode::one_byte_nonconformant: r.status=416;r.headers["Content-Range"]="bytes */1"; break;
            case FixtureMode::source_changed: if(*call==1){r.status=206;r.headers["Content-Range"]="bytes 0-0/6";}else{r.status=200;r.body.assign(body.begin(),body.end());} break;
            case FixtureMode::body_cap: r.status=206;r.headers["Content-Range"]="bytes 0-99/100";r.body_cap_hit=true; break;
            case FixtureMode::timeout_sim: break; // no response
            case FixtureMode::redirect_ok: r.status=301;r.headers["Location"]="http://redirect.example.com/file.mp4"; break;
            case FixtureMode::rate_limit_429: r.status=429;r.headers["Retry-After"]="5"; break;
            case FixtureMode::temp_unavailable_503: r.status=503;r.headers["Retry-After"]="10"; break;
            case FixtureMode::compressed_range: r.status=206;r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total);r.headers["Content-Encoding"]="gzip";r.body_cap_hit=true; break;
            case FixtureMode::multipart_range: r.status=206;r.headers["Content-Type"]="multipart/byteranges"; break;
            case FixtureMode::weak_etag: r.status=206;r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total);r.headers["ETag"]="W/\"abc\""; break;
            case FixtureMode::last_modified_only: r.status=206;r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total);r.headers["Last-Modified"]="Wed, 21 Oct 2015 07:28:00 GMT"; break;
            case FixtureMode::length_only: r.status=206;r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total); break;
            case FixtureMode::auth_required_401: r.status=401;r.headers["WWW-Authenticate"]="Basic realm=\"test\""; break;
            case FixtureMode::cancel_sim: r.status=499; break;
            }
            return r;
        };
    }();
}
