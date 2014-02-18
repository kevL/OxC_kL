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

#ifndef OPENXCOM_GRAPHSSTATE_H
#define OPENXCOM_GRAPHSSTATE_H

#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class InteractiveSurface;
//class NumberText; // kL
class Region;
class Surface;
class Text;
class TextButton;
class TextList;
class ToggleTextButton;

struct GraphBtnInfo;


/**
 * Graphs screen for displaying graphs of various
 * monthly game data like activity and funding.
 */
class GraphsState
	:
		public State
{

private:
	static const unsigned int GRAPH_MAX_BUTTONS = 16;

	bool
		_alien,
		_country,
		_finance,
		_income;

	unsigned int // will be only between 0 and size()
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
//	NumberText* _numScore;
	Text
		* _numScore,
		* _txtFactor,
		* _txtTitle;
	TextList
		* _txtMonths,
		* _txtYears;
	ToggleTextButton
		* _btnCountryTotal,
		* _btnRegionTotal;

	std::vector<bool> _financeToggles;
	std::vector<GraphBtnInfo*>
		_regionToggles,
		_countryToggles;
	std::vector<Surface*>
		_alienCountryLines,
		_alienRegionLines,
		_financeLines,
		_incomeLines,
		_xcomCountryLines,
		_xcomRegionLines;
//	std::vector<NumberText*>
	std::vector<Text*>
		_numCountryActivityAlien,
		_numCountryActivityXCom,
		_numRegionActivityAlien,
		_numRegionActivityXCom,
		_txtScale;
	std::vector<ToggleTextButton*>
		_btnRegions,
		_btnCountries,
		_btnFinances;

	/// Scroll button lists: scroll and repaint buttons functions
	void scrollButtons(
			std::vector<GraphBtnInfo*>& toggles,
			std::vector<ToggleTextButton*>& buttons,
			unsigned int& offset,
			int step);
	///
	void updateButton(
			GraphBtnInfo* from,
			ToggleTextButton* to);
	/// Show the latest month's value as NumberText beside the buttons.
	void latestTally();


	public:
		/// Creates the Graphs state.
		GraphsState(Game* game);
		/// Cleans up the Graphs state.
		~GraphsState();

		/// Handler for clicking the Geoscape icon.
		void btnGeoscapeClick(Action* action);
		/// Handler for clicking the ufo region icon.
		void btnUfoRegionClick(Action* action);
		/// Handler for clicking the ufo country icon.
		void btnUfoCountryClick(Action* action);
		/// Handler for clicking the xcom region icon.
		void btnXcomRegionClick(Action* action);
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
		/// Mouse wheel handler for shifting up/down the buttons
		void shiftButtons(Action* action);

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
};

}

#endif
