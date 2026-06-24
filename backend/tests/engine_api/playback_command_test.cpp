#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/engine_api/fake_playback_command_service.hpp"

using namespace kivo::cinema_engine;

static void test_open_command() {
    auto cmd = build_open_command("session.test.001");
    assert(cmd.kind == EngineCommandKind::Open);
    assert(cmd.payload_ref == "session.test.001");
    std::cout << "  PASS: open_command\n";
}

static void test_play_command() {
    auto cmd = build_play_command(1);
    assert(cmd.kind == EngineCommandKind::Play);
    assert(cmd.command_id == "cmd.play.1");
    std::cout << "  PASS: play_command\n";
}

static void test_seek_command() {
    auto cmd = build_seek_command(5, "seek.target.us.1000000");
    assert(cmd.kind == EngineCommandKind::Seek);
    assert(cmd.payload_ref == "seek.target.us.1000000");
    std::cout << "  PASS: seek_command\n";
}

static void test_close_command() {
    auto cmd = build_close_command(10);
    assert(cmd.kind == EngineCommandKind::Close);
    std::cout << "  PASS: close_command\n";
}

static void test_generation_valid() {
    auto play = build_play_command(3);
    assert(is_command_generation_valid(play, 3));   // exact match
    assert(is_command_generation_valid(play, 2));   // ahead
    assert(!is_command_generation_valid(play, 4));  // stale
    std::cout << "  PASS: generation_valid\n";
}

static void test_close_always_valid() {
    auto close = build_close_command(1);
    assert(is_command_generation_valid(close, 100));  // always valid
    std::cout << "  PASS: close_always_valid\n";
}

int main() {
    std::cout << "playback_command_test:\n";
    test_open_command();
    test_play_command();
    test_seek_command();
    test_close_command();
    test_generation_valid();
    test_close_always_valid();
    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}
