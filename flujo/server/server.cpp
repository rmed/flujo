#include "server.hpp"

#include <sys/socket.h>

#include <iostream>

namespace flujo::server
{
    Server::Server()
        : kouta::base::Root{}
        , m_config_loader{}
        , m_endpoint{}
        , m_acceptor{context()}
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
                std::cout << "Failed to remove old socket at " << socket_path << std::endl;
                return false;
            }
        }

        m_endpoint = std::move(boost::asio::local::stream_protocol::endpoint{socket_path});

        boost::system::error_code err{};
        m_acceptor.open(m_endpoint.protocol(), err);

        if (err)
        {
            std::cout << "Failed to open acceptor: " << err.what() << std::endl;
            return false;
        }

        m_acceptor.bind(m_endpoint, err);

        if (err)
        {
            std::cout << "Failed to bind acceptor: " << err.what() << std::endl;
            return false;
        }

        m_acceptor.listen(m_config_loader.config().general.max_clients, err);

        if (err)
        {
            std::cout << "Failed to listen: " << err.what() << std::endl;
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
        if (ec)
        {
            std::cout << "Accept error: " << ec.what() << std::endl;
            return;
        }

        // Check number of sessions
        if (m_sessions.size() >= m_config_loader.config().general.max_clients)
        {
            // Cannot allocate new sessions
            std::cout << "Maximum number of sessions reached, discarding connection" << std::endl;
            socket.close();

            return do_accept();
        }

        // Retrieve credentials
        ucred credentials{};
        socklen_t bytes_read{sizeof(ucred)};
        int result = getsockopt(socket.native_handle(), SOL_SOCKET, SO_PEERCRED, &credentials, &bytes_read);

        std::cout << "RESULT " << result << std::endl;

        if (bytes_read != sizeof(ucred))
        {
            std::cout << "Read " << bytes_read << " instead of " << sizeof(ucred) << std::endl;
            socket.close();

            return do_accept();
        }

        // Establish session
        std::string id{compute_session_id()};

        auto [session, inserted] = m_sessions.emplace(
            id,
            std::make_unique<Session>(
                this,
                id,
                std::move(socket),
                m_config_loader.config().general.cmd_timeout,
                m_config_loader.config().general.session_timeout,
                m_config_loader.config().general.buffer_size,
                Session::Connections{.connection_closed{kouta::base::callback::DeferredCallback<>{
                    this, std::bind_front(&Server::on_session_closed, this, id)}}}));

        if (!inserted)
        {
            std::cout << "Failed to create session " << id << std::endl;
            socket.close();
        }

        std::cout << "PID " << credentials.pid << std::endl;
        std::cout << "UID " << credentials.uid << std::endl;
        std::cout << "GID " << credentials.gid << std::endl;

        do_accept();
    }

    void Server::on_session_closed(const std::string& id)
    {
        if (m_sessions.erase(id) == 0)
        {
            std::cout << "Could not find session " << id << " to clean" << std::endl;
        }
    }
}  // namespace flujo::server
