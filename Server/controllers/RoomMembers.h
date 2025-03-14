#pragma once

#include "models/Room.h"
#include "models/User.h"

#include <drogon/orm/RestfulController.h>

namespace server::api
{
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::postgres;

class RoomMembers : public HttpController<RoomMembers>, public RestfulController
{
  public:
    METHOD_LIST_BEGIN
    // ADD_METHOD_TO(RoomMembers::GetList, "/rooms/{id}/members", "AuthenticationCoroFilter", Get, Options);
    ADD_METHOD_TO(RoomMembers::Join, "/rooms/{id}/join", "AuthenticationCoroFilter", Post, Options);
    
    // ADD_METHOD_TO(RoomMembers::Leave, "/rooms/{id}/leave", "AuthenticationCoroFilter", Post, Options);
    // ADD_METHOD_TO(RoomMembers::Invite, "/rooms/{id}/invite", "AuthenticationCoroFilter", Post, Options);
    // ADD_METHOD_TO(RoomMembers::Update, "/rooms/{id}/members/{member_id}", "AuthenticationCoroFilter", Put, Options);
    METHOD_LIST_END

    RoomMembers();
    // Task<HttpResponsePtr> GetList(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> Join(HttpRequestPtr req, Room::PrimaryKeyType id);
    Task<HttpResponsePtr> Leave(HttpRequestPtr req, Room::PrimaryKeyType id);
    // Task<HttpResponsePtr> Invite(HttpRequestPtr req, Room::PrimaryKeyType id);
    // Task<HttpResponsePtr> Update(HttpRequestPtr req, Room::PrimaryKeyType id, User::PrimaryKeyType member_id);
};

} // namespace server::api
