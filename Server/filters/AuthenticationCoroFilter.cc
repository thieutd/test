/**
 *
 *  AuthenticationCoroFilter.cc
 *
 */

#include "AuthenticationCoroFilter.h"
#include "plugins/JwtTokenManager.h"
#include "plugins/RedisManager.h"
#include "utilities/HttpResponseUtil.h"

#include <drogon/drogon.h>
#include <fmt/std.h>

using namespace drogon;
using namespace drogon::orm;
using namespace drogon::nosql;
using namespace drogon_model::postgres;
using namespace server;

Task<HttpResponsePtr> AuthenticationCoroFilter::doFilter(const HttpRequestPtr &req)
{
    if (auto &auth_header = req->getHeader("Authorization"); auth_header.starts_with("Bearer "))
    {
        const auto token = auth_header.substr(7);
        if (auto validation_result = app().getPlugin<JwtTokenManager>()->ValidateToken(token, true); validation_result)
        {
            if (const auto token_exists_result = co_await app().getPlugin<RedisManager>()->HasAccessToken(token); token_exists_result)
            {
                if (*token_exists_result)
                {
                    const auto decoded = jwt::decode(token);
                    try
                    {
                        const User::PrimaryKeyType user_id = std::stoi(decoded.get_subject());
                        req->getAttributes()->insert("id", user_id);
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR << e.what();
                        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kInternalServerError>();
                    }
                    req->getAttributes()->insert("role", decoded.get_payload_claim("role").as_string());
                    req->getAttributes()->insert("refresh_id", decoded.get_payload_claim("refresh_id").as_string());
                    co_return nullptr;
                }
            }
            else
            {
                LOG_INFO << fmt::format("Invalid access token {}", token_exists_result.error());
            }
        }
        else
        {
            LOG_INFO << fmt::format("Invalid access token {}", validation_result.error());
        }
    }

    co_return utilities::NewJsonErrorResponse<HttpErrorCode::kInvalidAccessTokenError>();
}