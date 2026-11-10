#include "video/video_plane/backends/ffmpeg/audio/ffmpeg_audio_decoder.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
}

using kivo::video::ffmpeg::FfmpegAudioDecoder;
using kivo::video::core::audio::PcmChunk;
using kivo::video::core::audio::PcmEncoding;
using kivo::video::core::audio::PcmFormat;
using kivo::video::core::decode::SampleView;

namespace {
void require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
struct Packet { std::vector<std::uint8_t> bytes; std::int64_t pts; };
struct Facts {
    std::int64_t frames = 0;
    std::uint64_t hash = 14695981039346656037ULL;
    int again = 0;
    std::vector<std::vector<float>> channels;
};

Facts decode(FfmpegAudioDecoder& decoder, const std::vector<Packet>& packets, int channels) {
    Facts facts;
    facts.channels.resize(channels);
    auto drain = [&]() {
        for (;;) {
            PcmChunk chunk;
            const auto rc = decoder.receive(chunk, 0);
            if (rc.try_again()) return false;
            if (rc.end_of_stream()) return true;
            require(rc.ok(), rc.message.c_str());
            require(decoder.output_format().encoding == PcmEncoding::kFloat32, "expected float PCM fixture");
            require(chunk.size == static_cast<std::size_t>(chunk.frame_count) * channels * sizeof(float), "PCM size/stride");
            for (std::size_t b = 0; b < chunk.size; ++b) {
                facts.hash ^= chunk.data[b]; facts.hash *= 1099511628211ULL;
            }
            for (int n = 0; n < chunk.frame_count; ++n) for (int c = 0; c < channels; ++c) {
                float value;
                std::memcpy(&value, chunk.data + (n * channels + c) * sizeof(float), sizeof(value));
                require(std::isfinite(value), "non-finite PCM");
                facts.channels[c].push_back(value);
            }
            facts.frames += chunk.frame_count;
        }
    };
    // Deliberately fill the codec before receiving, exercising caller-owned packet retry.
    for (std::size_t i = 0; i < packets.size();) {
        const auto& p = packets[i];
        const auto rc = decoder.submit({p.bytes.data(), p.bytes.size(), p.pts, false, false}, 0);
        if (rc.try_again()) { ++facts.again; require(!drain(), "early EOS"); }
        else { require(rc.ok(), rc.message.c_str()); ++i; }
    }
    for (;;) {
        const auto rc = decoder.signal_end_of_stream(0);
        if (rc.try_again()) { ++facts.again; require(!drain(), "early EOS on send"); continue; }
        require(rc.ok(), rc.message.c_str()); break;
    }
    require(drain(), "missing EOS after drain packet");
    require(facts.frames > 48000 && facts.again > 0, "too little PCM or backpressure untested");
    return facts;
}
}

int main(int argc, char** argv) {
    try {
        require(argc >= 4, "usage: probe fixture mime frequency-per-channel...");
        const int channels = argc - 3;
        require(channels <= 8, "too many fixture channels");
        AVFormatContext* format = nullptr;
        require(avformat_open_input(&format, argv[1], nullptr, nullptr) >= 0, "open fixture");
        require(avformat_find_stream_info(format, nullptr) >= 0, "fixture stream info");
        const int stream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        require(stream >= 0, "fixture audio stream");
        std::vector<Packet> packets;
        AVPacket* packet = av_packet_alloc();
        while (av_read_frame(format, packet) >= 0) {
            if (packet->stream_index == stream) packets.push_back({
                {packet->data, packet->data + packet->size},
                av_rescale_q(packet->pts, format->streams[stream]->time_base, AVRational{1, 1000000})});
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        avformat_close_input(&format);
        require(!packets.empty(), "no compressed packets");
        FfmpegAudioDecoder decoder;
        SampleView sample{packets[0].bytes.data(), packets[0].bytes.size(), packets[0].pts, false, false};
        require(!decoder.submit(sample, 0).ok(), "submit before start accepted");
        require(!decoder.flush().ok(), "flush before start accepted");
        require(decoder.configure(argv[2], {48000, channels, PcmEncoding::kFloat32}).ok(), "configure");
        require(decoder.start().ok(), "start");
        sample.is_encrypted = true;
        const auto encrypted = decoder.submit(sample, 0);
        require(!encrypted.ok() && !encrypted.message.empty(), "encrypted sample accepted without reason");
        const auto first = decode(decoder, packets, channels);
        require(decoder.flush().ok(), "flush after EOS");
        const auto repeat = decode(decoder, packets, channels);
        require(first.frames == repeat.frames && first.hash == repeat.hash, "flush/replay PCM differs");
        std::printf("{\"codec\":\"%s\",\"frames\":%lld,\"eagain_retries\":%d,\"channels\":[",
                    decoder.codec_name().c_str(), static_cast<long long>(first.frames), first.again);
        for (int c = 0; c < channels; ++c) {
            // Skip codec warmup, then measure two seconds, independently of its layout metadata.
            const auto& pcm = first.channels[c];
            const std::size_t begin = 8192, end = begin + 96000;
            require(pcm.size() > end, "short fixture");
            int crossings = 0;
            double squares = 0;
            for (std::size_t n = begin; n < end; ++n) {
                if (pcm[n-1] <= 0 && pcm[n] > 0) ++crossings;
                squares += pcm[n] * pcm[n];
            }
            const double frequency = crossings / 2.0;
            const double rms = std::sqrt(squares / (end - begin));
            require(std::abs(frequency - std::atof(argv[c+3])) < 2.0, "channel frequency/order mismatch");
            require(rms > 0.01 && rms < 0.15, "channel RMS mismatch");
            std::printf("%s{\"index\":%d,\"frequency_hz\":%.1f,\"rms\":%.6f}", c ? "," : "", c, frequency, rms);
        }
        decoder.stop();
        require(!decoder.submit(sample, 0).ok(), "submit after stop accepted");
        FfmpegAudioDecoder mismatch;
        require(mismatch.configure(argv[2], {48000, channels == 1 ? 2 : 1, PcmEncoding::kFloat32}).ok(), "mismatch configure");
        require(mismatch.start().ok(), "mismatch start");
        sample.is_encrypted = false;
        require(mismatch.submit(sample, 0).ok(), "mismatch submit");
        PcmChunk chunk;
        const auto rejected = mismatch.receive(chunk, 0);
        require(!rejected.ok() && !rejected.try_again() && !rejected.message.empty(), "format mismatch accepted");
        std::puts("],\"eos\":true,\"flush_identical\":true,\"encrypted_rejected\":true,\"format_mismatch_rejected\":true,\"pass\":true}");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what()); return 1;
    }
}
