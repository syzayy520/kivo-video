#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "source_core/test_helpers.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;
namespace lf = kivo::video::providers::local_file;

static std::string mk(const std::string& c) {
    char b[256]; std::snprintf(b,sizeof(b),"k_e_%d.tmp",std::rand());
    char abs[512]; _fullpath(abs,b,512); std::string p=abs;
    std::ofstream f(p,std::ios::binary); f.write(c.data(),static_cast<std::streamsize>(c.size())); return p;
}

int main() {
    auto tmp = mk("abcdefghij");
    lf::LocalFileSessionStore store;
    auto orr = lf::LocalFileProvider::open(SourceOpenRequest{SourceUri::from_raw(tmp)}, store);
    CHECK_TRUE(orr.is_success());
    auto sid = orr.session_if_success()->session_id;

    // read + seek + close should all append evidence
    lf::LocalFileProvider::read(SourceReadRequest{sid, std::nullopt, 3}, store);
    lf::LocalFileProvider::seek(SourceSeekRequest{sid, 8, SeekOrigin::begin}, store);
    lf::LocalFileProvider::read(SourceReadRequest{sid, 4ULL, 2}, store);
    lf::LocalFileProvider::read(SourceReadRequest{sid, std::nullopt, 0}, store); // zero-size
    lf::LocalFileProvider::close(sid, store);

    auto ev = lf::LocalFileProvider::get_evidence(sid, store);
    CHECK_TRUE(ev.has_runtime_pass());
    CHECK_TRUE(ev.items.size() >= 5); // open + read + seek + pos_read + close

    // Check operation types present
    bool has_read=false, has_pos=false, has_seek=false, has_close=false;
    for (auto& item : ev.items) {
        if (item.operation == SourceEvidenceOperation::read && item.pass_kind == SourceEvidencePassKind::runtime_pass) has_read=true;
        if (item.operation == SourceEvidenceOperation::positioned_read) has_pos=true;
        if (item.operation == SourceEvidenceOperation::seek) has_seek=true;
        if (item.operation == SourceEvidenceOperation::close) has_close=true;
        // sequence must be monotonic
    }
    CHECK_TRUE(has_read); CHECK_TRUE(has_pos); CHECK_TRUE(has_seek); CHECK_TRUE(has_close);

    // No raw path in evidence
    for (auto& item : ev.items)
        CHECK_TRUE(item.safe_description.find(tmp) == std::string::npos);

    std::remove(tmp.c_str());
    return 0;
}
