#include "kivo/playback/control_plane/worker/reader_worker.hpp"
#include "kivo/playback/control_plane/reader/reader_orchestrator.hpp"
#include "kivo/playback/control_plane/reader/reader_read_payload_kind.hpp"
#include "kivo/playback/control_plane/reader/reader_read_result.hpp"
#include "kivo/playback/control_plane/buffer/stream_end_kind.hpp"
#include "kivo/playback/control_plane/buffer/buffer_item.hpp"
#include "kivo/playback/control_plane/buffer/stream_end_marker.hpp"
#include "kivo/playback/control_plane/memory/mutable_memory_block.hpp"
#include "kivo/playback/control_plane/memory/immutable_memory_block.hpp"
#include "kivo/playback/control_plane/memory/memory_lease.hpp"
#include "kivo/playback/control_plane/memory/read_only_memory_chain.hpp"
#include "kivo/playback/control_plane/memory/memory_pool_policy.hpp"
#include "kivo/playback/control_plane/contracts/stream_pull_status.hpp"
#include "kivo/playback/control_plane/contracts/stream_pull_result.hpp"
#include "kivo/playback/control_plane/output_boundary/i_media_stream_pipeline.hpp"

#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

// P4G-031: ReaderWorker
static void test_reader_worker_post_status()
{
    worker::ReaderWorkerPostStatus s = worker::ReaderWorkerPostStatus::Accepted;
    assert(s == worker::ReaderWorkerPostStatus::Accepted);
    s = worker::ReaderWorkerPostStatus::RejectedQueueFull;
    assert(s == worker::ReaderWorkerPostStatus::RejectedQueueFull);
    std::cout << "PASS: reader_worker_post_status\n";
}

static void test_reader_worker_requests()
{
    worker::ReaderReadRequest req;
    req.stamp.session_id = revision::SessionId{1};
    req.instance_id = revision::ReaderInstanceId{2};
    assert(req.stamp.session_id.value == 1);
    assert(req.instance_id.value == 2);
    std::cout << "PASS: reader_worker_requests\n";
}

// P4G-032: ReaderOrchestrator
static void test_read_priority()
{
    reader::ReadPriority p = reader::ReadPriority::Urgent;
    assert(p == reader::ReadPriority::Urgent);
    std::cout << "PASS: read_priority\n";
}

// P4G-033: ReaderReadResult legal matrix
static void test_reader_read_payload_kind()
{
    reader::ReaderReadPayloadKind k = reader::ReaderReadPayloadKind::DataOnly;
    assert(k == reader::ReaderReadPayloadKind::DataOnly);
    k = reader::ReaderReadPayloadKind::DataAndEndMarker;
    assert(k == reader::ReaderReadPayloadKind::DataAndEndMarker);
    std::cout << "PASS: reader_read_payload_kind\n";
}

static void test_stream_end_kind()
{
    buffer::StreamEndKind k = buffer::StreamEndKind::EndOfFile;
    assert(k == buffer::StreamEndKind::EndOfFile);
    k = buffer::StreamEndKind::LiveEdgeReached;
    assert(k == buffer::StreamEndKind::LiveEdgeReached);
    std::cout << "PASS: stream_end_kind\n";
}

static void test_reader_read_result_data_only_legal()
{
    reader::ReaderReadResult r;
    r.payload_kind = reader::ReaderReadPayloadKind::DataOnly;
    r.data = reader::ReaderDataPayload{};
    assert(r.is_legal());
    std::cout << "PASS: reader_read_result_data_only_legal\n";
}

static void test_reader_read_result_data_only_illegal_with_endmarker()
{
    reader::ReaderReadResult r;
    r.payload_kind = reader::ReaderReadPayloadKind::DataOnly;
    r.data = reader::ReaderDataPayload{};
    r.end_marker = reader::StreamEndMarker{}; // forbidden for DataOnly
    assert(!r.is_legal());
    std::cout << "PASS: reader_read_result_data_only_illegal_with_endmarker\n";
}

static void test_reader_read_result_data_and_endmarker_legal()
{
    reader::ReaderReadResult r;
    r.payload_kind = reader::ReaderReadPayloadKind::DataAndEndMarker;
    r.data = reader::ReaderDataPayload{};
    r.end_marker = reader::StreamEndMarker{};
    assert(r.is_legal());
    std::cout << "PASS: reader_read_result_data_and_endmarker_legal\n";
}

static void test_reader_read_result_wouldblock_legal()
{
    reader::ReaderReadResult r;
    r.payload_kind = reader::ReaderReadPayloadKind::WouldBlock;
    assert(r.is_legal());
    r.data = reader::ReaderDataPayload{}; // forbidden
    assert(!r.is_legal());
    std::cout << "PASS: reader_read_result_wouldblock_legal\n";
}

static void test_reader_read_result_failure_requires_failure()
{
    reader::ReaderReadResult r;
    r.payload_kind = reader::ReaderReadPayloadKind::Failure;
    assert(!r.is_legal()); // failure missing
    r.failure = reader::ReaderFailure{};
    assert(r.is_legal());
    std::cout << "PASS: reader_read_result_failure_requires_failure\n";
}

static void test_reader_read_result_cancelled_optional_endmarker()
{
    reader::ReaderReadResult r;
    r.payload_kind = reader::ReaderReadPayloadKind::Cancelled;
    assert(r.is_legal()); // no end_marker ok
    r.end_marker = reader::StreamEndMarker{};
    r.end_marker->kind = buffer::StreamEndKind::Cancelled;
    assert(r.is_legal()); // end_marker with Cancelled kind ok
    r.end_marker->kind = buffer::StreamEndKind::EndOfFile;
    assert(!r.is_legal()); // wrong kind
    std::cout << "PASS: reader_read_result_cancelled_optional_endmarker\n";
}

// P4G-034: Memory Lease / Freeze / Publish
static void test_memory_block_state()
{
    memory::MemoryBlockState s = memory::MemoryBlockState::FreeInPool;
    assert(s == memory::MemoryBlockState::FreeInPool);
    s = memory::MemoryBlockState::FrozenPublished;
    assert(s == memory::MemoryBlockState::FrozenPublished);
    std::cout << "PASS: memory_block_state\n";
}

static void test_mutable_memory_block()
{
    memory::MutableMemoryBlock b;
    b.id = revision::MemoryBlockId{5};
    b.capacity = primitives::Bytes{4096};
    assert(b.id.value == 5);
    assert(b.capacity.value == 4096);
    std::cout << "PASS: mutable_memory_block\n";
}

static void test_mutable_block_lease()
{
    memory::MutableBlockLease lease;
    lease.block_id = revision::MemoryBlockId{1};
    lease.lease_nonce = 99;
    assert(lease.lease_nonce == 99);
    std::cout << "PASS: mutable_block_lease\n";
}

static void test_immutable_memory_block()
{
    memory::ImmutableMemoryBlock b;
    b.size = primitives::Bytes{100};
    b.capacity = primitives::Bytes{200};
    assert(b.size.value <= b.capacity.value);
    std::cout << "PASS: immutable_memory_block\n";
}

static void test_memory_lease_status()
{
    memory::MemoryLeaseResult r;
    r.status = memory::MemoryLeaseStatus::Granted;
    assert(r.status == memory::MemoryLeaseStatus::Granted);
    assert(!r.lease.has_value());
    std::cout << "PASS: memory_lease_status\n";
}

// P4G-035: ReadOnlyMemoryChain no-heap
static void test_read_only_memory_segment()
{
    memory::ReadOnlyMemorySegment seg;
    seg.offset = primitives::Bytes{0};
    seg.size = primitives::Bytes{10};
    seg.block.size = primitives::Bytes{10};
    seg.block.capacity = primitives::Bytes{20};
    assert(seg.size.value <= seg.block.size.value);
    std::cout << "PASS: read_only_memory_segment\n";
}

static void test_read_only_memory_chain_inline()
{
    memory::ReadOnlyMemoryChain chain;
    chain.inline_count = 2;
    chain.inline_segments[0].offset = primitives::Bytes{0};
    chain.inline_segments[0].size = primitives::Bytes{5};
    chain.inline_segments[0].block.size = primitives::Bytes{5};
    chain.inline_segments[0].block.capacity = primitives::Bytes{10};
    assert(chain.is_inline_only());
    assert(chain.bounds_valid());
    std::cout << "PASS: read_only_memory_chain_inline\n";
}

static void test_read_only_memory_chain_bounds_violation()
{
    memory::ReadOnlyMemoryChain chain;
    chain.inline_count = 1;
    chain.inline_segments[0].offset = primitives::Bytes{0};
    chain.inline_segments[0].size = primitives::Bytes{15};
    chain.inline_segments[0].block.size = primitives::Bytes{10}; // size > block.size
    chain.inline_segments[0].block.capacity = primitives::Bytes{20};
    assert(!chain.bounds_valid());
    std::cout << "PASS: read_only_memory_chain_bounds_violation\n";
}

// P4G-036: MemoryPoolPolicy
static void test_memory_pool_strategy()
{
    memory::MemoryPoolStrategy s = memory::MemoryPoolStrategy::FixedSlab;
    assert(s == memory::MemoryPoolStrategy::FixedSlab);
    s = memory::MemoryPoolStrategy::LargeBlockFallback;
    assert(s == memory::MemoryPoolStrategy::LargeBlockFallback);
    std::cout << "PASS: memory_pool_strategy\n";
}

static void test_memory_pool_policy()
{
    memory::MemoryPoolPolicy p;
    p.default_block_size = primitives::Bytes{65536};
    p.allow_reuse = true;
    p.reclaim_on_seek = true;
    assert(p.default_block_size.value == 65536);
    assert(p.allow_reuse);
    std::cout << "PASS: memory_pool_policy\n";
}

// P4G-037: BufferItem
static void test_buffer_item_kind()
{
    buffer::BufferItemKind k = buffer::BufferItemKind::Data;
    assert(k == buffer::BufferItemKind::Data);
    k = buffer::BufferItemKind::EndMarker;
    assert(k == buffer::BufferItemKind::EndMarker);
    std::cout << "PASS: buffer_item_kind\n";
}

static void test_buffer_item_data()
{
    buffer::BufferItem item;
    item.kind = buffer::BufferItemKind::Data;
    item.chunk = memory::ReadOnlyMemoryChain{};
    assert(item.kind == buffer::BufferItemKind::Data);
    assert(item.chunk.has_value());
    std::cout << "PASS: buffer_item_data\n";
}

static void test_buffer_item_endmarker()
{
    buffer::BufferItem item;
    item.kind = buffer::BufferItemKind::EndMarker;
    item.end_marker = buffer::BufferEndMarker{};
    item.end_marker->kind = buffer::StreamEndKind::EndOfFile;
    assert(item.end_marker->kind == buffer::StreamEndKind::EndOfFile);
    std::cout << "PASS: buffer_item_endmarker\n";
}

// P4G-038: EndProjectionKind
static void test_end_projection_kind()
{
    buffer::EndProjectionKind k = buffer::EndProjectionKind::TrueEof;
    assert(k == buffer::EndProjectionKind::TrueEof);
    k = buffer::EndProjectionKind::LiveEdgeProjectedAsEof;
    assert(k == buffer::EndProjectionKind::LiveEdgeProjectedAsEof);
    std::cout << "PASS: end_projection_kind\n";
}

// P4G-039: StreamPullResult legal matrix
static void test_stream_pull_status()
{
    contracts::StreamPullStatus s = contracts::StreamPullStatus::Data;
    assert(s == contracts::StreamPullStatus::Data);
    s = contracts::StreamPullStatus::LiveEdgeReached;
    assert(s == contracts::StreamPullStatus::LiveEdgeReached);
    std::cout << "PASS: stream_pull_status\n";
}

static void test_stream_pull_result_data_legal()
{
    contracts::StreamPullResult r;
    r.status = contracts::StreamPullStatus::Data;
    r.chunk = memory::ReadOnlyMemoryChain{};
    assert(r.is_legal());
    std::cout << "PASS: stream_pull_result_data_legal\n";
}

static void test_stream_pull_result_data_illegal_no_chunk()
{
    contracts::StreamPullResult r;
    r.status = contracts::StreamPullStatus::Data;
    assert(!r.is_legal()); // chunk missing
    std::cout << "PASS: stream_pull_result_data_illegal_no_chunk\n";
}

static void test_stream_pull_result_wouldblock_legal()
{
    contracts::StreamPullResult r;
    r.status = contracts::StreamPullStatus::WouldBlock;
    assert(!r.is_legal()); // wait_hint missing
    r.wait_hint = primitives::Milliseconds{50};
    assert(r.is_legal());
    std::cout << "PASS: stream_pull_result_wouldblock_legal\n";
}

static void test_stream_pull_result_eof_legal()
{
    contracts::StreamPullResult r;
    r.status = contracts::StreamPullStatus::EndOfStream;
    r.end_marker = buffer::BufferEndMarker{};
    r.end_marker->kind = buffer::StreamEndKind::EndOfFile;
    assert(r.is_legal());
    r.end_marker->kind = buffer::StreamEndKind::LiveEdgeReached; // wrong kind
    assert(!r.is_legal());
    std::cout << "PASS: stream_pull_result_eof_legal\n";
}

static void test_stream_pull_result_error_requires_failure()
{
    contracts::StreamPullResult r;
    r.status = contracts::StreamPullStatus::Error;
    assert(!r.is_legal());
    r.failure = contracts::StreamPullFailure{};
    assert(r.is_legal());
    std::cout << "PASS: stream_pull_result_error_requires_failure\n";
}

// P4G-040: IMediaStreamPipeline
static void test_pull_request()
{
    output_boundary::PullRequest req;
    req.stamp.session_id = revision::SessionId{3};
    req.max_bytes = primitives::Bytes{1024};
    assert(req.stamp.session_id.value == 3);
    assert(req.max_bytes.value == 1024);
    std::cout << "PASS: pull_request\n";
}

int main()
{
    test_reader_worker_post_status();
    test_reader_worker_requests();
    test_read_priority();
    test_reader_read_payload_kind();
    test_stream_end_kind();
    test_reader_read_result_data_only_legal();
    test_reader_read_result_data_only_illegal_with_endmarker();
    test_reader_read_result_data_and_endmarker_legal();
    test_reader_read_result_wouldblock_legal();
    test_reader_read_result_failure_requires_failure();
    test_reader_read_result_cancelled_optional_endmarker();
    test_memory_block_state();
    test_mutable_memory_block();
    test_mutable_block_lease();
    test_immutable_memory_block();
    test_memory_lease_status();
    test_read_only_memory_segment();
    test_read_only_memory_chain_inline();
    test_read_only_memory_chain_bounds_violation();
    test_memory_pool_strategy();
    test_memory_pool_policy();
    test_buffer_item_kind();
    test_buffer_item_data();
    test_buffer_item_endmarker();
    test_end_projection_kind();
    test_stream_pull_status();
    test_stream_pull_result_data_legal();
    test_stream_pull_result_data_illegal_no_chunk();
    test_stream_pull_result_wouldblock_legal();
    test_stream_pull_result_eof_legal();
    test_stream_pull_result_error_requires_failure();
    test_pull_request();
    std::cout << "ALL PASS: p4_reader_memory_buffer_tests (31 tests)\n";
    return 0;
}
