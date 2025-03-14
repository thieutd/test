#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <json/json.h>
#include <magic_enum/magic_enum.hpp>

namespace server
{
enum class HttpErrorCode
{
    // user-defined error codes
    kNoJsonObjectError,
    kFieldTypeError,
    kInvalidAccessTokenError,
    kPermissionDeniedError,
    // system-defined error codes
    kDatabaseError,
    kCacheDatabaseError,
    kInternalServerError
};

namespace utilities
{
namespace internal
{
consteval const char *HttpErrorCodeToString(HttpErrorCode error_code);
} // namespace internal

using namespace drogon;
inline HttpResponsePtr NewHttpResponse(const HttpStatusCode status_code = k200OK)
{
    return HttpResponse::newHttpResponse(status_code, CT_TEXT_PLAIN);
}

template <typename... Args> HttpResponsePtr NewHttpResponse(const HttpStatusCode status_code, Args &&...args)
{
    auto resp = HttpResponse::newHttpResponse(status_code, CT_TEXT_PLAIN);
    std::string joined;
    if constexpr (sizeof...(Args) == 1)
    {
        joined = fmt::format("{}", std::forward<Args>(args)...);
    }
    else
    {
        joined = fmt::format("{}", fmt::join({fmt::format("{}", std::forward<Args>(args))...}, " - "));
    }
    resp->setBody(std::move(joined));
    return resp;
}

template <typename... Args> HttpResponsePtr NewHttpErrorResponse(const HttpStatusCode status_code, Args &&...args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        return NewHttpResponse(status_code);
    }
    else if constexpr (sizeof...(Args) == 1)
    {
        return NewHttpResponse(status_code, fmt::format("Error: {}", std::forward<Args>(args)...));
    }
    const std::string joined = fmt::format("{}", fmt::join({fmt::format("{}", std::forward<Args>(args))...}, " - "));
    return NewHttpResponse(status_code, fmt::format("Error: {}", joined));
}

template <HttpErrorCode error_code, typename... Args> HttpResponsePtr NewHttpErrorResponse(Args &&...args)
{
    constexpr const char *error_message = internal::HttpErrorCodeToString(error_code);
    if constexpr (error_code == HttpErrorCode::kNoJsonObjectError || error_code == HttpErrorCode::kFieldTypeError)
    {
        return NewHttpErrorResponse(k400BadRequest, error_message, std::forward<Args>(args)...);
    }
    else if constexpr (error_code == HttpErrorCode::kInvalidAccessTokenError)
    {
        return NewHttpErrorResponse(k401Unauthorized, error_message, std::forward<Args>(args)...);
    }
    else if constexpr (error_code == HttpErrorCode::kPermissionDeniedError)
    {
        return NewHttpErrorResponse(k403Forbidden, error_message, std::forward<Args>(args)...);
    }
    else if constexpr (error_code == HttpErrorCode::kDatabaseError ||
                       error_code == HttpErrorCode::kCacheDatabaseError ||
                       error_code == HttpErrorCode::kInternalServerError)
    {
        return NewHttpErrorResponse(k500InternalServerError, error_message, std::forward<Args>(args)...);
    }
    else
    {
        static_assert(false, "Invalid error code");
    }
}

template<typename JsonValue>
HttpResponsePtr NewJsonResponse(JsonValue &&json_value, const HttpStatusCode status_code = k200OK)
{
    auto resp = HttpResponse::newHttpJsonResponse(std::forward<JsonValue>(json_value));
    resp->setStatusCode(status_code);
    return resp;
}

template <typename... Args>
HttpResponsePtr NewJsonErrorResponse(const HttpStatusCode status_code, Args &&...args)
{
    Json::Value error;
    error["error"]["code"] = status_code;
    
    if constexpr (sizeof...(Args) == 1)
    {
        error["error"]["message"] = fmt::format("{}", std::forward<Args>(args)...);
    }
    else if constexpr (sizeof...(Args) > 1)
    {
        const std::string joined =
            fmt::format("{}", fmt::join({fmt::format("{}", std::forward<Args>(args))...}, " - "));
        error["error"]["message"] = joined;
    }
    return NewJsonResponse(std::move(error), status_code);
}

template <HttpErrorCode error_code, typename... Args>
HttpResponsePtr NewJsonErrorResponse(Args &&...args)
{
    constexpr const char *error_message = internal::HttpErrorCodeToString(error_code);
    Json::Value error;
    HttpStatusCode status_code;

    if constexpr (error_code == HttpErrorCode::kNoJsonObjectError || error_code == HttpErrorCode::kFieldTypeError)
    {
        status_code = k400BadRequest;
    }
    else if constexpr (error_code == HttpErrorCode::kInvalidAccessTokenError)
    {
        status_code = k401Unauthorized;
    }
    else if constexpr (error_code == HttpErrorCode::kPermissionDeniedError)
    {
        status_code = k403Forbidden;
    }
    else if constexpr (error_code == HttpErrorCode::kDatabaseError ||
                       error_code == HttpErrorCode::kCacheDatabaseError ||
                       error_code == HttpErrorCode::kInternalServerError)
    {
        status_code = k500InternalServerError;
    }
    else
    {
        static_assert(false, "Invalid error code");
    }

    error["error"]["code"] = static_cast<int>(status_code);
    
    if constexpr (sizeof...(Args) == 0)
    {
        error["error"]["message"] = error_message;
    }
    else
    {
        const std::string joined = fmt::format("{}", fmt::join({fmt::format("{}", std::forward<Args>(args))...}, " - "));
        error["error"]["message"] = fmt::format("{} - {}", error_message, joined);
    }

    return NewJsonResponse(error, status_code);
}
} // namespace utilities
} // namespace server

namespace server::utilities::internal
{
consteval const char *HttpErrorCodeToString(HttpErrorCode error_code)
{
    switch (error_code)
    {
    case HttpErrorCode::kNoJsonObjectError:
        return "No Json object found in the request";
    case HttpErrorCode::kFieldTypeError:
        return "Field type error";
    case HttpErrorCode::kInvalidAccessTokenError:
        return "Invalid access token";
    case HttpErrorCode::kPermissionDeniedError:
        return "Permission denied";
    case HttpErrorCode::kDatabaseError:
        return "Database error";
    case HttpErrorCode::kCacheDatabaseError:
        return "Cache database error";
    case HttpErrorCode::kInternalServerError:
        return "Internal server error";
    default:
        throw std::invalid_argument("Invalid error code");
    }
}
} // namespace server::utilities::internal