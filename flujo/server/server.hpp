#pragma once

#include <filesystem>

#include <kouta/base/root.hpp>

#include "config/loader.hpp"
#include "session.hpp"

namespace flujo::server
{
    /// @brief Flujo server.
    ///
    /// @details
    /// The server is intended to be executed as a service that accepts connections via a UNIX socket and runs the
    /// requested notification methods.
    class Server : public kouta::base::Root
    {
    public:
        // Default constructor
        Server();

        // Not copyable
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        // Movable
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        virtual ~Server() = default;

        /// @brief Configure the server.
        ///
        /// @details
        /// This overload will attempt to load the configuration from the default paths.
        ///
        /// @note This method must be called before @ref run().
        ///
        /// @return Whether setup was successful.
        bool setup();

        /// @brief Configure the server using the provided configuration.
        ///
        /// @details
        /// This overload will attempt to load the provided configuration and stop execution if there is an error with
        /// said configuration.
        ///
        /// @note This method must be called before @ref run().
        ///
        /// @param[in] config_path          Path to the configuration file.
        ///
        /// @return Whether the setup was successful.
        bool setup(const std::filesystem::path& config_path);

        /// @brief Start the server and the event loop.
        void run() override;

    private:
        /// @brief Setup the socket connection.
        ///
        /// @details
        /// The socket file specified in the configuration will be (re)created and the acceptor initialized.
        ///
        /// @return Whether the setup was successful.
        bool setup_socket();

        /// @brief Accept new incoming connections on the socket.
        void do_accept();

        /// @brief Compute a (unique) session ID comprised of the current timestamp (in ms) and the session serial.
        ///
        /// @details
        /// The session serial is increased every time it is used as a means to prevent duplicate IDs and is expected to
        /// overflow and return to zero when reaching its maximum value.
        ///
        /// @returns ID to assign to a session.
        std::string compute_session_id();

        // Handlers

        /// @brief Handle new connections.
        ///
        /// @details
        /// If the maximum number of sessions has not been reached, a new session will be activated and assigned a
        /// unique ID. This ID will be used by the other server components to send a response via the appropriate
        /// socket.
        ///
        /// @param[in] ec           Error code (if any).
        /// @param[in] socket       Opened socket object.
        void on_accepted(boost::system::error_code ec, boost::asio::local::stream_protocol::socket socket);

        /// @brief Handle closure of a session.
        ///
        /// @details
        /// The resources of the session will be released.
        ///
        /// @param[in] id           ID of the session that was closed.
        void on_session_closed(const std::string& id);

        config::Loader m_config_loader;
        boost::asio::local::stream_protocol::endpoint m_endpoint;
        boost::asio::local::stream_protocol::acceptor m_acceptor;

        std::uint8_t m_session_serial;
        std::map<std::string, std::unique_ptr<Session>> m_sessions;
    };
}  // namespace flujo::server
