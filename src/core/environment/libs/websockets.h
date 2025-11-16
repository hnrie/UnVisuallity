//
// Created by user on 03/05/2025.
//

//#include <ixwebsocket/ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocket.h>

/**
 * @brief Wraps a Lua-visible websocket client and its callbacks.
 */
class websocket_object {
private:

    std::string url;
public:
    ix::WebSocket websocket_client;
    lua_State *L;
    int L_ref;

    bool closed;

    int on_message_ref;
    int on_close_ref;

    /**
     * @brief Initializes the websocket connection and callback references.
     *
     * @param url Destination websocket URL.
     * @param ref_1 Lua reference to the message handler.
     * @param ref_2 Lua reference to the close handler.
     * @return bool True when initialization succeeds.
     */
    bool initialize(const std::string& url, int ref_1, int ref_2);

    /**
     * @brief Sends a message through the websocket connection.
     *
     * @param message Serialized payload forwarded to the remote server.
     */
    void send_message(const std::string& message) {
      this->websocket_client.send(message);
    };

    /**
     * @brief Stops the websocket client and tears down callbacks.
     */
    void stop() {
        this->websocket_client.stop();
    }
};
