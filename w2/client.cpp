#include <enet/enet.h>
#include <raylib.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cstring>

struct Player {
    int id;
    std::string name;
    float x, y;
    uint32_t ping;
};

std::unordered_map<int, Player> players;
int myId = -1;
std::string myName;
float posX = 100.0f, posY = 100.0f;
ENetPeer* gamePeer = nullptr;
bool gameStarted = false;

void sendStartCommand(ENetPeer* lobbyPeer) {
    const char* msg = "start";
    ENetPacket* packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(lobbyPeer, 0, packet);
}

void connectToGameServer(ENetHost* client, const std::string& ip, enet_uint16 port) {
    ENetAddress address;
    enet_address_set_host(&address, ip.c_str());
    address.port = port;
    gamePeer = enet_host_connect(client, &address, 2, 0);
    if (!gamePeer) {
        std::cerr << "Failed to connect to game server." << std::endl;
    }
}

void sendPosition() {
    if (!gamePeer) return;
    std::ostringstream oss;
    oss << "pos:" << myId << ":" << static_cast<int>(posX) << ":" << static_cast<int>(posY);
    std::string msg = oss.str();
    ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(gamePeer, 1, packet);
}

void drawPlayers() {
    for (const auto& [id, player] : players) {
        if (id == myId) continue;
        DrawCircle(static_cast<int>(player.x), static_cast<int>(player.y), 10, RED);
        DrawText(player.name.c_str(), static_cast<int>(player.x) + 12, static_cast<int>(player.y) - 10, 10, WHITE);
        DrawText(TextFormat("Ping: %d", player.ping), static_cast<int>(player.x) + 12, static_cast<int>(player.y) + 2, 10, WHITE);
    }
}

int main(int argc, const char** argv) {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetHost* client = enet_host_create(nullptr, 2, 2, 0, 0);
    if (!client) {
        std::cerr << "Failed to create ENet client." << std::endl;
        return EXIT_FAILURE;
    }

    ENetAddress lobbyAddress;
    enet_address_set_host(&lobbyAddress, "localhost");
    lobbyAddress.port = 10887;
    ENetPeer* lobbyPeer = enet_host_connect(client, &lobbyAddress, 2, 0);
    if (!lobbyPeer) {
        std::cerr << "Failed to connect to lobby server." << std::endl;
        return EXIT_FAILURE;
    }

    InitWindow(800, 600, "ENet Client");
    SetTargetFPS(60);

    uint32_t lastSendTime = enet_time_get();

    while (!WindowShouldClose()) {
        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                std::string data(reinterpret_cast<char*>(event.packet->data));
                if (data.rfind("gameserver:", 0) == 0) {
                    std::string addr = data.substr(11);
                    size_t colonPos = addr.find(':');
                    if (colonPos != std::string::npos) {
                        std::string ip = addr.substr(0, colonPos);
                        enet_uint16 port = static_cast<enet_uint16>(std::stoi(addr.substr(colonPos + 1)));
                        connectToGameServer(client, ip, port);
                        gameStarted = true;
                    }
                } else if (data.rfind("id:", 0) == 0) {
                    std::istringstream iss(data);
                    std::string token;
                    std::getline(iss, token, ':'); // "id"
                    std::getline(iss, token, ':');
                    myId = std::stoi(token);
                    std::getline(iss, myName);
                    players[myId] = { myId, myName, posX, posY, 0 };
                } else if (data.rfind("playerlist:", 0) == 0) {
                    std::string list = data.substr(11);
                    std::istringstream iss(list);
                    std::string entry;
                    while (std::getline(iss, entry, ',')) {
                        size_t colonPos = entry.find(':');
                        if (colonPos != std::string::npos) {
                            int id = std::stoi(entry.substr(0, colonPos));
                            std::string name = entry.substr(colonPos + 1);
                            if (players.find(id) == players.end()) {
                                players[id] = { id, name, 0.0f, 0.0f, 0 };
                            }
                        }
                    }
                } else if (data.rfind("pos:", 0) == 0) {
                    std::istringstream iss(data);
                    std::string token;
                    std::getline(iss, token, ':'); // "pos"
                    std::getline(iss, token, ':');
                    int id = std::stoi(token);
                    std::getline(iss, token, ':');
                    float x = std::stof(token);
                    std::getline(iss, token);
                    float y = std::stof(token);
                    if (players.find(id) != players.end()) {
                        players[id].x = x;
                        players[id].y = y;
                    }
                } else if (data.rfind("ping:", 0) == 0) {
                    std::istringstream iss(data);
                    std::string token;
                    std::getline(iss, token, ':'); // "ping"
                    std::getline(iss, token, ':');
                    int id = std::stoi(token);
                    std::getline(iss, token);
                    uint32_t ping = static_cast<uint32_t>(std::stoi(token));
                    if (players.find(id) != players.end()) {
                        players[id].ping = ping;
                    }
                }
                enet_packet_destroy(event.packet);
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            sendStartCommand(lobbyPeer);
        }

        float speed = 100.0f * GetFrameTime();
        if (IsKeyDown(KEY_RIGHT)) posX += speed;
        if (IsKeyDown(KEY_LEFT)) posX -= speed;
        if (IsKeyDown(KEY_DOWN)) posY += speed;
        if (IsKeyDown(KEY_UP)) posY -= speed;

        if (gameStarted && enet_time_get() - lastSendTime > 100) {
            sendPosition();
            lastSendTime = enet_time_get();
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText(TextFormat("My ID: %d", myId), 10, 10, 20, WHITE);
        DrawText(TextFormat("My Name: %s", myName.c_str()), 10, 30, 20, WHITE);
        DrawCircle(static_cast<int>(posX), static_cast<int>(posY), 10, GREEN);
        drawPlayers();
        EndDrawing();
    }

    CloseWindow();
    enet_host_destroy(client);
    return 0;
}

