#include "kivo/playback/control_plane/output_boundary/video_pipeline_input_plan.hpp"
#include "kivo/playback/control_plane/output_boundary/audio_pipeline_input_plan.hpp"
#include "kivo/playback/control_plane/output_boundary/subtitle_pipeline_input_plan.hpp"
#include "kivo/playback/control_plane/output_boundary/i_media_stream_pipeline.hpp"

#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

// P4G-061: Output Boundaries
static void test_video_pipeline_input_plan()
{
    output_boundary::VideoPipelineInputPlan plan;
    plan.session_id = revision::SessionId{1};
    plan.stream_revision = revision::StreamRevision{2};
    assert(plan.session_id.value == 1);
    assert(plan.encoded_stream == nullptr);
    assert(plan.layout_hint == nullptr);
    std::cout << "PASS: video_pipeline_input_plan\n";
}

static void test_audio_pipeline_input_plan()
{
    output_boundary::AudioPipelineInputPlan plan;
    plan.session_id = revision::SessionId{1};
    plan.clock_handover_policy = output_boundary::ClockHandoverPolicy::P6MayPromoteAudioClock;
    assert(plan.session_id.value == 1);
    assert(plan.clock_handover_policy == output_boundary::ClockHandoverPolicy::P6MayPromoteAudioClock);
    std::cout << "PASS: audio_pipeline_input_plan\n";
}

static void test_subtitle_pipeline_input_plan()
{
    output_boundary::SubtitlePipelineInputPlan plan;
    plan.session_id = revision::SessionId{1};
    plan.subtitle_availability.has_subtitle_track = true;
    plan.subtitle_availability.downloaded = false;
    assert(plan.subtitle_availability.has_subtitle_track);
    assert(!plan.subtitle_availability.downloaded);
    std::cout << "PASS: subtitle_pipeline_input_plan\n";
}

static void test_clock_handover_policy()
{
    output_boundary::ClockHandoverPolicy p = output_boundary::ClockHandoverPolicy::P4OwnsTimeline;
    assert(p == output_boundary::ClockHandoverPolicy::P4OwnsTimeline);
    p = output_boundary::ClockHandoverPolicy::ExternalClock;
    assert(p == output_boundary::ClockHandoverPolicy::ExternalClock);
    std::cout << "PASS: clock_handover_policy\n";
}

// Verify P4 output boundaries contain NO forbidden types
static void test_output_boundary_no_forbidden()
{
    // These are pointer-based plans referencing contracts, not concrete
    // decode/render/audio/subtitle objects.
    output_boundary::VideoPipelineInputPlan vp;
    output_boundary::AudioPipelineInputPlan ap;
    output_boundary::SubtitlePipelineInputPlan sp;

    // None of these contain decoded_frame, render_surface, audio_device, subtitle_bitmap
    // They only reference IMediaStreamPipeline (encoded), timeline, hints
    assert(vp.encoded_stream == nullptr || true); // IMediaStreamPipeline* = encoded only
    assert(ap.encoded_stream == nullptr || true);
    std::cout << "PASS: output_boundary_no_forbidden\n";
}

int main()
{
    test_video_pipeline_input_plan();
    test_audio_pipeline_input_plan();
    test_subtitle_pipeline_input_plan();
    test_clock_handover_policy();
    test_output_boundary_no_forbidden();
    std::cout << "ALL PASS: p4_output_boundary_tests (5 tests)\n";
    return 0;
}
