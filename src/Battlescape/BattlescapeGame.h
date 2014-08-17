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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPEGAME_H
#define OPENXCOM_BATTLESCAPEGAME_H

#include <list>
#include <string>
#include <vector>

#include <SDL.h>

#include "Position.h"


namespace OpenXcom
{

class BattleItem;
class BattlescapeState;
class BattleState;
class BattleUnit;
class InfoboxOKState;
class Map;
class Pathfinding;
class ResourcePack;
class Ruleset;
class SavedBattleGame;
// class SoldierDiary;
class TileEngine;


enum BattleActionType
{
	BA_NONE,		// 0
	BA_TURN,		// 1
	BA_WALK,		// 2
	BA_PRIME,		// 3
	BA_THROW,		// 4
	BA_AUTOSHOT,	// 5
	BA_SNAPSHOT,	// 6
	BA_AIMEDSHOT,	// 7
	BA_STUN,		// 8
	BA_HIT,			// 9
	BA_USE,			// 10
	BA_LAUNCH,		// 11
	BA_MINDCONTROL,	// 12
	BA_PANIC,		// 13
	BA_RETHINK,		// 14
	BA_DEFUSE		// 15, kL_add.
};


struct BattleAction
{
	bool
		autoShotKill, // kL
		desperate, // ignoring newly-spotted units
		finalAction,
		run,
		strafe,
		targeting;
	int
		autoShotCount,
		diff,
		finalFacing,
		number, // first action of turn, second, etc.?
		TU,
		value;

	std::string result;

	BattleActionType type;
	BattleItem* weapon;
	BattleUnit* actor;
	Position
		cameraPosition,
		target;

	std::list<Position> waypoints;


	BattleAction()
		:
			type(BA_NONE),
			actor(NULL),
			weapon(NULL),
			TU(0),
			targeting(false),
			value(0),
			result(""),
			strafe(false),
			run(false),
			diff(0),
			autoShotCount(0),
			autoShotKill(false), // kL
			cameraPosition(0, 0,-1),
			desperate(false),
			finalFacing(-1),
			finalAction(false),
			number(0)
	{
	}
};


/**
 * Battlescape game - the core game engine of the battlescape game.
 */
class BattlescapeGame
{

private:
	bool
		_AISecondMove,
		_endTurnRequested,
//		_kneelReserved,
		_playedAggroSound,
		_playerPanicHandled;
	int _AIActionCounter;

	BattleAction _currentAction;

	BattlescapeState* _parentState;
//	BattleActionType
//		_batReserved,
//		_playerBATReserved;
	SavedBattleGame* _save;

	std::list<BattleState*>
		_deleted, // delete CTD
		_states;
	std::vector<InfoboxOKState*> _infoboxQueue;


	/// Ends the turn.
	void endTurn();
	/// Picks the first soldier that is panicking.
	bool handlePanickingPlayer();
	/// Common function for hanlding panicking units.
	bool handlePanickingUnit(BattleUnit* unit);
	/// Determines whether there are any actions pending for the given unit.
	bool noActionsPending(BattleUnit* bu);
	/// Shows the infoboxes in the queue (if any).
	void showInfoBoxQueue();


	public:
		/// Creates the BattlescapeGame state.
		BattlescapeGame(
				SavedBattleGame* save,
				BattlescapeState* parentState);
		/// Cleans up the BattlescapeGame state.
		~BattlescapeGame();

		/// Checks for units panicking or falling and so on.
		void think();
		/// Initializes the Battlescape game.
		void init();

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected();
		/// Handles states timer.
		void handleState();
		/// Pushes a state to the front of the list.
		void statePushFront(BattleState* bs);
		/// Pushes a state to second on the list.
		void statePushNext(BattleState* bs);
		/// Pushes a state to the back of the list.
		void statePushBack(BattleState* bs);
		/// Handles the result of non target actions, like priming a grenade.
		void handleNonTargetAction();
		/// Removes current state.
		void popState();
		/// Sets state think interval.
		void setStateInterval(Uint32 interval);
		/// Checks for casualties in battle.
		void checkForCasualties(
				BattleItem* weapon,
				BattleUnit* slayer,
				bool hidden = false,
				bool terrain = false);
		/// Checks if a unit panics.
//kL	void checkForPanic(BattleUnit *unit);
		/// Checks reserved tu.
		bool checkReservedTU(
				BattleUnit* bu,
				int tu,
				bool test = false);
		/// Handles unit AI.
		void handleAI(BattleUnit* unit);
		/// Drops an item and affects it with gravity.
		void dropItem(
				const Position& position,
				BattleItem* item,
				bool newItem = false,
				bool removeItem = false);
		/// Converts a unit into a unit of another type.
		BattleUnit* convertUnit(
				BattleUnit* unit,
				std::string convertType,
				int dirFace = 3); // kL_add.
		/// Handles kneeling action.
		bool kneel(
				BattleUnit* bu,
				bool calcFoV = true);
		/// Cancels the current action.
		bool cancelCurrentAction(bool bForce = false);
		/// Gets a pointer to access action members directly.
		BattleAction* getCurrentAction();
		/// Determines whether there is an action currently going on.
		bool isBusy();
		/// Activates primary action (left click).
		void primaryAction(const Position& pos);
		/// Activates secondary action (right click).
		void secondaryAction(const Position& pos);
		/// Handler for the blaster launcher button.
		void launchAction();
		/// Handler for the psi button.
		void psiButtonAction();
		/// Moves a unit up or down.
		void moveUpDown(
				BattleUnit* unit,
				int dir);
		/// Requests the end of the turn (wait for explosions etc to really end the turn).
		void requestEndTurn();
		/// Sets the TU reserved type.
		void setTUReserved(
				BattleActionType bat);
//				bool player);
		/// Sets up the cursor taking into account the action.
		void setupCursor();

		/// Gets the map.
		Map* getMap();
		/// Gets the save.
		SavedBattleGame* getSave();
		/// Gets the tilengine.
		TileEngine* getTileEngine();
		/// Gets the pathfinding.
		Pathfinding* getPathfinding();
		/// Gets the resourcepack.
		ResourcePack* getResourcePack();

		/// Gets the ruleset.
		const Ruleset* getRuleset() const;
		///
		static bool _debugPlay;
		/// Returns whether panic has been handled.
		bool getPanicHandled()
		{
			return _playerPanicHandled;
		}
		/// Tries to find an item and pick it up if possible.
		void findItem(BattleAction* action);
		/// Checks through all the items on the ground and picks one.
		BattleItem* surveyItems(BattleAction* action);
		/// Evaluates if it's worthwhile to take this item.
		bool worthTaking(
				BattleItem* item,
				BattleAction* action);
		/// Picks the item up from the ground.
		int takeItemFromGround(
				BattleItem* item,
				BattleAction* action);
		/// Assigns the item to a slot (stolen from battlescapeGenerator::addItem()).
		bool takeItem(
				BattleItem* item,
				BattleAction* action);
		/// Returns the type of action that is reserved.
		BattleActionType getReservedAction();
		/// Tallies the living units, converting them if necessary.
		void tallyUnits(
				int& liveAliens,
				int& liveSoldiers,
				bool convert = false);
		/// Sets the kneel reservation setting.
		void setKneelReserved(bool reserved);
		/// Checks the kneel reservation setting.
		bool getKneelReserved();
		/// Checks for and triggers proximity grenades.
		bool checkForProximityGrenades(BattleUnit* unit);

		/// Delete all battlestates that are queued for deletion.
		void cleanupDeleted();

		/// kL. Gets the BattlescapeState.
		BattlescapeState* getBattlescapeState() const;
};

}

#endif
