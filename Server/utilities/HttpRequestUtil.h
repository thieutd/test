#pragma once

namespace server
{
enum class QueryParameterErrorCode
{
    kSuccess,
    kInvalidLimit = 10,
    kInvalidOffset,
    kInvalidSort,
    kInvalidFilter
};
}

namespace std
{
template <> struct is_error_code_enum<server::QueryParameterErrorCode> : true_type
{
};
}

namespace
{
class QueryParameterErrorCategory : public std::error_category
{
  public:
    const char *name() const noexcept override
    {
        return "QueryParameterError";
    }
    std::string message(int ev) const override
    {
        switch (static_cast<server::QueryParameterError>(ev))
        {
        case server::QueryParameterError::kSuccess:
            return "Success";
        case server::QueryParameterError::kInvalidLimit:
            return "Invalid limit";
        case server::QueryParameterError::kInvalidOffset:
            return "Invalid offset";
        case server::QueryParameterError::kInvalidSort:
            return "Invalid sort";
        case server::QueryParameterError::kInvalidFilter:
            return "Invalid filter";
        default:
            return "Unknown error " + std::to_string(ev);
        }
    }
};
}

std::error_code make_error_code(server::QueryParameterError e)
{
    return {static_cast<int>(e), QueryParameterErrorCategory()};
}

namespace utilities
{
using namespace drogon;

//template <typename Mapper, size_t offset = 0, size_t limit = 100, bool allow_sort = false, bool allow_filter = false>
//std::optional<Criteria> ProcessQueryParameters(const HttpRequestPtr &req, Mapper &mapper)
//{
//    auto &parameters = req->parameters();
//
//    if (const auto it = parameters.find("offset"); it != parameters.end())
//    {
//        offset = std::stoull(it->second);
//    }
//    mapper.offset(offset);
//
//    if (const auto it = parameters.find("limit"); it != parameters.end())
//    {
//        limit = min(std::stoull(it->second), limit);
//    }
//    mapper.limit(limit);
//
//    if constexpr (allow_sort)
//    {
//        if (const auto it = parameters.find("sort"); it != parameters.end())
//        {
//            for (auto sort_fields = utils::splitString(it->second, ","); auto &field : sort_fields)
//            {
//                if (field.empty())
//                    continue;
//                if (field[0] == '+')
//                {
//                    field = field.substr(1);
//                    mapper.orderBy(field, SortOrder::ASC);
//                }
//                else if (field[0] == '-')
//                {
//                    field = field.substr(1);
//                    mapper.orderBy(field, SortOrder::DESC);
//                }
//                else
//                {
//                    mapper.orderBy(field, SortOrder::ASC);
//                }
//            }
//        }
//    }
//   
//   if constexpr (allow_filter)
//    {
//        if (const auto &json_ptr = req->jsonObject(); json_ptr && json_ptr->isMember("filter"))
//        {
//            return makeCriteria((*json_ptr)["filter"]);
//        }
//    }
//
//    if (const auto it = parameters.find("limit"); it != parameters.end())
//    {
//        try
//        {
//            limit = min(std::stoull(it->second), limit);
//        }
//        catch (...)
//        {
//            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid limit");
//        }
//    }
//    mapper.limit(limit);
//
//    std::optional<Criteria> criteria_opt;
//    if (const auto &json_ptr = req->jsonObject(); json_ptr && json_ptr->isMember("filter"))
//    {
//        try
//        {
//            criteria_opt = makeCriteria((*json_ptr)["filter"]);
//        }
//        catch (const std::exception &e)
//        {
//            LOG_ERROR << e.what();
//            co_return utilities::NewJsonErrorResponse(k400BadRequest, "Invalid filter");
//        }
//    }
//}
} // namespace server::utilities