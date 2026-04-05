#pragma once

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
