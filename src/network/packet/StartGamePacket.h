#ifndef NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__

#include "../Packet.h"
#include <cstdint>

class StartGamePacket : public Packet
{
public:
	// Full 64-bit world seed: beta worlds use the whole Java long range,
	// truncating it desyncs client-side terrain prediction (stripes).
	int64_t levelSeed;
	int levelGeneratorVersion;
	int gameType;

	int entityId;
	double x, y, z;

	StartGamePacket()
	{
	}

	StartGamePacket(int64_t seed, int levelGeneratorVersion, int gameType, int entityId, double x, double y, double z)
	:	levelSeed(seed),
		levelGeneratorVersion(levelGeneratorVersion),
		gameType(gameType),
		entityId(entityId),
		x(x),
		y(y),
		z(z)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_STARTGAME));

		bitStream->Write(levelSeed);
		bitStream->Write(levelGeneratorVersion);
		bitStream->Write(gameType);
		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(levelSeed);
		bitStream->Read(levelGeneratorVersion);
		bitStream->Read(gameType);
		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (StartGamePacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__*/
