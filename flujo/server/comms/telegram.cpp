#include "telegram.hpp"

#include <spdlog/spdlog.h>

namespace flujo::server::comms
{
    Telegram::Telegram(kouta::base::Component* parent, const config::AppConfig& config)
        : kouta::base::Component{parent}
        , m_config{config}
        , m_worker{}
        , m_running{}
        , m_bot_error_received{kouta::base::callback::DeferredCallback<>{this, &Telegram::start}}
        , m_bot{nullptr}
        , m_long_poll(nullptr)
    {
    }

    Telegram::~Telegram()
    {
        stop();
    }

    void Telegram::start()
    {
        stop();

        spdlog::info("Starting Telegram bot");

        m_bot = std::make_unique<TgBot::Bot>(m_config.telegram.token);
        m_long_poll = std::make_unique<TgBot::TgLongPoll>(*m_bot.get(), 100, 10);

        // Configure /me command
        // TODO: Ignore older messages
        m_bot->getEvents().onCommand(
            "start",
            [&bot = m_bot](TgBot::Message::Ptr message)
            {
                spdlog::debug("User {} requested their ID", message->chat->id);
                bot->getApi().sendMessage(message->chat->id, std::format("Your ID: {}", message->chat->id));
            });

        m_bot->getEvents().onCommand(
            "me",
            [&bot = m_bot](TgBot::Message::Ptr message)
            {
                spdlog::debug("User {} requested their ID", message->chat->id);
                bot->getApi().sendMessage(message->chat->id, std::format("Your ID: {}", message->chat->id));
            });

        m_running = true;
        m_worker = std::thread{&Telegram::run_polling, this};
    }

    void Telegram::stop()
    {
        if (m_worker.joinable())
        {
            spdlog::info("Stopping Telegram bot");

            // Must stop it first
            m_running = false;
            m_worker.join();
        }
    }

    bool Telegram::send_message(const std::string& id, const std::string& message)
    {
        // Blind delivery, don't care if we succeed or not
        if (m_bot.get())
        {
            spdlog::info("Sending Telegram message to user {}", id);
            TgBot::Message::Ptr result{m_bot->getApi().sendMessage(id, message)};

            spdlog::debug("Telegram delivery to {}: {}", id, result->messageId != 0 ? "success" : "failure");
            return result->messageId != 0;
        }

        spdlog::warn("Telegram bot is not initialized");
        return false;
    }

    void Telegram::run_polling()
    {
        while (m_running)
        {
            try
            {
                m_long_poll->start();
            }
            catch (TgBot::TgException& e)
            {
                spdlog::error("Telegram bot exception: {}", e.what());
                spdlog::info("Attempting to restart Telegram bot");

                m_bot_error_received();
            }
        }
    }
}  // namespace flujo::server::comms
