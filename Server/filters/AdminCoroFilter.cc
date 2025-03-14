/**
 *
 *  AdminCoroFilter.cc
 *
 */

#include "AdminCoroFilter.h"
#include "utilities/HttpResponseUtil.h"

using namespace drogon;

Task<HttpResponsePtr> AdminCoroFilter::doFilter(const HttpRequestPtr &req)
{
    if (req->getAttributes()->get<std::string>("role") == "admin")
    {
        co_return nullptr;
    }
    co_return server::utilities::NewJsonErrorResponse<server::HttpErrorCode::kPermissionDeniedError>();
}
