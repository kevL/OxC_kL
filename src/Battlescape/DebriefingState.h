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

#ifndef OPENXCOM_DEBRIEFINGSTATE_H
#define OPENXCOM_DEBRIEFINGSTATE_H

//#include <map>
//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 *
 */
struct DebriefingStat
{
	bool recover;
	int
		qty,
		score;
	std::string item;

	/// cTor.
	DebriefingStat(
			const std::string& type,
			bool recover = false)
		:
			item(type),
			score(0),
			qty(0),
			recover(recover)
	{};
};

/**
 *
 */
struct ReequipStat
{
	std::string type;
	int qtyLost;
	std::wstring craft;
};

/**
 *
 */
struct SpecialType
{
	std::string type;
	int value;
};


class Base;
class BattleItem;
class BattleUnit;
class Country;
class Craft;
class Region;
class RuleItem;
class Ruleset;
class SavedGame;
class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Debriefing screen shown after a Battlescape mission that displays the results.
 */
class DebriefingState
	:
		public State
{

private:
	bool
		_destroyXCOMBase,
		_manageContainment,
		_noContainment,
		_skirmish;
	int
		_aliensControlled,
		_aliensKilled,
		_aliensStunned,
		_missionCost,
		_diff,
		_limitsEnforced;

	std::string _music;

	Base* _base;
	Craft* _craft;
	Country* _country;
	Region* _region;
	Ruleset* _rules;
	SavedGame* _gameSave;
	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtItem,
		* _txtQuantity,
		* _txtRating,
		* _txtRecovery,
		* _txtScore,
		* _txtTitle;
	TextButton* _btnOk;
	TextList
		* _lstRecovery,
		* _lstStats,
		* _lstTotal;
	Window* _window;

	MissionStatistics* _missionStatistics;

	std::map<int, SpecialType*> _specialTypes;
	std::map<RuleItem*, int> _rounds;

	std::vector<ReequipStat> _missingItems;
	std::vector<DebriefingStat*> _stats;
	std::vector<Soldier*> _soldiersMedalled;
	std::vector<SoldierDead*> _soldiersLost;

	/// Adds to the debriefing stats.
	void addStat(
			const std::string& type,
			int score,
			int quantity = 1);
	/// Prepares debriefing.
	void prepareDebriefing();
	/// Recovers items from the battlescape.
	void recoverItems(std::vector<BattleItem*>* const battleItems);
	/// Recovers an alien from the battlescape.
	void recoverLiveAlien(BattleUnit* const unit);
	/// Reequips a craft after a mission.
	void reequipCraft(Craft* craft = NULL);


	public:
		/// Creates the Debriefing state.
		DebriefingState();
		/// Cleans up the Debriefing state.
		~DebriefingState();

		/// Initializes the state.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
