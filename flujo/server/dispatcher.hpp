#pragma once

#include <jsonrpcpp.hpp>

#include <kouta/base/callback.hpp>
#include <kouta/base/component.hpp>

#include "comms/telegram.hpp"
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

        /// @brief Start communications services.
        ///
        /// @details
        /// This method should be called after the configuration has been correctly parsed.
        void start_services();

        /// @brief Handle the delivery of a message.
        ///
        /// @details
        /// This will use the appropriate communication method to attempt to deliver the message.
        ///
        /// @note Even if false is returned, some messages may have been delivered.
        ///
        /// @param[in] user         User to send the message to.
        /// @param[in] topic        Topic to send the message to.
        /// @param[in] message      Message to send.
        ///
        /// @return
        /// Whether the message could be delivered.
        bool do_send_cmd(
            const std::optional<std::string>& user,
            const std::optional<std::string>& topic,
            const std::string& message);

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
        /// @brief Process a `send` command.
        ///
        /// @details
        /// This command expects
        jsonrpcpp::Response on_send_cmd_received(const jsonrpcpp::Request& request);

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

        /// @brief Send a message to a user.
        ///
        /// @param[in] user             Details of the user to send the message to.
        /// @param[in] message          Message to send.
        ///
        /// @return Whether the message could be delivered.
        bool send_to_user(const config::domains::Users::UserDetails& user, const std::string& message);

        /// @brief Application configuration.
        const config::AppConfig& m_config;

        /// @brief Callbacks used to interact with the server.
        Connections m_connections;

        /// @brief JSON-RPC parser
        jsonrpcpp::Parser m_parser;

        /// @brief Telegram gateway.
        comms::Telegram m_telegram;
    };
}  // namespace flujo::server
