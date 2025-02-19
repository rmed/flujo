#include "server.hpp"

#include <sys/socket.h>
#include <iostream>

#include <spdlog/spdlog.h>

namespace flujo::server
{
    Server::Server()
        : kouta::base::Root{}
        , m_config_loader{}
        , m_endpoint{}
        , m_acceptor{context()}  // clang-format off
        , m_dispatcher{this, m_config_loader.config(), {
            kouta::base::callback::DeferredCallback{this, &Server::on_send_response}
        }}
        // clang-format on
        , m_sessions{}
    {
    }

    bool Server::setup()
    {
        if (!m_config_loader.load())
        {
            // Cannot continue without configuration
            return false;
        }

        // Initialize socket
        if (!setup_socket())
        {
            return false;
        }

        // Initialize services
        m_dispatcher.start_services();

        return true;
    }

    bool Server::setup(const std::filesystem::path& config_path)
    {
        if (!m_config_loader.load(config_path))
        {
            // Cannot continue without configuration
            return false;
        }

        // Initialize socket
        if (!setup_socket())
        {
            return false;
        }

        m_dispatcher.start_services();

        return true;
    }

    void Server::run()
    {
        post(&Server::do_accept);

        // Call original run() to start the event loop
        kouta::base::Root::run();
    }

    bool Server::setup_socket()
    {
        std::filesystem::path socket_path{m_config_loader.config().general.socket_path};

        // Remove and re-create socket if needed
        if (std::filesystem::exists(socket_path))
        {
            std::error_code err{};

            if (!std::filesystem::remove(socket_path, err))
            {
                spdlog::error("Failed to remove old socket at {}", socket_path.c_str());
                return false;
            }
        }

        m_endpoint = std::move(boost::asio::local::stream_protocol::endpoint{socket_path});

        boost::system::error_code err{};
        m_acceptor.open(m_endpoint.protocol(), err);

        if (err)
        {
            spdlog::error("Failed to open acceptor: {}", err.what());
            return false;
        }

        m_acceptor.bind(m_endpoint, err);

        if (err)
        {
            spdlog::error("Failed to bind acceptor: {}", err.what());
            return false;
        }

        m_acceptor.listen(m_config_loader.config().general.max_clients, err);

        if (err)
        {
            spdlog::error("Failed to listen: {}", err.what());
            return false;
        }

        return true;
    }

    void Server::do_accept()
    {
        m_acceptor.async_accept(std::bind_front(&Server::on_accepted, this));
    }

    std::string Server::compute_session_id()
    {
        const auto ms{
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count()};

        return std::to_string(ms) + std::to_string(m_session_serial++);
    }

    void Server::on_accepted(boost::system::error_code ec, boost::asio::local::stream_protocol::socket socket)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // Closing down
            return;
        }

        if (ec)
        {
            spdlog::error("Accept error: {}", ec.what());
            return do_accept();
        }

        // Check number of sessions
        if (m_sessions.size() >= m_config_loader.config().general.max_clients)
        {
            // Cannot allocate new sessions
            spdlog::warn("Maximum number of sessions reached, discarding connection");
            socket.close();

            return do_accept();
        }

        // Establish session
        std::string id{compute_session_id()};

        spdlog::debug("Creating session {}", id);

        auto [session, inserted] = m_sessions.emplace(
            id,
            std::make_unique<Session>(
                this,
                id,
                std::move(socket),
                m_config_loader.config().general.session_timeout,
                m_config_loader.config().general.buffer_size,
                Session::Connections{
                    .connection_closed{kouta::base::callback::DeferredCallback<>{
                        this, std::bind_front(&Server::on_session_closed, this, id)}},
                    .message_received{kouta::base::callback::DeferredCallback<const protocol::Message&>{
                        &m_dispatcher, std::bind_front(&Dispatcher::on_message_received, &m_dispatcher, id)}}}));

        if (!inserted)
        {
            spdlog::error("Failed to create session {}", id);
            socket.close();

            return do_accept();
        }

        // Start session
        session->second->start();

        do_accept();
    }

    void Server::on_session_closed(const std::string& id)
    {
        spdlog::debug("Deleting session {}", id);

        if (m_sessions.erase(id) == 0)
        {
            spdlog::warn("Could not find session {} to clean", id);
        }
    }

    void Server::on_send_response(const std::string& session_id, const std::string& message)
    {
        auto session{m_sessions.find(session_id)};

        if (session == m_sessions.end())
        {
            spdlog::warn("Could not find session \"{}\" to send a response through", session_id);
            return;
        }

        session->second->send_response(message);
    }
}  // namespace flujo::server
