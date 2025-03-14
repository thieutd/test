#pragma once
#include "models/User.h"

#include <drogon/WebSocketController.h>
#include <shared_mutex>

namespace server::ws
{
using namespace drogon;
using namespace drogon_model::postgres;

class ChatSocketController : public WebSocketController<ChatSocketController>,
                             public std::enable_shared_from_this<ChatSocketController>
{
  public:
    ChatSocketController();
    void handleNewMessage(const WebSocketConnectionPtr &, std::string &&, const WebSocketMessageType &) override;
    void handleNewConnection(const HttpRequestPtr &, const WebSocketConnectionPtr &) override;
    void handleConnectionClosed(const WebSocketConnectionPtr &) override;
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/chat", "AuthenticationCoroFilter");
    WS_PATH_LIST_END

  private:
    using RefreshTokenToWebSocketConn = std::unordered_map<std::string, WebSocketConnectionPtr>;
    std::unordered_map<User::PrimaryKeyType, RefreshTokenToWebSocketConn> websocket_connections_;
    std::shared_mutex websocket_connections_mutex_;
    std::chrono::seconds user_online_update_interval_;
    std::shared_ptr<nosql::RedisSubscriber> refresh_token_change_subscriber_;
};

} // namespace server::ws