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

#ifndef OPENXCOM_MONTHLYREPORTSTATE_H
#define OPENXCOM_MONTHLYREPORTSTATE_H

//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class SavedGame;
class Soldier;
class Text;
class TextButton;
class Window;


/**
 * Report screen shown monthly to display the player's performance and funding.
 */
class MonthlyReportState
	:
		public State
{

private:
	bool
		_gameOver;
//		_psi;
	int
		_deltaFunds,
		_ratingLast,
		_ratingTotal;

	SavedGame* _gameSave;
	Text
		* _txtChange,
		* _txtDesc,
		* _txtFailure,
		* _txtMonth,
		* _txtRating,
		* _txtTitle;
	TextButton
		* _btnOk,
		* _btnBigOk;
	Window* _window;

	std::vector<std::string>
		_happyList,
		_pactList,
		_sadList;

	std::vector<Soldier*> _soldiersMedalled;

	/// Builds a country list string.
	std::wstring countryList(
			const std::vector<std::string>& countries,
			const std::string& singular,
			const std::string& plural);
	/// Calculates monthly scores.
	void calculateChanges();


	public:
		/// Creates the Monthly Report state.
		MonthlyReportState();
//				bool psi,
//				Globe* globe);
		/// Cleans up the Monthly Report state.
		~MonthlyReportState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
