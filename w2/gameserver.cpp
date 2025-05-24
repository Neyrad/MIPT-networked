#include <enet/enet.h>
#include <iostream>
#include <unordered_map>
#include <sstream>

struct PlayerInfo {
    std::string name;
    float x = 100.0f, y = 100.0f;
};

std::unordered_map<ENetPeer*, int> peerToId;
std::unordered_map<int, PlayerInfo> players;
int nextId = 1;

void broadcastPlayerList(ENetHost* server) {
    std::ostringstream oss;
    oss << "playerlist:";
    bool first = true;
    for (const auto& [id, info] : players) {
        if (!first) oss << ",";
        oss << id << ":" << info.name;
        first = false;
    }
    std::string msg = oss.str();
    ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

void broadcastPosition(int id, float x, float y, ENetHost* server) {
    std::ostringstream oss;
    oss << "pos:" << id << ":" << static_cast<int>(x) << ":" << static_cast<int>(y);
    std::string msg = oss.str();
    ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_host_broadcast(server, 1, packet);
}

void broadcastPing(ENetHost* server) {
    for (const auto& [peer, id] : peerToId) {
        if (!peer) continue;
        std::ostringstream oss;
        oss << "ping:" << id << ":" << peer->roundTripTime;
        std::string msg = oss.str();
        ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
        enet_host_broadcast(server, 1, packet);
    }
}

int main(int argc, const char** argv) {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 10999;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server) {
        std::cerr << "Failed to create game server." << std::endl;
        return EXIT_FAILURE;
    }

    uint32_t lastPingTime = enet_time_get();

    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0) {
            if (event.type == ENET_EVENT_TYPE_CONNECT) {
                int id = nextId++;
                std::string name = "Player" + std::to_string(id);
                peerToId[event.peer] = id;
                players[id] = { name };
                std::string msg = "id:" + std::to_string(id) + ":" + name;
                ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 0, packet);
                broadcastPlayerList(server);
                std::cout << "New player connected: " << name << std::endl;
            } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                std::string msg(reinterpret_cast<char*>(event.packet->data));
                if (msg.rfind("pos:", 0) == 0) {
                    std::istringstream iss(msg);
                    std::string token;
                    std::getline(iss, token, ':');
                    std::getline(iss, token, ':');
                    int id = std::stoi(token);
                    std::getline(iss, token, ':');
                    float x = std::stof(token);
                    std::getline(iss, token);
                    float y = std::stof(token);
                    if (players.count(id)) {
                        players[id].x = x;
                        players[id].y = y;
                        broadcastPosition(id, x, y, server);
                    }
                }
                enet_packet_destroy(event.packet);
            } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                int id = peerToId[event.peer];
                peerToId.erase(event.peer);
                players.erase(id);
                broadcastPlayerList(server);
                std::cout << "Player " << id << " disconnected." << std::endl;
            }
        }

        if (enet_time_get() - lastPingTime > 1000) {
            broadcastPing(server);
            lastPingTime = enet_time_get();
        }
    }

    enet_host_destroy(server);
    return 0;
}

