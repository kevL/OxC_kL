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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPEGAME_H
#define OPENXCOM_BATTLESCAPEGAME_H

//#include <list>
//#include <string>
//#include <vector>
//#include <SDL.h>

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
class TileEngine;


enum BattleActionType
{
	BA_NONE,		//  0
	BA_TURN,		//  1
	BA_MOVE,		//  2
	BA_PRIME,		//  3
	BA_THROW,		//  4
	BA_AUTOSHOT,	//  5
	BA_SNAPSHOT,	//  6
	BA_AIMEDSHOT,	//  7
	BA_HIT,			//  8
	BA_USE,			//  9
	BA_LAUNCH,		// 10
	BA_PSICONTROL,	// 11
	BA_PSIPANIC,	// 12
	BA_RETHINK,		// 13
	BA_DEFUSE,		// 14, kL_add ->>
	BA_DROP,		// 15
	BA_PSICONFUSE,	// 16 reduces victim's TU
	BA_PSICOURAGE,	// 17 increases morale of ally
	BA_EXECUTE		// 18
};

/**
 *
 */
struct BattleAction
{
	bool
		desperate, // ignoring newly-spotted units
		finalAction,
		dash,
		strafe,
		takenXP,
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
	BattleUnit
		* actor,
		* targetUnit;
	Position
		cameraPosition,
		target;

	std::list<Position> waypoints;


	BattleAction()
		:
			type(BA_NONE),
			actor(NULL),
			targetUnit(NULL),
			weapon(NULL),
			TU(0),
			targeting(false),
			value(0),
			strafe(false),
			dash(false),
			diff(0),
			autoShotCount(0),
			cameraPosition(0,0,-1),
			desperate(false),
			finalFacing(-1),
			finalAction(false),
			number(0),
			takenXP(false)
	{}

	public:
		/// kL. heh This could cause problems.
		/// @note This is not to be used unless instantiating a BattlescapeGame.
		void clearAction() // kL
		{
			type = BA_NONE;
			actor = NULL;
			targetUnit = NULL;
			weapon = NULL;
			TU = 0;
			targeting = false;
			value = 0;
			result = "";
			strafe = false;
			dash = false;
			diff = 0;
			autoShotCount = 0;
			cameraPosition = Position(0,0,-1);
			desperate = false;
			finalFacing = -1;
			finalAction = false;
			number = 0;
			takenXP = false;
			waypoints.clear();
		}
};


/**
 * Battlescape game - the core game engine of the battlescape game.
 */
class BattlescapeGame // * no copy cTor.
{

private:
	bool
		_AISecondMove,
//		_endTurnProcessed,
		_endTurnRequested,
		_executeProgress,
		_firstInit,
		_playedAggroSound,
		_playerPanicHandled;
	int _AIActionCounter;

	BattleAction _currentAction;

	BattleItem
		* _alienPsi,
		* _universalFist;
	BattlescapeState* _parentState;
	SavedBattleGame* _battleSave;

	std::list<BattleState*>
		_deleted,
		_states;
	std::vector<InfoboxOKState*> _infoboxQueue;


	/// Determines whether there are any actions pending for the given unit.
	bool noActionsPending(const BattleUnit* const unit) const;
	/// Summary execution.
	void executeUnit();
	/// Ends the turn.
	void endTurnPhase();
	/// Picks the first soldier that is panicking.
	bool handlePanickingPlayer();
	/// Common function for hanlding panicking units.
	bool handlePanickingUnit(BattleUnit* const unit);
	/// Shows the infoboxes in the queue (if any).
	void showInfoBoxQueue();


	public:
		static bool _debugPlay;


		/// Creates the BattlescapeGame state.
		BattlescapeGame(
				SavedBattleGame* battleSave,
				BattlescapeState* parentState);
		/// Cleans up the BattlescapeGame state.
		~BattlescapeGame();

		/// Initializes the Battlescape game.
		void init();
		/// Checks for units panicking or falling and so on.
		void think();

		/// Handles states timer.
		void handleState();
		/// Pushes a state to the front of the list.
		void statePushFront(BattleState* const battleState);
		/// Pushes a state to second on the list.
		void statePushNext(BattleState* const battleState);
		/// Pushes a state to the back of the list.
		void statePushBack(BattleState* const battleState);
		/// Removes current state.
		void popState();
		/// Sets state think interval.
		void setStateInterval(Uint32 interval);

		/// Handles unit AI.
		void handleAI(BattleUnit* const unit);

		/// Handles the result of non target actions, like priming a grenade.
		void handleNonTargetAction();

		/// Sets up the cursor taking into account the action.
		void setupCursor();

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected();

		/// Handles kneeling action.
		bool kneel(BattleUnit* const bu);

		/// Checks for casualties in battle.
		void checkForCasualties(
				const BattleItem* const weapon = NULL,
				BattleUnit* attacker = NULL,
				bool hiddenExpl = false,
				bool terrainExpl = false,
				bool execution = false);
		/// Checks reserved tu.
		bool checkReservedTu(
				const BattleUnit* const unit,
				int tu,
				bool test = false);

		/// Cancels the current action.
		bool cancelCurrentAction(bool force = false);
		/// Gets a pointer to access action members directly.
		BattleAction* getCurrentAction();

		/// Determines whether there is an action currently going on.
		bool isBusy() const;

		/// Activates primary action (left click).
		void primaryAction(const Position& posTarget);
		/// Activates secondary action (right click).
		void secondaryAction(const Position& posTarget);
		/// Handler for the blaster launcher button.
		void launchAction();
		/// Handler for the psi button.
		void psiButtonAction();

		/// Moves a unit up or down.
		void moveUpDown(
				const BattleUnit* const unit,
				int dir);

		/// Requests the end of the turn (wait for explosions etc to really end the turn).
		void requestEndTurn();

		/// Drops an item and affects it with gravity.
		void dropItem(
				const Position& pos,
				BattleItem* const item,
				bool created = false,
				bool disown = false);

		/// Converts a unit into a unit of another type.
		BattleUnit* convertUnit(
				BattleUnit* const unit,
				const std::string& conType);

		/// Gets the map.
		Map* getMap() const;
		/// Gets the save.
		SavedBattleGame* getSave() const;
		/// Gets the tilengine.
		TileEngine* getTileEngine() const;
		/// Gets the pathfinding.
		Pathfinding* getPathfinding() const;
		/// Gets the resourcepack.
		ResourcePack* getResourcePack() const;
		/// Gets the ruleset.
		const Ruleset* const getRuleset() const;

		/// Returns whether player panic has been handled.
		bool getPanicHandled() const
		{ return _playerPanicHandled; }
		/// Sets var to start handling panic for Player.
		void setPlayerPanic() { _playerPanicHandled = false; }

		/// Tries to find an item and pick it up if possible.
		void findItem(BattleAction* const action) const;
		/// Checks through all the items on the ground and picks one.
		BattleItem* surveyItems(BattleUnit* const unit) const;
		/// Evaluates if it's worthwhile to take this item.
		bool worthTaking(
				BattleItem* const item,
				BattleUnit* const unit) const;
		/// Picks the item up from the ground.
		int takeItemFromGround(
				BattleItem* const item,
				BattleUnit* const unit) const;
		/// Assigns the item to a slot (stolen from battlescapeGenerator::addItem()).
		bool takeItem(
				BattleItem* const item,
				BattleUnit* const unit) const;

		/// Sets the TU reserved type.
		void setReservedAction(BattleActionType bat);
		/// Returns the type of action that is reserved.
		BattleActionType getReservedAction() const;

		/// Tallies the living units.
		bool tallyUnits(
				int& liveAliens,
				int& liveSoldiers) const;

		/// Sets the kneel reservation setting.
		void setKneelReserved(bool reserved);
		/// Checks the kneel reservation setting.
		bool getKneelReserved();

		/// Checks for and triggers proximity grenades.
		bool checkForProximityGrenades(BattleUnit* const unit);

		/// Cleans up all the deleted states. [Delete all battlestates that are queued for deletion.]
		void cleanupDeleted();

		/// Gets the BattlescapeState.
		BattlescapeState* getBattlescapeState() const;

		/// Gets the universal fist.
		BattleItem* getFist() const;
		/// Gets the alienPsi weapon.
		BattleItem* getAlienPsi() const;

		/// Sets up a mission complete notification.
		void objectiveDone();

		/// Sets if an execution is underway.
		void setExecution(bool execute = true);
		/// Gets if an execution is underway.
		bool getExecution() const;
};

}

#endif
