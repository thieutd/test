#include "Rooms.h"
#include "models/Helper.h"
#include "models/JoinedRoomsView.h"
#include "models/UserRoomsWithMessagesView.h"
#include "utilities/HttpResponseUtil.h"

using namespace server::api;
using namespace server::models;

Rooms::Rooms() : RestfulController(kRoomFields)
{
}

Task<HttpResponsePtr> Rooms::CreateOne(const HttpRequestPtr req)
{
    const auto &json_ptr = req->jsonObject();
    if (!json_ptr)
    {
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kNoJsonObjectError>();
    }

    std::string err;
    if (!doCustomValidations(*json_ptr, err))
    {
        co_return utilities::NewJsonErrorResponse(k400BadRequest, err);
    }

    if (!Room::validateMasqueradedJsonForCreation(
            *json_ptr, kRoomCreationFields, err))
    {
        co_return utilities::NewJsonErrorResponse(k400BadRequest, err);
    }

    Room room{*json_ptr};
    CoroMapper<Room> mapper{app().getDbClient()};

    try
    {
        room = co_await mapper.insert(room);
        co_return HttpResponse::newHttpJsonResponse(makeJson(req, room));
    }
    catch (DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>(e.base().what());
    }
}

Task<HttpResponsePtr> Rooms::GetOne(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    CoroMapper<Room> mapper{app().getDbClient()};
    try
    {
        auto rooms = co_await mapper.findBy(Criteria{Room::Cols::_id, CompareOperator::EQ, id} && 
                                          Criteria{Room::Cols::_deleted_at, CompareOperator::IsNull});
        if (rooms.empty())
        {
            co_return utilities::NewJsonErrorResponse(k404NotFound, "Room not found");
        }
        co_return HttpResponse::newHttpJsonResponse(makeJson(req, rooms[0]));
    }
    catch (const DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }
}

Task<HttpResponsePtr> Rooms::GetMultiple(const HttpRequestPtr req)
{
    CoroMapper<Room> mapper{app().getDbClient()};
    
    auto& parameters = req->getParameters();
    size_t offset = 0;
    if (const auto it = parameters.find("offset"); it != parameters.end())
    {
        try
        {
            offset = std::stoull(it->second);
        }
        catch (...)
        {
            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid offset");
        }
    }
    mapper.offset(offset);

    size_t limit = 100;
    if (const auto it = parameters.find("limit"); it != parameters.end())
    {
        try
        {
            limit = std::min(static_cast<size_t>(std::stoull(it->second)), limit);
        }
        catch (...)
        {
            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid limit");
        }
    }
    mapper.limit(limit);

    auto criteria = Criteria{Room::Cols::_deleted_at, CompareOperator::IsNull};
    if (const auto it = parameters.find("name"); it != parameters.end())
    {
        criteria = criteria && Criteria{Room::Cols::_name, CompareOperator::Like, it->second};
    }

    try
    {
        auto total = co_await mapper.count(criteria);
        auto rooms = co_await mapper.findBy(criteria);

        Json::Value ret;
        auto& data = ret["data"];
        for (const auto& room : rooms)
        {
            data.append(makeJson(req, room));
        }
        ret["metadata"]["total"] = total;
        ret["metadata"]["offset"] = offset;
        ret["metadata"]["limit"] = limit;
        co_return HttpResponse::newHttpJsonResponse(std::move(ret));
    }
    catch (const DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }
}

Task<HttpResponsePtr> Rooms::UpdateOne(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
 co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Rooms::DeleteOne(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
 co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Rooms::GetUserRooms(const HttpRequestPtr req, const User::PrimaryKeyType id)
{
    if (auto user_exists = co_await CoroMapper<User>{app().getDbClient()}.count(Criteria{User::Cols::_id, CompareOperator::EQ, id});
        user_exists == 0)
    {
        co_return utilities::NewJsonErrorResponse(k404NotFound, "User not found");
    }

    CoroMapper<JoinedRoomsView> mapper{app().getDbClient()};
    
    auto& parameters = req->getParameters();
    size_t offset = 0;
    if (const auto it = parameters.find("offset"); it != parameters.end())
    {
        try
        {
            offset = std::stoull(it->second);
        }
        catch (...)
        {
            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid offset");
        }
    }
    mapper.offset(offset);

    size_t limit = 100;
    if (const auto it = parameters.find("limit"); it != parameters.end())
    {
        try
        {
            limit = std::min(static_cast<size_t>(std::stoull(it->second)), limit);
        }
        catch (...)
        {
            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid limit");
        }
    }
    mapper.limit(limit);

    auto criteria = Criteria{JoinedRoomsView::Cols::_user_id, CompareOperator::EQ, id};
    auto total = co_await mapper.count(criteria);
    auto rooms = co_await mapper.findBy(criteria);
    
    Json::Value ret;
    auto& data = ret["data"];
    for (const auto& room : rooms)
    {
        auto json = room.toMasqueradedJson(kJoinedRoomsViewResultFields);
        json["membership"]["role"] = room.getValueOfRole();
        json["membership"]["joined_at"] = room.getValueOfJoinedAt().secondsSinceEpoch();
        data.append(std::move(json));
    }
    ret["metadata"]["total"] = total;
    ret["metadata"]["offset"] = offset;
    ret["metadata"]["limit"] = limit;
    co_return HttpResponse::newHttpJsonResponse(std::move(ret));
}

Task<HttpResponsePtr> Rooms::GetCurrentUserRooms(const HttpRequestPtr req)
{
    auto user_id = req->getAttributes()->get<User::PrimaryKeyType>("id");

    CoroMapper<UserRoomsWithMessagesView> mapper{app().getDbClient()};

    auto& parameters = req->getParameters();
    size_t offset = 0;
    if (const auto it = parameters.find("offset"); it != parameters.end())
    {
        try
        {
            offset = std::stoull(it->second);
        }
        catch (...)
        {
            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid offset");
        }
    }
    mapper.offset(offset);

    size_t limit = 100;
    if (const auto it = parameters.find("limit"); it != parameters.end())
    {
        try
        {
            limit = std::min(static_cast<size_t>(std::stoull(it->second)), limit);
        }
        catch (...)
        {
            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid limit");
        }
    }
    mapper.limit(limit);

    Criteria criteria = {UserRoomsWithMessagesView::Cols::_user_id, CompareOperator::EQ, user_id};
    if (const auto it = parameters.find("name"); it != parameters.end())
    {
        criteria = criteria && Criteria{UserRoomsWithMessagesView::Cols::_name, CompareOperator::Like, it->second};
    }

    auto total = co_await mapper.count(criteria);
    auto rooms = co_await mapper.findBy(Criteria{UserRoomsWithMessagesView::Cols::_user_id, CompareOperator::EQ, user_id});

    Json::Value ret;
    auto& data = ret["data"];
    for (const auto& room : rooms)
    {
        auto json = room.toJson();
        json.removeMember("user_id");
        if (auto &last_message = json["last_message"]; json["message_id"].isNull())
        {
            json.removeMember("message_id");
            json.removeMember("message_content");
            json.removeMember("message_created_at");
            json.removeMember("sender_id");
            json.removeMember("sender_username");
            json.removeMember("sender_avatar");
        }
        else
        {
            last_message["created_at"] = std::move(json["message_created_at"]);
            json.removeMember("message_created_at");
            last_message["id"] = std::move(json["message_id"]);
            json.removeMember("message_id");
            last_message["content"] = std::move(json["message_content"]);
            json.removeMember("message_content");

            last_message["sender"]["id"] = std::move(json["sender_id"]);
            json.removeMember("sender_id");
            last_message["sender"]["username"] = std::move(json["sender_username"]);
            json.removeMember("sender_username");
            last_message["sender"]["avatar_url"] = std::move(json["sender_avatar"]);
            json.removeMember("sender_avatar");
        }
        
        data.append(std::move(json));
    }

    ret["metadata"]["total"] = total;
    ret["metadata"]["offset"] = offset;
    ret["metadata"]["limit"] = limit;
    co_return HttpResponse::newHttpJsonResponse(std::move(ret));
}
