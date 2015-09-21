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

#ifndef OPENXCOM_PATHFINDING_H
#define OPENXCOM_PATHFINDING_H

//#include <vector>

#include "PathfindingNode.h"
#include "Position.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleUnit;
class SavedBattleGame;
class Tile;


enum bigWallTypes
{
	BIGWALL_NONE,	// 0
	BIGWALL_BLOCK,	// 1
	BIGWALL_NESW,	// 2
	BIGWALL_NWSE,	// 3
	BIGWALL_WEST,	// 4
	BIGWALL_NORTH,	// 5
	BIGWALL_EAST,	// 6
	BIGWALL_SOUTH,	// 7
	BIGWALL_E_S,	// 8
	BIGWALL_W_N		// 9
};


/**
 * A utility class that calculates the shortest path between two points on the
 * battlescape map.
 */
class Pathfinding
{

private:
	bool
		_Alt,
		_Ctrl,
		_previewed,
		_strafe;
	int
		_openDoor, // to get an accurate preview when dashing through doors.
		_tuCostTotal;

	BattleUnit* _unit;
	SavedBattleGame* _battleSave;

	BattleAction* _battleAction;

	MovementType _mType;

	std::vector<int> _path;

	std::vector<PathfindingNode> _nodes;


	/// Gets the node at a position.
	PathfindingNode* getNode(const Position& pos);

	/// Determines whether a tile blocks a movementType.
	bool isBlocked(
			const Tile* const tile,
			const MapDataType part,
			const BattleUnit* const missileTarget = NULL,
			const int bigWallExclusion = -1) const;
	/// Tries to find a straight line path between two positions.
	bool bresenhamPath(
			const Position& origin,
			const Position& target,
			const BattleUnit* const missileTarget,
			bool sneak = false,
			int maxTuCost = 1000);
	/// Tries to find a path between two positions.
	bool aStarPath(
			const Position& origin,
			const Position& target,
			const BattleUnit* const missileTarget,
			bool sneak = false,
			int maxTuCost = 1000);

	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(const Tile* const tile) const;
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(
			const Tile* const tile,
			int armorSize) const;

	/// Sets the movement type for the path.
	void setMoveType();

	/// Determines the additional TU cost of going one step from start to
	/// destination if going through a closed UFO door.
//	int getOpeningUfoDoorCost(int direction, Position start, Position destination);


	public:
		static const int
			DIR_UP		= 8,
			DIR_DOWN	= 9;

		static Uint8
			red,
			green,
			yellow;


		/// cTor
		explicit Pathfinding(SavedBattleGame* const battleSave);
		/// Cleans up the Pathfinding.
		~Pathfinding();

		/// Calculates the shortest path.
		void calculate(
				const BattleUnit* const unit,
				Position destPos,
				const BattleUnit* const missileTarget = NULL,
				int maxTuCost = 1000,
				bool strafeRejected = false);

		/// Determines whether or not movement between startTile and endTile is possible in the direction.
		bool isBlockedPath(
				const Tile* const startTile,
				const int dir,
				const BattleUnit* const missileTarget = NULL) const;

		/// Aborts the current path.
		void abortPath();

		/// Converts direction to a unit-vector.
		static void directionToVector(
				const int dir,
				Position* const posVect);
		/// Converts a unit-vector to a direction.
		static void vectorToDirection(
				const Position& posVect,
				int& dir);

		/// Checks whether a path is ready and returns the direction.
		int getStartDirection();
		/// Dequeues a path and returns the direction.
		int dequeuePath();

		/// Gets the TU cost to move from 1 tile to the other.
		int getTuCostPf(
				const Position& posStart,
				int dir,
				Position* const posDest,
				const BattleUnit* const missileTarget = NULL,
				bool missile = false);
		/// Gets _tuCostTotal; finds out whether we can hike somewhere in this turn or not.
		int getTotalTUCost() const
		{ return _tuCostTotal; }

		/// Checks if the movement is valid, for the up/down button.
		int validateUpDown(
				const Position& posStart,
				const int dir);

		/// Gets all reachable tiles, based on cost.
		std::vector<int> findReachable(
				const BattleUnit* const unit,
				int tuMax);

		/// Previews the path.
		bool previewPath(bool discard = false);
		/// Removes the path preview.
		bool removePreview();
		/// Gets the path preview setting.
		bool isPathPreviewed() const;

		/// Sets unit in order to exploit low-level pathing functions.
		void setPathingUnit(BattleUnit* const unit);
		/// Sets keyboard input modifiers.
		void setInputModifiers();

		/// Gets the CTRL modifier setting.
		bool isModCtrl() const;
		/// Gets the ALT modifier setting.
		bool isModAlt() const;

		/// Gets the current movementType.
		MovementType getMoveTypePf() const;

		/// Gets TU cost for opening a door.
		int getOpenDoor() const;

		/// Gets a reference to the path.
		const std::vector<int>& getPath();
		/// Makes a copy to the path.
		std::vector<int> copyPath() const;

		/// Determines whether the unit is going up a stairs.
//		bool isOnStairs(const Position& startPosition, const Position& endPosition);
};

}

#endif
