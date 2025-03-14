#pragma once

#include "models/Room.h"
#include "models/User.h"

#include <drogon/orm/RestfulController.h>

namespace server::api
{
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::postgres;

class Rooms : public HttpController<Rooms>, public RestfulController
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Rooms::CreateOne, "/rooms", "AuthenticationCoroFilter", Post, Options);

    ADD_METHOD_TO(Rooms::GetOne, "/rooms/{id}", "AuthenticationCoroFilter", Get, Options);
    ADD_METHOD_TO(Rooms::GetMultiple, "/rooms", "AuthenticationCoroFilter", Get, Options);

    // ADD_METHOD_TO(Rooms::UpdateOne, "/rooms/{id}", "AuthenticationCoroFilter", Put, Options);

    // ADD_METHOD_TO(Rooms::DeleteOne, "/rooms/{id}", "AuthenticationCoroFilter", Delete, Options);

    ADD_METHOD_TO(Rooms::GetUserRooms, "/users/{id}/rooms", "AuthenticationCoroFilter", Get, Options);
    ADD_METHOD_TO(Rooms::GetCurrentUserRooms, "/users/me/rooms", "AuthenticationCoroFilter", Get, Options);
    METHOD_LIST_END

    Rooms();
    Task<HttpResponsePtr> CreateOne(HttpRequestPtr req);
    Task<HttpResponsePtr> GetOne(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> GetMultiple(HttpRequestPtr req);
    Task<HttpResponsePtr> UpdateOne(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> DeleteOne(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> GetUserRooms(HttpRequestPtr req, User::PrimaryKeyType id);
    Task<HttpResponsePtr> GetCurrentUserRooms(HttpRequestPtr req);
};

} // namespace server::api
