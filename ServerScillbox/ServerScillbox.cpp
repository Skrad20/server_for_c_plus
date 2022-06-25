#include <iostream>
#include <string>
#include <uwebsockets/App.h>
// https://github.com/uNetworking/uWebSockets
#include <nlohmann/json.hpp>
#include <map>
using namespace std;
using json = nlohmann::json;

struct UserData {
    int id;
    string name;
};

typedef uWS::WebSocket < false, true, UserData> websock;

// map all user 
map<int, UserData*> all_user;

void processPublicMessage(websock* ws, json parsed) {
    UserData* data = ws->getUserData();
    json payload;
    payload["command"] = "public_msg";
    payload["text"] = parsed["text"];
    payload["user_from"] = data->id;
    payload["name"] = data->name;
    ws->publish("PUBLIC", payload.dump());
    cout << parsed["text"] << endl;
}

void processPrivateMessage(websock* ws, json parsed) {
    UserData* data = ws->getUserData();
    json payload;
    payload["command"] = "private_msg";
    payload["user_from"] = data->id;

    payload["name"] = data->name;
    payload["user_to"] = parsed["user_to"];
    payload["text"] = parsed["text"];
    ws->publish("USER" + to_string(parsed["user_to"]), payload.dump());
    cout << "Private " << parsed["text"] << endl;
}

void processPrivateMessageErrorData(websock* ws) {
    UserData* data = ws->getUserData();
    json payload;
    payload["command"] = "error";
    payload["text"] = "Error input data";
    ws->send(payload.dump(), uWS::OpCode::TEXT);
    cout << "Private " << payload["text"] << endl;
}



string user_info(UserData* data, bool online) {
    /*
    * STATUS
    * {"command": "user_info", "online": True/False, "user_id" : 20, "name": "Jon"}
    */
    json payload;
    payload["command"] = "user_info";
    payload["user_in"] = data->id;
    payload["name"] = data->name;
    payload["online"] = online;
    return payload.dump();
}

void processSetName(websock* ws, json parsed) {
    UserData* data = ws->getUserData();
    data->name = parsed["name"];
    ws->publish("PUBLIC", user_info(data, true));
}

int main()
{

    unsigned int latest_user_id = 10;
    // http server
    // wesocket - real-time
    uWS::App app = uWS::App();
        app.get("/hello", [](auto* res, auto* req) {

            /* You can efficiently stream huge files too */
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end("Hello HTTP, skillbox!");

            }).ws<UserData>("/*", {

                /* Just a few of the available handlers */
                .open = [&latest_user_id](auto* ws) {
                    UserData* data = ws->getUserData();
                    latest_user_id++;
                    data->id = latest_user_id;
                    cout << "USER id "<< data->id << endl;

                    ws->subscribe("USER" + to_string(data->id));
                    ws->subscribe("PUBLIC");

                    bool pub_res = ws->publish("PUBLIC", user_info(data, true));
                    if (!pub_res) {
                        cout << "Falied to publish" << endl;
                    }
                    for (auto elem : all_user) {
                        ws->send(user_info(elem.second, true), uWS::OpCode::TEXT);
                    }


                    all_user[data->id] = data;
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
                    } catch (const json::parse_error& e) {
                        cout << "Cannot pares data" << endl;
                        cout << e.what() << endl;
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
                    * {"command": "user_info", "online": True/False, "user_id" : 20, "name": "Jon"}
                    */

                    /*
                    * Set name
                    * {"command": "set_name", "name": "Joni"}
                    */
                    if (command == "set_name") {
                        processSetName(ws, parsed);
                    }
                }, 
                .close =[&app](auto* ws, int, std::string_view){
                    // user 
                    UserData* data = ws->getUserData();
                    app.publish("PUBLIC", user_info(data, false), uWS::OpCode::TEXT);
                    all_user.erase(data->id);
                }

                }).listen(9001, [](auto* listenSocket) {

                    if (listenSocket) {
                        std::cout << "Listening on port " << "http://localhost/" << 9001 << std::endl;
                    }

                }).run(); std::cout << "Hello World!\n";
}
