#include "kivo/cinema_engine/source_core/fake_local_file_source_service.hpp"

namespace kivo::cinema_engine {

ByteStreamContract build_local_file_stream(std::string stream_id, int64_t file_size) {
  ByteStreamContract stream;
  stream.metadata.schema_version = "1.1";
  stream.stream_id = std::move(stream_id);
  stream.source_kind = "local_file";
  stream.position = 0;
  stream.length = file_size;
  stream.is_seekable = true;
  stream.is_readable = true;
  return stream;
}

ByteStreamContract build_network_range_stream(std::string stream_id, int64_t content_length) {
  ByteStreamContract stream;
  stream.metadata.schema_version = "1.1";
  stream.stream_id = std::move(stream_id);
  stream.source_kind = "network_range";
  stream.position = 0;
  stream.length = content_length;
  stream.is_seekable = true;
  stream.is_readable = true;
  return stream;
}

ByteStreamContract build_cache_segment_stream(std::string stream_id, int64_t segment_size) {
  ByteStreamContract stream;
  stream.metadata.schema_version = "1.1";
  stream.stream_id = std::move(stream_id);
  stream.source_kind = "cache_segment";
  stream.position = 0;
  stream.length = segment_size;
  stream.is_seekable = true;
  stream.is_readable = true;
  return stream;
}

}  // namespace kivo::cinema_engine
