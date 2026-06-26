#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"
#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <variant>

namespace kivo::video::source_core {

class SourceOpenResult {
public:
    static SourceOpenResult success(SourceSession session) {
        SourceOpenResult r;
        r.data_ = std::move(session);
        return r;
    }

    static SourceOpenResult failure(SourceError error) {
        SourceOpenResult r;
        r.data_ = std::move(error);
        return r;
    }

    bool is_success() const { return std::holds_alternative<SourceSession>(data_); }
    bool is_failure() const { return std::holds_alternative<SourceError>(data_); }

    const SourceSession* session_if_success() const {
        return std::get_if<SourceSession>(&data_);
    }

    const SourceError* error_if_failure() const {
        return std::get_if<SourceError>(&data_);
    }

private:
    std::variant<SourceSession, SourceError> data_;
};

}  // namespace kivo::video::source_core
