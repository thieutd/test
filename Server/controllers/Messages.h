#pragma once

#include "models/Room.h"

#include <drogon/orm/RestfulController.h>

namespace server::api
{
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::postgres;

class Messages : public HttpController<Messages>, public RestfulController
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Messages::CreateOne, "/rooms/{id}/messages", "AuthenticationCoroFilter", Post);

    ADD_METHOD_TO(Messages::GetMultipleByRoomId, "/rooms/{id}/messages", "AuthenticationCoroFilter", Get);

    ADD_METHOD_TO(Messages::UpdateOne, "/rooms/{id}/messages/{id}", "AuthenticationCoroFilter", Put);

    ADD_METHOD_TO(Messages::DeleteOne, "/rooms/{id}/messages/{id}", "AuthenticationCoroFilter", Delete);

    ADD_METHOD_TO(Messages::Test, "/test", Get);
    METHOD_LIST_END

    Messages();
    Task<HttpResponsePtr> CreateOne(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> GetMultipleByRoomId(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> UpdateOne(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> DeleteOne(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> Test(HttpRequestPtr req);
};

} // namespace server::api
