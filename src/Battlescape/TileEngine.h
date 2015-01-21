/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_TILEENGINE_H
#define OPENXCOM_TILEENGINE_H

//#include <vector>
//#include <SDL.h>

#include "BattlescapeGame.h"
#include "Position.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SavedBattleGame;
class Tile;

struct BattleAction;


/**
 * A utility class that modifies tile properties on a battlescape map.
 * This includes lighting, destruction, smoke, fire, fog of war.
 * Note that this function does not handle any sounds or animations.
 */
class TileEngine
{

private:
	static const int
		MAX_VIEW_DISTANCE			= 20,
		MAX_VIEW_DISTANCE_SQR		= MAX_VIEW_DISTANCE * MAX_VIEW_DISTANCE,
		MAX_VOXEL_VIEW_DISTANCE		= 20 * 16,
		MAX_VOXEL_VIEW_DISTANCE_SQR	= MAX_VOXEL_VIEW_DISTANCE * MAX_VOXEL_VIEW_DISTANCE,
		MAX_SHADE_TO_SEE_UNITS		= 8,

		heightFromCenter[11];

	bool _unitLighting;
	int
		_missileDirection,
		_powerE, // kL, effective power that actually explodes on a tile that's hit by HE.
		_powerT; // kL, test power that checks if _powerE actually makes it to the next tile.

	SavedBattleGame* _battleSave;

	std::vector<Uint16>* _voxelData;

	///
	void addLight(
			const Position& pos,
			int power,
			int layer);
	/// Calculates blockage of various persuasions.
	int blockage(
			const Tile* const tile,
			const int part,
			const ItemDamageType type,
			const int dir = -1,
			const bool originTest = false,
			const bool trueDir = false);


	public:
		/// Creates a new TileEngine class.
		TileEngine(
				SavedBattleGame* battleSave,
				std::vector<Uint16>* voxelData);
		/// Cleans up the TileEngine.
		~TileEngine();

		/// Calculates sun shading of the whole map.
		void calculateSunShading();
		/// Calculates sun shading of a single tile.
		void calculateSunShading(Tile* const tile);
		/// Recalculates lighting of the battlescape for terrain.
		void calculateTerrainLighting();
		/// Recalculates lighting of the battlescape for units.
		void calculateUnitLighting();
		/// Turn XCom soldier's personal lighting on or off.
		void togglePersonalLighting();

		/// Calculates the field of view from a units view point.
		bool calculateFOV(BattleUnit* unit);
		/// Calculates Field of View, including line of sight of all units within range of the Position
		void calculateFOV(const Position& position);
		/// Recalculates FOV of all units in-game.
		void recalculateFOV();

		/// Checks visibility of a unit on this tile.
		bool visible(
				const BattleUnit* const unit,
				const Tile* const tile);
		/// Gets the origin voxel of a unit's eyesight.
		Position getSightOriginVoxel(const BattleUnit* const unit);
		/// Checks a unit's % exposure on a tile.
//kL	int checkVoxelExposure(Position* originVoxel, Tile* tile, BattleUnit* excludeUnit, BattleUnit* excludeAllBut);
		/// Checks validity for targetting a unit.
		bool canTargetUnit(
				const Position* const originVoxel,
				const Tile* const tile,
				Position* const scanVoxel,
				const BattleUnit* const excludeUnit,
				const BattleUnit* unit = NULL);
		/// Check validity for targetting a tile.
		bool canTargetTile(
				const Position* const originVoxel,
				const Tile* const tile,
				const int part,
				Position* const scanVoxel,
				const BattleUnit* const excludeUnit);

		/// Checks reaction fire.
		bool checkReactionFire(
							BattleUnit* unit,
							int tuSpent = 0);
		/// Creates a vector of units that can spot this unit.
		std::vector<BattleUnit*> getSpottingUnits(BattleUnit* unit);
		/// Given a vector of spotters, and a unit, picks the spotter with the highest reaction score.
		BattleUnit* getReactor(
				std::vector<BattleUnit*> spotters,
				BattleUnit* defender,
				int tuSpent = 0) const;
		/// Fires off a reaction shot.
		bool reactionShot(
				BattleUnit* const unit,
				const BattleUnit* const target);
		/// Selects a fire method based on range & time units.
		void selectFireMethod(BattleAction& action);

		/// Handles bullet/weapon hits.
		BattleUnit* hit(
				const Position& targetPos_voxel,
				int power,
				ItemDamageType type,
				BattleUnit* attacker,
				const bool melee = false);
		/// Handles explosions.
		void explode(
				const Position& voxelTarget,
				int power,
				ItemDamageType type,
				int maxRadius,
				BattleUnit* attacker = NULL,
				bool grenade = false);
		/// Checks the horizontal blockage of a tile.
		int horizontalBlockage(
				const Tile* const startTile,
				const Tile* const endTile,
				const ItemDamageType type);
		/// Checks the vertical blockage of a tile.
		int verticalBlockage(
				const Tile* const startTile,
				const Tile* const endTile,
				const ItemDamageType type);
		/// Sets the final direction from which a missile or thrown-object came.
		void setProjectileDirection(const int dir);
		/// Blows this tile up.
		bool detonate(Tile* const tile);
		/// Checks if a destroyed tile starts an explosion.
		Tile* checkForTerrainExplosions();

		/// Tries to open a door.
		int unitOpensDoor(
				BattleUnit* const unit,
				const bool rhtClick = false,
				int dir = -1);
		/// kL. Checks for a door connected to this wall at this position.
/*		bool TileEngine::testAdjacentDoor( // kL
				Position pos,
				int part,
				int dir); */
		/// Opens any doors this door is connected to.
		void openAdjacentDoors(
				Position pos,
				int part);
		/// Closes ufo doors.
		int closeUfoDoors();

		/// Calculates a line trajectory.
		int calculateLine(
				const Position& origin,
				const Position& target,
				const bool storeTrajectory,
				std::vector<Position>* const trajectory,
				const BattleUnit* const excludeUnit,
				const bool doVoxelCheck = true,
				const bool onlyVisible = false,
				const BattleUnit* const excludeAllBut = NULL);
		/// Calculates a parabola trajectory.
		int calculateParabola(
				const Position& origin,
				const Position& target,
				bool storeTrajectory,
				std::vector<Position>* const trajectory,
				const BattleUnit* const excludeUnit,
				const double arc,
				const Position delta = Position(0,0,0));
		/// Validates a throwing action.
		bool validateThrow(
				const BattleAction& action,
				const Position originVoxel,
				const Position targetVoxel,
				double* const arc = NULL,
				int* const voxelType = NULL);

		/// Checks the distance between two positions.
		int distance(
				const Position& pos1,
				const Position& pos2) const;
		/// Checks the distance squared between two positions.
		int distanceSq(
				const Position& pos1,
				const Position& pos2,
				bool considerZ = true) const;

		/// Attempts a panic or mind control action.
		bool psiAttack(BattleAction* action);

		/// Applies gravity to anything that occupies this tile.
		Tile* applyGravity(Tile* const tile);

		/// Returns melee validity between two units.
		bool validMeleeRange(
				const BattleUnit* const attacker,
				const BattleUnit* const target,
				const int dir);
		/// Returns validity of a melee attack from a given position.
		bool validMeleeRange(
				const Position origin,
				const int dir,
				const BattleUnit* const attacker,
				const BattleUnit* const target,
				Position* const dest,
				const bool preferEnemy = true);

		/// Gets the AI to look through a window.
		int faceWindow(const Position& position);

		/// Calculates the z voxel for shadows.
		int castedShade(const Position& voxel) const;

		/// Checks the visibility of a given voxel.
		bool isVoxelVisible(const Position& voxel) const;
		/// Checks what type of voxel occupies posTarget in voxel space.
		int voxelCheck(
				const Position& targetPos,
				const BattleUnit* const excludeUnit = NULL,
				const bool excludeAllUnits = false,
				const bool onlyVisible = false,
				const BattleUnit* const excludeAllBut = NULL) const;

		/// Get direction to a target-point
		int getDirectionTo(
				const Position& origin,
				const Position& target) const;

		/// determine the origin voxel of a given action.
		Position getOriginVoxel(
				const BattleAction& action,
				const Tile* tile);

		/// mark a region of the map as "dangerous" for a turn.
		void setDangerZone(
				const Position pos,
				const int radius,
				const BattleUnit* const unit);
};

}

#endif
