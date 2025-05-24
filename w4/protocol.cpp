#include <cstdint>
#include <enet/enet.h>
#include "entity.h"
#include "bitstream.h"
#include "protocol.h"

// ======== Serialization ========

void send_join(ENetPeer *peer)
{
  Bitstream bs;
  bs.write(E_CLIENT_TO_SERVER_JOIN);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.write(ent);
  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.write(eid);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y, float size)
{
  Bitstream bs;
  bs.write(E_CLIENT_TO_SERVER_STATE);
  bs.write(eid);
  bs.write(x);
  bs.write(y);
  bs.write(size);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), 0);
  enet_peer_send(peer, 0, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float size)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  bs.write(x);
  bs.write(y);
  bs.write(size);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), 0);   
  enet_peer_send(peer, 0, packet);
}

void send_score_update(ENetPeer* peer, uint16_t eid, float score) {
    Bitstream bs;
    bs.write(E_SERVER_TO_CLIENT_SCORE); 
    bs.write(eid);
    bs.write(score);
    ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), 0);
    enet_peer_send(peer, 0, packet);
}

// ======== Deserialization ========

MessageType get_packet_type(ENetPacket *packet)
{
  Bitstream bs(reinterpret_cast<uint8_t*>(packet->data), packet->dataLength);
  MessageType type;
  bs.read(type);
  return type;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  Bitstream bs(reinterpret_cast<uint8_t*>(packet->data), packet->dataLength);
  MessageType _;
  bs.read(_); // skip type
  bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  Bitstream bs(reinterpret_cast<uint8_t*>(packet->data), packet->dataLength);
  MessageType _;
  bs.read(_);
  bs.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &size)
{
  Bitstream bs(reinterpret_cast<uint8_t*>(packet->data), packet->dataLength);
  MessageType _;
  bs.read(_);
  bs.read(eid);
  bs.read(x);
  bs.read(y);
  bs.read(size);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &size)
{
  Bitstream bs(reinterpret_cast<uint8_t*>(packet->data), packet->dataLength);
  MessageType _;
  bs.read(_);
  bs.read(eid);
  bs.read(x);
  bs.read(y);
  bs.read(size);
}

void deserialize_score_update(ENetPacket *packet, uint16_t &eid, float &score)
{
  Bitstream bs(reinterpret_cast<uint8_t*>(packet->data), packet->dataLength);
  MessageType _;
  bs.read(_);
  bs.read(eid);
  bs.read(score);
}
