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
		_init,
		_forceVis,	// true to ensure values are displayed when scrolling buttons
		_reset;		// true to stop buttons blinking & reset activity

	size_t
		_btnCountryOffset;
//		_btnRegionOffset;

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
		* _lstMonths,
		* _lstYears;
	ToggleTextButton
		* _btnCountryTotal,
		* _btnRegionTotal;

	Timer* _blinkTimer;

	std::vector<bool>
		_blinkCountryAlien,
		_blinkCountryXCom,
		_blinkRegionAlien,
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

	/// Recalls buttons to their pre-Graph cTor row.
	void initButtons();

	/// Blinks recent activity-values.
	void blink();

	/// Resets aLien/xCom activity and the blink indicators.
	void btnResetPress(Action* action);

	/// Resets all the elements on screen.
	void resetScreen();
	/// Updates the scale.
	void updateScale(
			float valLow,
			float valHigh);

	/// Decides which line-drawing-routine to call.
	void drawLines();
	/// Draws Region lines.
	void drawRegionLines();
	/// Draws Country lines.
	void drawCountryLines();
	/// Draws Finances lines.
	void drawFinanceLines();

	/// Mouse-wheel handler for shifting up/down the buttons.
	void shiftButtons(Action* action);
	/// Scrolls button lists - scroll and repaint buttons' functions.
	void scrollButtons(
			int dirVal,
			bool init = false);
/*	void scrollButtons(
			std::vector<GraphBtnInfo*>& toggles,
			std::vector<ToggleTextButton*>& buttons,
			std::vector<Text*>& actA_vect,
			std::vector<Text*>& actX_vect,
			std::vector<bool>& blinkA,
			std::vector<bool>& blinkX,
			size_t& btnOffset,
			int dir,
			bool init = false); */
	///
	void updateButton(
			GraphBtnInfo* info,
			ToggleTextButton* btn,
			Text* aLiens,
			Text* xCom);


	public:
		/// Creates the Graphs state.
		GraphsState();
		/// Cleans up the Graphs state.
		~GraphsState();

		/// Handles state thinking.
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
};

}

#endif
