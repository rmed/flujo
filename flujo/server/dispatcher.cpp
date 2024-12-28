#include "dispatcher.hpp"

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
        if (!m_config.api_security.uids.contains(message.uid) && !m_config.api_security.uids.contains(message.gid))
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
        else
        {
            // Method not found
            return jsonrpcpp::Response{
                request,
                jsonrpcpp::Error{
                    protocol::error::to_string(protocol::error::MethodNotFound).data(),
                    protocol::error::MethodNotFound}};
        }
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
}  // namespace flujo::server
