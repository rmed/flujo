#pragma once

#include <cstdint>
#include <vector>

namespace flujo::config::domains
{
    /// @brief Security configuration of the service.
    struct Security
    {
        /// @brief List of system UIDs that may perform certain actions.
        std::vector<std::uint32_t> uids;
        /// @brief List of system GIDs that may perform certain actions.
        std::vector<std::uint32_t> gids;
    };
}  // namespace flujo::config::domains
