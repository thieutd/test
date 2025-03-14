#include "RoomMembers.h"
#include "utilities/HttpResponseUtil.h"

using namespace server::api;

RoomMembers::RoomMembers() : RestfulController({})
{
}

//Task<HttpResponsePtr> RoomMembers::GetMultipleByRoomId(const HttpRequestPtr req, const Room::PrimaryKeyType id)
//{
//    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
//}

Task<HttpResponsePtr> RoomMembers::Join(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}

Task<HttpResponsePtr> RoomMembers::Leave(const HttpRequestPtr req, const Room::PrimaryKeyType id)
{
    co_return utilities::NewJsonErrorResponse(k501NotImplemented);
}
