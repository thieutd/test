/**
 *
 *  JwtTokenManager.h
 *
 */

#pragma once

#include "models/User.h"

#include <drogon/plugins/Plugin.h>
#include <jwt-cpp/jwt.h>

/*
 * Use a generated UUID for the refresh token ID (jti) and another for the access token ID.
 * Reference the refresh token ID in the access token's JWT payload.
 * Store the refresh token ID in Redis using the pattern "refresh:<user_id>:<jti>", with the access token ID as its value.
 * For each request, validate the access token and check whether its ID is present in Redis.
 * This allows only one access token per refresh token at a time.
 * In WebSocketController, subscribe to Redis key event notifications for key expiration and deletion. If the refresh token ID is deleted, force-close the corresponding WebSocket connection.
 */

class JwtTokenManager : public virtual drogon::Plugin<JwtTokenManager>
{
    using User = drogon_model::postgres::User;
  public:
    void initAndStart(const Json::Value &config) override;
    void shutdown() override;

    using DecodeType = decltype(jwt::decode(std::declval<std::string>()));
    using ValidateError = std::variant<std::error_code, std::invalid_argument, std::runtime_error>;

    std::expected<void, ValidateError> ValidateToken(
        const std::string &token, bool is_access = false,
        const std::optional<std::string> &ref_token_id_opt = std::nullopt) const;
    std::string GenerateRefreshToken(const User &user) const;
    std::string GenerateAccessToken(const std::string &refresh_token,
                                    const User &user) const;

    private:
    std::string issuer_;
    std::string secret_;
    std::chrono::seconds access_token_expiry_{};
    std::chrono::seconds refresh_token_expiry_{};
};

