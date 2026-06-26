#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <atomic>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

int main() {
    char b[256]; std::snprintf(b,sizeof(b),"k_ts_%d.tmp",std::rand());
    char abs[512]; _fullpath(abs,b,512); std::string tmp=abs;
    { std::ofstream f(tmp); f<<"thread safety test content is longer to allow multiple concurrent reads"; }

    lf::LocalFileSessionStore store;
    auto orr = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(orr.is_success());
    auto sid = orr.session_if_success()->session_id;

    std::atomic<bool> running{true};
    std::atomic<int> errors{0};

    auto ev_thread = [&]() {
        while (running) {
            auto ev = lf::LocalFileProvider::get_evidence(sid, store);
            if (ev.uri_redacted.find(tmp) != std::string::npos) errors++;
        }
    };
    auto read_thread = [&]() {
        while (running) {
            lf::LocalFileProvider::read(SourceReadRequest{sid, 1ULL, 4}, store);
        }
    };
    auto di_thread = [&]() {
        while (running) {
            lf::LocalFileProvider::make_direct_play_input(sid, store);
        }
    };

    std::thread t1(ev_thread), t2(read_thread), t3(di_thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;
    t1.join(); t2.join(); t3.join();

    CHECK_EQ(errors.load(), 0);

    // Close tidies up
    auto cerr = lf::LocalFileProvider::close(sid, store);
    CHECK_TRUE(cerr.is_ok());

    // After close, read returns session_closed
    auto post_read = lf::LocalFileProvider::read(SourceReadRequest{sid, std::nullopt, 1}, store);
    CHECK_TRUE(!post_read.is_success());

    std::remove(tmp.c_str());
    return 0;
}
