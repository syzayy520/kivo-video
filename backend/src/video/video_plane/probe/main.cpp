#include <cstdio>
#include <string>

#include "video/video_plane/probe/decode_probe.hpp"
#include "video/video_plane/probe/probe_options.hpp"
#include "video/video_plane/probe/probe_report.hpp"
#include "video/video_plane/probe/software_baseline.hpp"

int main(int argc, char** argv) {
    namespace probe = kivo::video::video_plane::probe;

    probe::ProbeOptions options{};
    std::string error;
    if (!probe::parse_probe_options(argc, argv, options, error)) {
        std::fprintf(stderr, "%s\n\n%s", error.c_str(), probe::probe_usage());
        return 2;
    }

    std::printf("=== Kivo hardware decode probe (P11-A2) ===\n");
    std::printf("input: %s\n\n", options.input_path.c_str());

    probe::HardwareDecodeProbe hardware(options);
    probe::ProbeResult hardware_result = hardware.run();

    // The baseline runs first so the hardware block can print its CPU ratio;
    // a number without a denominator is not evidence.
    probe::DecodeMetrics software{};
    bool have_software = false;
    std::string software_error;
    if (options.compare_software) {
        have_software = probe::measure_software_baseline(options, software, software_error);
        if (have_software && software.cpu_cycles > 0.0 &&
            hardware_result.metrics.cpu_cycles > 0.0) {
            hardware_result.metrics.cpu_ratio =
                hardware_result.metrics.cpu_cycles / software.cpu_cycles;
            software.cpu_ratio = hardware_result.metrics.cpu_ratio;
        }
    }

    std::printf("[hardware / d3d11va]\n%s\n",
                probe::metrics_to_text(hardware_result.metrics).c_str());

    if (have_software) {
        std::printf("[software baseline]\n%s\n", probe::metrics_to_text(software).c_str());
        if (software.fps > 0.0 && hardware_result.metrics.fps > 0.0) {
            std::printf("  wall speedup   : %.2fx\n\n",
                        hardware_result.metrics.fps / software.fps);
        }
    } else if (options.compare_software) {
        std::printf("[software baseline] failed: %s\n\n", software_error.c_str());
    }

    if (!options.evidence_path.empty()) {
        const bool written =
            probe::write_metrics_file(options.evidence_path, hardware_result.metrics);
        std::printf("evidence: %s -> %s\n\n",
                    written ? "written" : "FAILED",
                    options.evidence_path.c_str());
    }

    std::printf("%s\n", hardware_result.ok ? "RESULT: PASS" : "RESULT: FAIL");
    return hardware_result.ok ? 0 : 1;
}
