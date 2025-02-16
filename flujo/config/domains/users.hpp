#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace flujo::config::domains
{
    namespace users_detail
    {
        /// @brief Communication method enumeration.
        enum class CommunicationMethod : std::uint8_t
        {
            Telegram,
        };

        /// @brief Structure containing details regarding how a user can be contacted.
        struct UserDetails
        {
            /// @brief ID of the user.
            std::string id;
            /// @brief List of topics the user is interested in.
            std::vector<std::string> topics;
            /// @brief Preferred communication method.
            CommunicationMethod preferred;

            /// @brief Telegram ID.
            std::string telegram;
            /// @brief Email address.
            std::string email;
        };
    }  // namespace users_detail

    /// @brief Users/Recipients of messages..
    struct Users
    {
        using CommunicationMethod = users_detail::CommunicationMethod;
        using UserDetails = users_detail::UserDetails;

        /// @brief Map of users.
        std::map<std::string, UserDetails> users;
        /// @brief Reverse map of topics and which users are interested in them.
        std::map<std::string, std::vector<std::string>> topics;
    };
}  // namespace flujo::config::domains
