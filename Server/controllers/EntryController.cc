#include "EntryController.h"
#include "config.h"
#include "utilities/HttpResponseUtil.h"

using namespace server;

void EntryController::asyncHandleHttpRequest(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback)
{
    callback(utilities::NewHttpResponse(k200OK, constants::kName, constants::kVersion, std::format("(drogon/{})", drogon::getVersion())));
}
