//
// Created by user on 03/05/2025.
//

//#include <ixwebsocket/ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocket.h>

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

    bool initialize(const std::string& url, int ref_1, int ref_2);
    void send_message(const std::string& message) {
      this->websocket_client.send(message);
    };

    void stop() {
        this->websocket_client.stop();
    }
};
