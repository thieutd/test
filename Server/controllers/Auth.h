#pragma once

#include <drogon/HttpController.h>

namespace server::api
{
using namespace drogon;

class Auth : public HttpController<Auth>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Auth::CoroRegister, "/auth/register", Post, Options);
    ADD_METHOD_TO(Auth::CoroLogin, "auth/login", Post, Options);
    ADD_METHOD_TO(Auth::CoroRefresh, "/auth/refresh", Post, Options);
    ADD_METHOD_TO(Auth::CoroLogout, "/auth/logout", Post, Options);
    METHOD_LIST_END

    Auth();
    Task<HttpResponsePtr> CoroRegister(HttpRequestPtr req);
    Task<HttpResponsePtr> CoroLogin(HttpRequestPtr req);
    Task<HttpResponsePtr> CoroRefresh(HttpRequestPtr req);
    Task<HttpResponsePtr> CoroLogout(HttpRequestPtr req);
};

} // namespace server::api