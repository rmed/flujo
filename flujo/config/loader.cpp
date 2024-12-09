#include "loader.hpp"

#include <type_traits>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

namespace flujo::config
{
    namespace
    {
        /// @brief Extract a (basic) value from a node view and store it in a member variable.
        ///
        /// @tparam TTomlType           Type of the TOML node view.
        /// @tparam TMemberType         Expected destination type.
        ///
        /// @param[in] src              Source node to extract the value from.
        /// @param[out] dst             Member variable to store the value in.
        template<class TTomlType, class TMemberType>
        bool extract_value(const toml::node_view<TTomlType>& src, TMemberType& dst)
        {
            bool result{true};

            src.visit(
                [&result, &dst](auto&& el) noexcept
                {
                    if constexpr (toml::is_number<decltype(el)> && std::is_integral_v<TMemberType>)
                    {
                        dst = static_cast<TMemberType>(el.get());
                    }
                    else if constexpr (toml::is_string<decltype(el)> && std::is_same_v<std::string, TMemberType>)
                    {
                        dst = el.get();
                    }
                    else if constexpr (toml::is_boolean<decltype(el)> && std::is_same_v<bool, TMemberType>)
                    {
                        dst = el.get();
                    }
                    else
                    {
                        result = false;
                    }
                });

            return result;
        }
    }  // namespace

    const AppConfig& Loader::config() const
    {
        return m_config;
    }

    bool Loader::load()
    {
        std::filesystem::path config_path{};

        // Try environment variable
        char* env{std::getenv(CONFIG_ENV_VARIABLE.data())};

        if (env != nullptr)
        {
            config_path = env;
        }

        if (!std::filesystem::is_regular_file(config_path))
        {
            // Try cwd
            std::error_code err{};
            config_path = std::filesystem::current_path(err);

            if (err.value())
            {
                config_path.clear();
            }
        }

        if (!std::filesystem::is_regular_file(config_path))
        {
            // Try default configuration path
            config_path = DEFAULT_CONFIG_PATH;
        }

        return load(config_path);
    }

    bool Loader::load(const std::filesystem::path& config_path)
    {
        if (!std::filesystem::is_regular_file(config_path))
        {
            return false;
        }

        toml::parse_result toml_result{toml::parse_file(config_path.c_str())};

        if (!toml_result)
        {
            return false;
        }

        bool result{true};

        result &= parse_general(toml_result.table());
        result &= parse_security_admin(toml_result.table());
        result &= parse_security_api(toml_result.table());
        result &= parse_telegram(toml_result.table());

        return result;
    }

    bool Loader::parse_general(const toml::table& table)
    {
        auto section{table["general"]};

        if (!section)
        {
            return false;
        }

        bool result{true};

        result &= extract_value(section["db"], m_config.general.db_path);
        result &= extract_value(section["max_clients"], m_config.general.max_clients);

        return result;
    }

    bool Loader::parse_security_admin(const toml::table& table)
    {
        auto section{table[toml::path{"security.admin"}]};

        if (!section)
        {
            return false;
        }

        // Clear any previous value
        m_config.admin_security.uids.clear();
        m_config.admin_security.gids.clear();

        // Get UIDs
        auto uids{section["uid"]};

        if (!uids || !uids.is_array())
        {
            return false;
        }

        for (auto&& uid : *uids.as_array())
        {
            uid.visit(
                [&dst = m_config.admin_security.uids](auto&& el) noexcept
                {
                    if constexpr (toml::is_number<decltype(el)>)
                    {
                        dst.insert(el.get());
                    }
                    else if constexpr (toml::is_string<decltype(el)>)
                    {
                        struct passwd* passwd_entry{getpwnam(el.get().c_str())};

                        if (passwd_entry != nullptr)
                        {
                            dst.insert(passwd_entry->pw_uid);
                        }
                    }
                });
        }

        // Always allow current user
        m_config.admin_security.uids.insert(getuid());

        // Get GIDs
        auto gids{section["gid"]};

        if (!gids || !gids.is_array())
        {
            return false;
        }

        for (auto&& gid : *gids.as_array())
        {
            gid.visit(
                [&dst = m_config.admin_security.gids](auto&& el) noexcept
                {
                    if constexpr (toml::is_number<decltype(el)>)
                    {
                        dst.insert(el.get());
                    }
                    else if constexpr (toml::is_string<decltype(el)>)
                    {
                        struct group* group_entry{getgrnam(el.get().c_str())};

                        if (group_entry != nullptr)
                        {
                            dst.insert(group_entry->gr_gid);
                        }
                    }
                });
        }

        return true;
    }

    bool Loader::parse_security_api(const toml::table& table)
    {
        auto section{table[toml::path{"security.api"}]};

        if (!section)
        {
            return false;
        }

        // Clear any previous value
        m_config.api_security.uids.clear();
        m_config.api_security.gids.clear();

        // Get UIDs
        auto uids{section["uid"]};

        if (!uids || !uids.is_array())
        {
            return false;
        }

        for (auto&& uid : *uids.as_array())
        {
            uid.visit(
                [&dst = m_config.api_security.uids](auto&& el) noexcept
                {
                    if constexpr (toml::is_number<decltype(el)>)
                    {
                        dst.insert(el.get());
                    }
                    else if constexpr (toml::is_string<decltype(el)>)
                    {
                        struct passwd* passwd_entry{getpwnam(el.get().c_str())};

                        if (passwd_entry != nullptr)
                        {
                            dst.insert(passwd_entry->pw_uid);
                        }
                    }
                });
        }

        // Get GIDs
        auto gids{section["gid"]};

        if (!gids || !gids.is_array())
        {
            return false;
        }

        for (auto&& gid : *gids.as_array())
        {
            gid.visit(
                [&dst = m_config.api_security.gids](auto&& el) noexcept
                {
                    if constexpr (toml::is_number<decltype(el)>)
                    {
                        dst.insert(el.get());
                    }
                    else if constexpr (toml::is_string<decltype(el)>)
                    {
                        struct group* group_entry{getgrnam(el.get().c_str())};

                        if (group_entry != nullptr)
                        {
                            dst.insert(group_entry->gr_gid);
                        }
                    }
                });
        }

        return true;
    }

    bool Loader::parse_telegram(const toml::table& table)
    {
        auto section{table["telegram"]};

        if (!section)
        {
            return false;
        }

        bool result{true};

        result &= extract_value(section["token"], m_config.telegram.token);
        return result;
    }
}  // namespace flujo::config
