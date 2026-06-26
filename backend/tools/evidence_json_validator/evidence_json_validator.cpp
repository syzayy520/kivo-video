#include "evidence_json_validator.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
using namespace kivo::video;

static bool contains_any(const std::string& s, const std::vector<std::string>& patterns) {
    for (auto& p : patterns) if (s.find(p) != std::string::npos) return true;
    return false;
}

EvidenceJsonValidator validate_evidence_json(const std::string& path) {
    EvidenceJsonValidator v;
    std::ifstream f(path); if (!f) { v.add_error("cannot open file"); return v; }
    std::stringstream ss; ss << f.rdbuf(); auto content = ss.str();

    // Required keywords
    if (content.find("\"schema_version\"") == std::string::npos) v.add_error("missing schema_version");
    if (content.find("\"task_id\"") == std::string::npos) v.add_error("missing task_id");
    if (content.find("\"status\"") == std::string::npos) v.add_error("missing status");

    // RunTime consistency
    bool has_runtime = content.find("\"RUNTIME_PASS\"") != std::string::npos;
    bool has_verified = content.find("\"runtime_verified\"") != std::string::npos &&
                        content.find("\"runtime_verified\": true") != std::string::npos;
    if (has_runtime && !has_verified) v.add_error("RUNTIME_PASS without runtime_verified=true");

    // Secret/path leaks
    std::vector<std::string> secrets = {
        "C:\\Users\\", "/Users/", "/home/", "Authorization:", "Cookie:",
        "access_token", "refresh_token", "api_key=", "secret=",
        "Bearer ", "x-api-key", "Set-Cookie", "username", "password"
    };
    if (contains_any(content, secrets)) {
        auto lc = content;
        std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
        if (contains_any(lc, {"access_token", "refresh_token", "api_key=", "Bearer ", "cookie:", "secret="}))
            v.add_error("credential leak detected");
        if (lc.find("c:\\users\\") != std::string::npos || lc.find("/users/") != std::string::npos || lc.find("/home/") != std::string::npos)
            v.add_error("path leak detected");
    }

    // Not_claimed boundaries
    if (content.find("\"not_claimed\"") == std::string::npos && content.find("\"not_implemented\"") == std::string::npos)
        v.add_error("missing not_claimed/not_implemented boundary");

    return v;
}
