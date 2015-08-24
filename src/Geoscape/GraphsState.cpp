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

static int curRowCountry = 0;


/**
 * Helper struct for scrolling past GRAPH_MAX_BUTTONS.
 */
struct GraphBtnInfo
{
	LocalizedText _name;
	Uint8
		_color,
		_colorTxt;
	int
		_alienAct,
		_xcomAct;
	bool
		_blink,
		_blinkXCOM,
		_pushed;

	/// Builds this struct.
	GraphBtnInfo(
			const LocalizedText& name,
			Uint8 color,
			int alienAct,
			int xcomAct,
			Uint8 colorTxt,
			bool blink,
			bool blinkXCOM)
		:
			_name(name),
			_color(color),
			_alienAct(alienAct),
			_xcomAct(xcomAct),
			_colorTxt(colorTxt),
			_blink(blink),
			_blinkXCOM(blinkXCOM),
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
		_vis(true),
		_reset(false)
{
	_bg = new InteractiveSurface(320, 200);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDL_BUTTON_WHEELUP);
	_bg->onMousePress(
				(ActionHandler)& GraphsState::shiftButtons,
				SDL_BUTTON_WHEELDOWN);

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

	_txtMonths		= new TextList(205, 9, 115, 183);
	_txtYears		= new TextList(200, 9, 121, 191);

	_txtScore		= new Text(36, 9, 46, 82);


	_blinkTimer = new Timer(250);
	_blinkTimer->onTimer((StateHandler)& GraphsState::blink);
	_blinkTimer->start();

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
	add(_txtMonths,	"scale",	"graphs");
	add(_txtYears,	"scale",	"graphs");
	add(_txtTitle,	"text",		"graphs");
	add(_txtFactor,	"text",		"graphs");
	add(_txtScore);

	for (size_t
			scaleText = 0;
			scaleText != 10;
			++scaleText)
	{
		_txtScale.push_back(new Text(
									32,10,
									91,
									171 - (static_cast<int>(scaleText) * 14)));
		add(_txtScale.at(scaleText), "scale", "graphs");
	}

	size_t offset = 0;
	Uint8 colorOffset = 0;
	int
		alienAct,
		xcomAct;
	bool
		blink,
		blinkXCOM;

	/* REGIONS */
	for (std::vector<Region*>::const_iterator
			region = _game->getSavedGame()->getRegions()->begin();
			region != _game->getSavedGame()->getRegions()->end();
			++region)
	{
		if (colorOffset == 17) colorOffset = 0;

		alienAct = (*region)->getActivityAlien().back();
		xcomAct = (*region)->getActivityXcom().back();
		blink = (*region)->recentActivity(false, true);
		blinkXCOM = (*region)->recentActivityXCOM(false, true);

		// always save all the regions in toggles
		_regionToggles.push_back(new GraphBtnInfo(
												tr((*region)->getRules()->getType()), // name of Region
												colorOffset * 8 + 13,
												alienAct,
												xcomAct,
												colorOffset * 8 + 16,
												blink,
												blinkXCOM));

		// first add the GRAPH_MAX_BUTTONS ( having the respective country's information )
		if (offset < GRAPH_MAX_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnRegions.push_back(new ToggleTextButton(
													65,10,
													0,
													static_cast<int>(offset) * 10));
			_btnRegions.at(offset)->setText(tr((*region)->getRules()->getType())); // name of Region
			_btnRegions.at(offset)->setInvertColor(colorOffset * 8 + 13);
			_btnRegions.at(offset)->onMousePress(
							(ActionHandler)& GraphsState::btnRegionListClick,
							SDL_BUTTON_LEFT);
			_btnRegions.at(offset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELUP);
			_btnRegions.at(offset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELDOWN);

			add(_btnRegions.at(offset), "button", "graphs");

			_txtRegionActivityAlien.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(offset) * 10) + 1));
			_txtRegionActivityAlien.at(offset)->setColor(colorOffset * 8 + 16);
			_txtRegionActivityAlien.at(offset)->setText(Text::formatNumber(alienAct));

			add(_txtRegionActivityAlien.at(offset));

			_blinkRegion.push_back(blink);
			_blinkRegionXCOM.push_back(blinkXCOM);


			_txtRegionActivityXCom.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(offset) * 10) + 1));
			_txtRegionActivityXCom.at(offset)->setColor(colorOffset * 8 + 16);
			_txtRegionActivityXCom.at(offset)->setText(Text::formatNumber(xcomAct));

			add(_txtRegionActivityXCom.at(offset));
		}


		_alienRegionLines.push_back(new Surface(320, 200));
		add(_alienRegionLines.at(offset));

		_xcomRegionLines.push_back(new Surface(320, 200));
		add(_xcomRegionLines.at(offset));


		++offset;
		++colorOffset;
	}


	int btnTotal_y;
	if (_regionToggles.size() < GRAPH_MAX_BUTTONS)
		btnTotal_y = static_cast<int>(_regionToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_MAX_BUTTONS) * 10;

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

	_alienRegionLines.push_back(new Surface(320, 200));
	add(_alienRegionLines.at(offset));

	_xcomRegionLines.push_back(new Surface(320, 200));
	add(_xcomRegionLines.at(offset));


	offset = 0;
	colorOffset = 0;

	/* COUNTRIES */
	for (std::vector<Country*>::const_iterator
			country = _game->getSavedGame()->getCountries()->begin();
			country != _game->getSavedGame()->getCountries()->end();
			++country)
	{
		if (colorOffset == 17) colorOffset = 0;

		alienAct = (*country)->getActivityAlien().back();
		xcomAct = (*country)->getActivityXcom().back();
		blink = (*country)->recentActivity(false, true);
		blinkXCOM = (*country)->recentActivityXCOM(false, true);

		// always save all the countries in toggles
		_countryToggles.push_back(new GraphBtnInfo(
												tr((*country)->getRules()->getType()), // name of Country
												colorOffset * 8 + 13,
												alienAct,
												xcomAct,
												colorOffset * 8 + 16,
												blink,
												blinkXCOM));

		// first add the GRAPH_MAX_BUTTONS ( having the respective country's information )
		if (offset < GRAPH_MAX_BUTTONS) // leave a slot for the TOTAL btn.
		{
			_btnCountries.push_back(new ToggleTextButton(
													65,10,
													0,
													static_cast<int>(offset) * 10));
			_btnCountries.at(offset)->setText(tr((*country)->getRules()->getType())); // name of Country
			_btnCountries.at(offset)->setInvertColor(colorOffset * 8 + 13);
			_btnCountries.at(offset)->onMousePress(
							(ActionHandler)& GraphsState::btnCountryListClick,
							SDL_BUTTON_LEFT);
			_btnCountries.at(offset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELUP);
			_btnCountries.at(offset)->onMousePress(
							(ActionHandler)& GraphsState::shiftButtons,
							SDL_BUTTON_WHEELDOWN);

			add(_btnCountries.at(offset), "button", "graphs");

			_txtCountryActivityAlien.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(offset) * 10) + 1));
			_txtCountryActivityAlien.at(offset)->setColor(colorOffset * 8 + 16);
			_txtCountryActivityAlien.at(offset)->setText(Text::formatNumber(alienAct));

			add(_txtCountryActivityAlien.at(offset));

			_blinkCountry.push_back(blink);
			_blinkCountryXCOM.push_back(blinkXCOM);


			_txtCountryActivityXCom.push_back(new Text(
													24,10,
													66,
													(static_cast<int>(offset) * 10) + 1));
			_txtCountryActivityXCom.at(offset)->setColor(colorOffset * 8 + 16);
			_txtCountryActivityXCom.at(offset)->setText(Text::formatNumber(xcomAct));

			add(_txtCountryActivityXCom.at(offset));
		}


		_alienCountryLines.push_back(new Surface(320, 200));
		add(_alienCountryLines.at(offset));

		_xcomCountryLines.push_back(new Surface(320, 200));
		add(_xcomCountryLines.at(offset));

		_incomeLines.push_back(new Surface(320, 200));
		add(_incomeLines.at(offset));


		++offset;
		++colorOffset;
	}


	if (_countryToggles.size() < GRAPH_MAX_BUTTONS)
		btnTotal_y = static_cast<int>(_countryToggles.size());
	else
		btnTotal_y = static_cast<int>(GRAPH_MAX_BUTTONS);

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

	_alienCountryLines.push_back(new Surface(320, 200));
	add(_alienCountryLines.at(offset));

	_xcomCountryLines.push_back(new Surface(320, 200));
	add(_xcomCountryLines.at(offset));

	_incomeLines.push_back(new Surface(320, 200));
	add(_incomeLines.at(offset));


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

		_financeLines.push_back(new Surface(320, 200));
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
		else if (i < GRAPH_MAX_BUTTONS)
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
		else if (i < GRAPH_MAX_BUTTONS)
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
			grid = 0;
			grid != 5;
			++grid)
	{
		for (Sint16
				y = 50 + grid;
				y <= 163 + grid;
				y += 14)
		{
			for (Sint16
					x = 126 + grid;
					x <= 297 + grid;
					x += 17)
			{
				color = gridColor + 1 + static_cast<Uint8>(grid);
				if (grid == 4) color = 0;

				_bg->drawRect(
							x,
							y,
							16 - (grid * 2),
							13 - (grid * 2),
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
	_txtMonths->setColumns(12,17,17,17,17,17,17,17,17,17,17,17,17);
	_txtMonths->addRow(12,L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ",L" ");

	_txtYears->setColumns(6,34,34,34,34,34,34);
	_txtYears->addRow(6,L" ",L" ",L" ",L" ",L" ",L" ");


	const GameTime* const gt = _game->getSavedGame()->getTime();
	const int yr = gt->getYear();
	size_t mth = static_cast<size_t>(gt->getMonth());

	std::wostringstream woststr;
	for (size_t
			i = 0;
			i != 12;
			++i)
	{
		if (mth > 11)
		{
			mth = 0;
			woststr.str(L"");
			woststr << yr;
			_txtYears->setCellText(
								0,
								i / 2,
								woststr.str());

			if (i > 2)
			{
				woststr.str(L"");
				woststr << (yr - 1);
				_txtYears->setCellText(
									0,0,
									woststr.str());
			}
		}

		_txtMonths->setCellText(
							0,
							i,
							tr(months[mth++]));
	}

	for (std::vector<Text*>::const_iterator // set up the vertical measurement units
			i = _txtScale.begin();
			i != _txtScale.end();
			++i)
	{
		(*i)->setAlign(ALIGN_RIGHT);
	}


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
					Options::keyGeoGraphs);

	centerAllSurfaces();
}

/**
 * dTor.
 */
GraphsState::~GraphsState()
{
	delete _blinkTimer;

	std::string
		graphRegionToggles,
		graphCountryToggles,
		graphFinanceToggles;

	for (size_t
			i = 0;
			i != _regionToggles.size();
			++i)
	{
		graphRegionToggles.push_back(_regionToggles[i]->_pushed? '1': '0');
		delete _regionToggles[i];
	}

	for (size_t
			i = 0;
			i != _countryToggles.size();
			++i)
	{
		graphCountryToggles.push_back(_countryToggles[i]->_pushed? '1': '0');
		delete _countryToggles[i];
	}

	for (size_t
			i = 0;
			i != _financeToggles.size();
			++i)
	{
		graphFinanceToggles.push_back(_financeToggles[i]? '1': '0');
	}

	_game->getSavedGame()->setGraphRegionToggles(graphRegionToggles);
	_game->getSavedGame()->setGraphCountryToggles(graphCountryToggles);
	_game->getSavedGame()->setGraphFinanceToggles(graphFinanceToggles);
}

/**
 * Blinks current activity.
 */
void GraphsState::think()
{
	_blinkTimer->think(this, NULL);
}

/**
 * Makes recent activity Text blink.
 */
void GraphsState::blink()
{
	if (_reset == true)
		_vis = true;
	else
		_vis = !_vis;

	size_t offset = 0;

	if (_alien == true
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkRegion.begin();
				i != _blinkRegion.end();
				++i)
		{
			if (*i == true)
				_txtRegionActivityAlien.at(offset)->setVisible(_vis);
			else
				_txtRegionActivityAlien.at(offset)->setVisible();

			++offset;
		}
	}
	else if (_alien == true
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkCountry.begin();
				i != _blinkCountry.end();
				++i)
		{
			if (*i == true)
				_txtCountryActivityAlien.at(offset)->setVisible(_vis);
			else
				_txtCountryActivityAlien.at(offset)->setVisible();

			++offset;
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkRegionXCOM.begin();
				i != _blinkRegionXCOM.end();
				++i)
		{
			if (*i == true)
				_txtRegionActivityXCom.at(offset)->setVisible(_vis);
			else
				_txtRegionActivityXCom.at(offset)->setVisible();

			++offset;
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<bool>::const_iterator
				i = _blinkCountryXCOM.begin();
				i != _blinkCountryXCOM.end();
				++i)
		{
			if (*i == true)
				_txtCountryActivityXCom.at(offset)->setVisible(_vis);
			else
				_txtCountryActivityXCom.at(offset)->setVisible();

			++offset;
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnGeoscapeClick(Action*)
{
	_game->popState();
	kL_soundPop->play(Mix_GroupAvailable(0));
}

/**
 * Switches to the UFO Regional Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnUfoRegionClick(Action*)
{
	if (_current == 0)
		return;

	_current = 0;
	_game->getSavedGame()->setCurrentGraph(0);

	_alien = true;
	_income = false;
	_country = false;
	_finance = false;

	_btnReset->setVisible();
	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnRegions.begin();
			i != _btnRegions.end();
			++i)
	{
		(*i)->setVisible();
	}

	_btnRegionTotal->setVisible();
	_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_AREAS"));

	for (std::vector<Text*>::iterator
			i = _txtRegionActivityAlien.begin();
			i != _txtRegionActivityAlien.end();
			++i)
	{
		(*i)->setVisible();
	}
}

/**
 * Switches to the xCom Regional Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnXcomRegionClick(Action*)
{
	if (_current == 1)
		return;

	_current = 1;
	_game->getSavedGame()->setCurrentGraph(1);

	_alien = false;
	_income = false;
	_country = false;
	_finance = false;

	_btnReset->setVisible();
	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnRegions.begin();
			i != _btnRegions.end();
			++i)
	{
		(*i)->setVisible();
	}

	_btnRegionTotal->setVisible();
	_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_AREAS"));

	for (std::vector<Text*>::iterator
			i = _txtRegionActivityXCom.begin();
			i != _txtRegionActivityXCom.end();
			++i)
	{
		(*i)->setVisible();
	}
}

/**
 * Switches to the UFO Country Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnUfoCountryClick(Action*)
{
	if (_current == 2)
		return;

	_current = 2;
	_game->getSavedGame()->setCurrentGraph(2);

	_alien = true;
	_income = false;
	_country = true;
	_finance = false;

	_btnReset->setVisible();
	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnCountries.begin();
			i != _btnCountries.end();
			++i)
	{
		(*i)->setVisible();
	}

	_btnCountryTotal->setVisible();
	_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_COUNTRIES"));

	for (std::vector<Text*>::iterator
			i = _txtCountryActivityAlien.begin();
			i != _txtCountryActivityAlien.end();
			++i)
	{
		(*i)->setVisible();
	}
}

/**
 * Switches to the xCom Country Activity screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnXcomCountryClick(Action*)
{
	if (_current == 3)
		return;

	_current = 3;
	_game->getSavedGame()->setCurrentGraph(3);

	_alien = false;
	_income = false;
	_country = true;
	_finance = false;

	_btnReset->setVisible();
	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnCountries.begin();
			i != _btnCountries.end();
			++i)
	{
		(*i)->setVisible();
	}

	_btnCountryTotal->setVisible();
	_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_COUNTRIES"));

	for (std::vector<Text*>::iterator
			i = _txtCountryActivityXCom.begin();
			i != _txtCountryActivityXCom.end();
			++i)
	{
		(*i)->setVisible();
	}
}

/**
 * Switches to the Income screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnIncomeClick(Action*)
{
	if (_current == 4)
		return;

	_current = 4;
	_game->getSavedGame()->setCurrentGraph(4);

	_alien = false;
	_income = true;
	_country = true;
	_finance = false;

	_btnReset->setVisible(false);
	resetScreen();
	drawLines();

	_txtFactor->setVisible();

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnCountries.begin();
			i != _btnCountries.end();
			++i)
	{
		(*i)->setVisible();
	}

	_btnCountryTotal->setVisible();

	_txtTitle->setText(tr("STR_INCOME"));
}

/**
 * Switches to the Finances screen.
 * @param action - pointer to an Action
 */
void GraphsState::btnFinanceClick(Action*)
{
	if (_current == 5)
		return;

	_current = 5;
	_game->getSavedGame()->setCurrentGraph(5);

	_alien = false;
	_income = false;
	_country = false;
	_finance = true;

	_btnReset->setVisible(false);
	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnFinances.begin();
			i != _btnFinances.end();
			++i)
	{
		(*i)->setVisible();
	}

	_txtScore->setVisible();

	_txtTitle->setText(tr("STR_FINANCE"));

}

/**
 * Handles a click on a region button.
 * @param action - pointer to an Action
 */
void GraphsState::btnRegionListClick(Action* action)
{
	const size_t row = (action->getSender()->getY() - _game->getScreen()->getDY()) / 10;

	ToggleTextButton* btn;
	size_t ent;

	if ((_regionToggles.size() <= GRAPH_MAX_BUTTONS + 1
			&& row == _regionToggles.size() - 1)
		|| (_regionToggles.size() > GRAPH_MAX_BUTTONS + 1
			&& row == GRAPH_MAX_BUTTONS))
	{
		btn = _btnRegionTotal;
		ent = _regionToggles.size() - 1;
	}
	else
	{
		btn = _btnRegions.at(row);
		ent = row + _btnRegionsOffset;
	}

	_regionToggles.at(ent)->_pushed = btn->getPressed();

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
	size_t ent;

	if ((_countryToggles.size() <= GRAPH_MAX_BUTTONS + 1
			&& row == _countryToggles.size() - 1)
		|| (_countryToggles.size() > GRAPH_MAX_BUTTONS + 1
			&& row == GRAPH_MAX_BUTTONS))
	{
		btn = _btnCountryTotal;
		ent = _countryToggles.size() - 1;
	}
	else
	{
		btn = _btnCountries.at(row);
		ent = row + _btnCountriesOffset;
	}

	_countryToggles.at(ent)->_pushed = btn->getPressed();

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
void GraphsState::btnResetPress(Action*)
{
	_reset = true;

	for (std::vector<Region*>::iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		(*i)->resetActivity();
	}

	for (std::vector<Country*>::iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		(*i)->resetActivity();
	}
}

/**
 * Remove all elements from view.
 */
void GraphsState::resetScreen()
{
	for (std::vector<Surface*>::iterator
			i = _alienRegionLines.begin();
			i != _alienRegionLines.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator
			i = _alienCountryLines.begin();
			i != _alienCountryLines.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator
			i = _xcomRegionLines.begin();
			i != _xcomRegionLines.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator
			i = _xcomCountryLines.begin();
			i != _xcomCountryLines.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator
			i = _incomeLines.begin();
			i != _incomeLines.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator
			i = _financeLines.begin();
			i != _financeLines.end();
			++i)
	{
		(*i)->setVisible(false);
	}


	for (std::vector<ToggleTextButton*>::iterator
			i = _btnRegions.begin();
			i != _btnRegions.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnCountries.begin();
			i != _btnCountries.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<ToggleTextButton*>::iterator
			i = _btnFinances.begin();
			i != _btnFinances.end();
			++i)
	{
		(*i)->setVisible(false);
	}


	for (std::vector<Text*>::iterator
			i = _txtRegionActivityAlien.begin();
			i != _txtRegionActivityAlien.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Text*>::iterator
			i = _txtCountryActivityAlien.begin();
			i != _txtCountryActivityAlien.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Text*>::iterator
			i = _txtRegionActivityXCom.begin();
			i != _txtRegionActivityXCom.end();
			++i)
	{
		(*i)->setVisible(false);
	}

	for (std::vector<Text*>::iterator
			i = _txtCountryActivityXCom.begin();
			i != _txtCountryActivityXCom.end();
			++i)
	{
		(*i)->setVisible(false);
	}


	_btnRegionTotal->setVisible(false);
	_btnCountryTotal->setVisible(false);
	_txtFactor->setVisible(false);
	_txtScore->setVisible(false);
}

/**
 * Update the text on the vertical scale.
 * @param lowerLimit	- minimum value
 * @param upperLimit	- maximum value
 * @param grid			- (default 9)
 */
void GraphsState::updateScale(
		double lowerLimit,
		double upperLimit,
		int grid)
{
	double increment = ((upperLimit - lowerLimit) / static_cast<double>(grid));
	if (increment < 10.) increment = 10.;

	double text = lowerLimit;
	for (size_t
			i = 0;
			i != static_cast<size_t>(grid) + 1;
			++i)
	{
		_txtScale.at(i)->setText(Text::formatNumber(static_cast<int>(text)));
		text += increment;
	}
}

/**
 * instead of having all our line drawing in one giant ridiculous routine, just use the one we need.
 */
void GraphsState::drawLines()
{
	if (_country == false
		&& _finance == false)
	{
		drawRegionLines();
	}
	else if (_finance == false)
		drawCountryLines();
	else
		drawFinanceLines();
}

/**
 * Sets up the screens and draws the lines for country buttons to toggle on and off.
 */
void GraphsState::drawCountryLines()
{
	// calculate the totals, and set up the upward maximum
	int
		upperLimit = 0,
		lowerLimit = 0,
		total,
		act,

		totals[] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			itMonth = 0;
			itMonth < _game->getSavedGame()->getFundsList().size();
			++itMonth)
	{
		total = 0;

		for (size_t
				itCountry = 0;
				itCountry < _game->getSavedGame()->getCountries()->size();
				++itCountry)
		{
			if (_alien == true)
				act = _game->getSavedGame()->getCountries()->at(itCountry)->getActivityAlien().at(itMonth);
			else if (_income == true)
				act = _game->getSavedGame()->getCountries()->at(itCountry)->getFunding().at(itMonth) / 1000;
			else
				act = _game->getSavedGame()->getCountries()->at(itCountry)->getActivityXcom().at(itMonth);

			total += act;

			if (act > upperLimit
				&& _countryToggles.at(itCountry)->_pushed == true)
			{
				upperLimit = act;
			}

			if (_alien == false && _income == false // aLien & Income never go into negative values.
				&& act < lowerLimit
				&& _countryToggles.at(itCountry)->_pushed == true)
			{
				lowerLimit = act;
			}
		}

		if (_countryToggles.back()->_pushed == true
			&& total > upperLimit)
		{
			upperLimit = total;
		}
	}

	// adjust the scale to fit the upward maximum
	const double low = static_cast<double>(lowerLimit);
	double range = static_cast<double>(upperLimit - lowerLimit);

//	int test = _income? 50: 100;
	int
		test = 10,
		grid = 9; // cells in grid
//	if (low<0) grids--;

	while (range > static_cast<double>(test * grid))
		test *= 2;

	lowerLimit = 0;
	upperLimit = test * grid;

	if (low < 0.)
	{
		while (low < static_cast<double>(lowerLimit))
		{
			lowerLimit -= test;
			upperLimit -= test;
		}
	}

	range = static_cast<double>(upperLimit - lowerLimit);
	const double units = range / 126.;

	Uint8 color = 0;
	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Country* country;

	// draw country lines
	for (size_t
			itCountry = 0;
			itCountry < _game->getSavedGame()->getCountries()->size();
			++itCountry)
	{
		country = _game->getSavedGame()->getCountries()->at(itCountry);

		_alienCountryLines.at(itCountry)->clear();
		_xcomCountryLines.at(itCountry)->clear();
		_incomeLines.at(itCountry)->clear();

		lineVector.clear();
		if (color == 17) color = 0;

		for (size_t
				i = 0;
				i != 12;
				++i)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<double>(lowerLimit) / units));

			if (_alien == true)
			{
				if (i < country->getActivityAlien().size())
				{
					reduction = country->getActivityAlien().at(country->getActivityAlien().size() - (i + 1));
					y -= static_cast<Sint16>(Round(static_cast<double>(reduction) / units));
					totals[i] += reduction;
				}
			}
			else if (_income == true)
			{
				if (i < country->getFunding().size())
				{
					reduction = country->getFunding().at(country->getFunding().size() - (i + 1));
					y -= static_cast<Sint16>(Round(static_cast<double>(reduction) / 1000. / units));
					totals[i] += static_cast<int>(Round(static_cast<double>(reduction) / 1000.));
				}
			}
			else
			{
				if (i < country->getActivityXcom().size())
				{
					reduction = country->getActivityXcom().at(country->getActivityXcom().size() - (i + 1));
					y -= static_cast<Sint16>(Round(static_cast<double>(reduction) / units));
					totals[i] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				x = 312 - static_cast<Sint16>(i) * 17;

				if (_alien == true)
					_alienCountryLines.at(itCountry)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
				else if (_income == true)
					_incomeLines.at(itCountry)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
				else
					_xcomCountryLines.at(itCountry)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
			}
		}

		if (_alien == true)
			_alienCountryLines.at(itCountry)->setVisible(_countryToggles.at(itCountry)->_pushed);
		else if (_income == true)
			_incomeLines.at(itCountry)->setVisible(_countryToggles.at(itCountry)->_pushed);
		else
			_xcomCountryLines.at(itCountry)->setVisible(_countryToggles.at(itCountry)->_pushed);

		++color;
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
		y = 175 + static_cast<Sint16>(Round(static_cast<double>(lowerLimit) / units));

		if (totals[i] > 0)
			y -= static_cast<Sint16>(Round(static_cast<double>(totals[i]) / units));

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
			static_cast<double>(lowerLimit),
			static_cast<double>(upperLimit));

	_txtFactor->setVisible(_income);
}

/**
 * Sets up the screens and draws the lines for region buttons
 * to toggle on and off
 */
void GraphsState::drawRegionLines()
{
	int // calculate totals and set the upward maximum
		upperLimit = 0,
		lowerLimit = 0,
		total,
		act,

		totals[] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (size_t
			itMonth = 0;
			itMonth < _game->getSavedGame()->getFundsList().size();
			++itMonth)
	{
		total = 0;

		for (size_t
				itRegion = 0;
				itRegion < _game->getSavedGame()->getRegions()->size();
				++itRegion)
		{
			if (_alien == true)
				act = _game->getSavedGame()->getRegions()->at(itRegion)->getActivityAlien().at(itMonth);
			else
				act = _game->getSavedGame()->getRegions()->at(itRegion)->getActivityXcom().at(itMonth);

			total += act;

			if (act > upperLimit
				&& _regionToggles.at(itRegion)->_pushed == true)
			{
				upperLimit = act;
			}

			if (_alien == false // aLiens never get into negative scores.
				&& act < lowerLimit
				&& _regionToggles.at(itRegion)->_pushed == true)
			{
				lowerLimit = act;
			}
		}

		if (_regionToggles.back()->_pushed == true
			&& total > upperLimit)
		{
			upperLimit = total;
		}
	}

	// adjust the scale to fit the upward maximum
	const double low = static_cast<double>(lowerLimit);
	double range = static_cast<double>(upperLimit - lowerLimit);

	int
		test = 10,
		grid = 9; // cells in grid
//	if (low<0) grids--;

	while (range > static_cast<double>(test * grid))
		test *= 2;

	lowerLimit = 0;
	upperLimit = test * grid;

	if (low < 0.)
	{
		while (low < static_cast<double>(lowerLimit))
		{
			lowerLimit -= test;
			upperLimit -= test;
		}
	}

	range = static_cast<double>(upperLimit - lowerLimit);
	const double units = range / 126.;

	Uint8 color = 0;
	int reduction;
	Sint16
		x,y;
	std::vector<Sint16> lineVector;
	Region* region;

	// draw region lines
	for (size_t
			itRegion = 0;
			itRegion < _game->getSavedGame()->getRegions()->size();
			++itRegion)
	{
		region = _game->getSavedGame()->getRegions()->at(itRegion);

		_alienRegionLines.at(itRegion)->clear();
		_xcomRegionLines.at(itRegion)->clear();

		lineVector.clear();
		if (color == 17) color = 0;

		for (size_t
				i = 0;
				i != 12;
				++i)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<double>(lowerLimit) / units));

			if (_alien == true)
			{
				if (i < region->getActivityAlien().size())
				{
					reduction = region->getActivityAlien().at(region->getActivityAlien().size() - (i + 1));
					y -= static_cast<Sint16>(Round(static_cast<double>(reduction) / units));
					totals[i] += reduction;
				}
			}
			else
			{
				if (i < region->getActivityXcom().size())
				{
					reduction = region->getActivityXcom().at(region->getActivityXcom().size() - (i + 1));
					y -= static_cast<Sint16>(Round(static_cast<double>(reduction) / units));
					totals[i] += reduction;
				}
			}

			if (y > 175) y = 175;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				x = 312 - static_cast<Sint16>(i) * 17;

				if (_alien == true)
					_alienRegionLines.at(itRegion)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
				else
					_xcomRegionLines.at(itRegion)->drawLine(
							x,y,
							x + 17,
							lineVector.at(lineVector.size() - 2),
							color * 8 + 16);
			}
		}

		if (_alien == true)
			_alienRegionLines.at(itRegion)->setVisible(_regionToggles.at(itRegion)->_pushed);
		else
			_xcomRegionLines.at(itRegion)->setVisible(_regionToggles.at(itRegion)->_pushed);

		++color;
	}


	if (_alien == true) // set up the TOTAL line
		_alienRegionLines.back()->clear();
	else
		_xcomRegionLines.back()->clear();

	color = static_cast<Uint8>(
		   _game->getRuleset()->getInterface("graphs")->getElement("regionTotal")->color2);
	lineVector.clear();

	for (size_t
			i = 0;
			i != 12;
			++i)
	{
		y = 175 + static_cast<Sint16>(Round(static_cast<double>(lowerLimit) / units));

		if (totals[i] > 0)
			y -= static_cast<Sint16>(Round(static_cast<double>(totals[i]) / units));

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
			static_cast<double>(lowerLimit),
			static_cast<double>(upperLimit));

	_txtFactor->setVisible(false);
}

/**
 * Sets up the screens and draws the lines for the finance buttons to toggle on and off
 */
void GraphsState::drawFinanceLines() // Council Analytics
{
	int
		upperLimit = 0,
		lowerLimit = 0,

		income[]		= {0,0,0,0,0,0,0,0,0,0,0,0},
		expenditure[]	= {0,0,0,0,0,0,0,0,0,0,0,0},
		maintenance[]	= {0,0,0,0,0,0,0,0,0,0,0,0},
		balance[]		= {0,0,0,0,0,0,0,0,0,0,0,0},
		score[]			= {0,0,0,0,0,0,0,0,0,0,0,0},

		baseIncomes = 0,
		baseExpenditures = 0;

	// start filling those arrays with score values;
	// determine which is the highest one being displayed, so we can adjust the scale
	size_t rit;
	for (size_t
			itMonth = 0;
			itMonth != _game->getSavedGame()->getFundsList().size(); // use Balance as template.
			++itMonth)
	{
		rit = _game->getSavedGame()->getFundsList().size() - (itMonth + 1);

		if (itMonth == 0)
		{
			for (std::vector<Base*>::const_iterator
					i = _game->getSavedGame()->getBases()->begin();
					i != _game->getSavedGame()->getBases()->end();
					++i)
			{
				baseIncomes += (*i)->getCashIncome();
				baseExpenditures += (*i)->getCashSpent();
			}

			income[itMonth] = baseIncomes / 1000; // perhaps add Country funding
			expenditure[itMonth] = baseExpenditures / 1000;
			maintenance[itMonth] = _game->getSavedGame()->getBaseMaintenances() / 1000; // use current
		}
		else
		{
			income[itMonth] = static_cast<int>(_game->getSavedGame()->getIncomeList().at(rit)) / 1000; // perhaps add Country funding
			expenditure[itMonth] = static_cast<int>(_game->getSavedGame()->getExpenditureList().at(rit)) / 1000;
			maintenance[itMonth] = static_cast<int>(_game->getSavedGame()->getMaintenanceList().at(rit)) / 1000;
		}

		balance[itMonth] = static_cast<int>(_game->getSavedGame()->getFundsList().at(rit)) / 1000; // note: these (int)casts render int64_t useless.
		score[itMonth] = _game->getSavedGame()->getResearchScores().at(rit);


		for (std::vector<Region*>::const_iterator
				itRegion = _game->getSavedGame()->getRegions()->begin();
				itRegion != _game->getSavedGame()->getRegions()->end();
				++itRegion)
		{
			score[itMonth] += (*itRegion)->getActivityXcom().at(rit) - (*itRegion)->getActivityAlien().at(rit);
		}

		if (itMonth == 0) // values are stored backwards. So take 1st value for last.
			_txtScore->setText(Text::formatNumber(score[itMonth]));


		if (_financeToggles.at(0) == true)			// INCOME
		{
			if (income[itMonth] > upperLimit)
				upperLimit = income[itMonth];

			if (income[itMonth] < lowerLimit)
				lowerLimit = income[itMonth];
		}

		if (_financeToggles.at(1) == true)			// EXPENDITURE
		{
			if (expenditure[itMonth] > upperLimit)
				upperLimit = expenditure[itMonth];

			if (expenditure[itMonth] < lowerLimit)
				lowerLimit = expenditure[itMonth];
		}

		if (_financeToggles.at(2) == true)			// MAINTENANCE
		{
			if (maintenance[itMonth] > upperLimit)
				upperLimit = maintenance[itMonth];

			if (maintenance[itMonth] < lowerLimit)
				lowerLimit = maintenance[itMonth];
		}

		if (_financeToggles.at(3) == true)			// BALANCE
		{
			if (balance[itMonth] > upperLimit)
				upperLimit = balance[itMonth];

			if (balance[itMonth] < lowerLimit)
				lowerLimit = balance[itMonth];
		}

		if (_financeToggles.at(4) == true)			// SCORE
		{
			if (score[itMonth] > upperLimit)
				upperLimit = score[itMonth];

			if (score[itMonth] < lowerLimit)
				lowerLimit = score[itMonth];
		}
	}


	const double low = static_cast<double>(lowerLimit);
	double range = static_cast<double>(upperLimit - lowerLimit);

	int
		test = 100,
		grid = 9;
//	if (low<0) grids--;

	while (range > static_cast<double>(test * grid))
		test *= 2;

	lowerLimit = 0;
	upperLimit = test * grid;

	if (low < 0.)
	{
		while (low < static_cast<double>(lowerLimit))
		{
			lowerLimit -= test;
			upperLimit -= test;
		}
	}

	for (size_t // toggle screens
			btn = 0;
			btn != 5;
			++btn)
	{
		_financeLines.at(btn)->setVisible(_financeToggles.at(btn));
		_financeLines.at(btn)->clear();
	}

	// Figure out how many units to the pixel
	// then plot the points for the graph and connect those dots.
	range = static_cast<double>(upperLimit - lowerLimit);
	const double units = range / 126.;

	Uint8 color;
	Sint16
		reduction,
		x,y;

	std::vector<Sint16> lineVector;

	for (size_t
			btn = 0;
			btn != 5;
			++btn)
	{
		lineVector.clear();

		for (size_t
				i = 0;
				i != 12;
				++i)
		{
			y = 175 + static_cast<Sint16>(Round(static_cast<double>(lowerLimit) / units));

			switch (btn)
			{
				case 0:
					reduction = static_cast<Sint16>(Round(static_cast<double>(income[i]) / units));
				break;
				case 1:
					reduction = static_cast<Sint16>(Round(static_cast<double>(expenditure[i]) / units));
				break;
				case 2:
					reduction = static_cast<Sint16>(Round(static_cast<double>(maintenance[i]) / units));
				break;
				case 3:
					reduction = static_cast<Sint16>(Round(static_cast<double>(balance[i]) / units));
				break;
				case 4:
				default: // avoid vc++ compiler warning.
					reduction = static_cast<Sint16>(Round(static_cast<double>(score[i]) / units));
			}

			y -= reduction;

			lineVector.push_back(y);

			if (lineVector.size() > 1)
			{
				if (btn % 2 != 0)
					color = 8;
				else
					color = 0;

				Uint8 multi; // switch colors for Income (was yellow) and Maintenance (was green)
				if (btn == 0)
					multi = 2;
				else if (btn == 2)
					multi = 0;
				else
					multi = static_cast<Uint8>(btn);

				color = Palette::blockOffset((multi / 2) + 1) + color;

				x = 312 - static_cast<Sint16>(i) * 17;
				_financeLines.at(btn)->drawLine(
											x,y,
											x + 17,
											lineVector.at(lineVector.size() - 2),
											color);
			}
		}
	}

	updateScale(
			static_cast<double>(lowerLimit),
			static_cast<double>(upperLimit));

	_txtFactor->setVisible();
}

/**
 * Shifts buttons to their pre-Graph cTor row.
 * Countries only, since total Regions still fit on the list.
 */
void GraphsState::initButtons()
{
	if (_countryToggles.size() <= GRAPH_MAX_BUTTONS) // if too few countries - return
		return;

	scrollButtons(
			_countryToggles,
			_btnCountries,
			_txtCountryActivityAlien,
			_txtCountryActivityXCom,
			_blinkCountry,
			_blinkCountryXCOM,
			_btnCountriesOffset,
			curRowCountry);
}

/**
 * 'Shift' the buttons to display only GRAPH_MAX_BUTTONS - reset their state from toggles.
 */
void GraphsState::shiftButtons(Action* action)
{
	if (_finance == true) // only if active 'screen' is other than finance
		return;

	if (_country == true) // select the data to process - regions or countries
	{
		if (_countryToggles.size() <= GRAPH_MAX_BUTTONS) // if too few countries - return
			return;
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			scrollButtons(
					_countryToggles,
					_btnCountries,
					_txtCountryActivityAlien,
					_txtCountryActivityXCom,
					_blinkCountry,
					_blinkCountryXCOM,
					_btnCountriesOffset,
					-1);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			scrollButtons(
					_countryToggles,
					_btnCountries,
					_txtCountryActivityAlien,
					_txtCountryActivityXCom,
					_blinkCountry,
					_blinkCountryXCOM,
					_btnCountriesOffset,
					1);
	}
	else // _region
	{
		if (_regionToggles.size() <= GRAPH_MAX_BUTTONS) // if too few regions - return
			return;
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			scrollButtons(
					_regionToggles,
					_btnRegions,
					_txtRegionActivityAlien,
					_txtRegionActivityXCom,
					_blinkRegion,
					_blinkRegionXCOM,
					_btnRegionsOffset,
					-1);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			scrollButtons(
					_regionToggles,
					_btnRegions,
					_txtRegionActivityAlien,
					_txtRegionActivityXCom,
					_blinkRegion,
					_blinkRegionXCOM,
					_btnRegionsOffset,
					1);
	}
}

/**
 * private. Helper for shiftButtons()
 */
void GraphsState::scrollButtons(
		std::vector<GraphBtnInfo*>& toggles,
		std::vector<ToggleTextButton*>& buttons,
		std::vector<Text*>& actAlien,
		std::vector<Text*>& actXCom,
		std::vector<bool>& blink,
		std::vector<bool>& blinkXCOM,
		size_t& offset,
		int step)
{
	// -1, 'cause the TOTAL button has already been added to the toggles-vector.
	// kL_note: but since Argentina was hiding behind TOTAL btn anyway, I took it out. Tks.
	if (step + static_cast<int>(offset) < 0
		|| step + static_cast<int>(offset) + static_cast<int>(GRAPH_MAX_BUTTONS) >= static_cast<int>(toggles.size()))
	{
		return;
	}

	// set the next offset - cheaper to do it from starters
	// kL_note: This changes either '_btnCountriesOffset'
	// or '_btnRegionsOffset' throughout this here class-object:
	offset += static_cast<size_t>(step);
	curRowCountry = offset; // aka _btnCountriesOffset ( note: would conflict w/ _btnRegionsOffset )

	size_t row = 0;

	std::vector<ToggleTextButton*>::const_iterator	btn = buttons.begin();
	std::vector<Text*>::const_iterator				aliens = actAlien.begin();
	std::vector<Text*>::const_iterator				xcom = actXCom.begin();
	std::vector<bool>::iterator						bling = blink.begin();
	std::vector<bool>::iterator						blingXCOM = blinkXCOM.begin();

	for (std::vector<GraphBtnInfo*>::const_iterator
			info = toggles.begin();
			info != toggles.end();
			++info,
				++row)
	{
		if (row < offset)
			continue;
		else if (row < offset + GRAPH_MAX_BUTTONS)
		{
			*bling = (*info)->_blink;
			*blingXCOM = (*info)->_blinkXCOM;

			++bling;
			++blingXCOM;

			updateButton(
						*info,
						*btn++,
						*aliens++,
						*xcom++);
		}
		else
			return;
	}
}

/**
 * private. Helper for scrollButtons()
 */
void GraphsState::updateButton(
		GraphBtnInfo* info,
		ToggleTextButton* btn,
		Text* aliens,
		Text* xcom)
{
	btn->setText(info->_name);
	btn->setInvertColor(info->_color);
	btn->setPressed(info->_pushed);

	aliens->setText(Text::formatNumber(info->_alienAct));
	aliens->setColor(info->_colorTxt);

	xcom->setText(Text::formatNumber(info->_xcomAct));
	xcom->setColor(info->_colorTxt);
}

}
