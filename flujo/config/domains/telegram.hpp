#pragma once

#include <string>

namespace flujo::config::domains
{
    /// @brief Telegram configuration of the service.
    struct Telegram
    {
        /// @brief Token to use in order to connect to the Telegram bot API.
        std::string token;
    };
}  // namespace flujo::config::domains
