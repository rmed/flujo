#pragma once

#include <chrono>
#include <queue>

#include <kouta/base/callback.hpp>
#include <kouta/base/component.hpp>
#include <kouta/io/packer.hpp>
#include <kouta/io/timer.hpp>

#include "protocol/message.hpp"

namespace flujo::server
{
    namespace session_detail
    {
        /// @brief Callbacks the session will use to interact with the server.
        struct Connections
        {
            /// @brief Callback to invoke whenever the socket is closed.
            kouta::base::Callback<> connection_closed;
            /// @brief Callback to invoke whenever a message has been received.
            kouta::base::Callback<const protocol::Message&> message_received;
        };
    }  // namespace session_detail

    /// @brief Session manager.
    class Session : kouta::base::Component
    {
    public:
        using Connections = session_detail::Connections;

        // Not default constructible
        Session() = delete;

        Session(
            kouta::base::Component* parent,
            const std::string& id,
            boost::asio::local::stream_protocol::socket socket,
            const std::chrono::milliseconds& session_timeout,
            std::size_t buffer_size,
            const Connections& connections);

        // Not copyable
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;

        // Not movable
        Session(Session&&) = delete;
        Session& operator=(Session&&) = delete;

        ~Session() override = default;

        /// @brief Start the session handling flow.
        void start();

        /// @brief Stop the session flow and close the socket.
        void stop();

        /// @brief Send a response to the remote client.
        ///
        /// @details
        /// Internally, this method enqueues the JSON message to send and sends each of them sequentially in an
        /// asynchronous manner.
        ///
        /// @param[in] message          JSON message to send.
        void send_response(const std::string& message);

    private:
        /// @brief Check whether the client is allowed to perform
        bool check_credentials(bool admin_cmd);

        /// @brief Attempt to read message size.
        void do_read_size();

        /// @brief Attempt to read a full JSON message.
        void do_read_message();

        /// @brief Discard N bytes from the remote client.
        ///
        /// @details
        /// This is called when a client specifies a message size that is too big in order to *flush* the socket. The
        /// number of bytes to discard is specified in @ref m_next_message_size.
        ///
        /// @note This does not reset the session timeout, so there is no danger of the session getting stuck.
        void do_discard_incoming();

        /// @brief Send the size of the next response in the queue.
        ///
        /// @details
        /// Responses are enqueued via the @ref send_response() method. Note that this method will be called
        /// automatically by @ref send_response() if no response is currently being sent, and again after a response has
        /// been sent if there are messages remaining in the queue.
        void do_send_response_size();

        /// @brief Send an encoded JSON-RPC response message.
        ///
        /// @details
        /// This method is called after the completion of @ref do_send_response_size() and will remove the response from
        /// the queue.
        void do_send_response_message();

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

        /// @brief Handle reception of bytes to discard.
        ///
        /// @details
        /// A new discard operation is enqueued until there are no more bytes to read.
        ///
        /// @param[in] ec           Error code (if any).
        /// @param[in] length       Number of bytes read.
        void on_discarded_received(boost::system::error_code ec, std::size_t length);

        /// @brief Handle completion of a response size delivery.
        ///
        /// @details
        /// After sending this information, the message itself is sent to the client.
        ///
        /// @param[in] ec           Error code (if any).
        /// @param[in] length       Number of bytes written.
        void on_response_size_sent(boost::system::error_code ec, std::size_t length);

        /// @brief Handle completion of a response delivery.
        ///
        /// @details
        /// After the response has been sent, a new one is sent if there are any responses remaining in the queue.
        ///
        /// @param[in] ec           Error code (if any).
        /// @param[in] length       Number of bytes written.
        void on_response_sent(boost::system::error_code ec, std::size_t length);

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

        /// @brief Socket credentials.
        ucred m_credentials;

        /// @brief Callbacks used to interact with the server.
        Connections m_connections;

        /// @brief Timer used to detect a stale session..
        kouta::io::Timer m_session_timer;

        /// @brief Size of the next message to receive.
        std::size_t m_next_message_size;

        /// @brief Buffer used to receive JSON-RPC commands.
        ///
        /// @details
        /// Although the vector can grow, the session will not accept messages that are too big for the buffer.
        std::vector<std::uint8_t> m_read_buffer;

        /// @brief Whether a response is being sent to the connected client.
        bool m_sending_response;

        /// @brief Queue of response messages to send to the connected client.
        std::queue<std::string> m_response_queue;

        /// @brief Packer used to send JSON-RPC responses.
        ///
        /// @details
        /// As opposed to the read buffer, the buffer within the packer may grow as needed to accommodate for responses
        /// to be sent.
        kouta::io::Packer m_response_packer;
    };
}  // namespace flujo::server
