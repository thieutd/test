#include "Users.h"

#include "models/CommonRoomsView.h"
#include "models/Helper.h"
#include "plugins/PasswordHasher.h"
#include "plugins/RedisManager.h"
#include "utilities/FormatterUtil.h"
#include "utilities/HttpResponseUtil.h"

using namespace server::api;
using namespace server::models;

Users::Users() : RestfulController(kUserFields)
{
    enableMasquerading(kUserInfoFields);
}

Task<HttpResponsePtr> Users::CreateOne(const HttpRequestPtr req)
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

    if (!User::validateMasqueradedJsonForCreation(*json_ptr, kUserCreationByAdminFields,
                                                  err))
    {
        co_return utilities::NewJsonErrorResponse(k400BadRequest, err);
    }

    User user{*json_ptr};
    if (auto password_hash_result = app().getPlugin<PasswordHasher>()->HashPassword(user.getValueOfPassword());
        !password_hash_result)
    {
        LOG_ERROR << fmt::format("{}", password_hash_result.error());
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kInternalServerError>(password_hash_result.error());
    }
    else
    {
        user.setPassword(std::move(password_hash_result).value());
    }

    CoroMapper<User> mapper{app().getDbClient()};

    try
    {
        user = co_await mapper.insert(user);
        co_return HttpResponse::newHttpJsonResponse(makeJson(req, user));
    }
    catch (DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>(e.base().what());
    }
}

Task<HttpResponsePtr> Users::GetOne(const HttpRequestPtr req, const User::PrimaryKeyType id)
{
    CoroMapper<User> mapper{app().getDbClient()};

    try
    {
        const auto user = co_await mapper.findByPrimaryKey(id);
        auto json = makeJson(req, user);
        if (auto user_last_online_result = co_await app().getPlugin<RedisManager>()->GetUserLastOnline(user.getValueOfId());
            user_last_online_result)
        {
            json["last_online"] = Json::Value(Json::nullValue);
            if (auto last_online = user_last_online_result.value(); last_online)
            {
                json["last_online"] = utilities::ToSeconds(last_online.value().time_since_epoch());
            }
        }

        auto current_user_id = req->getAttributes()->get<User::PrimaryKeyType>("id");
        if (current_user_id == id)
        {
            co_return HttpResponse::newHttpJsonResponse(std::move(json));
        }
        CoroMapper<CommonRoomsView> common_room_mapper{app().getDbClient()};
        Criteria criteria = Criteria{CommonRoomsView::Cols::_user1_id, CompareOperator::EQ, current_user_id} &&
                            Criteria{CommonRoomsView::Cols::_user2_id, CompareOperator::EQ, id};
        auto total = co_await common_room_mapper.count(criteria);
        size_t limit = 50;
        common_room_mapper.limit(limit);
        auto common_rooms = co_await common_room_mapper.findBy(criteria);
        auto &metadata = json["common_rooms"]["metadata"];
        metadata["total"] = total;
        metadata["count"] = common_rooms.size();
        metadata["limit"] = limit;
        auto& data = json["common_rooms"]["data"];
        data.resize(0);
        for (const auto &common_room : common_rooms)
        {
            data.append(common_room.toMasqueradedJson(kCommonRoomsViewResultFields));
        }
        co_return HttpResponse::newHttpJsonResponse(std::move(json));
    }
    catch (DrogonDbException &e)
    {
        if (dynamic_cast<const UnexpectedRows *>(&e.base()))
        {
            co_return utilities::NewJsonErrorResponse(k404NotFound, "User not found");
        }
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }
}

Task<HttpResponsePtr> Users::GetCurrent(const HttpRequestPtr req)
{
    try
    {
        auto user_id = req->getAttributes()->get<User::PrimaryKeyType>("id");
        co_return co_await GetOne(req, std::move(user_id));
    }
    catch (std::exception &e)
    {
        LOG_ERROR << e.what();
        co_return utilities::NewJsonErrorResponse(k500InternalServerError, "Internal server error");
    }
}

Task<HttpResponsePtr> Users::GetList(const HttpRequestPtr req)
{
    CoroMapper<User> mapper{app().getDbClient()};
    
    const auto &parameters = req->parameters();
    if (const auto it = parameters.find("sort"); it != parameters.end())
    {
        for (auto sort_fields = utils::splitString(it->second, ","); auto &field : sort_fields)
        {
            if (field.empty())
                continue;
            if (field[0] == '+')
            {
                field = field.substr(1);
                mapper.orderBy(field, SortOrder::ASC);
            }
            else if (field[0] == '-')
            {
                field = field.substr(1);
                mapper.orderBy(field, SortOrder::DESC);
            }
            else
            {
                mapper.orderBy(field, SortOrder::ASC);
            }
        }
    }

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

    auto criteria = Criteria{User::Cols::_deleted_at, CompareOperator::IsNull};
    if (const auto it = parameters.find("username"); it != parameters.end())
    {
        criteria = criteria && Criteria{User::Cols::_username, CompareOperator::Like, it->second};
    }

    // if (const auto &json_ptr = req->jsonObject(); json_ptr && json_ptr->isMember("filter"))
    // {
    //     try
    //     {
    //         criteria_opt = makeCriteria((*json_ptr)["filter"]);
    //     }
    //     catch (const std::exception &e)
    //     {
    //         LOG_ERROR << e.what();
    //         co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid filter");
    //     }
    // }

    try
    {
        auto total = co_await mapper.count(criteria);
        const auto users = co_await mapper.findBy(criteria);
        Json::Value ret;
        auto &users_array = ret["data"];
        users_array.resize(0);
        if (!users.empty())
        {
            std::vector<User::PrimaryKeyType> user_ids;
            user_ids.reserve(users.size());
            for (const auto &user : users)
            {
                user_ids.push_back(user.getValueOfId());
                users_array.append(makeJson(req, user));
                users_array.back()["last_online"] = Json::Value(Json::nullValue);
            }

            auto get_online_statuses_result = co_await app().getPlugin<RedisManager>()->GetUsersLastOnline(user_ids);
            if (!get_online_statuses_result)
            {
                LOG_ERROR << fmt::format("{}", get_online_statuses_result.error());
            }
            else
            {
                auto online_statuses = get_online_statuses_result.value();
                auto it1 = users_array.begin();
                auto it2 = online_statuses.begin();
                for (; it1 != users_array.end(); ++it1, ++it2)
                {
                    if (auto last_online = *it2; last_online)
                    {
                        (*it1)["last_online"] = utilities::ToSeconds(last_online.value().time_since_epoch());
                    }
                }
            }
        }
        
        ret["metadata"]["total"] = total;
        ret["metadata"]["offset"] = offset;
        ret["metadata"]["limit"] = limit;

        co_return HttpResponse::newHttpJsonResponse(std::move(ret));
    }
    catch (DrogonDbException &e)
    {
        LOG_ERROR << e.base().what();
        co_return utilities::NewJsonErrorResponse<HttpErrorCode::kDatabaseError>();
    }
}

Task<HttpResponsePtr> Users::UpdateOne(const HttpRequestPtr req, const User::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Users::UpdateCurrent(const HttpRequestPtr req)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> Users::DeleteOne(const HttpRequestPtr req, const User::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}
