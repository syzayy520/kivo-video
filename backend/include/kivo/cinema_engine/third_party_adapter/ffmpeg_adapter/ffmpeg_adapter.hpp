#pragma once

// FFmpeg Adapter - public boundary header.
// Contains ZERO FFmpeg forward declarations. All FFmpeg interactions are behind opaque handles.
// FFmpeg native types (AVFormatContext, AVCodecContext, AVPacket, AVFrame) are
// ONLY allowed inside backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.cpp.

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine {

// Opaque handle types
enum class FfmpegIoSeekOrigin : std::uint8_t {
    Begin = 0,
    Current = 1,
    End = 2,
    Size = 3
};

struct FfmpegInputCallbacks {
    void* user_data{nullptr};
    int (*read)(void* user_data, std::uint8_t* buffer, int buffer_size){nullptr};
    std::int64_t (*seek)(
        void* user_data,
        std::int64_t offset,
        FfmpegIoSeekOrigin origin){nullptr};
};

struct FfmpegInputConfig {
    FfmpegInputCallbacks callbacks{};
    int buffer_size{32768};
};

struct FfmpegFormatHandle {
    void* native = nullptr;
    void* custom_io_context = nullptr;
    void (*custom_io_destroy)(void* context) = nullptr;
};
struct FfmpegCodecHandle { void* native = nullptr; };

// Packet metadata (extracted from AVPacket)
struct FfmpegPacketInfo {
    int stream_index = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int duration = 0;
    bool is_key_frame = false;
};

struct FfmpegPacketSideData {
    int type = 0;
    std::vector<uint8_t> data;
};

// Read result (includes compressed data copy)
struct FfmpegReadResult {
    bool success = false;
    bool eof = false;
    std::string error_message;
    FfmpegPacketInfo packet;
    std::vector<uint8_t> packet_data;
    std::vector<FfmpegPacketSideData> side_data;
};

// Frame result (extracted from AVFrame)
struct FfmpegFrameResult {
    bool is_video = false;
    bool is_audio = false;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t duration = 0;
    int width = 0;
    int height = 0;
    std::string pixel_format;
    int sample_rate = 0;
    int channels = 0;
    std::string sample_format;
    // Real pixel/sample data extracted from AVFrame.
    std::vector<uint8_t> frame_data;
    int linesize[8]{0, 0, 0, 0, 0, 0, 0, 0};  // stride per plane
    int plane_count{0};            // number of planes
};

// Codec info (extracted from AVCodecContext)
struct FfmpegCodecInfo {
    int width = 0;
    int height = 0;
    std::string pixel_format;
};

// Send packet result
enum class FfmpegSendStatus { Success, NeedDrain, EndOfStream, Error };

struct FfmpegSendResult {
    FfmpegSendStatus status = FfmpegSendStatus::Error;
    std::string error_message;
};

class FfmpegAdapter {
public:
    // --- Availability ---
    static bool is_available();

    // --- Format context lifecycle ---
    static FfmpegFormatHandle open_file(const std::string& path);
    /// URL 打开（220）：http://127.0.0.1:port/... 本地 Range 代理（network+http 协议需启用）。
    static FfmpegFormatHandle open_url(const std::string& url);
    static FfmpegFormatHandle open_custom_io(const FfmpegInputConfig& config);
    static void free_format_context(FfmpegFormatHandle& handle);

    // --- Format context queries ---
    static bool probe_container(FfmpegFormatHandle handle,
                                std::string& container_format,
                                double& duration,
                                int64_t& bitrate_estimate);
    static bool enumerate_streams(FfmpegFormatHandle handle,
                                  std::vector<int>& video_indices,
                                  std::vector<int>& audio_indices,
                                  std::vector<int>& subtitle_indices);

    /// 只保留 [keep_stream] 一路，其余轨道标成 AVDISCARD_ALL。
    ///
    /// 存在理由（真机实测 2026-11-10）：1080p VC-1 片源里每个视频帧对应
    /// **约 50 个**非视频包，光是把它们从 demuxer 里取出来再丢掉，就要吃掉
    /// 每个视频帧 ~35 ms —— 和一个 24 fps 的帧间隔几乎一样长。标了 DISCARD_ALL
    /// 之后 demuxer 直接跳过载荷，不再为这些包付出代价。
    ///
    /// 只在解封装之后、循环读包之前调用一次；`avformat_find_stream_info`
    /// 之前调用无效（那时轨道还没建好）。
    static void discard_streams_except(FfmpegFormatHandle handle, int keep_stream);

    // --- Stream info extraction ---
    static bool get_stream_codec_name(FfmpegFormatHandle handle, int stream_index,
                                      std::string& codec_name);
    static bool get_stream_video_info(FfmpegFormatHandle handle, int stream_index,
                                      int& width, int& height, double& fps,
                                      std::string& pixel_format);
    static bool get_stream_audio_info(FfmpegFormatHandle handle, int stream_index,
                                      int& sample_rate, int& channels,
                                      std::string& channel_layout,
                                      std::string& sample_format);
    static bool get_stream_metadata(FfmpegFormatHandle handle, int stream_index,
                                    std::string& language, int64_t& duration,
                                    int64_t& bitrate);
    static bool rescale_stream_timestamp_us(FfmpegFormatHandle handle,
                                            int stream_index,
                                            int64_t timestamp,
                                            int64_t& out_microseconds);

    // --- Packet reading (wraps av_read_frame, returns data copy) ---
    static FfmpegReadResult read_packet(FfmpegFormatHandle handle);

    // --- Seeking ---
    static bool seek(FfmpegFormatHandle handle, double seconds);

    // --- Codec lifecycle ---
    static FfmpegCodecHandle open_decoder_by_name(const std::string& codec_name);
    static FfmpegCodecHandle create_codec_context(FfmpegFormatHandle format_handle,
                                                  int stream_index);
    static void free_codec_context(FfmpegCodecHandle& handle);
    static FfmpegCodecInfo get_codec_info(FfmpegCodecHandle handle);

    // --- Decode operations ---
    static FfmpegSendResult send_packet(FfmpegCodecHandle codec_handle,
                                        const FfmpegPacketInfo& info,
                                        const uint8_t* data, int size,
                                        const std::vector<FfmpegPacketSideData>& side_data = {});
    static bool receive_frame(FfmpegCodecHandle codec_handle,
                              FfmpegFrameResult& result);
    static bool receive_frame_rgba(FfmpegCodecHandle codec_handle,
                                   FfmpegFrameResult& result);
    static void flush_decoder(FfmpegCodecHandle codec_handle);
    /// URL/fd 通用 seek（220）：按流 timebase 重算目标时间戳后 avformat_seek_file。
    static bool seek_file_us(FfmpegFormatHandle handle, int stream_index,
                             std::int64_t target_us);

    // --- Utility ---
    static std::string map_error(int ffmpeg_error);
    static std::string get_codec_name_from_id(int codec_id);
    static std::string get_pixel_format_name(int pixel_format);
    static std::string get_sample_format_name(int sample_format);
    static std::string get_channel_layout_description(uint64_t channel_layout);

    /// 诊断计时（24x）：把 `receive_frame_rgba` 拆成"avcodec 真解码"与
    /// "swscale 转 RGBA"两段。存在的理由：整体 88 ms/帧 无法判断该优化谁——
    /// 可能是解码器慢，也可能是 1080p 逐帧转 8.3 MB 的 RGBA 慢。
    /// 全局累加，由会话在开始/结束时 reset 与读取。
    struct DecodeTimings {
        std::int64_t send_ns = 0;
        std::int64_t send_calls = 0;
        std::int64_t resize_ns = 0;
        std::int64_t avcodec_ns = 0;
        std::int64_t sws_ns = 0;
        std::int64_t frames = 0;
    };
    static DecodeTimings take_decode_timings();
    static void reset_decode_timings();

    /// 归还一帧的 RGBA 缓冲到线程内池子，供下一帧复用（24x）。
    ///
    /// 必须归还的理由是真机实测出来的：每帧新建 `FfmpegFrameResult` 会让
    /// `frame_data.resize(1920*1080*4)` 重新走一次 8.3 MB 的分配 + 清零，
    /// 单这一项就 **69 ms/帧**，比真正的解码（8.3 ms）贵 8 倍。池化之后
    /// 每帧只是交换一个指针，分配开销归零。
    /// 调用方必须保证这块内存已经用完（渲染完成）再归还。
    static void recycle_frame_buffer(std::vector<uint8_t>&& buffer);
};

}  // namespace kivo::cinema_engine
