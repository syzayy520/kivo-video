#pragma once

#include <memory>

namespace kivo::cinema_engine {

class CancellationToken {
 public:
  CancellationToken();
  bool is_cancellation_requested() const;

 private:
  friend class CancellationSource;
  explicit CancellationToken(std::shared_ptr<bool> state);

  std::shared_ptr<bool> state_;
};

class CancellationSource {
 public:
  CancellationSource();
  CancellationToken token() const;
  void request_cancellation();
  bool is_cancellation_requested() const;

 private:
  std::shared_ptr<bool> state_;
};

}  // namespace kivo::cinema_engine

