# P2-020 Minimal UserPerceivedQualitySnapshot

Updated: 2026-06-25

## Goal

Implement UserPerceivedQualitySnapshot contract and fake service for testing.

## UserPerceivedQualitySnapshot

A snapshot of user-perceived quality metrics aggregating visual, audio, buffering, latency, and stability scores.

### Contract

```cpp
enum class UserSentiment {
    Excellent,
    Good,
    Fair,
    Poor,
    Terrible,
};

struct UserPerceivedQualitySnapshot {
    ContractMetadata metadata;
    std::string quality_snapshot_id;
    std::optional<std::string> session_id;
    double overall_score{0.0};  // 0-100
    double video_quality_score{0.0};
    double audio_quality_score{0.0};
    double buffering_score{0.0};
    double latency_score{0.0};
    double stability_score{0.0};
    UserSentiment user_sentiment{UserSentiment::Fair};
    std::vector<std::string> major_issues;
    std::vector<std::string> minor_issues;
    std::vector<std::string> recommendations;
};
```

### Fake User Perceived Quality Service

Provides deterministic quality snapshots for testing:

- `build_good_quality_snapshot()` - returns Good sentiment with moderate scores
- `build_excellent_quality_snapshot()` - returns Excellent sentiment with high scores
- `build_poor_quality_snapshot()` - returns Poor sentiment with low scores and major issues
- `build_terrible_quality_snapshot()` - returns Terrible sentiment with critical failures
- `build_custom_quality_snapshot()` - returns custom scores with auto-determined sentiment

## Tests

1. Good quality snapshot with session ID
2. Excellent quality snapshot without session ID
3. Poor quality snapshot with major issues
4. Terrible quality snapshot with critical failures
5. Custom quality snapshot with Good sentiment
6. Custom quality snapshot with Fair sentiment

## Evidence Target

Snapshot contract/service/test.