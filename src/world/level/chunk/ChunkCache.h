#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__

//package net.minecraft.world.level.chunk;

#include "ChunkSource.h"
#include "storage/ChunkStorage.h"
#include "EmptyLevelChunk.h"
#include "../Level.h"
#include "../LevelConstants.h"
#include "../ChunkPos.h"
#include "../../../platform/log.h"

#include <map>

class ChunkCache: public ChunkSource {
    //static const int CHUNK_CACHE_WIDTH = CHUNK_CACHE_WIDTH; // WAS 32;
    static const int MAX_SAVES = 2;
public:
    ChunkCache(Level* level_, ChunkStorage* storage_, ChunkSource* source_)
	:	xLast(-999999999),
		zLast(-999999999),
		last(NULL),
		level(level_),
		storage(storage_),
		source(source_)
	{
		isChunkCache = true;
        //emptyChunk = new EmptyLevelChunk(level_, emptyChunkBlocks, 0, 0);
		emptyChunk = new EmptyLevelChunk(level_, NULL, 0, 0);
    }

	~ChunkCache() {
		delete source;
		delete emptyChunk;

		for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); ++it) {
			if (it->second) {
				it->second->deleteBlockData();
				delete it->second;
			}
		}
	}

    bool fits(int x, int z) {
        return true;
    }

    bool hasChunk(int x, int z) {
        if (x == xLast && z == zLast && last != NULL) {
            return true;
        }
        ChunkMap::iterator it = chunks.find(ChunkPos(x, z));
        return it != chunks.end() && it->second != NULL && it->second->isAt(x, z);
    }

    LevelChunk* create(int x, int z) {
        return getChunk(x, z);
    }

    LevelChunk* getChunk(int x, int z) {
		//static Stopwatch sw;
		//sw.start();

		if (x == xLast && z == zLast && last != NULL) {
            return last;
        }
        ChunkPos pos(x, z);
        if (!hasChunk(x, z)) {
            MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk begin (%d,%d), loaded=%d\n", x, z, (int)chunks.size());
            LevelChunk* newChunk = load(x, z);
            MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk storage %s (%d,%d)\n", newChunk ? "hit" : "miss", x, z);
            if (newChunk == NULL) {
                if (source == NULL) {
                    MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk no source, using emptyChunk (%d,%d)\n", x, z);
                    newChunk = emptyChunk;
                } else {
                    MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk generating (%d,%d)\n", x, z);
                    newChunk = source->getChunk(x, z);
                    MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk generated %p (%d,%d)\n", newChunk, x, z);
                }
            }
            if (newChunk == NULL) {
                MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk source returned NULL, using emptyChunk (%d,%d)\n", x, z);
                newChunk = emptyChunk;
            }
            chunks[pos] = newChunk;
            if (newChunk != emptyChunk) {
                newChunk->lightLava();
            }

            if (chunks[pos] != NULL) {
                chunks[pos]->load();
            }

            MCPE_CRASH_TRACE("[IW] ChunkCache::getChunk installed (%d,%d), terrainPopulated=%d, fromSave=%d\n",
                x, z, chunks[pos] ? chunks[pos]->terrainPopulated : -1, chunks[pos] ? chunks[pos]->createdFromSave : -1);

            if (!chunks[pos]->terrainPopulated && hasChunk(x + 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x + 1, z)) postProcess(this, x, z);
            if (hasChunk(x - 1, z) && !getChunk(x - 1, z)->terrainPopulated && hasChunk(x - 1, z + 1) && hasChunk(x, z + 1)) postProcess(this, x - 1, z);
            if (hasChunk(x, z - 1) && !getChunk(x, z - 1)->terrainPopulated && hasChunk(x + 1, z - 1) && hasChunk(x + 1, z)) postProcess(this, x, z - 1);
            if (hasChunk(x - 1, z - 1) && !getChunk(x - 1, z - 1)->terrainPopulated && hasChunk(x, z - 1) && hasChunk(x - 1, z)) postProcess(this, x - 1, z - 1);
        }
        xLast = x;
        zLast = z;
        last = chunks[pos];

		//sw.stop();
		//sw.printEvery(500000, "ChunkCache::load: ");

        return chunks[pos];
    }

	Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
		return source->getMobsAt(mobCategory, x, y, z);
	}

    void postProcess(ChunkSource* parent, int x, int z) {
        LevelChunk* chunk = getChunk(x, z);
        if (!chunk->terrainPopulated) {
            MCPE_CRASH_TRACE("[IW] ChunkCache::postProcess begin (%d,%d)\n", x, z);
            chunk->terrainPopulated = true;
            if (source != NULL) {
                source->postProcess(parent, x, z);
				chunk->clearUpdateMap();
				saveCluster(x, z);
            }
            MCPE_CRASH_TRACE("[IW] ChunkCache::postProcess end (%d,%d)\n", x, z);
        }
    }

	void saveCluster(int x, int z) {
		if (storage == NULL) return;
		static const int dx[4] = {0, 1, 0, 1};
		static const int dz[4] = {0, 0, 1, 1};
		for (int i = 0; i < 4; ++i) {
			int cx = x + dx[i];
			int cz = z + dz[i];
			ChunkMap::iterator it = chunks.find(ChunkPos(cx, cz));
			if (it != chunks.end()) {
				LevelChunk* c = it->second;
				if (c && c != emptyChunk && c->unsaved) {
					save(c);
					c->unsaved = false;
				}
			}
		}
	}

    //bool save(bool force, ProgressListener progressListener) {
    //    int saves = 0;
    //    int count = 0;
    //    if (progressListener != NULL) {
    //        for (int i = 0; i < chunks.length; i++) {
    //            if (chunks[i] != NULL && chunks[i].shouldSave(force)) {
    //                count++;
    //            }
    //        }
    //    }
    //    int cc = 0;
    //    for (int i = 0; i < chunks.length; i++) {
    //        if (chunks[i] != NULL) {
    //            if (force && !chunks[i].dontSave) saveEntities(chunks[i]);
    //            if (chunks[i].shouldSave(force)) {
    //                save(chunks[i]);
    //                chunks[i].unsaved = false;
    //                if (++saves == MAX_SAVES && !force) return false;
    //                if (progressListener != NULL) {
    //                    if (++cc % 10 == 0) {
    //                        progressListener.progressStagePercentage(cc * 100 / count);
    //                    }
    //                }
    //            }
    //        }
    //    }

    //    if (force) {
    //        if (storage == NULL) return true;
    //        storage.flush();
    //    }
    //    return true;
    //}

    bool tick() {
        if (storage != NULL) storage->tick();
		int saves = 0;
		for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); ++it) {
			LevelChunk* chunk = it->second;
			if (chunk && chunk != emptyChunk && chunk->shouldSave(false)) {
                MCPE_CRASH_TRACE("[IW] ChunkCache::tick saving (%d,%d)\n", chunk->x, chunk->z);
				save(chunk);
				chunk->unsaved = false;
				if (++saves == MAX_SAVES) break;
			}
		}
        return source->tick();
    }

    bool shouldSave() {
        return true;
    }

    std::string gatherStats() {
   //     return "ChunkCache: 1024";
		std::stringstream ss;
		ss << "ChunkCache: " << chunks.size();
		return ss.str();
    }
	
	void saveAll(bool onlyUnsaved) {
		if (storage != NULL) {
			std::vector<LevelChunk*> chunksToSave;
			for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); ++it) {
				LevelChunk* chunk = it->second;
				if (chunk && chunk != emptyChunk && (!onlyUnsaved || chunk->shouldSave(false)))
                {
                    MCPE_CRASH_TRACE("[IW] ChunkCache::saveAll queue (%d,%d), onlyUnsaved=%d\n", chunk->x, chunk->z, onlyUnsaved);
					chunksToSave.push_back(chunk);
					chunk->unsaved = false;
                }
			}
			storage->saveAll(level, chunksToSave);
		}
	}
private:
    LevelChunk* load(int x, int z) {
        if (storage == NULL) return NULL;
        //try {
            LevelChunk* levelChunk = storage->load(level, x, z);
            if (levelChunk != NULL) {
                levelChunk->lastSaveTime = level->getTime();
            }
            return levelChunk;
        //} catch (Exception e) {
        //    e.printStackTrace();
        //    return emptyChunk;
        //}
    }

    void saveEntities(LevelChunk* levelChunk) {
        if (storage == NULL) return;
        //try {
            storage->saveEntities(level, levelChunk);
        //} catch (Error e) {
        //    e.printStackTrace();
        //}
    }

    void save(LevelChunk* levelChunk) {
        if (storage == NULL) return;
        //try {
            levelChunk->lastSaveTime = level->getTime();
            storage->save(level, levelChunk);
        //} catch (IOException e) {
        //    e.printStackTrace();
        //}
    }

public:
	int xLast;
    int zLast;
private:
	//unsigned char emptyChunkBlocks[LevelChunk::ChunkBlockCount];
	typedef std::map<ChunkPos, LevelChunk*> ChunkMap;
    LevelChunk* emptyChunk;
    ChunkSource* source;
    ChunkStorage* storage;
    ChunkMap chunks;
    Level* level;

    LevelChunk* last;

};

#endif /*NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__*/
