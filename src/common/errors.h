#pragma once

#include <string_view>

enum class GatewayError
{
    REQUEST_ERROR,
    TIMEOUT
};

enum class DecodingError
{
    PAYLOAD_PARSING_ERROR,
    INVALID_PAYLOAD,
    UNEXPECTED_VALUE
};

enum class SequencingError
{
    NOT_INITIALIZED,
    GAP_DETECTED
};

enum class RecoveringError
{
    GATEWAY_START_ERROR,
    SNAPSHOT_REQUEST_ERROR,
    SNAPSHOT_PARSING_ERROR,
    UPDATE_APPLY_ERROR
};

enum class RuntimeError
{
    INITIALIZATION_ERROR,
    RECOVERY_ERROR
};

inline std::string_view error_to_string(GatewayError error) noexcept
{
    switch (error) {
        case GatewayError::REQUEST_ERROR: return "REQUEST_ERROR";
        case GatewayError::TIMEOUT: return "TIMEOUT";
    }

    return "UNKNOWN_GATEWAY_ERROR";
}

inline std::string_view error_to_string(DecodingError error) noexcept
{
    switch (error) {
        case DecodingError::PAYLOAD_PARSING_ERROR: return "PAYLOAD_PARSING_ERROR";
        case DecodingError::INVALID_PAYLOAD: return "INVALID_PAYLOAD";
        case DecodingError::UNEXPECTED_VALUE: return "UNEXPECTED_VALUE";
    }

    return "UNKNOWN_DECODING_ERROR";
}

inline std::string_view error_to_string(SequencingError error) noexcept
{
    switch (error) {
        case SequencingError::NOT_INITIALIZED: return "NOT_INITIALIZED";
        case SequencingError::GAP_DETECTED: return "GAP_DETECTED";
    }

    return "UNKNOWN_SEQUENCING_ERROR";
}

inline std::string_view error_to_string(RecoveringError error) noexcept
{
    switch (error) {
        case RecoveringError::GATEWAY_START_ERROR: return "GATEWAY_START_ERROR";
        case RecoveringError::SNAPSHOT_REQUEST_ERROR: return "SNAPSHOT_REQUEST_ERROR";
        case RecoveringError::SNAPSHOT_PARSING_ERROR: return "SNAPSHOT_PARSING_ERROR";
        case RecoveringError::UPDATE_APPLY_ERROR: return "UPDATE_APPLY_ERROR";
    }

    return "UNKNOWN_RECOVERING_ERROR";
}

inline std::string_view error_to_string(RuntimeError error) noexcept
{
    switch (error) {
        case RuntimeError::INITIALIZATION_ERROR: return "INITIALIZATION_ERROR";
        case RuntimeError::RECOVERY_ERROR: return "RECOVERY_ERROR";
    }

    return "UNKNOWN_RUNTIME_ERROR";
}
