#pragma once

#include <cstdint>
#include <string_view>

namespace flujo::server::protocol::error
{
    /// @brief JSON-RPC error codes.
    enum ErrorCode
    {
        // Custom codes
        /// Credentials are not valid
        Unauthorized = -403,

        // Protocol codes

        /// Parsing error.
        ParseError = -32700,
        /// Invalid request
        InvalidRequest = -32600,
        /// Method not found
        MethodNotFound = -32601,
        /// Invalid params
        InvalidParams = -32602,
        /// Internal error
        InternalError = -32603,
    };

    constexpr std::string_view to_string(ErrorCode code)
    {
        switch (code)
        {
        case ErrorCode::Unauthorized:
            return "Process is not authorized to use flujo";
        case ErrorCode::ParseError:
            return "Invalid JSON received";
        case ErrorCode::InvalidRequest:
            return "Invalid request received";
        case ErrorCode::MethodNotFound:
            return "Method does not exist or is not available";
        case ErrorCode::InvalidParams:
            return "Invalid parameters provided";
        case ErrorCode::InternalError:
            return "Internal JSON-RPC error";
        default:
            return "Unknown error";
        }
    }
}  // namespace flujo::server::protocol::error
