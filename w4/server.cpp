#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <cmath>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

float rand_range(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

static uint16_t create_random_entity()
{
  uint16_t newEid = entities.size();
  uint32_t color = 0xff000000 +
                   0x00440000 * (1 + rand() % 4) +
                   0x00004400 * (1 + rand() % 4) +
                   0x00000044 * (1 + rand() % 4);
  float x = (rand() % 40 - 20) * 5.f;
  float y = (rand() % 40 - 20) * 5.f;
  float size = rand_range(3.f, 10.f);
  Entity ent = {color, x, y, newEid, false, 0.f, 0.f, size};
  entities.push_back(ent);
  return newEid;
}

constexpr float ENTITY_SIZE_LOWER_LIMIT = 3.f;
constexpr float ENTITY_SIZE_UPPER_LIMIT = 200.f;
constexpr float EPS = 1e-6;
void consume(Entity& eater, Entity& food) {
    eater.size += food.size * 0.5f;
    if (eater.size > ENTITY_SIZE_UPPER_LIMIT) eater.size = ENTITY_SIZE_UPPER_LIMIT;
    
    food.x = rand_range(-400.f, 400.f);
    food.y = rand_range(-300.f, 300.f);
    food.size /= 2;
    // eps helps to avoid being stuck ar lower limit
    if (food.size < ENTITY_SIZE_LOWER_LIMIT) food.size = ENTITY_SIZE_LOWER_LIMIT + rand_range(0.f, EPS);

    if (eater.serverControlled && food.serverControlled) return;

    for (const auto& pair : controlledMap) {
        ENetPeer* peer = pair.second;
        if (peer) { 
            send_score_update(peer, eater.eid, eater.size);
            send_score_update(peer, food.eid, food.size);
        }
    }
}

void check_collisions(ENetHost* server) {
    for (size_t i = 0; i < entities.size(); ++i) {
        for (size_t j = i + 1; j < entities.size(); ++j) {
            Entity& a = entities[i];
            Entity& b = entities[j];

            float dx = a.x - b.x;
            float dy = a.y - b.y;
            if (fabs(dx) < a.size + b.size && fabs(dy) < a.size + b.size) {
	
	        if (a.size > b.size) consume(a, b);
                else if (b.size > a.size) consume(b, a);
		

                for (size_t i = 0; i < server->peerCount; ++i) {
                    ENetPeer *peer = &server->peers[i];
                    send_snapshot(peer, a.eid, a.x, a.y, a.size);
                    send_snapshot(peer, b.eid, b.x, b.y, b.size);
                }
            }
        }
    }
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t newEid = create_random_entity();
  const Entity& ent = entities[newEid];

  controlledMap[newEid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  float size = 0.f;
  deserialize_entity_state(packet, eid, x, y, size);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      e.size = size;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  constexpr int numAi = 10;

  srand(time(nullptr));
  for (int i = 0; i < numAi; ++i)
  {
    uint16_t eid = create_random_entity();
    entities[eid].serverControlled = true;
    controlledMap[eid] = nullptr;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    check_collisions(server);

    for (Entity &e : entities)
    {
      if (e.serverControlled)
      {
        const float diffX = e.targetX - e.x;
        const float diffY = e.targetY - e.y;
        const float dirX = diffX > 0.f ? 1.f : -1.f;
        const float dirY = diffY > 0.f ? 1.f : -1.f;
        constexpr float spd = 50.f;
        e.x += dirX * spd * dt;
        e.y += dirY * spd * dt;
        if (fabsf(diffX) < 10.f && fabsf(diffY) < 10.f)
        {
          e.targetX = (rand() % 40 - 20) * 15.f;
          e.targetY = (rand() % 40 - 20) * 15.f;
        }
      }
    }
    for (const Entity &e : entities)
    {
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.x, e.y, e.size);
      }
    }
    //usleep(400000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}
