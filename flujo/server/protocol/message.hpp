#pragma once

#include <sys/socket.h>

#include <string>

namespace flujo::server::protocol
{
    /// @brief Message received from a session.
    struct Message
    {
        /// @brief PID of the remote client (can be used for permission checking).
        pid_t pid;
        /// @brief UID of the remote client (can be used for permission checking).
        uid_t uid;
        /// @brief GID of the remote client (can be used for permission checking).
        gid_t gid;
        /// @brief JSON message received from the client (prior to validation/parsing).
        std::string json;
    };
}  // namespace flujo::server::protocol
