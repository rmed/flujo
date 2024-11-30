#pragma once

#include "domains/general.hpp"
#include "domains/security.hpp"
#include "domains/telegram.hpp"

namespace flujo::config
{
    /// @brief Application configuration structure.
    struct AppConfig
    {
        domains::General general;
        domains::Security admin_security;
        domains::Security api_security;
        domains::Telegram telegram;
    };
}  // namespace flujo::config
