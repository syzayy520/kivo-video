#include "kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp"

#include <iostream>

namespace kivo::cinema_engine {

bool WasapiSharedPcmWriter::initialize(const AudioEndpointRuntimeContract& endpoint) {
    std::cout << "[WasapiSharedPcmWriter] initialize: endpoint=" << endpoint.endpoint_name
              << ", sample_rate=" << endpoint.sample_rate
              << ", channels=" << endpoint.channels
              << ", bit_depth=" << endpoint.bit_depth
              << ", output_mode=" << endpoint.output_mode << std::endl;

    endpoint_ = endpoint;
    initialized_ = true;
    return true;
}

WriteResult WasapiSharedPcmWriter::write(const uint8_t* pcm_data, int32_t byte_size) {
    if (!initialized_) {
        WriteResult result;
        result.success = false;
        result.error_message = "Not initialized";
        return result;
    }

    std::cout << "[WasapiSharedPcmWriter] write: bytes=" << byte_size << std::endl;

    WriteResult result;
    result.success = true;
    result.samples_written = byte_size / (endpoint_.channels * (endpoint_.bit_depth / 8));
    result.frames_written = result.samples_written / endpoint_.channels;
    result.is_buffer_full = false;
    result.available_frames = 1024;  // stub: always have space

    total_bytes_written_ += byte_size;
    total_samples_written_ += result.samples_written;

    return result;
}

int32_t WasapiSharedPcmWriter::available_frames() const {
    return initialized_ ? 1024 : 0;
}

std::string WasapiSharedPcmWriter::last_error() const { return last_error_; }

bool WasapiSharedPcmWriter::is_ready() const { return initialized_; }

bool WasapiSharedPcmWriter::start() {
    if (!initialized_) {
        last_error_ = "Not initialized";
        return false;
    }
    std::cout << "[WasapiSharedPcmWriter] start" << std::endl;
    playing_ = true;
    return true;
}

bool WasapiSharedPcmWriter::stop() {
    std::cout << "[WasapiSharedPcmWriter] stop" << std::endl;
    playing_ = false;
    return true;
}

void WasapiSharedPcmWriter::release() {
    std::cout << "[WasapiSharedPcmWriter] release: total_bytes=" << total_bytes_written_
              << ", total_samples=" << total_samples_written_ << std::endl;
    initialized_ = false;
    playing_ = false;
    last_error_.clear();
}

}  // namespace kivo::cinema_engine
