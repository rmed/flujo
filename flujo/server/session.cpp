#include "session.hpp"

#include <iostream>

#include <spdlog/spdlog.h>

#include <kouta/io/parser.hpp>

namespace flujo::server
{
    namespace
    {
        /// @brief Size, in bytes, of the message size payload sent before a message.
        constexpr std::size_t MSG_SIZE_LENGTH{sizeof(std::uint64_t)};
    }  // namespace

    Session::Session(
        kouta::base::Component* parent,
        const std::string& id,
        boost::asio::local::stream_protocol::socket socket,
        const std::chrono::milliseconds& session_timeout,
        std::size_t buffer_size,
        const Connections& connections)
        : kouta::base::Component{parent}
        , m_id{id}
        , m_socket{std::move(socket)}
        , m_credentials{}
        , m_connections{connections}
        , m_session_timer{this, session_timeout, std::bind_front(&Session::on_session_timer_expired, this)}
        , m_next_message_size{}
        , m_buffer(buffer_size)
    {
        // TODO set custom logger with session id
    }

    void Session::start()
    {
        // Retrieve credentials
        socklen_t bytes_read{sizeof(ucred)};

        int result = getsockopt(m_socket.native_handle(), SOL_SOCKET, SO_PEERCRED, &m_credentials, &bytes_read);

        if (bytes_read != sizeof(ucred))
        {
            spdlog::error(
                "{}: Failed to read socket credentials, read {} instead of {} bytes", m_id, bytes_read, sizeof(ucred));
            return stop();
        }

        spdlog::info("{}: Started session for PID {}", m_id, m_credentials.pid);

        m_session_timer.start();
        do_read_size();
    }

    void Session::stop()
    {
        // Stop processing events
        m_session_timer.stop();

        if (m_socket.is_open())
        {
            m_socket.close();
        }
    }

    void Session::send_response(const std::string& message)
    {
        // Enqueue message

        // Send message if not sending anything
    }

    void Session::do_read_size()
    {
        m_next_message_size = 0;

        boost::asio::async_read(
            m_socket,
            boost::asio::buffer(m_buffer, MSG_SIZE_LENGTH),
            std::bind_front(&Session::on_size_received, this));
    }

    void Session::do_read_message()
    {
        boost::asio::async_read(
            m_socket,
            boost::asio::buffer(m_buffer, m_next_message_size),
            std::bind_front(&Session::on_message_received, this));
    }

    void Session::do_discard_incoming()
    {
        // Compute bytes to read based on what remains to be discarded
        std::size_t to_read{m_next_message_size};

        if (m_next_message_size > m_buffer.size())
        {
            to_read = m_buffer.size();
        }

        boost::asio::async_read(
            m_socket, boost::asio::buffer(m_buffer, to_read), std::bind_front(&Session::on_discarded_received, this));
    }

    void Session::on_size_received(boost::system::error_code ec, std::size_t length)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // We closed the socket
            return m_connections.connection_closed();
        }

        if (ec)
        {
            spdlog::error("{}: Error reading message: {}", m_id, ec.what());

            // Special handling
            if (ec == boost::asio::error::eof)
            {
                // Closed by the remote client.
                return m_connections.connection_closed();
            }
        }

        if (length != MSG_SIZE_LENGTH)
        {
            spdlog::warn("{}: Could not read full message size, read {} bytes", m_id, length);
            return do_read_size();
        }

        // Determine size and check whether it fits in the buffer
        kouta::io::Parser parser{std::span<const std::uint8_t>{m_buffer.begin(), m_buffer.begin() + MSG_SIZE_LENGTH}};

        m_next_message_size = parser.extract_integral<std::uint64_t>(0);

        if (m_next_message_size > m_buffer.size())
        {
            // Message is too big, notify client and discard message
            // TODO
            spdlog::warn(
                "{}: Client is attempting to send {} bytes, which do not fit in the buffer ({} bytes)",
                m_id,
                m_next_message_size,
                m_buffer.size());

            return do_discard_incoming();
        }

        // Read JSON
        do_read_message();
    }

    void Session::on_message_received(boost::system::error_code ec, std::size_t length)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // We closed the socket
            return m_connections.connection_closed();
        }

        if (ec)
        {
            spdlog::error("{}: Error reading message: {}", m_id, ec.what());

            // Special handling
            if (ec == boost::asio::error::eof)
            {
                // Closed by the remote client.
                return m_connections.connection_closed();
            }
        }

        if (length != m_next_message_size)
        {
            spdlog::warn(
                "{}: Could not read full message, read {} bytes instead of {}", m_id, length, m_next_message_size);
            return do_read_message();
        }

        // Provide message to the dispatcher
        kouta::io::Parser parser{std::span<const std::uint8_t>{m_buffer}};

        m_connections.message_received(
            {m_credentials.pid, m_credentials.uid, m_credentials.gid, parser.extract_string(0, m_next_message_size)});

        // Restart session timer
        m_session_timer.start();

        // Receive new messages
        do_read_size();
    }

    void Session::on_discarded_received(boost::system::error_code ec, std::size_t length)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // We closed the socket
            return m_connections.connection_closed();
        }

        if (ec)
        {
            spdlog::error("{}: Error reading discarded bytes: {}", m_id, ec.what());

            // Special handling
            if (ec == boost::asio::error::eof)
            {
                // Closed by the remote client.
                return m_connections.connection_closed();
            }
        }

        m_next_message_size -= length;

        if (m_next_message_size != 0)
        {
            // Still has bytes to discard
            return do_discard_incoming();
        }

        // Continue normal operation
        do_read_size();
    }

    void Session::on_session_timer_expired(kouta::io::Timer& timer)
    {
        stop();
    }
}  // namespace flujo::server
