# P2-013 Frame Queue / Memory Budget

Updated: 2026-06-25

## Goal

Implement bounded frame queues for decoded frames and memory budget enforcement.

## Frame Queue Contract

```cpp
struct FrameQueueConfig {
    ContractMetadata metadata;
    std::string queue_id;
    size_t max_frames{10};
    size_t max_bytes{100 * 1024 * 1024}; // 100MB
    bool drop_on_overflow{false};
};

struct FrameQueueStatus {
    ContractMetadata metadata;
    std::string queue_id;
    size_t current_frames{0};
    size_t current_bytes{0};
    bool is_full{false};
    bool is_empty{true};
};
```

## Memory Budget Contract

```cpp
struct MemoryBudgetConfig {
    ContractMetadata metadata;
    std::string budget_id;
    size_t total_budget_bytes{512 * 1024 * 1024}; // 512MB
    size_t video_budget_bytes{256 * 1024 * 1024}; // 256MB
    size_t audio_budget_bytes{64 * 1024 * 1024};  // 64MB
    size_t subtitle_budget_bytes{16 * 1024 * 1024}; // 16MB
};

struct MemoryBudgetStatus {
    ContractMetadata metadata;
    std::string budget_id;
    size_t total_used_bytes{0};
    size_t video_used_bytes{0};
    size_t audio_used_bytes{0};
    size_t subtitle_used_bytes{0};
    bool is_over_budget{false};
};
```

## Fake Services

- `FakeFrameQueueService` - simulates frame queue operations
- `FakeMemoryBudgetService` - simulates memory budget tracking

## Tests

1. Frame queue add/remove frames
2. Frame queue overflow detection
3. Memory budget allocation/deallocation
4. Memory budget overflow detection
5. Budget status accuracy