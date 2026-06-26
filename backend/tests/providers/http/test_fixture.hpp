#pragma once
#include "kivo/video/providers/http/http_range_provider.hpp"
#include <functional>
namespace ph = kivo::video::providers::http;
enum class FixtureMode { range_ok, empty_416, no_range_200, unknown_length, one_byte_valid, one_byte_nonconformant, source_changed, body_cap, timeout_sim };
inline ph::HttpTransportFn make_fixture(FixtureMode mode, std::uint64_t total=6, const std::string& body="abcdef") {
    return [=]() mutable -> ph::HttpTransportFn {
        int call=0;
        return [=](const ph::HttpTransportRequest& req) mutable -> ph::HttpTransportResponse {
            ph::HttpTransportResponse r;
            call++;
            switch(mode) {
            case FixtureMode::range_ok:
                if (req.range_start.has_value()) {
                    auto s=req.range_start.value(), e=req.range_end.value();
                    if (s==0&&e==0){r.status=206;r.headers["Content-Range"]="bytes 0-0/"+std::to_string(total);}
                    else if(s<total){auto len=std::min(e-s+1,total-s);r.status=206;r.headers["Content-Range"]="bytes "+std::to_string(s)+"-"+std::to_string(s+len-1)+"/"+std::to_string(total);if(s<static_cast<std::uint64_t>(body.size())){auto end=std::min(static_cast<std::uint64_t>(body.size()),s+len);r.body.assign(body.begin()+s,body.begin()+end);}}
                }
                break;
            case FixtureMode::empty_416:
                r.status=416; r.headers["Content-Range"]="bytes */0";
                break;
            case FixtureMode::no_range_200:
                r.status=200; r.body.assign(body.begin(),body.end());
                break;
            case FixtureMode::unknown_length:
                if(req.range_start.has_value()&&req.range_start.value()<static_cast<std::uint64_t>(body.size())){r.status=206;r.headers["Content-Range"]="bytes "+std::to_string(req.range_start.value())+"-0/*";}
                break;
            case FixtureMode::one_byte_valid:
                r.status=206;r.headers["Content-Range"]="bytes 0-0/1";r.body.push_back(static_cast<std::uint8_t>(body[0]));
                break;
            case FixtureMode::one_byte_nonconformant:
                r.status=416;r.headers["Content-Range"]="bytes */1";
                break;
            case FixtureMode::source_changed:
                if(call==1){r.status=206;r.headers["Content-Range"]="bytes 0-0/6";}
                else{r.status=200;r.body.assign(body.begin(),body.end());}
                break;
            case FixtureMode::body_cap:
                r.status=206;r.headers["Content-Range"]="bytes 0-99/100";r.body_cap_hit=true;
                break;
            case FixtureMode::timeout_sim:
                break;
            }
            return r;
        };
    }();
}
