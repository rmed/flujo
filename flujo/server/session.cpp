#include "session.hpp"

#include <iostream>

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
        const std::chrono::milliseconds& cmd_timeout,
        const std::chrono::milliseconds& session_timeout,
        std::size_t buffer_size,
        const Connections& connections)
        : kouta::base::Component{parent}
        , m_id{id}
        , m_socket{std::move(socket)}
        , m_connections{connections}
        , m_cmd_timer{this, cmd_timeout, std::bind_front(&Session::on_cmd_timer_expired, this)}
        , m_session_timer{this, session_timeout, std::bind_front(&Session::on_session_timer_expired, this)}
        , m_next_message_size{}
        , m_buffer(buffer_size)
    {
    }

    void Session::start()
    {
        m_session_timer.start();
        do_read_size();
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

    void Session::on_size_received(boost::system::error_code ec, std::size_t length)
    {
        if (ec)
        {
            std::cout << "Error reading message size: " << ec.what() << std::endl;
        }

        if (length != MSG_SIZE_LENGTH)
        {
            std::cout << "Could not read message size, read " << length << " bytes" << std::endl;
        }

        // Determine size and check whether it fits in the buffer
        kouta::io::Parser parser{std::span<const std::uint8_t>{m_buffer.begin(), m_buffer.begin() + MSG_SIZE_LENGTH}};

        m_next_message_size = parser.extract_integral<std::uint64_t>(0);

        if (m_next_message_size > m_buffer.size())
        {
            // Message is too big, notify client and discard message
            // TODO
        }

        // Read JSON
        return do_read_message();
    }

    void Session::on_message_received(boost::system::error_code ec, std::size_t length)
    {
        if (ec)
        {
            std::cout << "Error reading message: " << ec.what() << std::endl;
        }

        if (length != m_next_message_size)
        {
            std::cout << "Could not read message, read " << length << " bytes instead of " << m_next_message_size
                      << std::endl;
        }

        // Parse JSON message

        // JSON is valid, provide to the dispatcher

        // Restart session timer and command timer
        m_session_timer.start();

        m_cmd_timer.start();
    }

    void Session::on_cmd_timer_expired(kouta::io::Timer& timer)
    {
        // Notify client
    }

    void Session::on_session_timer_expired(kouta::io::Timer& timer)
    {
        // Stop processing events
        m_cmd_timer.stop();
        m_socket.close();

        // Notify server for cleanup
    }
}  // namespace flujo::server
