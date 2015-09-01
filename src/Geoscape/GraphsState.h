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

#ifndef OPENXCOM_GRAPHSSTATE_H
#define OPENXCOM_GRAPHSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class InteractiveSurface;
class Region;
class Sound;
class Surface;
class Text;
class TextButton;
class TextList;
class Timer;
class ToggleTextButton;

struct GraphBtnInfo;


/**
 * Graphs screen for displaying graphs of various monthly game data like
 * activity and funding.
 */
class GraphsState
	:
		public State
{

private:
	static const size_t GRAPH_BUTTONS = 19; // # visible btns. Does not include TOTAL btn.

	bool
		_alien,
		_country,
		_finance,
		_income,
		_forceVis,	// true to ensure values are displayed when scrolling buttons
		_reset;		// true to stop buttons blinking & reset activity

	int _current;

	size_t
		_btnCountriesOffset,
		_btnRegionsOffset;

	InteractiveSurface
		* _bg,
		* _btnFinance,
		* _btnGeoscape,
		* _btnIncome,
		* _btnUfoCountry,
		* _btnUfoRegion,
		* _btnXcomCountry,
		* _btnXcomRegion;
	Text
		* _txtScore,
		* _txtFactor,
		* _txtTitle;
	TextButton* _btnReset;
	TextList
		* _txtMonths,
		* _txtYears;
	ToggleTextButton
		* _btnCountryTotal,
		* _btnRegionTotal;

	Timer* _blinkTimer;

	std::vector<bool>
		_blinkCountry,
		_blinkCountryXCom,
		_blinkRegion,
		_blinkRegionXCom,
		_financeToggles;

	std::vector<GraphBtnInfo*>
		_countryToggles,
		_regionToggles;
	std::vector<Surface*>
		_alienCountryLines,
		_alienRegionLines,
		_financeLines,
		_incomeLines,
		_xcomCountryLines,
		_xcomRegionLines;
	std::vector<Text*>
		_txtCountryActivityAlien,
		_txtCountryActivityXCom,
		_txtRegionActivityAlien,
		_txtRegionActivityXCom,
		_txtScale;
	std::vector<ToggleTextButton*>
		_btnCountries,
		_btnFinances,
		_btnRegions;

	/// Blinks recent activity.
	void blinkButtons();
	/// Shifts buttons to their pre-Graph cTor row.
	void initButtons();
	/// Scroll button lists: scroll and repaint buttons' functions.
	void scrollButtons(
			std::vector<GraphBtnInfo*>& toggles,
			std::vector<ToggleTextButton*>& buttons,
			std::vector<Text*>& actAlien,
			std::vector<Text*>& actXCom,
			std::vector<bool>& blink,
			std::vector<bool>& blinkXCom,
			size_t& btnOffset,
			int dir);
	///
	void updateButton(
			GraphBtnInfo* info,
			ToggleTextButton* btn,
			Text* aliens,
			Text* xcom);


	public:
		/// Creates the Graphs state.
		explicit GraphsState(int curGraph = 0);
		/// Cleans up the Graphs state.
		~GraphsState();

		/// Handles the blink timer.
		void think();

		/// Handler for clicking the Geoscape icon.
		void btnGeoscapeClick(Action* action);
		/// Handler for clicking the ufo region icon.
		void btnUfoRegionClick(Action* action);
		/// Handler for clicking the xcom region icon.
		void btnXcomRegionClick(Action* action);
		/// Handler for clicking the ufo country icon.
		void btnUfoCountryClick(Action* action);
		/// Handler for clicking the xcom country icon.
		void btnXcomCountryClick(Action* action);
		/// Handler for clicking the income icon.
		void btnIncomeClick(Action* action);
		/// Handler for clicking the finance icon.
		void btnFinanceClick(Action* action);
		/// Handler for clicking on a region button.
		void btnRegionListClick(Action* action);
		/// Handler for clicking on a country button.
		void btnCountryListClick(Action* action);
		/// Handler for clicking  on a finances button.
		void btnFinanceListClick(Action* action);
		/// Resets aLien/xCom activity and the blink indicators.
		void btnResetPress(Action* action);

		/// Reset all the elements on screen.
		void resetScreen();
		/// Update the scale
		void updateScale(
				double lowerLimit,
				double upperLimit,
				int grid = 9);

		/// Decide which lines to draw
		void drawLines();
		/// Draw Region Lines.
		void drawRegionLines();
		/// Draw Country Lines.
		void drawCountryLines();
		/// Draw Finances Lines.
		void drawFinanceLines();

		/// Mouse wheel handler for shifting up/down the buttons
		void shiftButtons(Action* action);
};

}

#endif
