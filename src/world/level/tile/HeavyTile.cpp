#include "HeavyTile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/item/FallingTile.h"

bool HeavyTile::instaFall = false;

HeavyTile::HeavyTile( int id, int tex )
:   super(id, tex, Material::sand)
{
}

HeavyTile::HeavyTile( int id, int tex, const Material* material )
:   super(id, tex, material)
{
}

void HeavyTile::onPlace( Level* level, int x, int y, int z ) {
    level->addToTickNextTick(x, y, z, id, getTickDelay(level));
}

void HeavyTile::neighborChanged( Level* level, int x, int y, int z, int type ) {
    level->addToTickNextTick(x, y, z, id, getTickDelay(level));
}

void HeavyTile::tick( Level* level, int x, int y, int z, Random* random ) {
    if (!level->isClientSide) {
        checkSlide(level, x, y, z);
    }
}

int HeavyTile::getTickDelay( Level* level ) {
    return 2;
}

bool HeavyTile::isFree( Level* level, int x, int y, int z ) {
    int t = level->getTile(x, y, z);
    if (t == 0) return true;
    if (t < 0 || t >= Tile::NUM_BLOCK_TYPES || Tile::tiles[t] == NULL) return true;
    if (t == ((Tile*)Tile::fire)->id) return true;
    const Material* material = Tile::tiles[t]->material;
    if (material == Material::water) return true;
    if (material == Material::lava) return true;
    if (material->isReplaceable()) return true;
    if (Tile::tiles[t]->getAABB(level, x, y, z) == NULL) return true;
    return false;
}

void HeavyTile::checkSlide( Level* level, int x, int y, int z ) {
    int x2 = x;
    int y2 = y;
    int z2 = z;
    if (isFree(level, x2, y2 - 1, z2) && y2 >= 0) {
        int r = 16;
        int data = level->getData(x, y, z);
        if (instaFall || !level->hasChunksAt(x - r, y - r, z - r, x + r, y + r, z + r)) {
            level->setTile(x, y, z, 0);
            while (isFree(level, x, y - 1, z) && y > 0)
                y--;
            if (y > 0) {
                level->setTileAndData(x, y, z, id, data);
            }
        } else if (!level->isClientSide) {
            level->setTile(x, y, z, 0);
            FallingTile* e = new FallingTile(level, x + 0.5f, y + 0.5f, z + 0.5f, id, data);
            falling(e);
            level->addEntity(e);
        }
    }
}

void HeavyTile::falling( FallingTile* entity ) {
}

void HeavyTile::onLand( Level* level, int xt, int yt, int zt, int data ) {
}
