/**
 *
 *  RedisManager.cc
 *
 */

#include "RedisManager.h"
#include "models/Helper.h"
#include "utilities/FormatterUtil.h"

#include <drogon/HttpAppFramework.h>
#include "fmt/ranges.h"

#include <jwt-cpp/jwt.h>

using namespace drogon;
using namespace server::utilities;
using namespace server::models;

void RedisManager::initAndStart(const Json::Value &config)
{
}

void RedisManager::shutdown()
{
}

Task<std::expected<void, RedisManager::RedisOperationError>> RedisManager::StoreRefreshTokenId(
    const std::string &refresh_token, const std::optional<std::string> &access_token_opt)
{
    const auto redis_client = app().getRedisClient();
    const auto decoded = jwt::decode(refresh_token);
    const auto value = access_token_opt
                           .and_then([](const auto &access_token) -> std::optional<std::string> {
                               const auto token_decoded = jwt::decode(access_token);
                               return std::format("{};exp:{:%Y-%m-%d_%H:%M:%S}", token_decoded.get_id(),
                                                  token_decoded.get_expires_at());
                           })
                           .value_or("1");

    const auto insertion_command =
        std::format("SET refresh_token:{}:{} {} EXAT {}", decoded.get_subject(), decoded.get_id(), value,
                    ToSeconds(decoded.get_expires_at().time_since_epoch()));
    try
    {
        const auto insertion_result = co_await redis_client->execCommandCoro(insertion_command);
        if (insertion_result.asString() == "OK")
        {
            co_return {};
        }

        co_return std::unexpected(insertion_result.asString());
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
}

Task<std::expected<bool, RedisManager::RedisOperationError>> RedisManager::HasRefreshToken(
    const std::string &refresh_token)
{
    const auto redis_client = app().getRedisClient();
    const auto decoded = jwt::decode(refresh_token);
    const auto retrieval_command = std::format("EXISTS refresh_token:{}:{}", decoded.get_subject(), decoded.get_id());
    try
    {
        const auto retrieval_result = co_await redis_client->execCommandCoro(retrieval_command);
        co_return retrieval_result.asInteger();
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
}

Task<std::expected<bool, RedisManager::RedisOperationError>> RedisManager::DeleteRefreshToken(
    const std::string &refresh_token)
{
    const auto redis_client = app().getRedisClient();
    const auto decoded = jwt::decode(refresh_token);
    const auto deletion_command = std::format("DEL refresh_token:{}:{}", decoded.get_subject(), decoded.get_id());
    try
    {
        const auto deletion_result = co_await redis_client->execCommandCoro(deletion_command);
        co_return deletion_result.asInteger();
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
}

Task<std::expected<bool, RedisManager::RedisOperationError>> RedisManager::HasAccessToken(
    const std::string &access_token)
{
    const auto redis_client = app().getRedisClient();
    const auto decoded = jwt::decode(access_token);
    const auto user_id = decoded.get_subject();
    const auto token_id = decoded.get_id();
    const auto refresh_token_id = decoded.get_payload_claim("refresh_id").as_string();
    const auto retrieval_command = std::format("GET refresh_token:{}:{}", user_id, refresh_token_id);
    try
    {
        if (const auto retrieval_result = co_await redis_client->execCommandCoro(retrieval_command);
            retrieval_result.asString().starts_with(token_id))
        {
            co_return true;
        }
        co_return false;
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
}

Task<std::expected<std::optional<drogon_model::postgres::User>, RedisManager::RedisOperationError>> RedisManager::
    GetUserFromRedis(const drogon_model::postgres::User::PrimaryKeyType user_id)
{
    const auto redis_client = app().getRedisClient();
    const auto retrieval_command = std::format("GET user:{}", user_id);
    try
    {
        const auto retrieval_result = co_await redis_client->execCommandCoro(retrieval_command);
        if (retrieval_result.isNil())
        {
            co_return std::nullopt;
        }
        Json::CharReaderBuilder reader;
        Json::Value json;
        std::string errs;
        if (std::istringstream is(retrieval_result.asString()); !Json::parseFromStream(reader, is, &json, &errs))
        {
            co_return std::unexpected(errs);
        }
        co_return drogon_model::postgres::User{json};
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
    catch (const std::exception &e)
    {
        co_return std::unexpected(e.what());
    }
    catch (...)
    {
        throw std::runtime_error("Unknown error");
    }
}

AsyncTask RedisManager::StoreUserInRedisAsync(const drogon_model::postgres::User user)
{
    try
    {
        const auto redis_client = app().getRedisClient();
        const auto masqueraded_json = user.toMasqueradedJson(kUserRedisInfoFields);
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        const auto insertion_command =
            std::format("SET user:{} {}", user.getValueOfId(), Json::writeString(writer, masqueraded_json));
        co_await redis_client->execCommandCoro(insertion_command);
    }
    catch (const nosql::RedisException &e)
    {
        LOG_ERROR << e.what();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << e.what();
    }
    catch (...)
    {
        LOG_ERROR << "Unknown error";
    }
}

AsyncTask RedisManager::SetUserLastOnline(const UserPrimaryKeyType user_id, const TimePoint time)
{
    const auto redis_client = app().getRedisClient();
    const auto insertion_command = std::format("SET last_online:{} {}", user_id, ToSeconds(time.time_since_epoch()));
    co_await redis_client->execCommandCoro(insertion_command);
}

Task<std::expected<RedisManager::LastOnlineOpt, RedisManager::RedisOperationError>> RedisManager::GetUserLastOnline(
    const UserPrimaryKeyType user_id)
{
    const auto redis_client = app().getRedisClient();
    const auto retrieval_command = std::format("GET last_online:{}", user_id);
    try
    {
        const auto retrieval_result = co_await redis_client->execCommandCoro(retrieval_command);
        if (retrieval_result.isNil())
        {
            co_return std::nullopt;
        }
        const auto timestamp = std::stoll(retrieval_result.asString());
        co_return std::chrono::system_clock::time_point{std::chrono::seconds{timestamp}};
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
}
drogon::Task<std::expected<std::vector<RedisManager::LastOnlineOpt>, RedisManager::RedisOperationError>> RedisManager::
    GetUsersLastOnline(const std::span<const UserPrimaryKeyType> user_ids)
{
    const auto redis_client = app().getRedisClient();
    std::vector<std::string> keys;
    keys.reserve(user_ids.size());
    for (const auto &user_id : user_ids)
    {
        keys.push_back(std::format("last_online:{}", user_id));
    }
    const auto retrieval_command = fmt::format("MGET {}", fmt::join(keys, " "));
    try
    {
        const auto retrieval_result = co_await redis_client->execCommandCoro(retrieval_command);
        std::vector<LastOnlineOpt> result;
        result.reserve(user_ids.size());
        for (const auto &value : retrieval_result.asArray())
        {
            if (value.isNil())
            {
                result.push_back(std::nullopt);
            }
            else
            {
                const auto timestamp = std::stoll(value.asString());
                result.push_back(std::chrono::system_clock::time_point{std::chrono::seconds{timestamp}});
            }
        }
        co_return result;
    }
    catch (const nosql::RedisException &e)
    {
        co_return std::unexpected(e);
    }
}
