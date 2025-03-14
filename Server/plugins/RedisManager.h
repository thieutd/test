/**
 *
 *  RedisManager.h
 *
 */

#pragma once

#include "models/User.h"

#include <drogon/nosql/RedisException.h>
#include <drogon/plugins/Plugin.h>
#include <drogon/utils/coroutine.h>

class RedisManager : public drogon::Plugin<RedisManager>
{
  public:
    using RedisOperationError = std::variant<std::string, drogon::nosql::RedisException>;
    using User = drogon_model::postgres::User;
    using UserPrimaryKeyType = User::PrimaryKeyType;
    using TimePoint = std::chrono::system_clock::time_point;
    using LastOnlineOpt = std::optional<TimePoint>;

    void initAndStart(const Json::Value &config) override;
    void shutdown() override;

    drogon::Task<std::expected<void, RedisOperationError>> StoreRefreshTokenId(
        const std::string &refresh_token, const std::optional<std::string> &access_token_opt = std::nullopt);
    drogon::Task<std::expected<bool, RedisOperationError>> DeleteRefreshToken(const std::string &refresh_token);
    drogon::Task<std::expected<bool, RedisOperationError>> HasRefreshToken(const std::string &refresh_token);
    drogon::Task<std::expected<bool, RedisOperationError>> HasAccessToken(const std::string &access_token);

    drogon::Task<std::expected<std::optional<User>, RedisOperationError>> GetUserFromRedis(
        const UserPrimaryKeyType user_id);
    drogon::AsyncTask StoreUserInRedisAsync(User user);

    drogon::AsyncTask SetUserLastOnline(const UserPrimaryKeyType user_id,
                                const TimePoint time = std::chrono::system_clock::now());
    drogon::Task<std::expected<LastOnlineOpt, RedisOperationError>> GetUserLastOnline(const UserPrimaryKeyType user_id);
    drogon::Task<std::expected<std::vector<LastOnlineOpt>, RedisOperationError>> GetUsersLastOnline(
        const std::span<const UserPrimaryKeyType> user_ids);
};
