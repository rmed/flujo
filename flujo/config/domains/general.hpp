#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace flujo::config::domains
{
    /// @brief General configuration of the service.
    struct General
    {
        /// @brief Absolute path to the UNIX socket to listen at.
        std::string socket_path;
        /// @brief Absolute path to the SQLite database file.
        std::string db_path;
        /// @brief Maximum number of clients to manage.
        std::size_t max_clients;
        /// @brief Buffer size for sessions (in bytes).
        std::size_t buffer_size;
        /// @brief Period after which a command will be considered to have timed-out (in ms).
        std::chrono::milliseconds cmd_timeout;
        /// @brief Period after which a session will be considered to be stale.
        std::chrono::milliseconds session_timeout;
    };
}  // namespace flujo::config::domains
