#include "kivo/cinema_engine/engine_api/cancellation_token.hpp"

#include <utility>

namespace kivo::cinema_engine {

CancellationToken::CancellationToken() : state_(std::make_shared<bool>(false)) {}

CancellationToken::CancellationToken(std::shared_ptr<bool> state) : state_(std::move(state)) {}

bool CancellationToken::is_cancellation_requested() const {
  return state_ != nullptr && *state_;
}

CancellationSource::CancellationSource() : state_(std::make_shared<bool>(false)) {}

CancellationToken CancellationSource::token() const {
  return CancellationToken(state_);
}

void CancellationSource::request_cancellation() {
  *state_ = true;
}

bool CancellationSource::is_cancellation_requested() const {
  return state_ != nullptr && *state_;
}

}  // namespace kivo::cinema_engine
