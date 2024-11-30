#pragma once

#include <filesystem>
#include <string>

#include <toml++/toml.hpp>

#include "compile-detail.h"

#include "app-config.hpp"

namespace flujo::config
{
    /// @brief Environment variable to check for the configuration file path.
    constexpr std::string_view CONFIG_ENV_VARIABLE{"FLUJO_CONFIG"};

    /// @brief Default path to the configuration file.
    constexpr std::string_view DEFAULT_CONFIG_PATH{FLUJO_DEFAULT_CONFIG};

    /// @brief Application configuration loader.
    ///
    /// @details
    /// This class is in charge of loading/parsing the configuration at startup and providing the configuration
    /// structure to the rest of the application as needed.
    ///
    ///
    class Loader
    {
    public:
        // Default constructor
        Loader() = default;

        // Copyable
        Loader(const Loader&) = default;
        Loader& operator=(const Loader&) = default;

        // Movable
        Loader(Loader&&) = default;
        Loader& operator=(Loader&&) = default;

        virtual ~Loader() = default;

        /// @brief Obtain a reference to the parsed configuration.
        ///
        /// @note This should only by called after successful configuration loading via @ref load().
        const AppConfig& config() const;

        /// @brief Load application configuration.
        ///
        /// @details
        /// This overload will attempt to load the configuration from the following sources, in order:
        ///
        /// 1. `FLUJO_CONFIG` environment variable
        /// 2. File "flujo.toml" in current working directory
        /// 3. Path specified in @ref DEFAULT_CONFIG_PATH
        ///
        /// If none of these are found, the loader considers there is no configuration to load.
        ///
        /// The actual loading/parsing of the configuration is performed by the other overload.
        ///
        /// @returns Whether the configuration file could be found and loaded.
        bool load();

        /// @brief Load application configuration.
        ///
        /// @details
        /// This overload will attempt to load the configuration specified in path @p config_path and fill the internal
        /// application configuration structure.
        ///
        /// @returns Whether the configuration file could be found and loaded.
        bool load(const std::filesystem::path& config_path);

    private:
        /// @brief Parse the general section of the configuration.
        ///
        /// @param[in] table            Parsed TOML table.
        ///
        /// @returns Whether the section was correctly parsed.
        bool parse_general(const toml::table& table);

        /// @brief Parse the admin security section of the configuration.
        ///
        /// @param[in] table            Parsed TOML table.
        ///
        /// @returns Whether the section was correctly parsed.
        bool parse_security_admin(const toml::table& table);

        /// @brief Parse the API security section of the configuration.
        ///
        /// @param[in] table            Parsed TOML table.
        ///
        /// @returns Whether the section was correctly parsed.
        bool parse_security_api(const toml::table& table);

        /// @brief Parse the Telegram section of the configuration.
        ///
        /// @param[in] table            Parsed TOML table.
        ///
        /// @returns Whether the section was correctly parsed.
        bool parse_telegram(const toml::table& table);

        AppConfig m_config;
    };
}  // namespace flujo::config
