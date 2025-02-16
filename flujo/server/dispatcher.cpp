#include "dispatcher.hpp"

#include <spdlog/spdlog.h>

#include "protocol/error.hpp"

namespace flujo::server
{
    Dispatcher::Dispatcher(
        kouta::base::Component* parent, const config::AppConfig& config, const Connections& connections)
        : kouta::base::Component{parent}
        , m_config{config}
        , m_connections{connections}
        , m_parser{}
    {
    }

    bool Dispatcher::do_send_cmd(
        const std::optional<std::string>& user, const std::optional<std::string>& topic, const std::string& message)
    {
        bool result{user.has_value() || topic.has_value()};

        // Try to find user
        if (user.has_value())
        {
            const auto dst{m_config.users.users.find(user.value())};

            if (dst != m_config.users.users.cend())
            {
                // Send message via preferred method
                result &= send_to_user(dst->second, message);
            }
        }

        // Try to find topic
        if (topic.has_value())
        {
            const auto dst{m_config.users.topics.find(topic.value())};

            if (dst != m_config.users.topics.cend())
            {
                // Send message to all users in the topic, but ignore the original recipient (if any)
                std::string original_recipient{user.value_or("")};

                for (const auto& u : dst->second)
                {
                    if (u == original_recipient)
                    {
                        continue;
                    }

                    result &= send_to_user(m_config.users.users.at(u), message);
                }
            }
        }

        return result;
    }

    void Dispatcher::on_message_received(const std::string& session_id, const protocol::Message& message)
    {
        // Parse message
        jsonrpcpp::entity_ptr entity{nullptr};

        try
        {
            entity = m_parser.parse(message.json);
        }
        catch (const jsonrpcpp::RpcException& e)
        {
            // TODO
        }
        catch (...)
        {
            // TODO
        }

        if (entity == nullptr)
        {
            // Failed to parse for some reason
            // TODO
            return;
        }

        if (entity->is_batch())
        {
            jsonrpcpp::Batch responses{
                on_batch_received(*std::dynamic_pointer_cast<jsonrpcpp::Batch>(entity).get(), message)};

            if (!responses.entities.empty())
            {
                // Send responses
                m_connections.send_response(session_id, responses.to_json().dump());
            }
        }
        else if (entity->is_request())
        {
            jsonrpcpp::Response response{
                on_request_received(*std::dynamic_pointer_cast<jsonrpcpp::Request>(entity).get(), message)};

            // Send response
            m_connections.send_response(session_id, response.to_json().dump());
        }
        else if (entity->is_notification())
        {
            // Nothing to send as response
            on_notification_received(*std::dynamic_pointer_cast<jsonrpcpp::Notification>(entity).get(), message);
        }
    }

    jsonrpcpp::Response Dispatcher::on_request_received(
        const jsonrpcpp::Request& request, const protocol::Message& message)
    {
        // Check credentials
        if (!m_config.api_security.uids.contains(message.uid) && !m_config.api_security.gids.contains(message.gid))
        {
            // Cannot use the service
            return jsonrpcpp::Response{
                request,
                jsonrpcpp::Error{
                    protocol::error::to_string(protocol::error::Unauthorized).data(), protocol::error::Unauthorized}};
        }

        // Check method
        if (request.method() == "ping")
        {
            // Pong
            return jsonrpcpp::Response{request, Json{}};
        }
        else if (request.method() == "send")
        {
            // Send a message
            std::optional<std::string> user{std::nullopt};
            std::optional<std::string> topic{std::nullopt};
            std::string message{};

            Json parser{};

            if (request.params().is_array())
            {
                // User
                parser = request.params().get(0);
                if (!parser.is_null() && parser.is_string())
                {
                    user.emplace(parser.get<std::string>());
                }

                // Topic
                parser = request.params().get(1);
                if (!parser.is_null() && parser.is_string())
                {
                    topic.emplace(parser.get<std::string>());
                }

                // Message
                message = request.params().get<std::string>(2);
            }
            else
            {
                // User
                parser = request.params().get("user");
                if (!parser.is_null() && parser.is_string())
                {
                    user.emplace(parser.get<std::string>());
                }

                // Topic
                parser = request.params().get("topic");
                if (!parser.is_null() && parser.is_string())
                {
                    topic.emplace(parser.get<std::string>());
                }

                // Message
                message = request.params().get<std::string>("message");
            }

            if ((!user.has_value() && !topic.has_value()) || message.empty())
            {
                return jsonrpcpp::Response{
                    request,
                    jsonrpcpp::Error{
                        protocol::error::to_string(protocol::error::InvalidParams).data(),
                        protocol::error::InvalidParams}};
            }

            if (do_send_cmd(user, topic, message))
            {
                return jsonrpcpp::Response{request, true};
            }
            else
            {
                return jsonrpcpp::Response{
                    request,
                    jsonrpcpp::Error{
                        protocol::error::to_string(protocol::error::InternalError).data(),
                        protocol::error::InternalError}};
            }
        }

        // Method not found
        return jsonrpcpp::Response{
            request,
            jsonrpcpp::Error{
                protocol::error::to_string(protocol::error::MethodNotFound).data(), protocol::error::MethodNotFound}};
    }

    void Dispatcher::on_notification_received(
        const jsonrpcpp::Notification& notification, const protocol::Message& message)
    {
        // Discard response
        on_request_received(jsonrpcpp::Request{"dummyid", notification.method(), notification.params()}, message);
    }

    jsonrpcpp::Batch Dispatcher::on_batch_received(const jsonrpcpp::Batch& batch, const protocol::Message& message)
    {
        jsonrpcpp::Batch responses{};

        for (const auto& entity : batch.entities)
        {
            if (entity->is_request())
            {
                responses.add(
                    on_request_received(*std::dynamic_pointer_cast<jsonrpcpp::Request>(entity).get(), message));
            }
            else if (entity->is_notification())
            {
                on_notification_received(*std::dynamic_pointer_cast<jsonrpcpp::Notification>(entity).get(), message);
            }
        }

        return responses;
    }

    bool Dispatcher::send_to_user(const config::domains::Users::UserDetails& user, const std::string& message)
    {
        // Select preferred method
        switch (user.preferred)
        {
        case config::domains::Users::CommunicationMethod::Telegram:
            // Send via telegram
            spdlog::debug("Sending message to {} via telegram", user.id);
            break;
        default:
            spdlog::error("Cannot find preferred communication method for user {}", user.id);
            break;
        }

        return false;
    }
}  // namespace flujo::server
