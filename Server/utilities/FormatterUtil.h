#pragma once

#include <botan/exceptn.h>
#include <fmt/std.h>

template <> struct fmt::formatter<drogon::orm::DrogonDbException> : formatter<std::string>
{
    template <typename FormatContext> auto format(const drogon::orm::DrogonDbException &ex, FormatContext &ctx) const
    {
        const std::string formatted_message = fmt::format("DrogonDbException: {}", ex.base());
        return formatter<std::string>::format(formatted_message, ctx);
    }
};

template <> struct fmt::formatter<drogon::nosql::RedisException> : formatter<std::string>
{
    template <typename FormatContext> auto format(const drogon::nosql::RedisException &ex, FormatContext &ctx) const
    {
        const std::string formatted_message = fmt::format("RedisException: {}", ex.what());
        return formatter<std::string>::format(formatted_message, ctx);
    }
};

template <> struct fmt::formatter<Botan::Exception> : formatter<std::string>
{
    template <typename FormatContext> auto format(const Botan::Exception &ex, FormatContext &ctx) const
    {
        const std::string formatted_message = fmt::format("BotanException: {}", ex.what());
        return formatter<std::string>::format(formatted_message, ctx);
    }
};

namespace server::utilities
{
constexpr auto ToSeconds = []<typename Duration>(Duration &&duration) {
    return std::chrono::duration_cast<std::chrono::seconds>(std::forward<Duration>(duration)).count();
};
} // namespace server::utilities