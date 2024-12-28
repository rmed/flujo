#pragma once

#include <jsonrpcpp.hpp>

#include <kouta/base/callback.hpp>
#include <kouta/base/component.hpp>

#include "config/app-config.hpp"
#include "protocol/message.hpp"

namespace flujo::server
{
    namespace dispatcher_detail
    {
        /// @brief Callbacks the dispatcher will use to interact with the server.
        struct Connections
        {
            /// @brief Callback to invoke whenever a message needs to be sent to a remote client.
            kouta::base::Callback<const std::string&, const std::string&> send_response;
        };
    }  // namespace dispatcher_detail

    class Dispatcher : public kouta::base::Component
    {
    public:
        using Connections = dispatcher_detail::Connections;

        // Not default constructible
        Dispatcher() = delete;

        /// @brief Constructor.
        ///
        /// @param[in] parent           Parent component.
        /// @param[in] config           Parsed application configuration.
        /// @param[in] connections      Callbacks used to interact with the server.
        Dispatcher(kouta::base::Component* parent, const config::AppConfig& config, const Connections& connections);

        // Not copyable
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(const Dispatcher&) = delete;

        // Not movable
        Dispatcher(Dispatcher&&) = delete;
        Dispatcher& operator=(Dispatcher&&) = delete;

        ~Dispatcher() override = default;

        /// @brief Handle reception of a JSON-RPC message.
        ///
        /// @details
        /// This method attempts to parse the JSON message and dispatches it to the appropriate endpoint(s) before
        /// sending a response back to the remote client.
        ///
        /// @param[in] session_id               ID of the session that sent the message (used for replies).
        /// @param[in] message                  Received message.
        void on_message_received(const std::string& session_id, const protocol::Message& message);

    private:
        /// @brief Process a JSON-RPC request.
        ///
        /// @param[in] request          Request to process.
        /// @param[in] message          Original message structure, used to check credentials.
        ///
        /// @return Response to send (if it applies).
        jsonrpcpp::Response on_request_received(const jsonrpcpp::Request& request, const protocol::Message& message);

        /// @brief Process a JSON-RPC notification.
        ///
        /// @param[in] notification     Notification to process.
        /// @param[in] message          Original message structure, used to check credentials.
        void on_notification_received(const jsonrpcpp::Notification& notification, const protocol::Message& message);

        /// @brief Process a JSON-RPC batch of messages.
        ///
        /// If there are requests in the batch, responses will be added to the resulting batch.
        ///
        /// @param[in] batch            Batch to process.
        /// @param[in] message          Original message structure, used to check credentials.
        ///
        /// @return Batch of responses (if it applies).
        jsonrpcpp::Batch on_batch_received(const jsonrpcpp::Batch& batch, const protocol::Message& message);

        /// @brief Application configuration.
        const config::AppConfig& m_config;

        /// @brief Callbacks used to interact with the server.
        Connections m_connections;

        /// @brief JSON-RPC parser
        jsonrpcpp::Parser m_parser;
    };
}  // namespace flujo::server
