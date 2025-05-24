#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <string>

std::vector<ENetPeer*> connectedPeers;
bool gameStarted = false;
std::string gameServerAddress = "gameserver:127.0.0.1:10999";

int main(int argc, const char** argv) {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 10887;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server) {
        std::cerr << "Failed to create ENet server." << std::endl;
        return EXIT_FAILURE;
    }

    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_CONNECT) {
                connectedPeers.push_back(event.peer);
                if (gameStarted) {
                    ENetPacket* packet = enet_packet_create(
                        gameServerAddress.c_str(),
                        gameServerAddress.size() + 1,
                        ENET_PACKET_FLAG_RELIABLE
                    );
                    enet_peer_send(event.peer, 0, packet);
                }
            } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                std::string msg(reinterpret_cast<char*>(event.packet->data));
                if (msg == "start" && !gameStarted) {
                    gameStarted = true;
                    for (ENetPeer* peer : connectedPeers) {
                        ENetPacket* packet = enet_packet_create(
                            gameServerAddress.c_str(),
                            gameServerAddress.size() + 1,
                            ENET_PACKET_FLAG_RELIABLE
                        );
                        enet_peer_send(peer, 0, packet);
                    }
                    std::cout << "Game started, sent game server info to all clients." << std::endl;
                }
                enet_packet_destroy(event.packet);
            } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                std::cout << "Client disconnected." << std::endl;
            }
        }
    }

    enet_host_destroy(server);
    return 0;
}

