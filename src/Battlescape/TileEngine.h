/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_TILEENGINE_H
#define OPENXCOM_TILEENGINE_H

#include <vector>

#include <SDL.h>

#include "BattlescapeGame.h"
#include "Position.h"

#include "../Ruleset/MapData.h"

#include "../Savegame/BattleUnit.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SavedBattleGame;
class Tile;


/**
 * A utility class that modifies tile properties on a battlescape map.
 * This includes lighting, destruction, smoke, fire, fog of war.
 * Note that this function does not handle any sounds or animations.
 */
class TileEngine
{

private:
	static const int MAX_VIEW_DISTANCE			= 20;
	static const int MAX_VOXEL_VIEW_DISTANCE	= MAX_VIEW_DISTANCE * 16;
	static const int MAX_DARKNESS_TO_SEE_UNITS	= 9;

	static const int heightFromCenter[11];

	SavedBattleGame* _save;
	std::vector<Uint16>* _voxelData;
	void addLight(
			const Position& voxelTarget,
			int power,
			int layer);
	int blockage(
			Tile* tile,
			const int part,
			ItemDamageType type,
			int direction = -1);
	bool _personalLighting;


	public:
		/// Creates a new TileEngine class.
		TileEngine(
				SavedBattleGame* save,
				std::vector<Uint16>* voxelData);
		/// Cleans up the TileEngine.
		~TileEngine();

		/// Calculates sun shading of the whole map.
		void calculateSunShading();
		/// Calculates sun shading of a single tile.
		void calculateSunShading(Tile* tile);
		/// Recalculates lighting of the battlescape for terrain.
		void calculateTerrainLighting();
		/// Recalculates lighting of the battlescape for units.
		void calculateUnitLighting();

		/// Calculates the field of view from a units view point.
		bool calculateFOV(BattleUnit* unit);
		/// Calculates Field of View, including line of sight of all units within range of the Position
		void calculateFOV(const Position& position);
		/// Recalculates FOV of all units in-game.
		void recalculateFOV();

		/// Gets the origin voxel of a unit's eyesight.
		Position getSightOriginVoxel(BattleUnit* currentUnit);
		/// Checks visibility of a unit on this tile.
		bool visible(BattleUnit* currentUnit, Tile* tile);
		/// Creates a vector of units that can spot this unit.

		std::vector<BattleUnit*> getSpottingUnits(BattleUnit* unit);
		/// Checks validity of a snap shot to this position.
		bool canMakeSnap(BattleUnit* unit, BattleUnit* target);
		/// Checks reaction fire.
		bool checkReactionFire(BattleUnit* unit);
		/// Given a vector of spotters, and a unit, picks the spotter with the highest reaction score.
		BattleUnit* getReactor(
				std::vector<BattleUnit*> spotters,
				BattleUnit* unit);
		/// Tries to perform a reaction snap shot to this location.
		bool tryReactionSnap(
				BattleUnit* unit,
				BattleUnit* target);

		/// Handles bullet/weapon hits.
		BattleUnit* hit(
				const Position& pTarget_voxel,
				int power,
				ItemDamageType type,
				BattleUnit* attacker,
				bool hit = false);	// kL add.
		/// Handles explosions.
		void explode(
				const Position& voxelTarget,
				int power,
				ItemDamageType type,
				int maxRadius,
				BattleUnit* unit = 0);
		/// Blows this tile up.
		bool detonate(Tile* tile);
		/// Checks if a destroyed tile starts an explosion.
		Tile* checkForTerrainExplosions();
		/// Checks the vertical blockage of a tile.
		int verticalBlockage(
				Tile* startTile,
				Tile* endTile,
				ItemDamageType type);
		/// Checks the horizontal blockage of a tile.
		int horizontalBlockage(
				Tile* startTile,
				Tile* endTile,
				ItemDamageType type);
		/// Unit opens door?
		int unitOpensDoor(
				BattleUnit* unit,
				bool rClick = false,
				int dir = -1);
		/// Opens any doors this door is connected to.
		void checkAdjacentDoors(Position pos, int part);
		/// Closes ufo doors.
		int closeUfoDoors();

		/// Calculates a line trajectory.
		int calculateLine(
				const Position& origin,
				const Position& target,
				bool storeTrajectory,
				std::vector<Position>*
				trajectory,
				BattleUnit* excludeUnit,
				bool doVoxelCheck = true,
				bool onlyVisible = false,
				BattleUnit* excludeAllBut = 0);
		/// Calculates a parabola trajectory.
		int calculateParabola(
				const Position& origin,
				const Position& target,
				bool storeTrajectory,
				std::vector<Position>* trajectory,
				BattleUnit* excludeUnit,
				double arc,
				double acu);
//				const Position delta);	// Wb.131129, supercedes 'acu'
		/// Validates a throwing action.
		bool validateThrow(BattleAction* action);
/*		bool validateThrow( // Wb.131129
				BattleAction& action,
				Position originVoxel,
				Position targetVoxel,
				double* curve = 0,
				int* voxelType = 0); */

		/// Turn XCom soldier's personal lighting on or off.
		void togglePersonalLighting();
		/// Checks the distance between two positions.
		int distance(const Position& pos1, const Position& pos2) const;
		/// Checks the distance squared between two positions.
		int distanceSq(const Position& pos1, const Position& pos2, bool considerZ = true) const;
		/// Attempts a panic or mind control action.
		bool psiAttack(BattleAction* action);
		/// Applies gravity to anything that occupy this tile.
		Tile* applyGravity(Tile* t);
		/// Returns melee validity between two units.
		bool validMeleeRange(
				BattleUnit* attacker,
				BattleUnit* target,
				int dir);
		/// Returns validity of a melee attack from a given position.
		bool validMeleeRange(
				Position pos,
				int direction,
				BattleUnit* attacker,
				BattleUnit* target,
				Position* dest);
		/// Gets the AI to look through a window.
		int faceWindow(const Position& position);
		/// Checks a unit's % exposure on a tile.
//kL		int checkVoxelExposure(Position* originVoxel, Tile* tile, BattleUnit* excludeUnit, BattleUnit* excludeAllBut);
		/// Checks validity for targetting a unit.
		bool canTargetUnit(
				Position* originVoxel,
				Tile* tile,
				Position* scanVoxel,
				BattleUnit* excludeUnit,
				BattleUnit* potentialUnit = 0);
		/// Check validity for targetting a tile.
		bool canTargetTile(Position* originVoxel,
				Tile* tile,
				int part,
				Position* scanVoxel,
				BattleUnit* excludeUnit);
		/// Calculates the z voxel for shadows.
		int castedShade(const Position& voxel);
		/// Checks the visibility of a given voxel.
		bool isVoxelVisible(const Position& voxel);
		/// Checks what type of voxel occupies pTarget_voxel space.
		int voxelCheck(
				const Position& pTarget_voxel,
				BattleUnit* excludeUnit,
				bool excludeAllUnits = false,
				bool onlyVisible = false,
				BattleUnit* excludeAllBut = 0,
				bool hit = false); // kL add.
		/// Get direction to a target-point
		int getDirectionTo(
				const Position& origin,
				const Position& target) const;
		/// determine the origin voxel of a given action.
		Position getOriginVoxel(
				BattleAction& action,
				Tile* tile);
		/// mark a region of the map as "dangerous" for a turn.
		void setDangerZone(
				Position pos,
				int radius,
				BattleUnit* unit);
};

}

#endif
