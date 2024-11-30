#pragma once

#include <cstdint>
#include <string>

namespace flujo::config::domains
{
    /// @brief General configuration of the service.
    struct General
    {
        /// @brief Absolute path to the SQLite database file.
        std::string db_path;
        /// @brief Maximum number of clients to manage.
        std::size_t max_clients;
    };
}  // namespace flujo::config::domains
