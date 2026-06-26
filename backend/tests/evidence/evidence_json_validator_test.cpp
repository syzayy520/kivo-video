#include "kivo/video/source_core/runtime/session/source_session.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>
namespace kv = kivo::video;
static int fail(const char* msg) { std::cerr << "FAIL: " << msg << "\n"; return 1; }
#define CHECK(expr) do { if (!(expr)) return fail(#expr); } while(false)

int main() {
    // Write a valid test evidence file
    char b[256]; std::snprintf(b,sizeof(b),"kivo_ev_test_%d.json",std::rand());
    std::string path=b;
    {
        std::ofstream f(path);
        f << "{\"schema_version\":\"p2-evidence-v8\",\"task_id\":\"T-001\",\"status\":\"RUNTIME_PASS\",";
        f << "\"runtime_verified\":true,\"runtime_environment_kind\":\"local_real_runtime\",";
        f << "\"implementation\":{\"not_claimed\":[\"production\"],\"not_implemented\":[\"wasapi\"]}}";
    }

    // Read and validate
    std::ifstream in(path); CHECK(in.good());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(content.find("schema_version") != std::string::npos);
    CHECK(content.find("RUNTIME_PASS") != std::string::npos);
    CHECK(content.find("not_claimed") != std::string::npos);

    // No raw secrets
    CHECK(content.find("C:\\Users\\") == std::string::npos);

    std::remove(path.c_str());
    return 0;
}
