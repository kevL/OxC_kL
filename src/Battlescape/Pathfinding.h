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


/**
 * A utility class that calculates the shortest path between two points on the battlescape map.
 */
class Pathfinding
{

private:
	bool
		_modifALT,
		_modifCTRL,
		_previewed,
		_strafe;
	int
		_openDoor, // kL, to give accurate preview # when dashing through doors.
		_totalTuCost;

	BattleUnit* _unit;
	SavedBattleGame* _battleSave;

	BattleAction* _battleAction;

	MovementType _moveType;

	std::vector<int> _path;

	std::vector<PathfindingNode> _nodes;


	/// Gets the node at a position.
	PathfindingNode* getNode(const Position& pos);

	/// Determines whether a tile blocks a movementType.
	bool isBlocked(
			const Tile* const tile,
			const int part,
			const BattleUnit* const missileTarget = NULL,
			const int bigWallExclusion = -1) const;
	/// Tries to find a straight line path between two positions.
	bool bresenhamPath(
			const Position& origin,
			const Position& target,
			const BattleUnit* const missileTarget,
			bool sneak = false,
			int maxTUCost = 1000);
	/// Tries to find a path between two positions.
	bool aStarPath(
			const Position& origin,
			const Position& target,
			const BattleUnit* const missileTarget,
			bool sneak = false,
			int maxTUCost = 1000);

	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(const Tile* const tile) const;
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(
			const Tile* const tile,
			int unitSize) const;
	/// Determines the additional TU cost of going one step from
	/// start to destination if going through a closed UFO door.
//	int getOpeningUfoDoorCost(int direction, Position start, Position destination);


	public:
		static const int
			DIR_UP		=  8,
			DIR_DOWN	=  9,
			O_BIGWALL	= -1;

		static int
			red,
			green,
			yellow;

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


		/// cTor
		Pathfinding(SavedBattleGame* battleSave);
		/// Cleans up the Pathfinding.
		~Pathfinding();

		/// Calculates the shortest path.
		void calculate(
				BattleUnit* const unit,
				Position destPos,
				const BattleUnit* const missileTarget = NULL,
				int maxTUCost = 1000,
				bool strafeRejected = false);

		/// Determines whether or not movement between starttile and endtile is possible in the direction.
		bool isBlocked(
				const Tile* const startTile,
				const Tile* endTile,
				const int dir,
				const BattleUnit* const missileTarget = NULL) const;

		/// Aborts the current path.
		void abortPath();

		/// Converts direction to a unit-vector.
		static void directionToVector(
				const int dir,
				Position* unitVect);
		/// Converts a unit-vector to a direction.
		static void vectorToDirection(
				const Position& unitVect,
				int& dir);

		/// Checks whether a path is ready and returns the direction.
		int getStartDirection();
		/// Dequeues a path and returns the direction.
		int dequeuePath();

		/// Gets the TU cost to move from 1 tile to the other.
		int getTUCost(
				const Position& startPos,
				const int dir,
				Position* destPos,
				BattleUnit* const unit,
				const BattleUnit* const missileTarget,
				bool missile);
		/// Gets _totalTuCost; finds out whether we can hike somewhere in this turn or not.
		int getTotalTUCost() const
		{ return _totalTuCost; }

		/// Checks if the movement is valid, for the up/down button.
		int validateUpDown(
				const BattleUnit* const bu,
				Position startPos,
				int const dir);

		/// Gets all reachable tiles, based on cost.
		std::vector<int> findReachable(
				BattleUnit* const unit,
				int tuMax);

		/// Previews the path.
		bool previewPath(bool discard = false);
		/// Removes the path preview.
		bool removePreview();
		/// Gets the path preview setting.
		bool isPathPreviewed() const;

		/// Gets the strafe move setting.
//		bool getStrafeMove() const;

		/// Sets _unit in order to abuse low-level pathfinding functions from outside the class.
		void setUnit(BattleUnit* unit);

		/// Gets the CTRL modifier setting.
		bool isModCTRL() const;
		/// Gets the ALT modifier setting.
		bool isModALT() const;

		/// Gets the current movementType.
		MovementType getMovementType() const;

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
