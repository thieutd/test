/**
 *
 *  JwtTokenManager.cc
 *
 */

#include "JwtTokenManager.h"

using namespace drogon;

void JwtTokenManager::initAndStart(const Json::Value &config)
{
    issuer_ = config.get("issuer", "server").asString();
    secret_ = config.get("secret", "secret").asString();
    access_token_expiry_ = std::chrono::seconds{config.get("access_token_expiry", 60 * 60).asUInt()};
    refresh_token_expiry_ = std::chrono::seconds{config.get("refresh_token_expiry", 30 * 24 * 60 * 60).asUInt()};
}

void JwtTokenManager::shutdown() 
{
}

std::expected<void, JwtTokenManager::ValidateError> JwtTokenManager::ValidateToken(
    const std::string &token, const bool is_access,
    const std::optional<std::string> &ref_token_id_opt) const
{
    try
    {
        const auto uuid_verification_fn = [](const auto &ctx, std::error_code &ec) {
            const auto jti = ctx.get_claim(false, ec);
            if (ec)
            {
                return;
            }
            if (jti.get_type() != jwt::json::type::string)
            {
                ec = jwt::error::token_verification_error::claim_type_missmatch;
                return;
            }
            static const std::regex reg("^[0-9a-f]{8}-[0-9a-f]{4}-[0-5][0-9a-f]{3}-[089ab][0-9a-f]{3}-[0-9a-f]{12}$",
                                        std::regex_constants::icase);
            if (!std::regex_match(jti.as_string(), reg))
            {
                ec = jwt::error::token_verification_error::claim_value_missmatch;
            }
        };

        const auto username_verification_fn = [](const auto &ctx, std::error_code &ec) {
            const auto username = ctx.get_claim(false, ec);
            if (ec)
            {
                return;
            }
            if (username.get_type() != jwt::json::type::string)
            {
                ec = jwt::error::token_verification_error::claim_type_missmatch;
                return;
            }
            if (username.as_string().empty())
            {
                ec = jwt::error::token_verification_error::claim_value_missmatch;
            }
        };

        const auto role_verification_fn = [](const auto &ctx, std::error_code &ec) {
            const auto role = ctx.get_claim(false, ec);
            if (ec)
            {
                return;
            }
            if (role.get_type() != jwt::json::type::string)
            {
                ec = jwt::error::token_verification_error::claim_type_missmatch;
                return;
            }

            static constexpr auto roles = {"admin", "user"};
            if (!std::ranges::contains(roles, role.as_string()))
            {
                ec = jwt::error::token_verification_error::claim_value_missmatch;
            }
        };

        const auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secret_})
                            .with_issuer(issuer_)
                            .with_type(is_access ? "access" : "refresh")
                            .with_claim("jti", uuid_verification_fn);
        if (is_access)
        {
            if (ref_token_id_opt)
            {
                verifier.with_claim("refresh_id", jwt::claim(*ref_token_id_opt));
            }
            else
            {
                verifier.with_claim("refresh_id", uuid_verification_fn);
            }
            verifier.with_claim("username", username_verification_fn).with_claim("role", role_verification_fn);
        }

        std::error_code ec;
        verifier.verify(decoded, ec);

        if (ec)
        {
            return std::unexpected(ec);
        }
        return {};
    }
    catch (const std::invalid_argument &e)
    {
        return std::unexpected(e);
    }
    catch (const std::runtime_error &e)
    {
        return std::unexpected(e);
    }
    catch (...)
    {
        throw std::runtime_error("Unknown error");
    }
}

std::string JwtTokenManager::GenerateRefreshToken(const drogon_model::postgres::User &user) const
{
    const auto uuid = utils::getUuid();
    return jwt::create()
        .set_issuer(issuer_)
        .set_id(uuid)
        .set_subject(std::to_string(user.getValueOfId()))
        .set_type("refresh")
        .set_expires_at(std::chrono::system_clock::now() + refresh_token_expiry_)
        .sign(jwt::algorithm::hs256{secret_});
}

std::string JwtTokenManager::GenerateAccessToken(const std::string &refresh_token,
                                                 const drogon_model::postgres::User &user) const
{
    const auto uuid = utils::getUuid();
    const auto decoded = jwt::decode(refresh_token);
    const auto refresh_id = decoded.get_id();
    return jwt::create()
        .set_issuer(issuer_)
        .set_id(uuid)
        .set_subject(decoded.get_subject())
        .set_payload_claim("refresh_id", jwt::claim(refresh_id))
        .set_payload_claim("username", jwt::claim(user.getValueOfUsername()))
        .set_payload_claim("role", jwt::claim(user.getValueOfRole()))
        .set_type("access")
        .set_expires_at(std::chrono::system_clock::now() + access_token_expiry_)
        .sign(jwt::algorithm::hs256{secret_});
}