#include <iostream>
#include <string>
#include <uwebsockets/App.h>
// https://github.com/uNetworking/uWebSockets
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

struct UserData {
    int id;
    string name;
};

typedef uWS::WebSocket < false, true, UserData> websock;

void processPublicMessage(websock* ws, json parsed) {
    UserData* data = ws->getUserData();
    json payload;
    payload["command"] = "public_msg";
    payload["text"] = parsed["text"];
    payload["user_from"] = data->id;
    ws->publish("public", payload.dump());
    cout << parsed["text"] << endl;
}

void processPrivateMessage(websock* ws, json parsed) {
    UserData* data = ws->getUserData();
    json payload;
    payload["command"] = "private_msg";
    payload["user_from"] = data->id;
    payload["user_to"] = parsed["user_to"];
    payload["text"] = parsed["text"];
    ws->publish("USER" + to_string(parsed["user_to"]), payload.dump());
    cout << "Private " << parsed["text"] << endl;
}

void processPrivateMessageErrorData(websock* ws) {
    UserData* data = ws->getUserData();
    json payload;
    payload["command"] = "erro";
    payload["text"] = "Error input data";
    ws->publish("USER" + to_string(data->id), payload.dump());
    cout << "Private " << payload["text"] << endl;
}

int main()
{

    unsigned int latest_user_id = 10;
    // http server
    // wesocket - real-time
    uWS::App()
        .get("/hello", [](auto* res, auto* req) {

            /* You can efficiently stream huge files too */
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end("Hello HTTP, skillbox!");

            }).ws<UserData>("/*", {

                /* Just a few of the available handlers */
                .open = [&latest_user_id](auto* ws) {
                    UserData* data = ws->getUserData();
                    latest_user_id++;
                    data->id = latest_user_id;
                    cout << "USER id "<< data->id << endl;
                    ws->subscribe("public");
                    ws->subscribe("USER" + to_string(data->id));
                },
                .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
                    UserData* data = ws->getUserData();
                    /* Тех сообщения
                    * Публичные сообщения
                    * client => server {"command": "public_msg", "text": "Hello, people!"}
                    * server => all client {
                        "command": "public_msg",
                        "text": "Hello, people!",
                        "user_from": 22
                    }*/
                    json parsed;
                    try {
                        parsed = json::parse(message);
                    } catch (exception) {
                        parsed["command"] = "error";
                    };

                    string command = parsed["command"];
                    if (command == "error") {
                        // STRAT NEW TREAD
                        processPrivateMessageErrorData(ws);
                    }

                    if (command == "public_msg") {
                        // STRAT NEW TREAD
                        processPublicMessage(ws, parsed);
                    }
                    /* {"command": "private_msg", }
                    * client => server {"command": "private_msg", "text": "Hello, people!", "user_to": 25}
                    * server => client {"command": "private_msg", "text": "Hello, people!", "user_from": 25}
                    */
                    if (command == "private_msg") {
                        processPrivateMessage(ws, parsed);
                    }
                    /*
                    * STATUS
                    * {"command": "status", "online": True/False, "user_id" : 20, "name": "Jon"}
                    */

                    /*
                    * Set name
                    * {"command": "set_name", "name": "Joni"}
                    */
                }

                }).listen(9001, [](auto* listenSocket) {

                    if (listenSocket) {
                        std::cout << "Listening on port " << 9001 << std::endl;
                    }

                }).run(); std::cout << "Hello World!\n";
}
