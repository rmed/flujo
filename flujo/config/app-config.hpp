#pragma once

#include "domains/general.hpp"
#include "domains/security.hpp"
#include "domains/telegram.hpp"
#include "domains/users.hpp"

namespace flujo::config
{
    /// @brief Application configuration structure.
    struct AppConfig
    {
        domains::General general;
        domains::Security admin_security;
        domains::Security api_security;
        domains::Telegram telegram;
        domains::Users users;
    };
}  // namespace flujo::config
