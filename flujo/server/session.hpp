#pragma once

#include <chrono>

#include <kouta/base/callback.hpp>
#include <kouta/base/component.hpp>
#include <kouta/io/timer.hpp>

#include <jsonrpcpp.hpp>

namespace flujo::server
{
    namespace session_detail
    {
        /// @brief Callbacks the session will use to interact with the server.
        struct Connections
        {
            /// @brief Callback to invoke whenever the socket is closed.
            kouta::base::Callback<> connection_closed;
        };
    }  // namespace session_detail

    /// @brief Session manager.
    class Session : kouta::base::Component
    {
    public:
        using Connections = session_detail::Connections;

        // Default constructor
        Session() = delete;

        Session(
            kouta::base::Component* parent,
            const std::string& id,
            boost::asio::local::stream_protocol::socket socket,
            const std::chrono::milliseconds& cmd_timeout,
            const std::chrono::milliseconds& session_timeout,
            std::size_t buffer_size,
            const Connections& connections);

        // Copyable
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;

        // Movable
        Session(Session&&) = delete;
        Session& operator=(Session&&) = delete;

        ~Session() override = default;

        /// @brief Start the session handling flow.
        void start();

    private:
        /// @brief Check whether the client is allowed to perform
        bool check_credentials(bool admin_cmd);

        /// @brief Attempt to read message size.
        void do_read_size();

        /// @brief Attempt to read a full JSON message.
        void do_read_message();

        // Handlers

        /// @brief Handle reception of a message size.
        ///
        /// @details
        /// After receiving this information, the message itself is read into the buffer (if the buffer is large
        /// enough).
        ///
        /// @param[in] ec           Error code (if any).
        /// @param[in] length       Number of bytes read.
        void on_size_received(boost::system::error_code ec, std::size_t length);

        /// @brief Handle reception of a JSON message.
        ///
        /// @details
        /// The session forwards the request to the server, which will determine whether the client is allowed to
        /// perform the action requested based onthe credentials provided.
        ///
        /// @param[in] ec           Error code (if any).
        /// @param[in] length       Number of bytes read.
        void on_message_received(boost::system::error_code ec, std::size_t length);

        /// @brief Handle expiration of the command timer.
        ///
        /// @details
        /// If a timeout occurs in a command, the client will be notified.
        ///
        /// @param[in,out] timer            Timer that expired.
        void on_cmd_timer_expired(kouta::io::Timer& timer);

        /// @brief Handle expiration of the session timer.
        ///
        /// @details
        /// If the session is stale it will be closed and the server notified for cleanup.
        ///
        /// @param[in,out] timer            Timer that expired.
        void on_session_timer_expired(kouta::io::Timer& timer);

        /// @brief Identifier for this session, used in callbacks.
        std::string m_id;

        /// @brief Socket used to communicate with the remote client.
        boost::asio::local::stream_protocol::socket m_socket;

        /// @brief Callbacks used to interact with the server.
        Connections m_connections;

        /// @brief Timer used to detect a timeout in the handling of a command.
        kouta::io::Timer m_cmd_timer;

        /// @brief Timer used to detect a stale session..
        kouta::io::Timer m_session_timer;

        /// @brief Size of the next message to receive.
        std::size_t m_next_message_size;

        /// @brief Buffer used to receive JSON commands.
        ///
        /// @details
        /// Although the vector can grow, the session will not accept messages that are too big for the buffer.
        std::vector<std::uint8_t> m_buffer;
    };
}  // namespace flujo::server
