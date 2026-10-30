#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/revision/cancellation.hpp"
#include "kivo/playback/control_plane/reader/reader_read_result.hpp"
#include "kivo/playback/control_plane/control/seek_target.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::worker {

/// ReaderWorkerPostStatus enumerates reader worker task post outcomes.
enum class ReaderWorkerPostStatus
{
    Accepted,
    RejectedStopped,
    RejectedQueueFull,
    RejectedStaleStamp
};

/// ReaderOpenRequest is a reader worker open task request.
struct ReaderOpenRequest
{
    revision::RevisionStamp stamp{};
    revision::ReaderInstanceId instance_id{};
};

/// ReaderReadRequest is a reader worker read task request.
struct ReaderReadRequest
{
    revision::RevisionStamp stamp{};
    revision::ReaderInstanceId instance_id{};
    primitives::ReadDeadline deadline{};
};

/// ReaderSeekRequest is a reader worker seek task request.
struct ReaderSeekRequest
{
    revision::RevisionStamp stamp{};
    revision::ReaderInstanceId instance_id{};
    control::SeekTarget target{};
};

/// ReaderCloseRequest is a reader worker close task request.
struct ReaderCloseRequest
{
    revision::RevisionStamp stamp{};
    revision::ReaderInstanceId instance_id{};
};

/// ReaderWorker executes blocking Reader methods on a worker thread.
/// Rules:
/// - ReaderWorker must NOT modify SessionState.
/// - ReaderWorker only submits ReaderEvent.
/// - ReaderWorker callback entry uses RevisionValidator::validate_fast.
/// - ReaderWorker queue full must return RejectedQueueFull.
/// - ReaderWorker must NOT directly call EventBus subscriber callback.
class ReaderWorker
{
public:
    virtual ~ReaderWorker() = default;

    virtual ReaderWorkerPostStatus post_open(const ReaderOpenRequest& request) = 0;
    virtual ReaderWorkerPostStatus post_read(const ReaderReadRequest& request) = 0;
    virtual ReaderWorkerPostStatus post_seek(const ReaderSeekRequest& request) = 0;
    virtual ReaderWorkerPostStatus post_close(const ReaderCloseRequest& request) = 0;

    virtual void request_stop_accepting_new_tasks() = 0;
    virtual void cancel_current_task(revision::CancellationScope scope) = 0;
    virtual bool join(primitives::Milliseconds timeout) = 0;
};

} // namespace kivo::playback::control_plane::worker
