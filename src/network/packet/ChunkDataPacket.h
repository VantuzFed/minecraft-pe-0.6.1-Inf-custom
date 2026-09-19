#ifndef NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__

#include "../Packet.h"
#include "../../world/level/chunk/LevelChunk.h"

class ChunkDataPacket : public Packet
{
public:

	int x, z;
	RakNet::BitStream chunkData;
	LevelChunk* chunk;
	// Full send: every column goes out, not just updateMap-dirty ones.
	// Used for initial chunk sync so the client renders exactly the
	// server blocks even if its local prediction diverged (other
	// generator build, seed or version). Live edits keep using deltas
	// via UpdateBlockPacket.
	bool full;

	ChunkDataPacket()
	:	chunk(NULL),
		full(false)
	{
	}

	ChunkDataPacket(int x, int z, LevelChunk* chunk, bool full = false)
	:	x(x),
		z(z),
		chunk(chunk),
		full(full)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CHUNKDATA));

		bitStream->Write(x);
		bitStream->Write(z);

		unsigned char* blockIds = chunk->getBlockData();
		DataLayer& blockData = chunk->data;

		const int setSize = LEVEL_HEIGHT / 8;
		const int setShift = 4; // power of LEVEL_HEIGHT / 8

		chunkData.Reset();
		for (int i = 0; i < CHUNK_COLUMNS; i++)
		{
			unsigned char updateBits = full ? 0xFF : chunk->updateMap[i];
			chunkData.Write(updateBits);

			if (updateBits > 0)
			{
				int colDataPosition = (i % CHUNK_WIDTH) << 11 | (i / CHUNK_WIDTH) << 7;

				for (int set = 0; set < 8; set++)
				{
					if ((updateBits & (1 << set)) != 0) 
					{
						chunkData.Write((const char*)(&blockIds[colDataPosition + (set << setShift)]), setSize);
						// block data is only 4 bits per block
						chunkData.Write((const char*)(&blockData.data[(colDataPosition + (set << setShift)) >> 1]), setSize >> 1);
					}
				}
			}
		}

		bitStream->Write(chunkData);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(z);
		bitStream->Read(chunkData);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (ChunkDataPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__*/
