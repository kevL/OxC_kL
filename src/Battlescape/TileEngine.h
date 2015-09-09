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

#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SavedBattleGame;
class Tile;

struct BattleAction;


/**
 * A utility class that handles lighting and calculations in 3D-space on the
 * battlefield - as well as opening and closing doors.
 * @note This function does not handle any graphics or sounds - except doggie
 * bark in calculateFOV().
 */
class TileEngine
{

private:
	static const int
		MAX_VIEW_DISTANCE		= 20,
		MAX_VIEW_DISTANCE_SQR	= MAX_VIEW_DISTANCE * MAX_VIEW_DISTANCE,
		MAX_VOXEL_VIEW_DISTANCE	= 20 * 16,
		MAX_VOXEL_VIEW_DIST_SQR	= MAX_VOXEL_VIEW_DISTANCE * MAX_VOXEL_VIEW_DISTANCE,
		MAX_SHADE_TO_SEE_UNITS	= 8,

		heightFromCenter[11];

	bool
		_spotSound,
		_unitLighting;
	int
//		_missileDirection,
		_powerE, // effective power that actually explodes on a tile that's hit by HE.
		_powerT; // test power that checks if _powerE actually makes it to the next tile.

	SavedBattleGame* _battleSave;
	Tile* _trueTile;

	BattleAction* _rfAction;
	std::map<int, Position> _rfShotList;

	const std::vector<Uint16>* _voxelData;

	///
	void addLight(
			const Position& pos,
			int power,
			size_t layer) const;
	/// Calculates blockage of various persuasions.
	int blockage(
			const Tile* const tile,
			const MapDataType part,
			const ItemDamageType dType,
			const int dir = -1,
			const bool originTest = false,
			const bool trueDir = false) const;
	/// Opens any doors this door is connected to.
	void openAdjacentDoors(
			const Position& pos,
			MapDataType part) const;
	/// Gets a Tile within melee range.
	Tile* getVerticalTile(
			const Position& posOrigin,
			const Position& posTarget) const;


	public:
		/// Creates a new TileEngine class.
		TileEngine(
				SavedBattleGame* const battleSave,
				const std::vector<Uint16>* const voxelData);
		/// Cleans up the TileEngine.
		~TileEngine();

		/// Calculates sun shading of the whole map.
		void calculateSunShading() const;
		/// Calculates sun shading of a single tile.
		void calculateSunShading(Tile* const tile) const;
		/// Recalculates lighting of the battlescape for terrain.
		void calculateTerrainLighting() const;
		/// Recalculates lighting of the battlescape for units.
		void calculateUnitLighting() const;

		/// Turn XCom soldier's personal lighting on or off.
		void togglePersonalLighting();

		/// Calculates Field of Vision from a unit's view point.
		bool calculateFOV(BattleUnit* const unit) const;
		/// Calculates Field of Vision including for all units within range of Position.
		void calculateFOV(
				const Position& pos,
				bool spotSound = false);
		/// Recalculates Field of Vision of all units.
		void recalculateFOV(bool spotSound = false);

		/// Checks visibility of a unit to a tile.
		bool visible(
				const BattleUnit* const unit,
				const Tile* const tile) const;

		/// Gets the origin voxel of a unit's eyesight.
		Position getSightOriginVoxel(const BattleUnit* const unit) const;
		/// Gets the origin voxel of a given action.
		Position getOriginVoxel(
				const BattleAction& action,
				const Tile* tile = NULL) const;
		/// Checks validity for targetting a unit.
		bool canTargetUnit(
				const Position* const originVoxel,
				const Tile* const targetTile,
				Position* const scanVoxel,
				const BattleUnit* const excludeUnit,
				const BattleUnit* targetUnit = NULL) const;
		/// Check validity for targetting a tile.
		bool canTargetTile(
				const Position* const originVoxel,
				const Tile* const targetTile,
				const MapDataType tilePart,
				Position* const scanVoxel,
				const BattleUnit* const excludeUnit) const;
		/// Checks a unit's % exposure on a tile.
//		int checkVoxelExposure(Position* originVoxel, Tile* tile, BattleUnit* excludeUnit, BattleUnit* excludeAllBut);

		/// Checks reaction fire.
		bool checkReactionFire(
				BattleUnit* const triggerUnit,
				int tuSpent = 0,
				bool autoSpot = true);
		/// Creates a vector of units that can spot this unit.
		std::vector<BattleUnit*> getSpottingUnits(const BattleUnit* const unit);
		/// Given a vector of spotters, and a unit, picks the spotter with the highest reaction score.
		BattleUnit* getReactor(
				std::vector<BattleUnit*> spotters,
				BattleUnit* const defender,
				const int tuSpent = 0,
				bool autoSpot = true) const;
		/// Fires off a reaction shot.
		bool reactionShot(
				BattleUnit* const unit,
				const BattleUnit* const targetUnit);
		/// Selects a fire method based on range & time units.
		void selectFireMethod(BattleAction& action);
		/// Gets the unique reaction fire BattleAction struct.
		BattleAction* getRfAction();
		/// Gets the reaction fire shot list.
		std::map<int, Position>* getRfShotList();

		/// Handles bullet/weapon hits.
		BattleUnit* hit(
				const Position& targetPos_voxel,
				int power,
				ItemDamageType dType,
				BattleUnit* const attacker,
				const bool melee = false);
		/// Handles explosions.
		void explode(
				const Position& targetVoxel,
				int power,
				ItemDamageType dType,
				int maxRadius,
				BattleUnit* const attacker = NULL,
				bool grenade = false,
				bool defusePulse = false);
		/// Checks the horizontal blockage of a tile.
		int horizontalBlockage(
				const Tile* const startTile,
				const Tile* const endTile,
				const ItemDamageType dType) const;
		/// Checks the vertical blockage of a tile.
		int verticalBlockage(
				const Tile* const startTile,
				const Tile* const endTile,
				const ItemDamageType dType) const;
		/// Sets the final direction from which a missile or thrown-object came.
		void setProjectileDirection(const int dir);
		/// Blows this tile up.
		bool detonate(Tile* const tile) const;
		/// Checks if a destroyed tile starts an explosion.
		Tile* checkForTerrainExplosions() const;

		/// Tries to open a door.
		int unitOpensDoor(
				BattleUnit* const unit,
				const bool rtClick = false,
				int dir = -1);
		/// kL. Checks for a door connected to this wall at this position.
/*		bool TileEngine::testAdjacentDoor( // kL
				Position pos,
				int part,
				int dir); */
		/// Closes ufo doors.
		int closeUfoDoors() const;

		/// Calculates a line trajectory.
		VoxelType plotLine(
				const Position& origin,
				const Position& target,
				const bool storeTrj,
				std::vector<Position>* const trj,
				const BattleUnit* const excludeUnit,
				const bool doVoxelCheck = true,
				const bool onlyVisible = false,
				const BattleUnit* const excludeAllBut = NULL) const;
		/// Calculates a parabola trajectory.
		VoxelType plotParabola(
				const Position& originVoxel,
				const Position& targetVoxel,
				bool storeTrj,
				std::vector<Position>* const trj,
				const BattleUnit* const excludeUnit,
				const double arc,
				const Position& deltaVoxel = Position(0,0,0)) const;
		/// Validates a throwing action.
		bool validateThrow(
				const BattleAction& action,
				const Position& originVoxel,
				const Position& targetVoxel,
				double* const arc = NULL,
				VoxelType* const voxelType = NULL) const;

		/// Checks the distance between two positions.
		int distance(
				const Position& pos1,
				const Position& pos2,
				const bool considerZ = true) const;
		/// Checks the distance squared between two positions.
		int distanceSq(
				const Position& pos1,
				const Position& pos2,
				const bool considerZ = true) const;
		/// Checks the distance between two positions precisely.
//		double distancePrecise(
//				const Position& pos1,
//				const Position& pos2) const;

		/// Performs a psionic action.
		bool psiAttack(BattleAction* const action); // removed, post-cosmetic

		/// Applies gravity to anything that occupies this tile.
		Tile* applyGravity(Tile* const tile) const;

		/// Validates the melee range between two BattleUnits.
		bool validMeleeRange(
				const BattleUnit* const actor,
				const int dir = -1,
				const BattleUnit* const targetUnit = NULL) const;
		/// Validates the melee range between a Position and a BattleUnit.
		bool validMeleeRange(
				const Position& pos,
				const int dir,
				const BattleUnit* const actor,
				const BattleUnit* const targetUnit = NULL) const;

		/// Gets an adjacent Position that can be attacked with melee.
		Position getMeleePosition(const BattleUnit* const actor) const;
		/// Gets an adjacent tile with an unconscious unit if any.
		Tile* getExecutionTile(const BattleUnit* const actor) const;

		/// Gets the AI to look through a window.
		int faceWindow(const Position& pos) const;

		/// Calculates the z voxel for shadows.
		int castShadow(const Position& voxel) const;

		/// Checks the visibility of a given voxel.
//		bool isVoxelVisible(const Position& voxel) const;
		/// Checks what type of voxel occupies posTarget in voxel space.
		VoxelType voxelCheck(
				const Position& posTarget,
				const BattleUnit* const excludeUnit = NULL,
				const bool excludeAllUnits = false,
				const bool onlyVisible = false,
				const BattleUnit* const excludeAllBut = NULL) const;

		/// Gets direction to a target-point.
		int getDirectionTo(
				const Position& origin,
				const Position& target) const;

		/// Marks a region of the map as "dangerous to aliens" for a turn.
		void setDangerZone(
				const Position& pos,
				const int radius,
				const BattleUnit* const unit) const;

		/// Sets a tile with a diagonal bigwall as the true epicenter of an explosion.
		void setTrueTile(Tile* const tile);

		/// Gets a valid target-unit given a Tile.
		BattleUnit* getTargetUnit(const Tile* const tile) const;
};

}

#endif
