#include "game/Defines.h"
#include "game/Projectile.h"
#include "game/Map.h"
#include "game/FixedMath.h"
#include "game/Particle.h"
#include "game/Enemy.h"
#include "game/Generated/SpriteTypes.h"
#include "game/Platform.h"
#include "game/Sounds.h"

Projectile ProjectileManager::projectiles[ProjectileManager::MAX_PROJECTILES];

Projectile* ProjectileManager::FireProjectile(Entity* owner, int16_t x, int16_t y, uint8_t angle)
{
    if(!owner) return nullptr;

    for(Projectile& p : projectiles) {
        if(p.life == 0) {
            uint8_t new_owner_id = Projectile::playerOwnerId;
            bool owner_found = false;
            if(owner == &Game::player) {
                owner_found = true;
            } else {
                for(uint8_t n = 0; n < EnemyManager::maxEnemies; n++) {
                    if(&EnemyManager::enemies[n] == owner) {
                        new_owner_id = n;
                        owner_found = true;
                        break;
                    }
                }
            }
            if(!owner_found) return nullptr;

            p.ownerId = new_owner_id;
            p.life = 255;
            p.x = x;
            p.y = y;
            p.angle = angle;
            return &p;
        }
    }

    return nullptr;
}

Entity* Projectile::GetOwner() const {
    if(ownerId == playerOwnerId) return &Game::player;
    if(ownerId >= EnemyManager::maxEnemies) return nullptr;
    return &EnemyManager::enemies[ownerId];
}

void ProjectileManager::Update() {
    for(Projectile& p : projectiles) {
        if(p.life > 0) {
            p.life--;

            int16_t deltaX = FixedCos(p.angle) / 4;
            int16_t deltaY = FixedSin(p.angle) / 4;

            p.x += deltaX;
            p.y += deltaY;

            bool hitAnything = false;
            Entity* owner = p.GetOwner();
            if(!owner) {
                p.life = 0;
                continue;
            }

            if(Map::IsBlockedAtWorldPosition(p.x, p.y)) {
                uint8_t cellX = p.x / CELL_SIZE;
                uint8_t cellY = p.y / CELL_SIZE;

                if(Map::GetCellSafe(cellX, cellY) == CellType::Urn) {
                    Map::SetCell(cellX, cellY, CellType::Empty);
                    ParticleSystemManager::CreateExplosion(
                        cellX * CELL_SIZE + CELL_SIZE / 2, cellY * CELL_SIZE + CELL_SIZE / 2, true);

                    switch((Random() % 5)) {
                    case 0:
                        EnemyManager::Spawn(
                            EnemyType::Spider,
                            cellX * CELL_SIZE + CELL_SIZE / 2,
                            cellY * CELL_SIZE + CELL_SIZE / 2);
                        break;
                    case 1:
                        Map::SetCell(cellX, cellY, CellType::Potion);
                        break;
                    case 2:
                        Map::SetCell(cellX, cellY, CellType::Coins);
                        break;
                    }
                    Platform::PlaySound(Sounds::Kill);
                } else {
                    Platform::PlaySound(Sounds::Hit);
                }

                hitAnything = true;
            } else {
                if(owner == &Game::player) {
                    Enemy* overlappingEnemy = EnemyManager::GetOverlappingEnemy(p.x, p.y);
                    if(overlappingEnemy) {
                        overlappingEnemy->Damage(Player::attackStrength);
                        hitAnything = true;
                    }
                } else if(Game::player.IsOverlappingPoint(p.x, p.y)) {
                    const EnemyArchetype* enemyArchetype = ((Enemy*)owner)->GetArchetype();
                    if(enemyArchetype) {
                        Game::player.Damage(enemyArchetype->GetAttackStrength());
                        if(Game::player.hp == 0) {
                            Game::stats.killedBy = ((Enemy*)owner)->GetType();
                        }
                    }
                    hitAnything = true;
                }
            }

            if(hitAnything) {
                ParticleSystemManager::CreateExplosion(p.x - deltaX, p.y - deltaY);
                p.life = 0;
            }
        }
    }
}

void ProjectileManager::Init()
{
	for (Projectile& p : projectiles)
	{
		p.life = 0;
	}
}

void ProjectileManager::Draw()
{
	for(Projectile& p : projectiles)
	{
		if (p.life > 0)
		{
			Renderer::DrawObject(p.ownerId == Projectile::playerOwnerId ? projectileSpriteData : enemyProjectileSpriteData, p.x, p.y, 32, AnchorType::BelowCenter);
		}
	}
}
