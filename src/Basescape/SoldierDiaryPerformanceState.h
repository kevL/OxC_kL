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
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_SOLDIERDIARYPERFORMANCESTATE_H
#define OPENXCOM_SOLDIERDIARYPERFORMANCESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Soldier;
class SoldierDead; // kL
class SoldierDiaryOverviewState;
class Surface;
class SurfaceSet;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Diary screen that lists totals.
 */
class SoldierDiaryPerformanceState
	:
		public State
{

private:
	static const int
		LIST_ROWS		= 12,
		LIST_SPRITES_y	= 49;

	bool
		_displayKills,
		_displayMissions,
		_displayAwards;
	size_t
		_lastScrollPos,
		_soldierID;

	std::vector<std::wstring> _commendationsListEntry;

	std::vector<Soldier*>* _list;
	std::vector<SoldierDead*>* _listDead; // kL
	std::vector<Surface*>
		_srfSprite,
		_srfDecor;

	Base* _base;
	Soldier* _soldier;
	SoldierDead* _soldierDead; // kL
	SoldierDiaryOverviewState* _soldierDiaryOverviewState;

	SurfaceSet
		* _sstSprite,
		* _sstDecor;
	Text
		* _txtTitle,
		* _txtBaseLabel, // kL

		* _txtRank,
		* _txtRace,
		* _txtWeapon,

		* _txtLocation,
		* _txtType,
		* _txtUFO,

		* _txtMedalName,
		* _txtMedalLevel,
		* _txtMedalClass, // kL
		* _txtMedalInfo;
	TextButton
		* _btnOk,
		* _btnPrev,
		* _btnNext,
		* _btnKills,
		* _btnMissions,
		* _btnAwards;
	TextList
		* _lstRank,
		* _lstRace,
		* _lstWeapon,
		* _lstKillTotals,

		* _lstLocation,
		* _lstType,
		* _lstUFO,
		* _lstMissionTotals,

		* _lstAwards;
	Window
		* _window;


	public:
		/// Creates the Soldiers state.
		SoldierDiaryPerformanceState(
				Base* const base,
				const size_t soldierID,
				SoldierDiaryOverviewState* const soldierDiaryState,
				const int display);
		/// Cleans up the Soldiers state.
		~SoldierDiaryPerformanceState();

		/// Updates the soldier info.
		void init();

		/// Draw sprites.
		void drawSprites();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);
		/// Handler for clicking the Kills button.
		void btnKillsToggle(Action* action);
		/// Handler for clicking the Missions button.
		void btnMissionsToggle(Action* action);
		/// Handler for clicking the Missions button.
		void btnCommendationsToggle(Action* action);
		/// Handler for moving the mouse over a medal.
		void lstInfoMouseOver(Action* action);
		/// Handler for moving the mouse outside the medals list.
		void lstInfoMouseOut(Action* action);

		/// Runs state functionality every cycle.
		void think();
};

}

#endif
