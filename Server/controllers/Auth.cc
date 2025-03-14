#include "Auth.h"
#include "models/Helper.h"
#include "plugins/JwtTokenManager.h"
#include "plugins/PasswordHasher.h"
#include "plugins/RedisManager.h"
#include "utilities/FormatterUtil.h"
#include "utilities/HttpResponseUtil.h"

#include <fmt/std.h>

using namespace drogon::orm;
using namespace drogon_model::postgres;
using namespace server::api;
using namespace server::models;

Auth::Auth()
{
    ASSERT(app().getPlugin<JwtTokenManager>() != nullptr, "JwtTokenManager plugin is not loaded");
    ASSERT(app().getPlugin<RedisManager>() != nullptr, "RedisManager plugin is not loaded");
    ASSERT(app().getPlugin<PasswordHasher>() != nullptr, "PasswordHasher plugin is not loaded");
}

Task<HttpResponsePtr> Auth::CoroRegister(const HttpRequestPtr req)
{
    const auto &json_ptr = req->jsonObject();
    if (!json_ptr)
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kNoJsonObjectError>();
    }

    if (std::string err; !User::validateMasqueradedJsonForCreation(*json_ptr, kUserRegistrationFields, err))
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kFieldTypeError>(err);
    }

    const auto username = (*json_ptr)["username"].asString();
    CoroMapper<User> mapper{app().getDbClient()};
    try
    {
        if (const auto count = co_await mapper.count({User::Cols::_username, CompareOperator::EQ, username}); count > 0)
        {
            co_return utilities::NewJsonErrorResponse(k409Conflict,
                                                      std::format("Username {} already exists", username));
        }
    }
    catch (const DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }

    auto password_hashing_result = app().getPlugin<PasswordHasher>()->HashPassword((*json_ptr)["password"].asString());
    if (!password_hashing_result)
    {
        LOG_ERROR << fmt::format("{}", password_hashing_result.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kInternalServerError>();
    }
    (*json_ptr)["password"] = std::move(password_hashing_result).value();

    User user{*json_ptr, kUserRegistrationFields};
    try
    {
        user = co_await mapper.insert(user);
    }
    catch (const DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }

    Json::Value ret = user.toMasqueradedJson(kUserInfoFields);
    auto resp = HttpResponse::newHttpJsonResponse(std::move(ret));
    resp->setStatusCode(k201Created);

    LOG_INFO << std::format("User {} registered", user.getValueOfUsername());
    co_return resp;
}

Task<HttpResponsePtr> Auth::CoroLogin(const HttpRequestPtr req)
{
    auto &json_ptr = req->jsonObject();
    if (!json_ptr)
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kNoJsonObjectError>();
    }

    if (!json_ptr->isMember("username") || !json_ptr->isMember("password"))
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kFieldTypeError>("Username and password are required");
    }

    const auto username = (*json_ptr)["username"].asString();
    const auto password = (*json_ptr)["password"].asString();
    CoroMapper<User> mapper{app().getDbClient()};
    User user;
    try
    {
        auto users = co_await mapper.findBy({User::Cols::_username, CompareOperator::EQ, username});
        if (users.empty())
        {
            co_return utilities::NewJsonErrorResponse(k401Unauthorized, "Invalid username");
        }
        user = std::move(users.front());
    }
    catch (const DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }

    auto password_check_result = app().getPlugin<PasswordHasher>()->VerifyPassword(password, user.getValueOfPassword());
    if (!password_check_result)
    {
        LOG_ERROR << fmt::format("{}", password_check_result.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kInternalServerError>();
    }
    if (!password_check_result.value())
    {
        co_return utilities::NewJsonErrorResponse(k401Unauthorized, "Invalid password");
    }

    const auto refresh_token = app().getPlugin<JwtTokenManager>()->GenerateRefreshToken(user);
    const auto access_token = app().getPlugin<JwtTokenManager>()->GenerateAccessToken(refresh_token, user);
    const auto redis_result = co_await app().getPlugin<RedisManager>()->StoreRefreshTokenId(refresh_token, access_token);
    if (!redis_result)
    {
        LOG_ERROR << fmt::format("{}", redis_result.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kCacheDatabaseError>();
    }

    app().getPlugin<RedisManager>()->StoreUserInRedisAsync(user);

    Json::Value ret;
    ret["access_token"] = access_token;
    ret["access_expiration"] = utilities::ToSeconds(jwt::decode(access_token).get_expires_at().time_since_epoch());
    ret["refresh_token"] = refresh_token;
    ret["refresh_expiration"] = utilities::ToSeconds(jwt::decode(refresh_token).get_expires_at().time_since_epoch());
    const auto resp = HttpResponse::newHttpJsonResponse(std::move(ret));

    LOG_INFO << std::format("User {} logged in", user.getValueOfUsername());
    co_return resp;
}

Task<HttpResponsePtr> Auth::CoroRefresh(const HttpRequestPtr req)
{
    const auto &json_ptr = req->jsonObject();
    if (!json_ptr)
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kNoJsonObjectError>();
    }

    if (!json_ptr->isMember("refresh_token"))
    {
        co_return utilities::NewJsonErrorResponse(k400BadRequest, "Refresh token is required");
    }

    const auto refresh_token = (*json_ptr)["refresh_token"].asString();
    if (const auto validation_result = app().getPlugin<JwtTokenManager>()->ValidateToken(refresh_token); !validation_result)
    {
        co_return utilities::NewJsonErrorResponse(k401Unauthorized, "Invalid refresh token");
    }


    const auto refresh_token_exists = co_await app().getPlugin<RedisManager>()->HasRefreshToken(refresh_token);
    if (!refresh_token_exists.has_value())
    {
        LOG_ERROR << fmt::format("{}", refresh_token_exists.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kCacheDatabaseError>();
    }

    if (!(*refresh_token_exists))
    {
        co_return utilities::NewJsonErrorResponse(k401Unauthorized, "Invalid refresh token");
    }

    std::optional<User> user;
    auto user_redis_result = co_await app().getPlugin<RedisManager>()->GetUserFromRedis(
        std::stoi(jwt::decode(refresh_token).get_subject().c_str()));
    if (!user_redis_result)
    {
        LOG_ERROR << fmt::format("{}", user_redis_result.error());
    }
    else if (user_redis_result->has_value())
    {
        user = std::move(user_redis_result).value();
    }

    if (!user)
    {
        CoroMapper<User> mapper{app().getDbClient()};
        try
        {
            User::PrimaryKeyType user_id = std::stoi(jwt::decode(refresh_token).get_subject());
            user = co_await mapper.findByPrimaryKey(user_id);
        }
        catch (const DrogonDbException &e)
        {
            if (dynamic_cast<const UnexpectedRows *>(&e.base()))
            {
                co_return utilities::NewJsonErrorResponse(k401Unauthorized, "User not found");
            }
            LOG_ERROR << e.base().what();
            co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
        }
        app().getPlugin<RedisManager>()->StoreUserInRedisAsync(*user);
    }

    const auto access_token = app().getPlugin<JwtTokenManager>()->GenerateAccessToken(refresh_token, *user);
    if (const auto store_result = co_await app().getPlugin<RedisManager>()->StoreRefreshTokenId(refresh_token, access_token); !store_result)
    {
        LOG_ERROR << fmt::format("{}", store_result.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kCacheDatabaseError>();
    }

    Json::Value ret;
    ret["access_token"] = access_token;
    ret["access_expiration"] = utilities::ToSeconds(jwt::decode(access_token).get_expires_at().time_since_epoch());
    const auto resp = HttpResponse::newHttpJsonResponse(std::move(ret));

    LOG_INFO << std::format("User {} refreshed token", user->getValueOfUsername());
    co_return resp;
}

Task<HttpResponsePtr> Auth::CoroLogout(const HttpRequestPtr req)
{
    const auto &json_ptr = req->jsonObject();
    if (!json_ptr)
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kNoJsonObjectError>();
    }

    if (!json_ptr->isMember("refresh_token"))
    {
        co_return utilities::NewJsonErrorResponse(k400BadRequest, "Refresh token is required");
    }

    const auto refresh_token = (*json_ptr)["refresh_token"].asString();
    if (const auto validation_result = app().getPlugin<JwtTokenManager>()->ValidateToken(refresh_token); !validation_result)
    {
        co_return utilities::NewJsonErrorResponse(k401Unauthorized, "Invalid refresh token");
    }

    if (const auto deleted_count = co_await app().getPlugin<RedisManager>()->DeleteRefreshToken(refresh_token); !deleted_count)
    {
        LOG_ERROR << fmt::format("{}", deleted_count.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kCacheDatabaseError>();
    }

    co_return HttpResponse::newHttpResponse();
}