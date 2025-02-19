#pragma once

#include <thread>

#include <kouta/base.hpp>
#include <tgbot/tgbot.h>

#include "config/app-config.hpp"

namespace flujo::server::comms
{
    /// @brief Telegram communications gateway.
    class Telegram : public kouta::base::Component
    {
    public:
        // Not default constructible
        Telegram() = delete;

        /// @brief Constructor.
        ///
        /// @param[in] parent       Parent component.
        /// @param[in] config           Parsed application configuration.
        Telegram(kouta::base::Component* parent, const config::AppConfig& config);

        // Not copyable
        Telegram(const Telegram&) = delete;
        Telegram& operator=(const Telegram&) = delete;

        // Not movable
        Telegram(Telegram&&) = delete;
        Telegram& operator=(Telegram&&) = delete;

        /// @brief Destructor.
        virtual ~Telegram();

        /// @brief Re(start) the bot.
        ///
        /// @details
        /// This will stop a previous execution of the bot and launch its long polling loop again.
        void start();

        /// @brief Stop the bot.
        void stop();

        /// @brief Send a message to the specified (chat) ID.
        ///
        /// @param[in] id           Telegram (chat) ID to send the message to.
        /// @param[in] message      Message to send.
        ///
        /// @return Whether the message could be delivered.
        bool send_message(const std::string& id, const std::string& message);

    private:
        /// @brief Run the long polling of the Telegram bot.
        void run_polling();

        /// @brief Application configuration.
        const config::AppConfig& m_config;

        /// @brief Thread running the long polling of the bot.
        std::thread m_worker;

        /// @brief Whether the bot should be running or not.
        std::atomic_bool m_running;

        /// @brief Callback used to restart the bot.
        kouta::base::Callback<> m_bot_error_received;

        /// @brief Telegram bot.
        std::unique_ptr<TgBot::Bot> m_bot;

        /// @brief Long polling.
        std::unique_ptr<TgBot::TgLongPoll> m_long_poll;
    };
}  // namespace flujo::server::comms
