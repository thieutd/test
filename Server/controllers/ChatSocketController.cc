#include "ChatSocketController.h"
#include "plugins/RedisManager.h"

using namespace server::ws;

struct ClientContext
{
    User::PrimaryKeyType user_id;
    std::chrono::system_clock::time_point last_online_update;
};

ChatSocketController::ChatSocketController()
{
    user_online_update_interval_ =
        std::chrono::seconds(app().getCustomConfig().get("user_online_update_interval", 2 * 60).asUInt());
    ASSERT(app().getPlugin<RedisManager>() != nullptr, "RedisManager plugin is not loaded");
    const auto redis_client = app().getRedisClient();
    try
    {
        auto success = redis_client->execCommandSync<bool>(
            [](const nosql::RedisResult &result) { return result.asString() == "OK"; },
            "CONFIG SET notify-keyspace-events Kxg");
        ASSERT(success, "Failed to set notify-keyspace-events");
    }
    catch (const drogon::nosql::RedisException &e)
    {
        ASSERT(true, e.what());
    }
    refresh_token_change_subscriber_ = redis_client->newSubscriber();
    auto redis_db_index = app().getCustomConfig()["redis_clients"].get("db_index", 0).asUInt();
    refresh_token_change_subscriber_->psubscribe(
        std::format("__keyspace@{}__:refresh_token:*", redis_db_index),
        [this](const std::string &channel, const std::string &message) {
            if (message == "expired" || message == "del")
            {
                std::array<std::string, 2> parts;
                std::ranges::copy(
                    channel | std::views::split(':') | std::views::drop(2) |
                        std::views::transform([](auto &&range) { return std::string(range.begin(), range.end()); }),
                    parts.begin());

                auto user_id = std::stoull(parts[0]);
                if (auto it = websocket_connections_.find(user_id);
                    it != websocket_connections_.end())
                {
                    if (auto it2 = std::ranges::find_if(it->second,
                                                        [&parts](const auto &pair) { return pair.first == parts[1]; });
                        it2 != it->second.end())
                    {
                        auto &ws_conn = it2->second;
                        Json::Value ret = Json::objectValue;
                        ret["type"] = "disconnect";
                        ret["message"] = "refresh token expired";
                        ws_conn->sendJson(ret);
                        ws_conn->forceClose();
                    }
                }
            }
        });
}

void ChatSocketController::handleNewMessage(const WebSocketConnectionPtr &wsConnPtr, std::string &&message,
                                            const WebSocketMessageType &type)
{
    std::shared_lock lock(websocket_connections_mutex_);
    const auto user_id = wsConnPtr->getContextRef<ClientContext>().user_id;
    if (type == WebSocketMessageType::Ping || type == WebSocketMessageType::Pong || type == WebSocketMessageType::Close)
    {
        if (auto &last_online_update_ = wsConnPtr->getContextRef<ClientContext>().last_online_update;
            std::chrono::system_clock::now() - last_online_update_ > user_online_update_interval_)
        {
            last_online_update_ = std::chrono::system_clock::now();
            app().getPlugin<RedisManager>()->SetUserLastOnline(user_id, last_online_update_);
        }
        return;
    }
    if (type != WebSocketMessageType::Text)
    {
        wsConnPtr->send("Invalid message type");
        return;
    }

    //for (const auto &[id, conn] : websocket_connections_)
    //{
    //    if (id != user_id)
    //    {
    //        conn->send(message);
    //    }
    //}
}

void ChatSocketController::handleNewConnection(const HttpRequestPtr &req, const WebSocketConnectionPtr &wsConnPtr)
{
    auto user_id = req->getAttributes()->get<User::PrimaryKeyType>("id");
    auto refresh_token_id = req->getAttributes()->get<std::string>("refresh_id");
    auto now = std::chrono::system_clock::now();
    app().getPlugin<RedisManager>()->SetUserLastOnline(user_id, now);
    wsConnPtr->setContext(std::make_shared<ClientContext>(ClientContext{user_id, std::move(now)}));

    {
        std::shared_lock read_lock(websocket_connections_mutex_);
        if (websocket_connections_.contains(user_id) && websocket_connections_[user_id].contains(refresh_token_id))
        {
            auto &old_ws_conn = websocket_connections_.at(user_id).at(refresh_token_id);
            Json::Value message;
            message["type"] = "disconnect";
            message["message"] = "duplicate connection";
            old_ws_conn->sendJson(message);
            old_ws_conn->forceClose();
            read_lock.unlock();
            std::unique_lock write_lock(websocket_connections_mutex_);
            websocket_connections_[user_id][refresh_token_id] = wsConnPtr;
            return;
        }
    }
    {
        std::unique_lock write_lock(websocket_connections_mutex_);
        websocket_connections_[user_id][refresh_token_id] = wsConnPtr;
    }
}

void ChatSocketController::handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr)
{
    auto user_id = wsConnPtr->getContextRef<ClientContext>().user_id;
    std::shared_lock read_lock(websocket_connections_mutex_);
    if (auto it = websocket_connections_.find(user_id); it != websocket_connections_.end())
    {
        if (auto it2 =
                std::ranges::find_if(it->second, [wsConnPtr](const auto &pair) { return pair.second == wsConnPtr; });
            it2 != it->second.end())
        {
            read_lock.unlock();
            std::unique_lock write_lock(websocket_connections_mutex_);
            it->second.erase(it2);
            if (it->second.empty())
            {
                websocket_connections_.erase(it);
            }
        }
    }
}
