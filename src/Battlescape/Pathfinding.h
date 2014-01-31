/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#ifndef OPENXCOM_PATHFINDING_H
#define OPENXCOM_PATHFINDING_H

#include <vector>

#include "Position.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleUnit;
class SavedBattleGame;
class PathfindingNode;
class Tile;


/**
 * A utility class that calculates the shortest path between two points on the battlescape map.
 */
class Pathfinding
{

private:
	bool
		_modifierUsed,
		_pathPreviewed,
		_strafeMove;
	int
		_size,
		_totalTUCost;

	BattleUnit* _unit;
	MovementType _movementType;
	SavedBattleGame* _save;

	std::vector<PathfindingNode> _nodes;

	/// Gets the node at a position.
	PathfindingNode* getNode(const Position& pos);

	/// Determines whether a tile blocks a movementType.
	bool isBlocked(
			Tile* tile,
			const int part,
			BattleUnit* missileTarget,
			int bigWallExclusion = -1);
	/// Tries to find a straight line path between two positions.
	bool bresenhamPath(
			const Position& origin,
			const Position& target,
			BattleUnit* missileTarget,
			bool sneak = false,
			int maxTUCost = 1000);
	/// Tries to find a path between two positions.
	bool aStarPath(
			const Position& origin,
			const Position& target,
			BattleUnit* missileTarget,
			bool sneak = false,
			int maxTUCost = 1000);

	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(Tile* destinationTile);
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(
			Tile* destinationTile,
			int size);
	/// Determines the additional TU cost of going one step from
	/// start to destination if going through a closed UFO door.
//	int getOpeningUfoDoorCost(int direction, Position start, Position destination);


	public:
		static const int DIR_UP		= 8;
		static const int DIR_DOWN	= 9;
		static const int O_BIGWALL	= -1;

		enum bigWallTypes
		{
			BLOCK = 1,			// 1
			BIGWALLNESW,		// 2
			BIGWALLNWSE,		// 3
			BIGWALLWEST,		// 4
			BIGWALLNORTH,		// 5
			BIGWALLEAST,		// 6
			BIGWALLSOUTH,		// 7
			BIGWALLEASTANDSOUTH	// 8
		};

		std::vector<int> _path;


		/// cTor
		Pathfinding(SavedBattleGame* save);
		/// Cleans up the Pathfinding.
		~Pathfinding();

		/// Calculates the shortest path.
		void calculate(
				BattleUnit* unit,
				Position endPosition,
				BattleUnit* missileTarget = 0,
				int maxTUCost = 1000);

		/// Determines whether or not movement between starttile and endtile is possible in the direction.
		bool isBlocked(
				Tile* startTile,
				Tile* endTile,
				const int direction,
				BattleUnit* missileTarget);

		/// Aborts the current path.
		void abortPath();

		/// Converts direction to a vector.
		static void directionToVector(
				int const direction,
				Position* vector);
		/// Converts a vector to a direction.
		static void vectorToDirection(
				const Position& vector,
				int& dir);

		/// Checks whether a path is ready and returns the direction.
		int getStartDirection();
		/// Dequeues a path and returns the direction.
		int dequeuePath();

		/// Gets the TU cost to move from 1 tile to the other.
		int getTUCost(
				const Position& startPosition,
				const int direction,
				Position* endPosition,
				BattleUnit* unit,
				BattleUnit* target,
				bool missile);
		/// Gets _totalTUCost; finds out whether we can hike somewhere in this turn or not.
		int getTotalTUCost() const
		{
			return _totalTUCost;
		}

		/// Checks if the movement is valid, for the up/down button.
		int validateUpDown(
				BattleUnit* bu,
				Position startPosition,
				int const direction);

		/// Gets all reachable tiles, based on cost.
		std::vector<int> findReachable(
				BattleUnit* unit,
				int tuMax);

		/// Previews the path.
		bool previewPath(bool bRemove = false);
		/// Removes the path preview.
		bool removePreview();
		/// Gets the path preview setting.
		bool isPathPreviewed() const;

		/// Gets the strafe move setting.
		bool getStrafeMove() const;

		/// Sets _unit in order to abuse low-level pathfinding functions from outside the class.
		void setUnit(BattleUnit* unit);

		/// Determines whether the unit is going up a stairs.
//kL		bool isOnStairs(const Position& startPosition, const Position& endPosition);

		/// Gets the modifier setting.
		bool isModifierUsed() const;
};

}

#endif
