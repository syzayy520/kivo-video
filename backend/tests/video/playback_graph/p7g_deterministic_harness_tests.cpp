#include "video/playback_graph/attachment/attachment_operation_registry.hpp"
#include "video/playback_graph/command/command_conflict_matrix.hpp"
#include "video/playback_graph/test_support/fake_attachments.hpp"
#include "video/playback_graph/test_support/fake_clock.hpp"
#include "video/playback_graph/test_support/manual_event_pump.hpp"

using namespace kivo::video::playback_graph::runtime;
using namespace kivo::video::playback_graph::test_support;

int main() {
    FakeClock clock{};
    clock.advance_ms(7);
    if (clock.now_ms() != 7) {
        return 1;
    }

    ManualEventPump pump{};
    if (!pump.push(1) || !pump.push(2) || pump.run_all() != 2) {
        return 2;
    }

    FakeAttachments attachments{};
    const auto trace = attachments.run_seed(1701);
    if (trace.seed != 1701 || trace.command_count != 4 || trace.commands[3] != 1704) {
        return 3;
    }

    const auto conflict =
        resolve_command_conflict(GraphCommandKind::Start,
                                 kivo::video::playback_graph::PlaybackGraphState::Ready,
                                 false);
    if (conflict.decision != CommandConflictDecision::Accept) {
        return 4;
    }
    return 0;
}
