//#define CLIENT
#include "Gameplay.hpp"
#include <engine/Utils/Put.hpp>
#include <engine/Math/Utils.hpp>
#include <engine/Math/Transform.hpp>
#ifdef CLIENT
#include <client/Debug.hpp>
#endif 

// Not sure if its better to use floats or frames for storing elapsed time of spawners. Most spawners don't need to be replicated.

template<typename BulletIndex>
static BulletIndex createBulletIndex(GameplayState& gameplayState, FrameTime ownerFrame, PlayerIndex ownerPlayerIndex, i32 indexesToReserve = 1) {
	const auto spawnIndex = gameplayState.bulletsSpawnedThisFrame;
	gameplayState.bulletsSpawnedThisFrame += indexesToReserve;
	return BulletIndex{
		.ownerPlayerIndex = ownerPlayerIndex,
		.spawnFrame = ownerFrame,
		.spawnIndexInFrame = static_cast<u16>(spawnIndex),
	};
}

template<typename BulletIndex>
static BulletIndex createSpawnerBulletIndex(const UntypedBulletIndex& spawnerIndex, i32 bulletIndex) {
	return BulletIndex{
		.ownerPlayerIndex = spawnerIndex.ownerPlayerIndex,
		.spawnFrame = spawnerIndex.spawnFrame,
		.spawnIndexInFrame = static_cast<u16>(spawnerIndex.spawnIndexInFrame + 1 + bulletIndex),
	};
}

Vec2 applyMovementInput(Vec2 pos, ClientInputMessage::Input input, float dt) {
	Vec2 direction(0.0f);
	if (input.down) {
		direction += Vec2(0.0f, -1.0f);
	}
	if (input.up) {
		direction += Vec2(0.0f, 1.0f);
	}
	if (input.left) {
		direction += Vec2(-1.0f, 0.0f);
	}
	if (input.right) {
		direction += Vec2(1.0f, 0.0f);
	}
	const auto velocity = input.shift ? PLAYER_SHIFT_SPEED : PLAYER_SPEED;
	pos += direction.normalized() * dt * velocity;

	const auto dist = pos.length();
	const auto separation = dist - BORDER_RADIUS + PLAYER_HITBOX_RADIUS;
	const auto normal = pos / dist;
	if (separation > 0.0f) {
		pos -= separation * normal;
	}
	return pos;
}

static float calculateDt(ClientBulletSynchronizationData& synchronization, float dt) {
	// In addition to this could also accelerate the bullet at the start to the normal velocity (not sure what function maybe smoothstep or erf) to apprixomately match the correct position when the message is received from the server. It might also be good to implement some lag comensation so it is more fair. The issue with this that laggy players then have to accelerate bullets more (first to compensate for lag second to move them into the future).
#ifdef CLIENT
	/*if (synchronization.timeToSynchronize < 0.0f) {
		synchronization.timeToSynchronize = -synchronization.timeToSynchronize;
	}*/

	if (synchronization.synchronizationProgressT < 1.0f) {

		auto x = (synchronization.synchronizationProgressT - 0.5f); // from -0.5 to 0.5
		// Most of the values are concentrated in the interval -2 to 2
		const auto scale = 4.0f;
		x *= scale; // from -2 to 2

		const auto normalDistributionMax = 1.0f / sqrt(PI<float>);
		const auto value = exp(-pow(x, 2.0f)) / sqrt(PI<float>);

		const auto max = dt / 4.0f;
		// max = normalDistMax * functionStep * bullet.timeToSynchronize
		// functionStep = max / (normalDistMax * bullet.timeToSynchronize)
		// functionStep = timeStep * scale
		// timeStep = functionStep / scale
		const auto timeStep = max / (normalDistributionMax * synchronization.timeToSynchronize) / scale;

		synchronization.synchronizationProgressT += timeStep;
		const auto functionStep = timeStep * scale;
		dt -= value * functionStep * synchronization.timeToSynchronize;
	} else if (synchronization.timeToCatchUp > 0.0f) { // Else if so time doesn't become negative. TODO: Maybe handle it later if needed. Currently shouldn't happen.
		float speedUp = 0.0f;
		speedUp = (synchronization.timeToCatchUp * synchronization.catchUpPercentPerFrame);
		synchronization.timeToCatchUp -= speedUp;

		if (synchronization.timeToCatchUp <= dt / 2.0f) {
			speedUp += synchronization.timeToCatchUp;
			synchronization.timeToCatchUp = 0.0f;
		}
		dt += speedUp;
	}
#endif
	return dt;
}

void updateGemeplayStateBeforeProcessingInput(GameplayState& state) {
	state.bulletsSpawnedThisFrame = 0;
}

const auto SLASH_PATTERN_BULLETS = 10;
const auto SPINNING_PATTERN_BULLETS = 50;

void updateGameplayPlayer(
	PlayerIndex playerIndex, 
	GameplayPlayer& player, 
	GameplayState& state, 
	const ClientInputMessage::Input& input, 
	FrameTime ownerFrame,
	float dt) {
	player.position = applyMovementInput(player.position, input, dt);


	const auto direction = Vec2::oriented(input.rotation);

	auto spawnMoveForwardBullet = [&](Vec2 position, Vec2 velocity) {
		const auto index = createBulletIndex<MoveForwardBulletIndex>(state, ownerFrame, playerIndex);
		state.moveForwardBullets[index] = MoveForwardBullet{
			.position = player.position,
			.velocity = velocity
		};
		return index;
	};

	const Rotation deg45{ PI<float> / 2.0f };
	const auto deg90 = deg45 * deg45;
	const auto deg180 = deg90 * deg90;
	const auto deg270 = deg45.inversed();


	for (i32 type = 0; type < PatternType::PatternType::COUNT; type++) {
		auto& cooldown = player.cooldown.of[type];
		cooldown = std::max(0.0f, cooldown - dt);
	}

	using namespace PatternType;

	if (input.shoot) {
		if (input.selectedPattern == SINLGE_BULLET && player.cooldown.of[SINLGE_BULLET] == 0.0f) {
			const auto index = createBulletIndex<MoveForwardBulletIndex>(state, ownerFrame, playerIndex);
			state.moveForwardBullets[index] = MoveForwardBullet{
				.position = player.position,
				.velocity = direction
			};
			player.cooldown.of[SINLGE_BULLET] = patternInfos[SINLGE_BULLET].cooldown;
		} 

		if (input.selectedPattern == EXPANDING_TRIANGLE_STAR_PATTERN && player.cooldown.of[EXPANDING_TRIANGLE_STAR_PATTERN]== 0.0f) {
			// Expanding is just so the velocity has to be the vector from the center.
			auto spawnExpandingPolygon = [&](float rotation, i32 sides, i32 bulletsPerSide) {
				const auto step = TAU<float> / sides;
				for (int i = 0; i < sides; i++) {
					const auto startAngle = rotation + step * i;
					const auto endAngle = rotation + step * (i + 1);
					const auto start = Vec2::oriented(startAngle);
					const auto end = Vec2::oriented(endAngle);
					for (int j = 0; j < bulletsPerSide - 1; j++) {
						// Never get to 1 so the next sides adds the corner.
						float t = static_cast<float>(j) / static_cast<float>(bulletsPerSide - 1);
						const auto vectorFromCenter = lerp(start, end, t);
						spawnMoveForwardBullet(player.position, vectorFromCenter);
					}
				}
			};

			auto spawnTriangleStar = [&](float angle) {
				spawnExpandingPolygon(angle, 3, 10);
				spawnExpandingPolygon(angle + PI<float>, 3, 10);
			};

			auto spawnSquareStar = [&]() {
				spawnExpandingPolygon(0.0f, 4, 10);
				spawnExpandingPolygon(PI<float> / 4.0f, 4, 10);
			};

			spawnTriangleStar(input.rotation);
			player.cooldown.of[EXPANDING_TRIANGLE_STAR_PATTERN] = patternInfos[EXPANDING_TRIANGLE_STAR_PATTERN].cooldown;
		}

		if (input.selectedPattern == SPINNING_PATTERN && player.cooldown.of[SPINNING_PATTERN] == 0.0f) {
			auto spawnSpinningPattern = [&]() {
				const auto index = createBulletIndex<SpinningPatternSpawnerIndex>(state, ownerFrame, playerIndex, SPINNING_PATTERN_BULLETS + 1);
				state.spinningPatternSpawners[index] = SpinningPatternSpawner{
					.ownerPlayerIndex = playerIndex,
				};
			};
			spawnSpinningPattern();
			player.cooldown.of[SPINNING_PATTERN] = patternInfos[SPINNING_PATTERN].cooldown;
		}

		
		//spawnSpinningPattern();
		//player.cooldowns.singleBullet = SHOOT_COOLDOWN;

		/*auto spawnExpandingSquare = [&](Vec2 up) {
			
			for (i32 x = 1; x < size - 1; x++) {
				spawnMoveForwardBullet(player.position, up);
				spawnMoveForwardBullet(player.position, up * deg180);
			}

			for (i32 y = 1; y < size - 1; y++) {
				spawnMoveForwardBullet(player.position, up * deg90);
				spawnMoveForwardBullet(player.position, up * deg270);
			}

			spawnMoveForwardBullet()
		};*/
		//spawnExpandingSquare(Vec2(0.0f, 1.0f));

		/*player.shootCooldown = SHOOT_COOLDOWN;
		const auto index = createBulletIndex<SlashPatternSpawnerIndex>(state, ownerFrame, playerIndex, SLASH_PATTERN_BULLETS + 1);
		state.slashPatternSpawners[index] = SlashPatternSpawner{
			.position = player.position,
			.directionAngle = input.rotation,
		};*/

		
	}

	// 2 square star.
}

void updateGameplayStateAfterProcessingInput(GameplayState& state, GameplayContext& c, float dt) {
	std::erase_if(state.moveForwardBullets, [&](std::pair<const MoveForwardBulletIndex, MoveForwardBullet>& pair) {
		auto& [_, bullet] = pair;
		const auto bulletDt = calculateDt(bullet.synchronization, dt);
		bullet.position += bullet.velocity * bulletDt;
		bullet.elapsed += bulletDt;
		return bullet.elapsed > BULLET_ALIVE_SECONDS;
	});

	std::erase_if(state.slashPatternSpawners, [&](std::pair<const SlashPatternSpawnerIndex, SlashPatternSpawner>& pair) {
		auto& [spawnerIndex, spawner] = pair;

		const auto spawnerDt = dt;
		float halfArcLength = 0.5f;
		float startAngle = spawner.directionAngle - halfArcLength;
		const auto timeBetweenSpawns = FRAME_DT_SECONDS * 4.0f;
		for (i32 i = spawner.bulletsSpawned; i < SLASH_PATTERN_BULLETS; i++) {
			if (spawner.elapsed / timeBetweenSpawns >= i) {
				const auto index = createSpawnerBulletIndex<MoveForwardBulletIndex>(spawnerIndex.untypedIndex(), i);
				const auto angle = startAngle + (halfArcLength * 2.0f) / (SLASH_PATTERN_BULLETS - 1) * i;
				state.moveForwardBullets[index] = MoveForwardBullet{
					.position = spawner.position,
					.velocity = Vec2::oriented(angle)
				};
				spawner.bulletsSpawned++;
			} else {
				break;
			}
		}

		if (spawner.bulletsSpawned == SLASH_PATTERN_BULLETS) {
			return true;
		}

		spawner.elapsed += spawnerDt;
		return false;
	});

	std::erase_if(state.spinningPatternSpawners, [&](std::pair<const SpinningPatternSpawnerIndex, SpinningPatternSpawner>& pair) {
		auto& [spawnerIndex, spawner] = pair;
			// Could do this pattern by only using elapsed and not storing spawned bullets, having spawned bullets is more convinient.

		const auto bulletSpawnDelayFrames = 3;
		const auto bulletSpawnedEveryFrame = 2;

		if (spawner.elapsed % bulletSpawnDelayFrames == 0) {
			float angle = spawner.elapsed / 6.0f * 1.618f;

			const auto ownerPlayer = c.getPlayer(spawner.ownerPlayerIndex);
			// TODO: What if dead?
			if (!ownerPlayer.has_value()) {
				CHECK_NOT_REACHED();
				return true;
			}

			for (i32 i = 0; i < bulletSpawnedEveryFrame; i++) {
				const auto index = createSpawnerBulletIndex<MoveForwardBulletIndex>(spawnerIndex.untypedIndex(), spawner.bulletsSpawned);
				spawner.bulletsSpawned++;
				state.moveForwardBullets[index] = MoveForwardBullet{
					.position = ownerPlayer->position,
					.velocity = Vec2::oriented(angle + i * PI<float>)
				};
			}
		}
		spawner.elapsed++;

		return spawner.bulletsSpawned == SPINNING_PATTERN_BULLETS;
	});
}
