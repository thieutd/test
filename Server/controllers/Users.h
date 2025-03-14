#pragma once

#include "models/User.h"

#include <drogon/orm/RestfulController.h>

namespace server::api
{
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::postgres;

class Users : public HttpController<Users>, public RestfulController
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Users::CreateOne, "/admin/users", "AuthenticationCoroFilter", "AdminCoroFilter", Post, Options);

    ADD_METHOD_TO(Users::GetOne, "/users/{id}", "AuthenticationCoroFilter", Get, Options);
    ADD_METHOD_TO(Users::GetCurrent, "/users/me", "AuthenticationCoroFilter", Get, Options);
    ADD_METHOD_TO(Users::GetList, "/users", "AuthenticationCoroFilter", Get, Options);

    // ADD_METHOD_TO(Users::UpdateOne, "/users/{id}", "AuthenticationCoroFilter", Put, Options);
    // ADD_METHOD_TO(Users::UpdateCurrent, "/users/me", "AuthenticationCoroFilter", Put, Options);

    // ADD_METHOD_TO(Users::DeleteOne, "/users/{id}", "AuthenticationCoroFilter", Delete, Options);
    METHOD_LIST_END

    Users();
    Task<HttpResponsePtr> CreateOne(HttpRequestPtr req);
    Task<HttpResponsePtr> GetOne(HttpRequestPtr req, User::PrimaryKeyType id);
    Task<HttpResponsePtr> GetCurrent(HttpRequestPtr req);
    Task<HttpResponsePtr> GetList(HttpRequestPtr req);
    Task<HttpResponsePtr> UpdateOne(HttpRequestPtr req, User::PrimaryKeyType id);
    Task<HttpResponsePtr> UpdateCurrent(HttpRequestPtr req);
    Task<HttpResponsePtr> DeleteOne(HttpRequestPtr req, User::PrimaryKeyType id);
};

} // namespace server::api