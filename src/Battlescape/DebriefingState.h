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

#ifndef OPENXCOM_DEBRIEFINGSTATE_H
#define OPENXCOM_DEBRIEFINGSTATE_H

#include <map>
#include <string>
#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class BattleItem;
class Country;
class Craft;
class Region;
class RuleItem;
class Text;
class TextButton;
class TextList;
class Window;


struct DebriefingStat
{
	DebriefingStat(
			std::string _item,
			bool recovery = false)
		:
			item(_item),
			qty(0),
			score(0),
			recovery(recovery)
	{
	};

	bool recovery;
	int
		qty,
		score;
	std::string item;
};


struct ReequipStat
{
	std::string item;
	int qty;
	std::wstring craft;
};


/**
 * Debriefing screen shown after a Battlescape
 * mission that displays the results.
 */
class DebriefingState
	:
		public State
{

private:
	bool
		_destroyBase,
		_manageContainment,
		_noContainment;
	int _containmentLimit;

	Base* _base;
	Country* _country;
	Region* _region;
	Text
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

	std::map<RuleItem*, int> _rounds;
	std::vector<ReequipStat> _missingItems;
	std::vector<DebriefingStat*> _stats;

	/// Adds to the debriefing stats.
	void addStat(
			const std::string& name,
			int quantity,
			int score);
	/// Prepares debriefing.
	void prepareDebriefing();
	/// Recovers items from the battlescape.
	void recoverItems(
			std::vector<BattleItem*>* from,
			Base* base);
	/// Reequips a craft after a mission.
	void reequipCraft(
			Base* base,
			Craft* craft,
			bool vehicleItemsCanBeDestroyed);


	public:
		/// Creates the Debriefing state.
		DebriefingState(Game* game);
		/// Cleans up the Debriefing state.
		~DebriefingState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
