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

#include "GraphsState.h"

//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

static size_t recallRow = 0;


/**
 * Helper struct for scrolling past GRAPH_BUTTONS.
 */
struct GraphBtnInfo
{
	LocalizedText _name;
	Uint8
		_color,
		_colorTxt;
	int
		_actA,
		_actX;
	bool
		_blinkA,
		_blinkX,
		_pushed;

	/// Builds this struct.
	GraphBtnInfo(
			const LocalizedText& name,
			Uint8 color,
			int actA,
			int actX,
			Uint8 colorTxt,
			bool blinkA,
			bool blinkX)
		:
			_name(name),
			_color(color),
			_actA(actA),
			_actX(actX),
			_colorTxt(colorTxt),
			_blinkA(blinkA),
			_blinkX(blinkX),
			_pushed(false)
	{}
};


/**
 * Initializes all the elements in the Graphs screen.
 * @param curGraph - the graph that was last selected (default 0)
 */
GraphsState::GraphsState(int curGraph)
	:
		_btnRegionsOffset(0),
		_btnCountriesOffset(0),
		_current(-1),
		_reset(false),
		_forceVis(true)
{
	_bg = new InteractiveSurface(320, 200);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDL_BUTTON_WHEELUP);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDL_BUTTON_WHEELDOWN);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_UP);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP8);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_DOWN);
	_bg->onKeyboardPress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDLK_KP2);

	SDL_EnableKeyRepeat(
					180, //SDL_DEFAULT_REPEAT_DELAY,
					60); //SDL_DEFAULT_REPEAT_INTERVAL);

	_btnUfoRegion	= new InteractiveSurface(31, 24,  97);
	_btnXcomRegion	= new InteractiveSurface(31, 24, 129);
	_btnUfoCountry	= new InteractiveSurface(31, 24, 161);
	_btnXcomCountry	= new InteractiveSurface(31, 24, 193);
	_btnIncome		= new InteractiveSurface(31, 24, 225);
	_btnFinance		= new InteractiveSurface(31, 24, 257);
	_btnGeoscape	= new InteractiveSurface(31, 24, 289);

	_btnReset		= new TextButton(40, 16, 96, 26);

	_txtTitle		= new Text(220, 17, 100, 28);
	_txtFactor		= new Text( 35,  9,  96, 28);

	_lstMonths		= new TextList(215, 9, 117, 182); // note These go beyond 320px.
	_lstYears		= new TextList(215, 9, 117, 191);

	_txtScore		= new Text(36, 9, 46, 82);


	setInterface("graphs");

	add(_bg);
	add(_btnUfoRegion);
	add(_btnUfoCountry);
	add(_btnXcomRegion);
	add(_btnXcomCountry);
	add(_btnIncome);
	add(_btnFinance);
	add(_btnGeoscape);
	add(_btnReset,	"button",	"graphs");
	add(_lstMonths,	"scale",	"graphs");
	add(_lstYears,	"scale",	"graphs");
	add(_txtTitle,	"text",		"graphs");
	add(_txtFactor,	"text",		"graphs");
	add(_txtScore);

	for (size_t
			i = 0;
			i != 10;
			++i)
	{
		_txtScale.push_back(new Text(
									32,10,
									91,
									171 - (static_cast<int>(i) * 14)));
		add(_txtScale.at(i), "scale", "graphs");
	}

	size_t btnOffset = 0;
	Uint8 colorOffset = 0;
	int
		actA,
		actX;
	bool
		blinkA,
		blinkX;

	/* REGIONS */
	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i, ++colorOffset, ++btnOffset)
	{
		if (colorOffset == 17) colorOffset = 0;

		actA = (*i)->getActivityAlien().back();
		actX = (*i)->getActivityXCom().back();
		blinkA = (*i)->recentActivityAlien(false, true);
		blinkX = (*i)->recentActivityXCom(false, true);

		// put all the regions into toggles
		_regionToggles.push_back(new GraphBtnInfo(
												tr((*i)->getRules()->getType()), // name of Region
												colorOffset * 8 + 13,
												actA,
												actX,
												colorOffset * 8 + 16,
												blinkA,
												blinkX));

		// first add the GRAPH_BUTTONS (having the respective region's information)
		if (btnOffset < GRAPH_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnRegions.push_back(new ToggleTextButton(
													65,10,
													0,
													static_cast<int>(btnOffset) * 10));
			_btnRegions.at(btnOffset)->setText(tr((*i)->getRules()->getType())); // name of Region
			_btnRegions.at(btnOffset)->setInvertColor(colorOffset * 8 + 13);
			_btnRegions.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::btnRegionListClick,
							SDL_BUTTON_LEFT);
/*			_btnRegions.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELUP);
			_btnRegions.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELDOWN); */

			add(_btnRegions.at(btnOffset), "button", "graphs");

			_txtRegionActivityAlien.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(btnOffset) * 10) + 1));
			_txtRegionActivityAlien.at(btnOffset)->setColor(colorOffset * 8 + 16);
			_txtRegionActivityAlien.at(btnOffset)->setText(Text::formatNumber(actA));

			add(_txtRegionActivityAlien.at(btnOffset));

			_blinkRegionAlien.push_back(blinkA);
			_blinkRegionXCom.push_back(blinkX);


			_txtRegionActivityXCom.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(btnOffset) * 10) + 1));
			_txtRegionActivityXCom.at(btnOffset)->setColor(colorOffset * 8 + 16);
			_txtRegionActivityXCom.at(btnOffset)->setText(Text::formatNumber(actX));

			add(_txtRegionActivityXCom.at(btnOffset));
		}


		_alienRegionLines.push_back(new Surface(320,200));
		add(_alienRegionLines.at(btnOffset));

		_xcomRegionLines.push_back(new Surface(320,200));
		add(_xcomRegionLines.at(btnOffset));
	}


	int btnTotal_y;
	if (_regionToggles.size() < GRAPH_BUTTONS)
		btnTotal_y = static_cast<int>(_regionToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_BUTTONS) * 10;

	_btnRegionTotal = new ToggleTextButton( // TOTAL btn.
										65,10,
										0,
										btnTotal_y * 10);

	Uint8 totalColor = static_cast<Uint8>(
					   _game->getRuleset()->getInterface("graphs")->getElement("regionTotal")->color);

	_regionToggles.push_back(new GraphBtnInfo( // TOTAL btn is the last button in the vector.
											tr("STR_TOTAL_UC"),
											totalColor,
											0,0,0,
											false,false));
	_btnRegionTotal->setInvertColor(totalColor);
	_btnRegionTotal->setText(tr("STR_TOTAL_UC"));
	_btnRegionTotal->onMousePress(
					(ActionHandler)& GraphsState::btnRegionListClick,
					SDL_BUTTON_LEFT);

	add(_btnRegionTotal, "button", "graphs");

	_alienRegionLines.push_back(new Surface(320,200));
	add(_alienRegionLines.at(btnOffset));

	_xcomRegionLines.push_back(new Surface(320,200));
	add(_xcomRegionLines.at(btnOffset));


	btnOffset = 0;
	colorOffset = 0;

	/* COUNTRIES */
	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i, ++colorOffset, ++btnOffset)
	{
		if (colorOffset == 17) colorOffset = 0;

		actA = (*i)->getActivityAlien().back();
		actX = (*i)->getActivityXCom().back();
		blinkA = (*i)->recentActivityAlien(false, true);
		blinkX = (*i)->recentActivityXCom(false, true);

		// put all the countries into toggles
		_countryToggles.push_back(new GraphBtnInfo(
												tr((*i)->getRules()->getType()), // name of Country
												colorOffset * 8 + 13,
												actA,
												actX,
												colorOffset * 8 + 16,
												blinkA,
												blinkX));

		// first add the GRAPH_BUTTONS (having the respective country's information)
		if (btnOffset < GRAPH_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnCountries.push_back(new ToggleTextButton(
													65,10,
													0,
													static_cast<int>(btnOffset) * 10));
			_btnCountries.at(btnOffset)->setText(tr((*i)->getRules()->getType())); // name of Country
			_btnCountries.at(btnOffset)->setInvertColor(colorOffset * 8 + 13);
			_btnCountries.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::btnCountryListClick,
							SDL_BUTTON_LEFT);
/*			_btnCountries.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELUP);
			_btnCountries.at(btnOffset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELDOWN); */

			add(_btnCountries.at(btnOffset), "button", "graphs");

			_txtCountryActivityAlien.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(btnOffset) * 10) + 1));
			_txtCountryActivityAlien.at(btnOffset)->setColor(colorOffset * 8 + 16);
			_txtCountryActivityAlien.at(btnOffset)->setText(Text::formatNumber(actA));

			add(_txtCountryActivityAlien.at(btnOffset));

			_blinkCountryAlien.push_back(blinkA);
			_blinkCountryXCom.push_back(blinkX);


			_txtCountryActivityXCom.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(btnOffset) * 10) + 1));
			_txtCountryActivityXCom.at(btnOffset)->setColor(colorOffset * 8 + 16);
			_txtCountryActivityXCom.at(btnOffset)->setText(Text::formatNumber(actX));

			add(_txtCountryActivityXCom.at(btnOffset));
		}


		_alienCountryLines.push_back(new Surface(320,200));
		add(_alienCountryLines.at(btnOffset));

		_xcomCountryLines.push_back(new Surface(320,200));
		add(_xcomCountryLines.at(btnOffset));

		_incomeLines.push_back(new Surface(320,200));
		add(_incomeLines.at(btnOffset));
	}


	if (_countryToggles.size() < GRAPH_BUTTONS)
		btnTotal_y = static_cast<int>(_countryToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_BUTTONS);

	_btnCountryTotal = new ToggleTextButton( // TOTAL btn.
										65,10,
										0,
										btnTotal_y * 10);

	totalColor = static_cast<Uint8>(
				 _game->getRuleset()->getInterface("graphs")->getElement("countryTotal")->color);

	_countryToggles.push_back(new GraphBtnInfo( // TOTAL btn is the last button in the vector.
											tr("STR_TOTAL_UC"),
											totalColor,
											0,0,0,
											false,false));

	_btnCountryTotal->setInvertColor(totalColor);
	_btnCountryTotal->setText(tr("STR_TOTAL_UC"));
	_btnCountryTotal->onMousePress(
					(ActionHandler)& GraphsState::btnCountryListClick,
					SDL_BUTTON_LEFT);

	add(_btnCountryTotal, "button", "graphs");

	_alienCountryLines.push_back(new Surface(320,200));
	add(_alienCountryLines.at(btnOffset));

	_xcomCountryLines.push_back(new Surface(320,200));
	add(_xcomCountryLines.at(btnOffset));

	_incomeLines.push_back(new Surface(320,200));
	add(_incomeLines.at(btnOffset));


	/* FINANCE */
	for (size_t
			i = 0;
			i != 5;
			++i)
	{
		_btnFinances.push_back(new ToggleTextButton(
												82,16,
												0,
												static_cast<int>(i) * 16));
		_financeToggles.push_back(false);

		Uint8 multi; // switch colors for Income (was yellow) and Maintenance (was green)
		if (i == 0)
			multi = 2;
		else if (i == 2)
			multi = 0;
		else
			multi = static_cast<Uint8>(i);

		_btnFinances.at(i)->setInvertColor(multi * 8 + 13);
		_btnFinances.at(i)->onMousePress((ActionHandler)& GraphsState::btnFinanceListClick);

		add(_btnFinances.at(i), "button", "graphs");

		_financeLines.push_back(new Surface(320,200));
		add(_financeLines.at(i));
	}

	_btnFinances.at(0)->setText(tr("STR_INCOME"));
	_btnFinances.at(1)->setText(tr("STR_EXPENDITURE"));
	_btnFinances.at(2)->setText(tr("STR_MAINTENANCE"));
	_btnFinances.at(3)->setText(tr("STR_BALANCE"));
	_btnFinances.at(4)->setText(tr("STR_SCORE"));


	_txtScore->setColor(49); // Palette::blockOffset(3)+1
	_txtScore->setAlign(ALIGN_RIGHT);


	// Load back all the buttons' toggled states from SavedGame!
	/* REGION TOGGLES */
	std::string graphRegionToggles = _game->getSavedGame()->getGraphRegionToggles();
	while (graphRegionToggles.size() < _regionToggles.size())
	{
		graphRegionToggles.push_back('0');
	}

	for (size_t
			i = 0;
			i != _regionToggles.size();
			++i)
	{
		_regionToggles[i]->_pushed = (graphRegionToggles[i] == '0') ? false : true;

		if (_regionToggles.size() - 1 == i)
			_btnRegionTotal->setPressed(_regionToggles[i]->_pushed);
		else if (i < GRAPH_BUTTONS)
			_btnRegions.at(i)->setPressed(_regionToggles[i]->_pushed);
	}

	/* COUNTRY TOGGLES */
	std::string graphCountryToggles = _game->getSavedGame()->getGraphCountryToggles();
	while (graphCountryToggles.size() < _countryToggles.size())
	{
		graphCountryToggles.push_back('0');
	}

	for (size_t
			i = 0;
			i != _countryToggles.size();
			++i)
	{
		_countryToggles[i]->_pushed = (graphCountryToggles[i] == '0') ? false : true;

		if (_countryToggles.size() - 1 == i)
			_btnCountryTotal->setPressed(_countryToggles[i]->_pushed);
		else if (i < GRAPH_BUTTONS)
			_btnCountries.at(i)->setPressed(_countryToggles[i]->_pushed);
	}

	/* FINANCE TOGGLES */
	std::string graphFinanceToggles = _game->getSavedGame()->getGraphFinanceToggles();
	while (graphFinanceToggles.size() < _financeToggles.size())
	{
		graphFinanceToggles.push_back('0');
	}

	for (size_t
			i = 0;
			i != _financeToggles.size();
			++i)
	{
		_financeToggles[i] = (graphFinanceToggles[i] == '0') ? false : true;
		_btnFinances.at(i)->setPressed(_financeToggles[i]);
	}

	_btnReset->setText(tr("STR_RESET_UC"));
	_btnReset->onMousePress(
					(ActionHandler)& GraphsState::btnResetPress,
					SDL_BUTTON_LEFT);


	Uint8 color;
	const Uint8 gridColor = static_cast<Uint8>(
						   _game->getRuleset()->getInterface("graphs")->getElement("graph")->color);

	_bg->drawRect( // set up the grid
				125,49,
				188,127,
				gridColor);

	for (Sint16
			i = 0;
			i != 5;
			++i)
	{
		for (Sint16
				y = 50 + i;
				y <= 163 + i;
				y += 14)
		{
			for (Sint16
					x = 126 + i;
					x <= 297 + i;
					x += 17)
			{
				if (i == 4)
					color = 0;
				else
					color = gridColor + 1 + static_cast<Uint8>(i);

				_bg->drawRect(
							x,y,
							16 - (i * 2),
							13 - (i * 2),
							color);
			}
		}
	}

	std::string months[] = // set up the horizontal measurement units
	{
		"STR_JAN",
		"STR_FEB",
		"STR_MAR",
		"STR_APR",
		"STR_MAY",
		"STR_JUN",
		"STR_JUL",
		"STR_AUG",
		"STR_SEP",
		"STR_OCT",
		"STR_NOV",
		"STR_DEC"
	};

	// i know using textlist for this is ugly and brutal, but YOU try getting this damn text to line up.
	// also, there's nothing wrong with being ugly or brutal, you should learn tolerance. kL_note: and C++
	_lstMonths->setColumns(12, 17,17,17,17,17,17,17,17,17,17,17,17); // 204 total
	_lstMonths->addRow(12, L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ");
	_lstMonths->setMargin();

	_lstYears->setColumns(6, 34,34,34,34,34,34); // 204 total
	_lstYears->addRow(6, L" ",L" ",L" ",L" ",L" ",L" ");
	_lstYears->setMargin();


	const GameTime* const gt = _game->getSavedGame()->getTime();
	const int year = gt->getYear();
	size_t month = static_cast<size_t>(gt->getMonth());

	for (size_t
			i = 0;
			i != 12;
			++i)
	{
		if (month > 11)
		{
			month = 0;

			if (i == 1)
				_lstYears->setCellText(0, i / 2, Text::intWide(year));
			else
				_lstYears->setCellText(0, 0, Text::intWide(year - 1));
		}

		_lstMonths->setCellText(0, i, tr(months[month++]));
	}

	for (std::vector<Text*>::const_iterator // set up the vertical measurement units
			i = _txtScale.begin();
			i != _txtScale.end();
			++i)
	{
		(*i)->setAlign(ALIGN_RIGHT);
	}


	Surface* srf = _game->getResourcePack()->getSurface("GRAPH.BDY");
	if (srf == NULL)
		srf = _game->getResourcePack()->getSurface("GRAPHS.SPK");
	srf->blit(_bg);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtFactor->setText(L"$1000");


	_btnUfoRegion->onMousePress(
					(ActionHandler)& GraphsState::btnUfoRegionClick,
					SDL_BUTTON_LEFT);
	_btnXcomRegion->onMousePress(
					(ActionHandler)& GraphsState::btnXcomRegionClick,
					SDL_BUTTON_LEFT);
	_btnUfoCountry->onMousePress(
					(ActionHandler)& GraphsState::btnUfoCountryClick,
					SDL_BUTTON_LEFT);
	_btnXcomCountry->onMousePress(
					(ActionHandler)& GraphsState::btnXcomCountryClick,
					SDL_BUTTON_LEFT);

	_btnIncome->onMousePress(
					(ActionHandler)& GraphsState::btnIncomeClick,
					SDL_BUTTON_LEFT);
	_btnFinance->onMousePress(
					(ActionHandler)& GraphsState::btnFinanceClick,
					SDL_BUTTON_LEFT);

	_btnGeoscape->onMousePress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					SDL_BUTTON_LEFT);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyCancel);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyOk);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyOkKeypad);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& GraphsState::btnGeoscapeClick,
					Options::keyGeoGraphs);

	centerAllSurfaces();


	_blinkTimer = new Timer(250);
	_blinkTimer->onTimer((StateHandler)& GraphsState::blink);

	initButtons();

	switch (curGraph)
	{
		case 0: btnUfoRegionClick(NULL);	break;
		case 1: btnXcomRegionClick(NULL);	break;
		case 2: btnUfoCountryClick(NULL);	break;
		case 3: btnXcomCountryClick(NULL);	break;
		case 4: btnIncomeClick(NULL);		break;
		case 5: btnFinanceClick(NULL);		break;

		default:
			btnUfoRegionClick(NULL);
	}
}

/**
 * dTor.
 */
GraphsState::~GraphsState()
{
	delete _blinkTimer;

	std::string toggles;
	for (size_t
			i = 0;
			i != _regionToggles.size();
			++i)
	{
		toggles.push_back(_regionToggles[i]->_pushed? '1': '0');
		delete _regionToggles[i];
	}
	_game->getSavedGame()->setGraphRegionToggles(toggles);

	toggles.clear();
	for (size_t
			i = 0;
			i != _countryToggles.size();
			++i)
	{
		toggles.push_back(_countryToggles[i]->_pushed? '1': '0');
		delete _countryToggles[i];
	}
	_game->getSavedGame()->setGraphCountryToggles(toggles);

	toggles.clear();
	for (size_t
			i = 0;
			i != _financeToggles.size();
			++i)
	{
		toggles.push_back(_financeToggles[i]? '1': '0');
	}
	_game->getSavedGame()->setGraphFinanceToggles(toggles);
}

/**
 * Shifts buttons to their pre-Graph cTor row.
 * @note Countries only since total Regions still fit on the list.
 */
void GraphsState::initButtons() // private.
{
	if (_countryToggles.size() > GRAPH_BUTTONS)
		scrollButtons(
				_countryToggles,
				_btnCountries,
				_txtCountryActivityAlien,
				_txtCountryActivityXCom,
				_blinkCountryAlien,
				_blinkCountryXCom,
				_btnCountriesOffset,
				static_cast<int>(recallRow),
				true);

	for (std::vector<GraphBtnInfo*>::const_iterator
			i = _regionToggles.begin();
			i != _regionToggles.end();
			++i)
	{
		if ((*i)->_blinkA == true || (*i)->_blinkX == true)
		{
			_blinkTimer->start();
			return;
		}
	}
/*	for (std::vector<GraphBtnInfo*>::const_iterator	// not needed because Country-areas are all subsumed within Regions;
			i = _countryToggles.begin();			// that is, if a country is blinking its region will already be blinking.
			i != _countryToggles.end();
			++i)
	{
		if ((*i)->_blinkA == true || (*i)->_blinkX == true)
		{
			_blinkTimer->start();
			return;
		}
	} */
}

/**
 * Handles state thinking.
 */
void GraphsState::think()
{
	_blinkTimer->think(this, NULL);
}

/**
 * Makes recent activity Text blink.
 */
void GraphsState::blink() // private.
{
	static bool vis (true);

	if (_reset == true)
		vis = true;
	else if (_forceVis == true)
	{
		_forceVis = false;
		vis = true;
	}
	else
		vis = !vis;

	size_t offset = 0;

	if (_alien == true
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkRegionAlien.begin();
				i != _blinkRegionAlien.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtRegionActivityAlien.at(offset)->setVisible(vis);
			else
				_txtRegionActivityAlien.at(offset)->setVisible();
		}
	}
	else if (_alien == true
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkCountryAlien.begin();
				i != _blinkCountryAlien.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtCountryActivityAlien.at(offset)->setVisible(vis);
			else
				_txtCountryActivityAlien.at(offset)->setVisible();
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkRegionXCom.begin();
				i != _blinkRegionXCom.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtRegionActivityXCom.at(offset)->setVisible(vis);
			else
				_txtRegionActivityXCom.at(offset)->setVisible();
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkCountryXCom.begin();
				i != _blinkCountryXCom.end();
				++i, ++offset)
		{
			if (*i == true)
				_txtCountryActivityXCom.at(offset)->setVisible(vis);
			else
				_txtCountryActivityXCom.at(offset)->setVisible();
		}
	}

	if (_reset == true)
		_blinkTimer->stop();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnGeoscapeClick(Action*)
{
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

	_game->popState();
	kL_soundPop->play(Mix_GroupAvailable(0));
}

/**
 * Switches to the UFO Regional Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnUfoRegionClick(Action*)
{
	if (_current != 0)
	{
		_current = 0;
		_game->getSavedGame()->setCurrentGraph(0);

		_forceVis =

		_alien = true;
		_income =
		_country =
		_finance = false;

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		resetScreen();
		drawLines();

		_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_AREAS"));
		_btnRegionTotal->setVisible();

		for (std::vector<ToggleTextButton*>::iterator
				i = _btnRegions.begin();
				i != _btnRegions.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::iterator
				i = _txtRegionActivityAlien.begin();
				i != _txtRegionActivityAlien.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the xCom Regional Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnXcomRegionClick(Action*)
{
	if (_current != 1)
	{
		_current = 1;
		_game->getSavedGame()->setCurrentGraph(1);

		_forceVis = true;

		_alien =
		_income =
		_country =
		_finance = false;

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		resetScreen();
		drawLines();

		_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_AREAS"));
		_btnRegionTotal->setVisible();

		for (std::vector<ToggleTextButton*>::iterator
				i = _btnRegions.begin();
				i != _btnRegions.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::iterator
				i = _txtRegionActivityXCom.begin();
				i != _txtRegionActivityXCom.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the UFO Country Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnUfoCountryClick(Action*)
{
	if (_current != 2)
	{
		_current = 2;
		_game->getSavedGame()->setCurrentGraph(2);

		_forceVis =

		_alien =
		_country = true;
		_income =
		_finance = false;

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		resetScreen();
		drawLines();

		_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_COUNTRIES"));
		_btnCountryTotal->setVisible();

		for (std::vector<ToggleTextButton*>::iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::iterator
				i = _txtCountryActivityAlien.begin();
				i != _txtCountryActivityAlien.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the xCom Country Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnXcomCountryClick(Action*)
{
	if (_current != 3)
	{
		_current = 3;
		_game->getSavedGame()->setCurrentGraph(3);

		_forceVis =

		_country = true;
		_alien =
		_income =
		_finance = false;

		_btnReset->setVisible(_blinkTimer->isRunning() == true);
		resetScreen();
		drawLines();

		_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_COUNTRIES"));
		_btnCountryTotal->setVisible();

		for (std::vector<ToggleTextButton*>::iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setVisible();

		for (std::vector<Text*>::iterator
				i = _txtCountryActivityXCom.begin();
				i != _txtCountryActivityXCom.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the Income screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnIncomeClick(Action*)
{
	if (_current != 4)
	{
		_current = 4;
		_game->getSavedGame()->setCurrentGraph(4);

		_income =
		_country = true;
		_alien =
		_finance = false;

		_btnReset->setVisible(false);
		resetScreen();
		drawLines();

		_txtFactor->setVisible();

		_txtTitle->setText(tr("STR_INCOME"));
		_btnCountryTotal->setVisible();

		for (std::vector<ToggleTextButton*>::iterator
				i = _btnCountries.begin();
				i != _btnCountries.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Switches to the Finances screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnFinanceClick(Action*)
{
	if (_current != 5)
	{
		_current = 5;
		_game->getSavedGame()->setCurrentGraph(5);

		_finance = true;
		_alien =
		_income =
		_country = false;

		_btnReset->setVisible(false);
		resetScreen();
		drawLines();

		_txtTitle->setText(tr("STR_FINANCE"));
		_txtScore->setVisible();

		for (std::vector<ToggleTextButton*>::iterator
				i = _btnFinances.begin();
				i != _btnFinances.end();
				++i)
			(*i)->setVisible();
	}
}

/**
 * Handles a click on a region button.
 * @param action - pointer to an Action
 */
void GraphsState::btnRegionListClick(Action* action)
{
	const size_t row = (action->getSender()->getY() - _game->getScreen()->getDY()) / 10;

	ToggleTextButton* btn;
	size_t id;

	if ((_regionToggles.size() <= GRAPH_BUTTONS + 1
			&& row == _regionToggles.size() - 1)
		|| (_regionToggles.size() > GRAPH_BUTTONS + 1
			&& row == GRAPH_BUTTONS))
	{
		btn = _btnRegionTotal;
		id = _regionToggles.size() - 1;
	}
	else
	{
		btn = _btnRegions.at(row);
		id = row + _btnRegionsOffset;
	}

	_regionToggles.at(id)->_pushed = btn->getPressed();

	drawLines();
}

/**
 * Handles a click on a country button.
 * @param action - pointer to an Action
 */
void GraphsState::btnCountryListClick(Action* action)
{
	const size_t row = (action->getSender()->getY() - _game->getScreen()->getDY()) / 10;

	ToggleTextButton* btn;
	size_t id;

	if ((_countryToggles.size() <= GRAPH_BUTTONS + 1
			&& row == _countryToggles.size() - 1)
		|| (_countryToggles.size() > GRAPH_BUTTONS + 1
			&& row == GRAPH_BUTTONS))
	{
		btn = _btnCountryTotal;
		id = _countryToggles.size() - 1;
	}
	else
	{
		btn = _btnCountries.at(row);
		id = row + _btnCountriesOffset;
	}

	_countryToggles.at(id)->_pushed = btn->getPressed();

	drawLines();
}

/**
 * Handles a click on a finance button.
 * @param action - pointer to an Action
 */
void GraphsState::btnFinanceListClick(Action* action)
{
	const size_t row = (action->getSender()->getY() - _game->getScreen()->getDY()) / 16;
	const ToggleTextButton* const btn = _btnFinances.at(row);

	_financeLines.at(row)->setVisible(_financeToggles.at(row) == false);
	_financeToggles.at(row) = btn->getPressed();

	drawLines();
}

/**
 * Resets aLien/xCom activity and the blink indicators.
 */
void GraphsState::btnResetPress(Action*) // private.
{
	_reset = true;
	_btnReset->setVisible(false);

	for (std::vector<Region*>::iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
		(*i)->resetActivity();

	for (std::vector<Country*>::iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
		(*i)->resetActivity();
}

/**
 * Remove all elements from view.
 */
void GraphsState::resetScreen() // private.
{
	for (std::vector<Surface*>::iterator
			i = _alienRegionLines.begin();
			i != _alienRegionLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::iterator
			i = _alienCountryLines.begin();
			i != _alienCountryLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::iterator
			i = _xcomRegionLines.begin();
			i != _xcomRegionLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::iterator
			i = _xcomCountryLines.begin();
			i != _xcomCountryLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::iterator
			i = _incomeLines.begin();
			i != _incomeLines.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Surface*>::iterator
			i = _financeLines.begin();
			i != _financeLines.end();
			++i)
		(*i)->setVisible(false);


	for (std::vector<ToggleTextButton*>::iterator
			i = _btnRegions.begin();
			i != _btnRegions.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnCountries.begin();
			i != _btnCountries.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnFinances.begin();
			i != _btnFinances.end();
			++i)
		(*i)->setVisible(false);


	for (std::vector<Text*>::iterator
			i = _txtRegionActivityAlien.begin();
			i != _txtRegionActivityAlien.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Text*>::iterator
			i = _txtCountryActivityAlien.begin();
			i != _txtCountryActivityAlien.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Text*>::iterator
			i = _txtRegionActivityXCom.begin();
			i != _txtRegionActivityXCom.end();
			++i)
		(*i)->setVisible(false);

	for (std::vector<Text*>::iterator
			i = _txtCountryActivityXCom.begin();
			i != _txtCountryActivityXCom.end();
			++i)
		(*i)->setVisible(false);


	_btnRegionTotal->setVisible(false);
	_btnCountryTotal->setVisible(false);
	_txtFactor->setVisible(false);
	_txtScore->setVisible(false);
}

/**
 * Updates the text on the vertical scale.
 * @param valLow	- minimum value
 * @param valHigh	- maximum value
 */
void GraphsState::updateScale( // private.
		float valLow,
		float valHigh)
{
	float delta = std::max(
						10.f,
						(valHigh - valLow) / 9.f);
	for (size_t
			i = 0;
			i != 10;
			++i)
	{
		_txtScale.at(i)->setText(Text::formatNumber(static_cast<int>(valLow)));
		valLow += delta;
	}
}

/**
 * Instead of having all the line drawing in one giant ridiculous routine only
 * call the required routine.
 */
void GraphsState::drawLines() // private.
{
	if (_country == false && _finance == false)
		drawRegionLines();
	else if (_finance == false)
		drawCountryLines();
	else
		drawFinanceLines();
}

/**
 * Sets up the screens and draws the lines for region buttons to toggle on and off.
 */
void GraphsState::drawRegionLines() // private.
{
	int // calculate totals and set the upward maximum
		scaleHigh = 0,
		scaleLow = 0,
		total,
		act,

		totals[] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			i = 0;
			i != _game->getSavedGame()->getFundsList().size();
			++i)
	{
		total = 0;

		for (size_t
				j = 0;
				j != _game->getSavedGame()->getRegions()->size();
				++j)
		{
			if (_alien == true)
				act = _game->getSavedGame()->getRegions()->at(j)->getActivityAlien().at(i);
			else
				act = _game->getSavedGame()->getRegions()->at(j)->getActivityXCom().at(i);

			total += act;

			if (act > scaleHigh
				&& _regionToggles.at(j)->_pushed == true)
			{
				scaleHigh = act;
			}

			if (_alien == false // aLiens never get into negative scores.
				&& act < scaleLow
				&& _regionToggles.at(j)->_pushed == true)
			{
				scaleLow = act;
			}
		}

		if (_regionToggles.back()->_pushed == true
			&& total > scaleHigh)
		{
			scaleHigh = total;
		}
	}

	// adjust the scale to fit the upward maximum
	const float low = static_cast<float>(scaleLow);
	float delta = static_cast<float>(scaleHigh - scaleLow);

	int
		test = 10,
		grid = 9; // cells in grid

	while (delta > static_cast<float>(test * grid))
		test *= 2;

	scaleLow = 0;
	scaleHigh = test * grid;

	if (low < 0.f)
	{
		while (low < static_cast<float>(scaleLow))
		{
			scaleLow -= test;
			scaleHigh -= test;
		}
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	delta = static_cast<float>(scaleHigh - scaleLow);
	const float pixelUnits = delta / 126.f;

	Uint8 color = 0;
	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Region* region;

	// draw region lines
	for (size_t
			i = 0;
			i != _game->getSavedGame()->getRegions()->size();
			++i, ++color)
	{
		region = _game->getSavedGame()->getRegions()->at(i);

		_alienRegionLines.at(i)->clear();
		_xcomRegionLines.at(i)->clear();

		lineVector.clear();
		if (color == 17) color = 0;

		for (size_t
				j = 0;
				j != 12;
				++j)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

			if (_alien == true)
			{
				if (j < region->getActivityAlien().size())
				{
					reduction = region->getActivityAlien().at(region->getActivityAlien().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}
			else
			{
				if (j < region->getActivityXCom().size())
				{
					reduction = region->getActivityXCom().at(region->getActivityXCom().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				x = 312 - static_cast<Sint16>(j) * 17;

				if (_alien == true)
					_alienRegionLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
				else
					_xcomRegionLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
			}
		}

		if (_alien == true)
			_alienRegionLines.at(i)->setVisible(_regionToggles.at(i)->_pushed);
		else
			_xcomRegionLines.at(i)->setVisible(_regionToggles.at(i)->_pushed);
	}


	if (_alien == true) // set up the TOTAL line
		_alienRegionLines.back()->clear();
	else
		_xcomRegionLines.back()->clear();

	color = static_cast<Uint8>(_game->getRuleset()->getInterface("graphs")->getElement("regionTotal")->color2);
	lineVector.clear();

	for (size_t
			i = 0;
			i != 12;
			++i)
	{
		y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

		if (totals[i] > 0)
			y -= static_cast<Sint16>(Round(static_cast<float>(totals[i]) / pixelUnits));

		lineVector.push_back(y);

		if (lineVector.size() > 1)
		{
			x = 312 - static_cast<Sint16>(i) * 17;

			if (_alien == true)
				_alienRegionLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);
			else
				_xcomRegionLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);
		}
	}

	if (_alien == true)
		_alienRegionLines.back()->setVisible(_regionToggles.back()->_pushed);
	else
		_xcomRegionLines.back()->setVisible(_regionToggles.back()->_pushed);

	updateScale(
			static_cast<float>(scaleLow),
			static_cast<float>(scaleHigh));

	_txtFactor->setVisible(false);
}

/**
 * Sets up the screens and draws the lines for country buttons to toggle on and off.
 */
void GraphsState::drawCountryLines() // private.
{
	// calculate the totals, and set up the upward maximum
	int
		scaleHigh = 0,
		scaleLow = 0,
		total,
		act,

		totals[] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			i = 0;
			i != _game->getSavedGame()->getFundsList().size();
			++i)
	{
		total = 0;

		for (size_t
				j = 0;
				j != _game->getSavedGame()->getCountries()->size();
				++j)
		{
			if (_alien == true)
				act = _game->getSavedGame()->getCountries()->at(j)->getActivityAlien().at(i);
			else if (_income == true)
				act = _game->getSavedGame()->getCountries()->at(j)->getFunding().at(i) / 1000;
			else
				act = _game->getSavedGame()->getCountries()->at(j)->getActivityXCom().at(i);

			total += act;

			if (act > scaleHigh
				&& _countryToggles.at(j)->_pushed == true)
			{
				scaleHigh = act;
			}

			if (_alien == false && _income == false // aLien & Income never go into negative values.
				&& act < scaleLow
				&& _countryToggles.at(j)->_pushed == true)
			{
				scaleLow = act;
			}
		}

		if (_countryToggles.back()->_pushed == true
			&& total > scaleHigh)
		{
			scaleHigh = total;
		}
	}

	// adjust the scale to fit the upward maximum
	const float low = static_cast<float>(scaleLow);
	float delta = static_cast<float>(scaleHigh - scaleLow);

	int
		test = 10,
		grid = 9; // cells in grid

	while (delta > static_cast<float>(test * grid))
		test *= 2;

	scaleLow = 0;
	scaleHigh = test * grid;

	if (low < 0.f)
	{
		while (low < static_cast<float>(scaleLow))
		{
			scaleLow -= test;
			scaleHigh -= test;
		}
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	delta = static_cast<float>(scaleHigh - scaleLow);
	const float pixelUnits = delta / 126.f;

	Uint8 color = 0;
	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Country* country;

	// draw country lines
	for (size_t
			i = 0;
			i != _game->getSavedGame()->getCountries()->size();
			++i, ++color)
	{
		country = _game->getSavedGame()->getCountries()->at(i);

		_alienCountryLines.at(i)->clear();
		_xcomCountryLines.at(i)->clear();
		_incomeLines.at(i)->clear();

		lineVector.clear();
		if (color == 17) color = 0;

		for (size_t
				j = 0;
				j != 12;
				++j)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

			if (_alien == true)
			{
				if (j < country->getActivityAlien().size())
				{
					reduction = country->getActivityAlien().at(country->getActivityAlien().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}
			else if (_income == true)
			{
				if (j < country->getFunding().size())
				{
					reduction = country->getFunding().at(country->getFunding().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / 1000.f / pixelUnits));
					totals[j] += static_cast<int>(Round(static_cast<float>(reduction) / 1000.f));
				}
			}
			else
			{
				if (j < country->getActivityXCom().size())
				{
					reduction = country->getActivityXCom().at(country->getActivityXCom().size() - (j + 1));
					y -= static_cast<Sint16>(Round(static_cast<float>(reduction) / pixelUnits));
					totals[j] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				x = 312 - static_cast<Sint16>(j) * 17;

				if (_alien == true)
					_alienCountryLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
				else if (_income == true)
					_incomeLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
				else
					_xcomCountryLines.at(i)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
			}
		}

		if (_alien == true)
			_alienCountryLines.at(i)->setVisible(_countryToggles.at(i)->_pushed);
		else if (_income == true)
			_incomeLines.at(i)->setVisible(_countryToggles.at(i)->_pushed);
		else
			_xcomCountryLines.at(i)->setVisible(_countryToggles.at(i)->_pushed);
	}


	if (_alien == true) // set up the TOTAL line
		_alienCountryLines.back()->clear();
	else if (_income == true)
		_incomeLines.back()->clear();
	else
		_xcomCountryLines.back()->clear();

	color = static_cast<Uint8>(
		   _game->getRuleset()->getInterface("graphs")->getElement("countryTotal")->color2);
	lineVector.clear();

	for (size_t
			i = 0;
			i != 12;
			++i)
	{
		y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

		if (totals[i] > 0)
			y -= static_cast<Sint16>(Round(static_cast<float>(totals[i]) / pixelUnits));

		lineVector.push_back(y);

		if (lineVector.size() > 1)
		{
			x = 312 - static_cast<Sint16>(i) * 17;

			if (_alien == true)
				_alienCountryLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);
			else if (_income == true)
				_incomeLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);
			else
				_xcomCountryLines.back()->drawLine(
						x,y,
						x + 17,
						lineVector.at(lineVector.size() - 2),
						color);
		}
	}

	if (_alien == true)
		_alienCountryLines.back()->setVisible(_countryToggles.back()->_pushed);
	else if (_income == true)
		_incomeLines.back()->setVisible(_countryToggles.back()->_pushed);
	else
		_xcomCountryLines.back()->setVisible(_countryToggles.back()->_pushed);

	updateScale(
			static_cast<float>(scaleLow),
			static_cast<float>(scaleHigh));

	_txtFactor->setVisible(_income);
}

/**
 * Sets up the screens and draws the lines for the finance buttons to toggle on and off.
 */
void GraphsState::drawFinanceLines() // private. // Council Analytics
{
	int
		scaleHigh = 0,
		scaleLow = 0,

		income[]		= {0,0,0,0,0,0,0,0,0,0,0,0},
		expenditure[]	= {0,0,0,0,0,0,0,0,0,0,0,0},
		maintenance[]	= {0,0,0,0,0,0,0,0,0,0,0,0},
		balance[]		= {0,0,0,0,0,0,0,0,0,0,0,0},
		score[]			= {0,0,0,0,0,0,0,0,0,0,0,0},

		baseIncomes = 0,
		baseExpenses = 0;

	// start filling those arrays with score values;
	// determine which is the highest one being displayed, so we can adjust the scale
	size_t rit;
	for (size_t
			i = 0;
			i != _game->getSavedGame()->getFundsList().size(); // use Balance as template.
			++i)
	{
		rit = _game->getSavedGame()->getFundsList().size() - (i + 1);

		if (i == 0)
		{
			for (std::vector<Base*>::const_iterator
					j = _game->getSavedGame()->getBases()->begin();
					j != _game->getSavedGame()->getBases()->end();
					++j)
			{
				baseIncomes += (*j)->getCashIncome();
				baseExpenses += (*j)->getCashSpent();
			}

			income[i] = baseIncomes / 1000; // perhaps add Country funding
			expenditure[i] = baseExpenses / 1000;
			maintenance[i] = _game->getSavedGame()->getBaseMaintenances() / 1000; // use current
		}
		else
		{
			income[i] = static_cast<int>(_game->getSavedGame()->getIncomeList().at(rit)) / 1000; // perhaps add Country funding
			expenditure[i] = static_cast<int>(_game->getSavedGame()->getExpenditureList().at(rit)) / 1000;
			maintenance[i] = static_cast<int>(_game->getSavedGame()->getMaintenanceList().at(rit)) / 1000;
		}

		balance[i] = static_cast<int>(_game->getSavedGame()->getFundsList().at(rit)) / 1000; // note: these (int)casts render int64_t useless.
		score[i] = _game->getSavedGame()->getResearchScores().at(rit);


		for (std::vector<Region*>::const_iterator
				j = _game->getSavedGame()->getRegions()->begin();
				j != _game->getSavedGame()->getRegions()->end();
				++j)
		{
			score[i] += (*j)->getActivityXCom().at(rit) - (*j)->getActivityAlien().at(rit);
		}

		if (i == 0) // values are stored backwards. So take 1st value for last.
			_txtScore->setText(Text::formatNumber(score[i]));


		if (_financeToggles.at(0) == true)	// INCOME
		{
			if (income[i] > scaleHigh)
				scaleHigh = income[i];

			if (income[i] < scaleLow)
				scaleLow = income[i];
		}

		if (_financeToggles.at(1) == true)	// EXPENDITURE
		{
			if (expenditure[i] > scaleHigh)
				scaleHigh = expenditure[i];

			if (expenditure[i] < scaleLow)
				scaleLow = expenditure[i];
		}

		if (_financeToggles.at(2) == true)	// MAINTENANCE
		{
			if (maintenance[i] > scaleHigh)
				scaleHigh = maintenance[i];

			if (maintenance[i] < scaleLow)
				scaleLow = maintenance[i];
		}

		if (_financeToggles.at(3) == true)	// BALANCE
		{
			if (balance[i] > scaleHigh)
				scaleHigh = balance[i];

			if (balance[i] < scaleLow)
				scaleLow = balance[i];
		}

		if (_financeToggles.at(4) == true)	// SCORE
		{
			if (score[i] > scaleHigh)
				scaleHigh = score[i];

			if (score[i] < scaleLow)
				scaleLow = score[i];
		}
	}


	const float low = static_cast<float>(scaleLow);
	float delta = static_cast<float>(scaleHigh - scaleLow);

	int
		test = 100,
		grid = 9; // cells in grid

	while (delta > static_cast<float>(test * grid))
		test *= 2;

	scaleLow = 0;
	scaleHigh = test * grid;

	if (low < 0.f)
	{
		while (low < static_cast<float>(scaleLow))
		{
			scaleLow -= test;
			scaleHigh -= test;
		}
	}

	for (size_t // toggle screens
			i = 0;
			i != 5;
			++i)
	{
		_financeLines.at(i)->setVisible(_financeToggles.at(i));
		_financeLines.at(i)->clear();
	}

	// Figure out how many units to the pixel then plot the points for the graph
	// and connect the dots.
	delta = static_cast<float>(scaleHigh - scaleLow);
	const float pixelUnits = delta / 126.f;

	Uint8 color;
	Sint16
		reduction,
		x,y;

	std::vector<Sint16> lineVector;

	for (size_t
			i = 0;
			i != 5;
			++i)
	{
		lineVector.clear();

		for (size_t
				j = 0;
				j != 12;
				++j)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<float>(scaleLow) / pixelUnits));

			switch (i)
			{
				case 0:
					reduction = static_cast<Sint16>(Round(static_cast<float>(income[j]) / pixelUnits));
				break;
				case 1:
					reduction = static_cast<Sint16>(Round(static_cast<float>(expenditure[j]) / pixelUnits));
				break;
				case 2:
					reduction = static_cast<Sint16>(Round(static_cast<float>(maintenance[j]) / pixelUnits));
				break;
				case 3:
					reduction = static_cast<Sint16>(Round(static_cast<float>(balance[j]) / pixelUnits));
				break;
				case 4:
				default: // avoid vc++ linker warning.
					reduction = static_cast<Sint16>(Round(static_cast<float>(score[j]) / pixelUnits));
			}

			y -= reduction;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				if (i % 2 != 0)
					color = 8;
				else
					color = 0;

				Uint8 multi; // switch colors for Income (was yellow) and Maintenance (was green)
				if (i == 0)
					multi = 2;
				else if (i == 2)
					multi = 0;
				else
					multi = static_cast<Uint8>(i);

				color = Palette::blockOffset((multi / 2) + 1) + color;

				x = 312 - static_cast<Sint16>(j) * 17;
				_financeLines.at(i)->drawLine(
											x,y,
											x + 17,
											lineVector.at(lineVector.size() - 2),
											color);
			}
		}
	}

	updateScale(
			static_cast<float>(scaleLow),
			static_cast<float>(scaleHigh));

	_txtFactor->setVisible();
}

/**
 * Shift the buttons to display only GRAPH_BUTTONS - reset their state from toggles.
 * @param action - pointer to an Action
 */
void GraphsState::shiftButtons(Action* action) // private.
{
	if (_finance == false)
	{
		if (_country == true)
		{
			if (_countryToggles.size() > GRAPH_BUTTONS)
			{
				int dir;
				if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP
					|| action->getDetails()->key.keysym.sym == SDLK_UP
					|| action->getDetails()->key.keysym.sym == SDLK_KP8)
				{
					dir = -1;
				}
				else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN
					|| action->getDetails()->key.keysym.sym == SDLK_DOWN
					|| action->getDetails()->key.keysym.sym == SDLK_KP2)
				{
					dir = 1;
				}
				else
					dir = 0;

				if (dir != 0)
					scrollButtons(
							_countryToggles,
							_btnCountries,
							_txtCountryActivityAlien,
							_txtCountryActivityXCom,
							_blinkCountryAlien,
							_blinkCountryXCom,
							_btnCountriesOffset,
							dir);
			}
		}
/*		else // _region -> not needed unless quantity of Regions increases over GRAPH_BUTTONS. Ain't likely to happen.
		{
			if (_regionToggles.size() > GRAPH_BUTTONS)
			{
				int dir = 0;
				if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
					dir = -1;
				else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
					dir = 1;

				if (dir != 0)
					scrollButtons(
							_regionToggles,
							_btnRegions,
							_txtRegionActivityAlien,
							_txtRegionActivityXCom,
							_blinkRegionAlien,
							_blinkRegionXCom,
							_btnRegionsOffset,
							dir);
			}
		} */
	}
}

/**
 * Helper for shiftButtons().
 * @param toggles	-
 * @param buttons	-
 * @param actA_vect	-
 * @param actX_vect	-
 * @param blinkA	-
 * @param blinkX	-
 * @param btnOffset	-
 * @param dir		-
 * @param init		- (default false)
 */
void GraphsState::scrollButtons( // private.
		std::vector<GraphBtnInfo*>& toggles,
		std::vector<ToggleTextButton*>& buttons,
		std::vector<Text*>& actA_vect,
		std::vector<Text*>& actX_vect,
		std::vector<bool>& blinkA,
		std::vector<bool>& blinkX,
		size_t& btnOffset,
		int dir,
		bool init)
{
	if (dir + static_cast<int>(btnOffset) > -1
		&& dir
			+ static_cast<int>(btnOffset)
			+ static_cast<int>(GRAPH_BUTTONS) < static_cast<int>(toggles.size()))
	{
		_forceVis = true;
		blink(); // show all activity-values before & during scrolling ...

		// set the next btnOffset - cheaper to do it from starters
		// This changes either '_btnCountriesOffset' or '_btnRegionsOffset' throughout this class-object:
		if (dir == 1)
			++btnOffset;
		else if (dir == -1)
			--btnOffset;

		if (init == true)
			btnOffset = recallRow;
		else
			recallRow = btnOffset; // aka _btnCountriesOffset (note: would conflict w/ _btnRegionsOffset if/when regions are scrollable.)

		std::vector<ToggleTextButton*>::const_iterator btn = buttons.begin();
		std::vector<Text*>::const_iterator actA_iter = actA_vect.begin();
		std::vector<Text*>::const_iterator actX_iter = actX_vect.begin();
		std::vector<bool>::iterator blingA = blinkA.begin();
		std::vector<bool>::iterator blingX = blinkX.begin();
		size_t row = 0;

		for (std::vector<GraphBtnInfo*>::const_iterator
				i = toggles.begin();
				i != toggles.end();
				++i, ++row)
		{
			if (row < btnOffset)
				continue;
			else if (row < btnOffset + GRAPH_BUTTONS)
			{
				*blingA = (*i)->_blinkA;
				*blingX = (*i)->_blinkX;

				++blingA;
				++blingX;

				updateButton(
						*i,
						*btn++,
						*actA_iter++,
						*actX_iter++);
			}
			else
				return;
		}
	}
}

/**
 * Helper for scrollButtons().
 */
void GraphsState::updateButton( // private.
		GraphBtnInfo* info,
		ToggleTextButton* btn,
		Text* aliens,
		Text* xcom)
{
	btn->setText(info->_name);
	btn->setInvertColor(info->_color);
	btn->setPressed(info->_pushed);

	aliens->setText(Text::formatNumber(info->_actA));
	aliens->setColor(info->_colorTxt);

	xcom->setText(Text::formatNumber(info->_actX));
	xcom->setColor(info->_colorTxt);
}

}
