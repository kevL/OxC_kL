/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#ifndef OPENXCOM_GRAPHSSTATE_H
#define OPENXCOM_GRAPHSSTATE_H

#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

class Surface;
class InteractiveSurface;
class Text;
class TextButton;
class ToggleTextButton;
class TextList;
class Region;
class NumberText; // kL
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
	InteractiveSurface* _bg;
	InteractiveSurface* _btnGeoscape;
	InteractiveSurface* _btnXcomCountry, * _btnUfoCountry;
	InteractiveSurface* _btnXcomRegion, * _btnUfoRegion;
	InteractiveSurface* _btnIncome, * _btnFinance;
	Text* _txtTitle, * _txtFactor;
	TextList* _txtMonths, * _txtYears;
	ToggleTextButton* _btnRegionTotal, * _btnCountryTotal;
//	NumberText* _numRegionAlien, * _numRegionXcom, * _numCountryAlien, * _numCountryXcom;

	std::vector<GraphBtnInfo*> _regionToggles, _countryToggles;
	std::vector<Surface*> _alienRegionLines, _alienCountryLines;
	std::vector<Surface*> _xcomRegionLines, _xcomCountryLines;
	std::vector<Surface*> _financeLines, _incomeLines;
	std::vector<Text*> _txtScale;
	std::vector<ToggleTextButton*> _btnRegions, _btnCountries, _btnFinances;
	std::vector<NumberText*> _numRegionActivityAlien, _numRegionActivityXCom, _numCountryActivityAlien, _numCountryActivityXCom;

	std::vector<bool> _financeToggles;

	bool _alien, _income, _country, _finance;

	static const unsigned int GRAPH_MAX_BUTTONS = 16;

	/// will be only between 0 and size()
	unsigned int _btnRegionsOffset, _btnCountriesOffset;
	/// Scroll button lists: scroll and repaint buttons functions
	void scrollButtons(
			std::vector<GraphBtnInfo*>& toggles,
			std::vector<ToggleTextButton*>& buttons,
			unsigned int& offset,
			int step);
	///
	void updateButton(GraphBtnInfo* from, ToggleTextButton* to);
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
		void updateScale(double lowerLimit, double upperLimit, int grid = 9);
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
