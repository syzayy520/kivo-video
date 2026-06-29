#include <type_traits>

#include "kivo/video/playback_graph/all_public_headers.hpp"

using namespace kivo::video::playback_graph;

static_assert(kP7ApiVersion.major == 1);
static_assert(std::is_trivially_copyable_v<GenerationStamp>);
static_assert(std::is_class_v<PlaybackSession>);

int main() {
    CommandToken token{};
    return token.accepted() ? 1 : 0;
}
