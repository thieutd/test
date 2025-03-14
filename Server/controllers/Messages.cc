#include "Messages.h"

#include "models/Helper.h"
#include "utilities/HttpResponseUtil.h"

using namespace server::models;
using namespace server::api;

Messages::Messages() : RestfulController({})
{
}

Task<HttpResponsePtr> Messages::CreateOne(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Messages::GetMultipleByRoomId(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Messages::UpdateOne(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Messages::DeleteOne(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Messages::Test(HttpRequestPtr req)
{
    auto db_client = app().getDbClient();
    auto result = co_await db_client->execSqlCoro(R"(SELECT 
    *
FROM 
    "user" u
JOIN 
    "room_membership" rm ON u.id = rm.user_id
JOIN 
    "room" r ON rm.room_id = r.id;)");

    Json::Value ret;
    ret.resize(0);
    //for (const auto &row : result)
    //{
    //    Json::Value json;
    //    UserRoomMembershipRoom user_room_membership_room;
    //    user_room_membership_room << row;
    //    json[User::tableName] = std::get<User>(user_room_membership_room).toJson();
    //    json[RoomMembership::tableName] = std::get<RoomMembership>(user_room_membership_room).toJson();
    //    json[Room::tableName] = std::get<Room>(user_room_membership_room).toJson();
    //    ret.append(json);
    //}

    co_return HttpResponse::newHttpJsonResponse(ret);
}

