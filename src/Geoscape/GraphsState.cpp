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

#include "GraphsState.h"

#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"

#include "../Interface/NumberText.h" // kL
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/Country.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

struct GraphBtnInfo
{
	LocalizedText _name;
	int _color;
	bool _pushed;

	GraphBtnInfo(const LocalizedText& name, int color)
		:
			_name(name),
			_color(color),
			_pushed(false)
	{
	}
};
/**
 * Initializes all the elements in the Graphs screen.
 * @param game Pointer to the core game.
 */
GraphsState::GraphsState(Game* game)
	:
		State(game),
		_btnRegionsOffset(0),
		_btnCountriesOffset(0)
{
	_bg				= new InteractiveSurface(320, 200, 0, 0);
	_bg->onMousePress((ActionHandler)& GraphsState::shiftButtons, SDL_BUTTON_WHEELUP);
	_bg->onMousePress((ActionHandler)& GraphsState::shiftButtons, SDL_BUTTON_WHEELDOWN);

	_btnUfoRegion	= new InteractiveSurface(32, 24, 96, 0);
	_btnXcomRegion	= new InteractiveSurface(32, 24, 128, 0);
	_btnUfoCountry	= new InteractiveSurface(32, 24, 160, 0);
	_btnXcomCountry	= new InteractiveSurface(32, 24, 192, 0);
	_btnIncome		= new InteractiveSurface(32, 24, 224, 0);
	_btnFinance		= new InteractiveSurface(32, 24, 256, 0);
	_btnGeoscape	= new InteractiveSurface(32, 28, 288, 0); // on-off trick using y to drop to Geo-btn.

	_txtTitle		= new Text(220, 17, 100, 28);
	_txtFactor		= new Text(35, 9, 96, 28);

	_txtMonths		= new TextList(205, 8, 115, 183);
	_txtYears		= new TextList(200, 8, 121, 191);


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_2")->getColors());
	
	add(_bg);
	add(_btnUfoRegion);
	add(_btnUfoCountry);
	add(_btnXcomRegion);
	add(_btnXcomCountry);
	add(_btnIncome);
	add(_btnFinance);
	add(_btnGeoscape);
	add(_txtMonths);
	add(_txtYears);
	add(_txtTitle);
	add(_txtFactor);


	for (int
			scaleText = 0;
			scaleText < 10;
			++scaleText)
	{
		_txtScale.push_back(new Text(36, 16, 86, 171 - (scaleText * 14)));

		add(_txtScale.at(scaleText));
	}

	unsigned int offset = 0;
	for (std::vector<Region*>::iterator
			iter = _game->getSavedGame()->getRegions()->begin();
			iter != _game->getSavedGame()->getRegions()->end();
			++iter)
	{
		// always save all the regions in toggles
		_regionToggles.push_back(new GraphBtnInfo(tr((*iter)->getRules()->getType()), (offset * 4) - 42));

		// initially add the GRAPH_MAX_BUTTONS having the first region's information
		if (offset < GRAPH_MAX_BUTTONS)
		{
			_btnRegions.push_back(new ToggleTextButton(65, 10, 0, offset * 10, true));
			_btnRegions.at(offset)->setColor(Palette::blockOffset(9)+7);
			_btnRegions.at(offset)->setInvertColor((offset * 4) - 42);
			_btnRegions.at(offset)->setText(tr((*iter)->getRules()->getType())); // unique name of Region
			_btnRegions.at(offset)->onMousePress((ActionHandler)& GraphsState::btnRegionListClick);
			_btnRegions.at(offset)->onMousePress((ActionHandler)& GraphsState::shiftButtons, SDL_BUTTON_WHEELUP);
			_btnRegions.at(offset)->onMousePress((ActionHandler)& GraphsState::shiftButtons, SDL_BUTTON_WHEELDOWN);

			add(_btnRegions.at(offset));

/*			std::wstring rPts;								// kL
			rPts = tr((*iter)->getRules()->getType());		// kL
			rPts += L" ";									// kL
			rPts += std::to_wstring(static_cast<long long>((*iter)->getActivityAlien().back()));	// kL, ho9ly fuck.
			_btnRegions.at(offset)->setText(rPts);			// kL
*/
			_numRegionActivityAlien.push_back(new NumberText(18, 10, 67, offset * 10 + 3));
//			_numRegionActivityAlien.at(offset)->setColor(Palette::blockOffset(9)+7); // grey
			_numRegionActivityAlien.at(offset)->setColor((offset * 8)+16);
			_numRegionActivityXCom.push_back(new NumberText(18, 10, 67, offset * 10 + 3));
//			_numRegionActivityXCom.at(offset)->setColor(Palette::blockOffset(9)+7);
			_numRegionActivityXCom.at(offset)->setColor((offset * 8)+16);
//			_numRegionActivityAlien.at(offset)->setColor((offset * 4) - 42);

			add(_numRegionActivityAlien.at(offset));
			add(_numRegionActivityXCom.at(offset));
		}

		_alienRegionLines.push_back(new Surface(320, 200, 0, 0));
		add(_alienRegionLines.at(offset));

		_xcomRegionLines.push_back(new Surface(320, 200, 0, 0));
		add(_xcomRegionLines.at(offset));

		++offset;
	}

	if (_regionToggles.size() < GRAPH_MAX_BUTTONS)
		_btnRegionTotal = new ToggleTextButton(65, 10, 0, _regionToggles.size() * 10, true);
	else
		_btnRegionTotal = new ToggleTextButton(65, 10, 0, GRAPH_MAX_BUTTONS * 10, true);

	_regionToggles.push_back(new GraphBtnInfo(tr("STR_TOTAL_UC"), 22));

	_btnRegionTotal->setColor(Palette::blockOffset(9)+7);
	_btnRegionTotal->setInvertColor(22);
	_btnRegionTotal->setText(tr("STR_TOTAL_UC"));
	_btnRegionTotal->onMousePress((ActionHandler)& GraphsState::btnRegionListClick);


	_alienRegionLines.push_back(new Surface(320, 200, 0, 0));
	add(_alienRegionLines.at(offset));

	_xcomRegionLines.push_back(new Surface(320, 200, 0, 0));
	add(_xcomRegionLines.at(offset));

	add(_btnRegionTotal);

	
	offset = 0;
	for (std::vector<Country*>::iterator
			iter = _game->getSavedGame()->getCountries()->begin();
			iter != _game->getSavedGame()->getCountries()->end();
			++iter)
	{
		// always save all the countries in toggles
		_countryToggles.push_back(new GraphBtnInfo(tr((*iter)->getRules()->getType()), (offset * 4) - 42));

		// initially add the GRAPH_MAX_BUTTONS having the first country's information
		if (offset < GRAPH_MAX_BUTTONS)
		{
			_btnCountries.push_back(new ToggleTextButton(65, 10, 0, offset * 10, true));
			_btnCountries.at(offset)->setColor(Palette::blockOffset(9)+7);
			_btnCountries.at(offset)->setInvertColor((offset * 4) - 42);
			_btnCountries.at(offset)->setText(tr((*iter)->getRules()->getType()));
			_btnCountries.at(offset)->onMousePress((ActionHandler)& GraphsState::btnCountryListClick);
			_btnCountries.at(offset)->onMousePress((ActionHandler)& GraphsState::shiftButtons, SDL_BUTTON_WHEELUP);
			_btnCountries.at(offset)->onMousePress((ActionHandler)& GraphsState::shiftButtons, SDL_BUTTON_WHEELDOWN);

			add(_btnCountries.at(offset));

			_numCountryActivityAlien.push_back(new NumberText(18, 10, 67, offset * 10 + 3));
//			_numCountryActivityAlien.at(offset)->setColor(Palette::blockOffset(9)+7);
			_numCountryActivityAlien.at(offset)->setColor((offset * 8)+16);
			_numCountryActivityXCom.push_back(new NumberText(18, 10, 67, offset * 10 + 3));
//			_numCountryActivityXCom.at(offset)->setColor(Palette::blockOffset(9)+7);
			_numCountryActivityXCom.at(offset)->setColor((offset * 8)+16);

			add(_numCountryActivityAlien.at(offset));
			add(_numCountryActivityXCom.at(offset));
		}

		_alienCountryLines.push_back(new Surface(320, 200, 0, 0));
		add(_alienCountryLines.at(offset));

		_xcomCountryLines.push_back(new Surface(320, 200, 0, 0));
		add(_xcomCountryLines.at(offset));

		_incomeLines.push_back(new Surface(320, 200, 0, 0));
		add(_incomeLines.at(offset));

		++offset;
	}
	
	if (_countryToggles.size() < GRAPH_MAX_BUTTONS)
		_btnCountryTotal = new ToggleTextButton(65, 10, 0, _countryToggles.size() * 10, true);
	else
		_btnCountryTotal = new ToggleTextButton(65, 10, 0, GRAPH_MAX_BUTTONS * 10, true);

	_countryToggles.push_back(new GraphBtnInfo(tr("STR_TOTAL_UC"), 22));

	_btnCountryTotal->setColor(Palette::blockOffset(9)+7);
    _btnCountryTotal->setInvertColor(22);
	_btnCountryTotal->setText(tr("STR_TOTAL_UC"));
	_btnCountryTotal->onMousePress((ActionHandler)& GraphsState::btnCountryListClick);


	_alienCountryLines.push_back(new Surface(320, 200, 0, 0));
	add(_alienCountryLines.at(offset));

	_xcomCountryLines.push_back(new Surface(320, 200, 0, 0));
	add(_xcomCountryLines.at(offset));

	_incomeLines.push_back(new Surface(320, 200, 0, 0));
	add(_incomeLines.at(offset));

	add(_btnCountryTotal);
	

	for (int
			iter = 0;
			iter < 5;
			++iter)
	{
		offset = iter;

//kL		_btnFinances.push_back(new ToggleTextButton(85, 10, 0, offset * 10));
		_btnFinances.push_back(new ToggleTextButton(82, 16, 0, offset * 16));		// kL
		_financeToggles.push_back(false);

		_btnFinances.at(offset)->setColor(Palette::blockOffset(9)+7);
        _btnFinances.at(offset)->setInvertColor((offset * 4) - 42);
		_btnFinances.at(offset)->onMousePress((ActionHandler)& GraphsState::btnFinanceListClick);

		add(_btnFinances.at(offset));

		_financeLines.push_back(new Surface(320, 200, 0, 0));
		add(_financeLines.at(offset));
	}

	_btnFinances.at(0)->setText(tr("STR_INCOME"));
	_btnFinances.at(1)->setText(tr("STR_EXPENDITURE"));
	_btnFinances.at(2)->setText(tr("STR_MAINTENANCE"));
	_btnFinances.at(3)->setText(tr("STR_BALANCE"));
	_btnFinances.at(4)->setText(tr("STR_SCORE"));

	// load back the button states
	std::string graphRegionToggles = _game->getSavedGame()->getGraphRegionToggles();
	std::string graphCountryToggles = _game->getSavedGame()->getGraphCountryToggles();
	std::string graphFinanceToggles = _game->getSavedGame()->getGraphFinanceToggles();

	while (graphRegionToggles.size() < _regionToggles.size())
		graphRegionToggles.push_back('0');
	while (graphCountryToggles.size() < _countryToggles.size())
		graphCountryToggles.push_back('0');
	while (graphFinanceToggles.size() < _financeToggles.size())
		graphFinanceToggles.push_back('0');

	for (size_t
			i = 0;
			i < _regionToggles.size();
			++i)
	{
		_regionToggles[i]->_pushed = ('0' == graphRegionToggles[i])? false: true;
		if (_regionToggles.size() - 1 == i)
			_btnRegionTotal->setPressed(_regionToggles[i]->_pushed);
		else if (i < GRAPH_MAX_BUTTONS) 
			_btnRegions.at(i)->setPressed(_regionToggles[i]->_pushed);
	}

	for (size_t
			i = 0;
			i < _countryToggles.size();
			++i)
	{
		_countryToggles[i]->_pushed = ('0' == graphCountryToggles[i])? false: true;
		if (_countryToggles.size() - 1 == i)
			_btnCountryTotal->setPressed(_countryToggles[i]->_pushed);
		else if (i < GRAPH_MAX_BUTTONS)
			_btnCountries.at(i)->setPressed(_countryToggles[i]->_pushed);
	}

	for (size_t
			i = 0;
			i < _financeToggles.size();
			++i)
	{
		_financeToggles[i] = ('0' == graphFinanceToggles[i])? false: true;
		_btnFinances.at(i)->setPressed(_financeToggles[i]);
	}

	// set up the grid
	SDL_Rect current;
	current.w = 188;
	current.h = 127;
	current.x = 125;
	current.y = 49;
	_bg->drawRect(&current, Palette::blockOffset(10));

	for (int
			grid = 0;
			grid < 5;
			++grid)
	{
		current.w = 16 - (grid * 2);
		current.h = 13 - (grid * 2);

		for (int
				y = 50 + grid;
				y <= 163 + grid;
				y += 14)
		{
			current.y = y;

			for (int
					x = 126 + grid;
					x <= 297 + grid;
					x += 17)
			{
				current.x = x;

				Uint8 color = Palette::blockOffset(10)+grid+1;
				if (grid == 4)
				{
					color = 0;
				} 

				_bg->drawRect(&current, color);
			}
		}
	}

	// set up the horizontal measurement units
	std::string months[] =
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
	_txtMonths->setColumns(12, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17);
	_txtMonths->addRow(12, L" ", L" ", L" ", L" ", L" ", L" ", L" ", L" ", L" ", L" ", L" ", L" ");
	_txtMonths->setRowColor(0, Palette::blockOffset(6)+7);

	_txtYears->setColumns(6, 34, 34, 34, 34, 34, 34);
	_txtYears->addRow(6, L" ", L" ", L" ", L" ", L" ", L" ");
	_txtYears->setRowColor(0, Palette::blockOffset(6)+7);


	int month = _game->getSavedGame()->getTime()->getMonth();

	for (int
			iter = 0;
			iter < 12;
			++iter)
	{
		if (month > 11)
		{
			month = 0;
			std::wstringstream ss;
			ss << _game->getSavedGame()->getTime()->getYear();
			_txtYears->setCellText(0, iter / 2, ss.str());

			if (iter > 2)
			{
				std::wstringstream ss2;
				ss2 << (_game->getSavedGame()->getTime()->getYear() - 1);
				_txtYears->setCellText(0, 0, ss2.str());
			}
		}

		_txtMonths->setCellText(0, iter, tr(months[month]));

		++month;
	}

	// set up the vertical measurement units
	for (std::vector<Text*>::iterator
			iter = _txtScale.begin();
			iter != _txtScale.end();
			++iter)
	{
		(*iter)->setAlign(ALIGN_RIGHT);
		(*iter)->setColor(Palette::blockOffset(6)+7);
	}

	btnUfoRegionClick(0);


	_game->getResourcePack()->getSurface("GRAPHS.SPK")->blit(_bg);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setColor(Palette::blockOffset(8)+7);
	
	_txtFactor->setText(L"$1000");
	_txtFactor->setColor(Palette::blockOffset(8)+7);


	_btnUfoRegion->onMousePress((ActionHandler)& GraphsState::btnUfoRegionClick);
	_btnUfoCountry->onMousePress((ActionHandler)& GraphsState::btnUfoCountryClick);
	_btnXcomRegion->onMousePress((ActionHandler)& GraphsState::btnXcomRegionClick);
	_btnXcomCountry->onMousePress((ActionHandler)& GraphsState::btnXcomCountryClick);
	_btnIncome->onMousePress((ActionHandler)& GraphsState::btnIncomeClick);
	_btnFinance->onMousePress((ActionHandler)& GraphsState::btnFinanceClick);
	_btnGeoscape->onMousePress((ActionHandler)& GraphsState::btnGeoscapeClick);
	_btnGeoscape->onKeyboardPress((ActionHandler)& GraphsState::btnGeoscapeClick, (SDLKey)Options::getInt("keyCancel"));
	_btnGeoscape->onKeyboardPress((ActionHandler)& GraphsState::btnGeoscapeClick, (SDLKey)Options::getInt("keyGeoGraphs"));

	centerAllSurfaces();
}

/**
 *
 */
GraphsState::~GraphsState()
{
	std::string graphRegionToggles = "";
	std::string graphCountryToggles = "";
	std::string graphFinanceToggles = "";

	for (size_t i = 0; i < _regionToggles.size(); ++i)
		graphRegionToggles.push_back(_regionToggles[i]->_pushed? '1': '0');
	for (size_t i = 0; i < _countryToggles.size(); ++i)
		graphCountryToggles.push_back(_countryToggles[i]->_pushed? '1': '0');
	for (size_t i = 0; i < _financeToggles.size(); ++i)
		graphFinanceToggles.push_back(_financeToggles[i]? '1': '0');

	_game->getSavedGame()->setGraphRegionToggles(graphRegionToggles);
	_game->getSavedGame()->setGraphCountryToggles(graphCountryToggles);
	_game->getSavedGame()->setGraphFinanceToggles(graphFinanceToggles);
}

/**
 * Show the latest month's value as NumberText beside the buttons.
 */
void GraphsState::latestTally()
{
	unsigned int offset = 0;

	if (_alien == true
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<Region*>::iterator
				iter = _game->getSavedGame()->getRegions()->begin();
				iter != _game->getSavedGame()->getRegions()->end();
				++iter)
		{
			if (offset < GRAPH_MAX_BUTTONS)
			{
				_numRegionActivityAlien.at(offset)->setValue((*iter)->getActivityAlien().back());

				offset++;
			}
		}
	}
	else if (_alien == true
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<Country*>::iterator
				iter = _game->getSavedGame()->getCountries()->begin();
				iter != _game->getSavedGame()->getCountries()->end();
				++iter)
		{
			if (offset < GRAPH_MAX_BUTTONS)
			{
				_numCountryActivityAlien.at(offset)->setValue((*iter)->getActivityAlien().back());

				offset++;
			}
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == false
		&& _finance == false)
	{
		for (std::vector<Region*>::iterator
				iter = _game->getSavedGame()->getRegions()->begin();
				iter != _game->getSavedGame()->getRegions()->end();
				++iter)
		{
			if (offset < GRAPH_MAX_BUTTONS)
			{
				_numRegionActivityXCom.at(offset)->setValue((*iter)->getActivityXcom().back());

				offset++;
			}
		}
	}
	else if (_alien == false
		&& _income == false
		&& _country == true
		&& _finance == false)
	{
		for (std::vector<Country*>::iterator
				iter = _game->getSavedGame()->getCountries()->begin();
				iter != _game->getSavedGame()->getCountries()->end();
				++iter)
		{
			if (offset < GRAPH_MAX_BUTTONS)
			{
				_numCountryActivityXCom.at(offset)->setValue((*iter)->getActivityXcom().back());

				offset++;
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void GraphsState::btnGeoscapeClick(Action*)
{
	_game->popState();
}

/**
 * Switches to the ufo region activity screen
 */
void GraphsState::btnUfoRegionClick(Action*)
{
	_alien = true;
	_income = false;
	_country = false;
	_finance = false;

	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			iter = _btnRegions.begin();
			iter != _btnRegions.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	_btnRegionTotal->setVisible(true);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_AREAS"));

	for (std::vector<NumberText*>::iterator
			iter = _numRegionActivityAlien.begin();
			iter != _numRegionActivityAlien.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	latestTally();
}

/**
 * Switches to the ufo country activity screen
 */
void GraphsState::btnUfoCountryClick(Action*)
{
	_alien = true;
	_income = false;
	_country = true;
	_finance = false;

	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			iter = _btnCountries.begin();
			iter != _btnCountries.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	_btnCountryTotal->setVisible(true);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_UFO_ACTIVITY_IN_COUNTRIES"));

	for (std::vector<NumberText*>::iterator
			iter = _numCountryActivityAlien.begin();
			iter != _numCountryActivityAlien.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	latestTally();
}

/**
 * Switches to the xcom region activity screen
 */
void GraphsState::btnXcomRegionClick(Action*)
{
	_alien = false;
	_income = false;
	_country = false;
	_finance = false;

	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			iter = _btnRegions.begin();
			iter != _btnRegions.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	_btnRegionTotal->setVisible(true);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_AREAS"));

	for (std::vector<NumberText*>::iterator
			iter = _numRegionActivityXCom.begin();
			iter != _numRegionActivityXCom.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	latestTally();
}

/**
 * Switches to the xcom country activity screen
 */
void GraphsState::btnXcomCountryClick(Action*)
{
	_alien = false;
	_income = false;
	_country = true;
	_finance = false;

	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			iter = _btnCountries.begin();
			iter != _btnCountries.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	_btnCountryTotal->setVisible(true);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_XCOM_ACTIVITY_IN_COUNTRIES"));

	for (std::vector<NumberText*>::iterator
			iter = _numCountryActivityXCom.begin();
			iter != _numCountryActivityXCom.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	latestTally();
}

/**
 * Switches to the income screen
 */
void GraphsState::btnIncomeClick(Action*)
{
	_alien = false;
	_income = true;
	_country = true;
	_finance = false;

	resetScreen();
	drawLines();

	_txtFactor->setVisible(true);

	for (std::vector<ToggleTextButton*>::iterator
			iter = _btnCountries.begin();
			iter != _btnCountries.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	_btnCountryTotal->setVisible(true);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_INCOME"));
}

/**
 * Switches to the finances screen
 */
void GraphsState::btnFinanceClick(Action*)
{
	_alien = false;
	_income = false;
	_country = false;
	_finance = true;

	resetScreen();
	drawLines();

	for (std::vector<ToggleTextButton*>::iterator
			iter = _btnFinances.begin();
			iter != _btnFinances.end();
			++iter)
	{
		(*iter)->setVisible(true);
	}

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_FINANCE"));

}

/**
 * handles a click on a region button
 */
void GraphsState::btnRegionListClick(Action* action)
{
	size_t number = (action->getSender()->getY() - Screen::getDY()) / 10;
	ToggleTextButton* button = 0;

	if ((_regionToggles.size() <= GRAPH_MAX_BUTTONS + 1
			&& number == _regionToggles.size() - 1)
		|| (_regionToggles.size() > GRAPH_MAX_BUTTONS + 1
			&& number == GRAPH_MAX_BUTTONS))
	{
		button = _btnRegionTotal;
	}
	else
	{
		button = _btnRegions.at(number);
	}

	_regionToggles.at(number + _btnRegionsOffset)->_pushed = button->getPressed();
	
	drawLines();
}

/**
 * handles a click on a country button
 */
void GraphsState::btnCountryListClick(Action* action)
{
	size_t number = (action->getSender()->getY() - Screen::getDY()) / 10;
	ToggleTextButton* button = 0;

	if ((_countryToggles.size() <= GRAPH_MAX_BUTTONS + 1
			&& number == _countryToggles.size() - 1)
		|| (_countryToggles.size() > GRAPH_MAX_BUTTONS + 1
			&& number == GRAPH_MAX_BUTTONS))
	{
		button = _btnCountryTotal;
	}
	else
	{
		button = _btnCountries.at(number);
	}

	_countryToggles.at(number + _btnCountriesOffset)->_pushed = button->getPressed();

	drawLines();
}

/**
 * handles a click on a finance button
 */
void GraphsState::btnFinanceListClick(Action* action)
{
	size_t number = (action->getSender()->getY() - Screen::getDY()) / 16;
	ToggleTextButton* button = _btnFinances.at(number);
	
	_financeLines.at(number)->setVisible(!_financeToggles.at(number));
	_financeToggles.at(number) = button->getPressed();

	drawLines();
}

/**
 * remove all elements from view
 */
void GraphsState::resetScreen()
{
	for (std::vector<Surface*>::iterator iter = _alienRegionLines.begin(); iter != _alienRegionLines.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator iter = _alienCountryLines.begin(); iter != _alienCountryLines.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator iter = _xcomRegionLines.begin(); iter != _xcomRegionLines.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator iter = _xcomCountryLines.begin(); iter != _xcomCountryLines.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator iter = _incomeLines.begin(); iter != _incomeLines.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<Surface*>::iterator iter = _financeLines.begin(); iter != _financeLines.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}


	for (std::vector<ToggleTextButton*>::iterator iter = _btnRegions.begin(); iter != _btnRegions.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<ToggleTextButton*>::iterator iter = _btnCountries.begin(); iter != _btnCountries.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<ToggleTextButton*>::iterator iter = _btnFinances.begin(); iter != _btnFinances.end(); ++iter)
	{
		(*iter)->setVisible(false);
	}


	for (std::vector<NumberText*>::iterator
			iter = _numRegionActivityAlien.begin();
			iter != _numRegionActivityAlien.end();
			++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<NumberText*>::iterator
			iter = _numCountryActivityAlien.begin();
			iter != _numCountryActivityAlien.end();
			++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<NumberText*>::iterator
			iter = _numRegionActivityXCom.begin();
			iter != _numRegionActivityXCom.end();
			++iter)
	{
		(*iter)->setVisible(false);
	}

	for (std::vector<NumberText*>::iterator
			iter = _numCountryActivityXCom.begin();
			iter != _numCountryActivityXCom.end();
			++iter)
	{
		(*iter)->setVisible(false);
	}


	_btnRegionTotal->setVisible(false);
	_btnCountryTotal->setVisible(false);
	_txtFactor->setVisible(false);
}

/**
 * updates the text on the vertical scale
 */
void GraphsState::updateScale(double lowerLimit, double upperLimit, int grid)
{
	double increment = ((upperLimit - lowerLimit) / static_cast<double>(grid));
	if (increment < 10.0)
	{
		increment = 10.0;
	}

	double text = lowerLimit;
	for (int
			i = 0;
			i < grid + 1;
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
	if (!_country && !_finance)
	{
		drawRegionLines();
	}
	else if (!_finance)
	{
		drawCountryLines();
	}
	else
	{
		drawFinanceLines();
	}
}

/**
 * Sets up the screens and draws the lines for country buttons to toggle on and off.
 */
void GraphsState::drawCountryLines()
{
	// calculate the totals, and set up our upward maximum
	int upperLimit = 0;
	int lowerLimit = 0;
	int totals[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	for (size_t
			entry = 0;
			entry < _game->getSavedGame()->getFundsList().size();
			++entry)
	{
		int total = 0;
		int amount = 0;

		if (_alien)
		{
			for (size_t
					iter = 0;
					iter < _game->getSavedGame()->getCountries()->size();
					++iter)
			{
				amount = _game->getSavedGame()->getCountries()->at(iter)->getActivityAlien().at(entry);
				total += amount;

				if (amount > upperLimit
					&& _countryToggles.at(iter)->_pushed)
				{
					upperLimit = amount;
				}

/*				if (amount < lowerLimit
					&& _countryToggles.at(iter)->_pushed)
				{
					lowerLimit = amount;
				} */
			}
		}
		else if (_income)
		{
			for (size_t
					iter = 0;
					iter < _game->getSavedGame()->getCountries()->size();
					++iter)
			{
				amount = _game->getSavedGame()->getCountries()->at(iter)->getFunding().at(entry) / 1000;
				total += amount;

				if (amount > upperLimit
					&& _countryToggles.at(iter)->_pushed)
				{
					upperLimit = amount;
				}

/*				if (amount < lowerLimit
					&& _countryToggles.at(iter)->_pushed)
				{
					lowerLimit = amount;
				} */
			}
		}
		else
		{
			for (size_t
					iter = 0;
					iter < _game->getSavedGame()->getCountries()->size();
					++iter)
			{
				amount = _game->getSavedGame()->getCountries()->at(iter)->getActivityXcom().at(entry);
				total += amount;

				if (amount > upperLimit
					&& _countryToggles.at(iter)->_pushed)
				{
					upperLimit = amount;
				}

				if (amount < lowerLimit
					&& _countryToggles.at(iter)->_pushed)
				{
					lowerLimit = amount;
				}
			}
		}

		if (_countryToggles.back()->_pushed
			&& total > upperLimit)
		{
			upperLimit = total;
		}
	}

	// adjust the scale to fit the upward maximum
	double range = static_cast<double>(upperLimit - lowerLimit);
	double low = static_cast<double>(lowerLimit);

//	int test = _income? 50: 100;
	int test = 50;
	int grid = 9; // cells in grid

	while (range > static_cast<double>(test * grid))
	{
		test *= 2;
	}

	lowerLimit = 0;
	upperLimit = test * grid;

	if (low < 0.0)
	{
		while (low < static_cast<double>(lowerLimit))
		{
			lowerLimit -= test;
			upperLimit -= test;
		}
	}

	range = static_cast<double>(upperLimit - lowerLimit);
	double units = range / 126.0;

	// draw country lines
	for (size_t
			entry = 0;
			entry < _game->getSavedGame()->getCountries()->size();
			++entry)
	{
		Country* country = _game->getSavedGame()->getCountries()->at(entry);

		_alienCountryLines.at(entry)->clear();
		_xcomCountryLines.at(entry)->clear();
		_incomeLines.at(entry)->clear();

		std::vector<Sint16> newLineVector;

		int reduction = 0;
		for (size_t
				iter = 0;
				iter != 12;
				++iter)
		{
			int x = 312 - (iter * 17);
			int y = 175 + static_cast<int>((static_cast<double>(lowerLimit) / units));

			if (_alien)
			{
				if (iter < country->getActivityAlien().size())
				{
					reduction = static_cast<int>(static_cast<double>(country->getActivityAlien()
							.at(country->getActivityAlien().size() - (1 + iter))) / units);
					y -= reduction;

					totals[iter] += country->getActivityAlien().at(country->getActivityAlien().size() - (1 + iter));
				}
			}
			else if (_income)
			{
				if (iter < country->getFunding().size())
				{
					reduction = static_cast<int>(static_cast<double>(country->getFunding()
							.at(country->getFunding().size() - (1 + iter))) / 1000.0 / units);
					y -= reduction;

					totals[iter] += static_cast<int>(static_cast<double>(country->getFunding()
							.at(country->getFunding().size() - (1 + iter))) / 1000.0);
				}
			}
			else
			{
				if (iter < country->getActivityXcom().size())
				{
					reduction = static_cast<int>(static_cast<double>(country->getActivityXcom()
							.at(country->getActivityXcom().size() - (1 + iter))) / units);
					y -= reduction;

					totals[iter] += country->getActivityXcom().at(country->getActivityXcom().size() - (1 + iter));
				}
			}

			if (y > 175) y = 175;
			newLineVector.push_back(y);

			int offset = 0;
			if (entry %2) offset = 8;

			if (newLineVector.size() > 1 && _alien)
				_alienCountryLines.at(entry)->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset((entry / 2) + 1) + offset);
			else if (newLineVector.size() > 1 && _income)
				_incomeLines.at(entry)->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset((entry / 2) + 1) + offset);
			else if (newLineVector.size() > 1)
				_xcomCountryLines.at(entry)->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset((entry / 2) + 1) + offset);
		}

		if (_alien)
			_alienCountryLines.at(entry)->setVisible(_countryToggles.at(entry)->_pushed);
		else if (_income)
			_incomeLines.at(entry)->setVisible(_countryToggles.at(entry)->_pushed);
		else
			_xcomCountryLines.at(entry)->setVisible(_countryToggles.at(entry)->_pushed);
	}

	if (_alien)
		_alienCountryLines.back()->clear();
	else if (_income)
		_incomeLines.back()->clear();
	else
		_xcomCountryLines.back()->clear();

	// set up the "total" line
	std::vector<Sint16> newLineVector;

	for (int
			iter = 0;
			iter < 12;
			++iter)
	{
		int x = 312 - (iter * 17);
		int y = 175 + static_cast<int>(static_cast<double>(lowerLimit) / units);

		if (totals[iter] > 0)
		{
			int reduction = static_cast<int>(static_cast<double>(totals[iter]) / units);
			y -= reduction;
		}

		newLineVector.push_back(y);
		if (newLineVector.size() > 1)
		{
			if (_alien)
				_alienCountryLines.back()->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset(9));
			else if (_income)
				_incomeLines.back()->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset(9));
			else
				_xcomCountryLines.back()->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset(9));
		}
	}

	if (_alien)
		_alienCountryLines.back()->setVisible(_countryToggles.back()->_pushed);
	else if (_income)
		_incomeLines.back()->setVisible(_countryToggles.back()->_pushed);
	else
		_xcomCountryLines.back()->setVisible(_countryToggles.back()->_pushed);

	updateScale(static_cast<double>(lowerLimit), static_cast<double>(upperLimit));

	_txtFactor->setVisible(_income);
}

/**
 * Sets up the screens and draws the lines for region buttons
 * to toggle on and off
 */
void GraphsState::drawRegionLines()
{
	// calculate the totals, and set up our upward maximum
	int upperLimit = 0;
	int lowerLimit = 0;
	int totals[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	for (size_t
			entry = 0;
			entry < _game->getSavedGame()->getFundsList().size();
			++entry)
	{
		int total = 0;
		int amount = 0;

		if (_alien)
		{
			for (size_t
					iter = 0;
					iter < _game->getSavedGame()->getRegions()->size();
					++iter)
			{
				amount = _game->getSavedGame()->getRegions()->at(iter)->getActivityAlien().at(entry);
				total += amount;

				if (amount > upperLimit
					&& _regionToggles.at(iter)->_pushed)
				{
					upperLimit = amount;
				}

/*				if (amount < lowerLimit
					&& _regionToggles.at(iter)->_pushed)
				{
					lowerLimit = amount;
				} */
			}
		}
		else
		{
			for (size_t
					iter = 0;
					iter < _game->getSavedGame()->getRegions()->size();
					++iter)
			{
				amount = _game->getSavedGame()->getRegions()->at(iter)->getActivityXcom().at(entry);
				total += amount;

				if (amount > upperLimit
					&& _regionToggles.at(iter)->_pushed)
				{
					upperLimit = amount;
				}

				if (amount < lowerLimit
					&& _regionToggles.at(iter)->_pushed)
				{
					lowerLimit = amount;
				}
			}
		}

		if (_regionToggles.back()->_pushed
			&& total > upperLimit)
		{
			upperLimit = total;
		}
	}

	// adjust the scale to fit the upward maximum
	double range = static_cast<double>(upperLimit - lowerLimit);
	double low = static_cast<double>(lowerLimit);

	int test = 50;
	int grid = 9; // cells in grid

	while (range > static_cast<double>(test * grid))
	{
		test *= 2;
	}

	lowerLimit = 0;
	upperLimit = test * grid;

	if (low < 0.0)
	{
		while (low < static_cast<double>(lowerLimit))
		{
			lowerLimit -= test;
			upperLimit -= test;
		}
	}

	range = static_cast<double>(upperLimit - lowerLimit);
	double units = range / 126.0;

	// draw region lines
	for (size_t
			entry = 0;
			entry < _game->getSavedGame()->getRegions()->size();
			++entry)
	{
		Region* region = _game->getSavedGame()->getRegions()->at(entry);

		_alienRegionLines.at(entry)->clear();
		_xcomRegionLines.at(entry)->clear();

		std::vector<Sint16> newLineVector;

		int reduction = 0;
		for (size_t
				iter = 0;
				iter < 12;
				++iter)
		{
			int x = 312 - (iter * 17);
			int y = 175 + static_cast<int>(static_cast<double>(lowerLimit) / units);

			if (_alien)
			{
				if (iter < region->getActivityAlien().size())
				{
					reduction = static_cast<int>(static_cast<double>(region->getActivityAlien()
							.at(region->getActivityAlien().size() - (1 + iter))) / units);
					y -= reduction;

					totals[iter] += region->getActivityAlien().at(region->getActivityAlien().size() - (1 + iter));
				}
			}
			else
			{
				if (iter < region->getActivityXcom().size())
				{
					reduction = static_cast<int>(static_cast<double>(region->getActivityXcom()
							.at(region->getActivityXcom().size() - (1 + iter))) / units);
					y -= reduction;

					totals[iter] += region->getActivityXcom().at(region->getActivityXcom().size() - (1 + iter));
				}
			}

			if (y > 175) y = 175;
			newLineVector.push_back(y);

			int offset = 0;
			if (entry %2) offset = 8;

			if (newLineVector.size() > 1 && _alien)
				_alienRegionLines.at(entry)->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset((entry / 2) + 1) + offset);
			else if (newLineVector.size() > 1)
				_xcomRegionLines.at(entry)->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset((entry / 2) + 1) + offset);
		}

		if (_alien)
			_alienRegionLines.at(entry)->setVisible(_regionToggles.at(entry)->_pushed);
		else
			_xcomRegionLines.at(entry)->setVisible(_regionToggles.at(entry)->_pushed);
	}

	if (_alien)
		_alienRegionLines.back()->clear();
	else
		_xcomRegionLines.back()->clear();

	// set up the "total" line
	std::vector<Sint16> newLineVector;

	for (int
			iter = 0;
			iter < 12;
			++iter)
	{
		int x = 312 - (iter * 17);
		int y = 175 + static_cast<int>(static_cast<double>(lowerLimit) / units);
//		int y = 175;

		if (totals[iter] > 0)
		{
			int reduction = static_cast<int>(static_cast<double>(totals[iter]) / units);
			y -= reduction;
		}

		newLineVector.push_back(y);
		if (newLineVector.size() > 1)
		{
			if (_alien)
				_alienRegionLines.back()->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset(9));
			else
				_xcomRegionLines.back()->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset(9));
		}
	}

	if (_alien)
		_alienRegionLines.back()->setVisible(_regionToggles.back()->_pushed);
	else
		_xcomRegionLines.back()->setVisible(_regionToggles.back()->_pushed);

	updateScale(static_cast<double>(lowerLimit), static_cast<double>(upperLimit));

	_txtFactor->setVisible(false);
}

/**
 * Sets up the screens and draws the lines for the finance buttons to toggle on and off
 */
void GraphsState::drawFinanceLines()
{
	// set up arrays
	int upperLimit = 0;
	int lowerLimit = 0;

	int income[]		= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int balance[]		= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int expenditure[]	= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int maintenance[]	= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int score[]			= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	maintenance[0] = _game->getSavedGame()->getBaseMaintenance() / 1000;

	// start filling those arrays with score values;
	// determine which is the highest one being displayed, so we can adjust the scale
	for (size_t
			entry = 0;
			entry != _game->getSavedGame()->getFundsList().size();
			++entry)
	{
		size_t invertedEntry = _game->getSavedGame()->getFundsList().size() - (entry + 1);

		maintenance[entry]	+= _game->getSavedGame()->getMaintenances().at(invertedEntry) / 1000;
		balance[entry]		= _game->getSavedGame()->getFundsList().at(invertedEntry) / 1000;
		score[entry]		= _game->getSavedGame()->getResearchScores().at(invertedEntry);

		for (std::vector<Country*>::iterator
				iter = _game->getSavedGame()->getCountries()->begin();
				iter != _game->getSavedGame()->getCountries()->end();
				++iter)
			income[entry] += (*iter)->getFunding().at(invertedEntry) / 1000;

		for (std::vector<Region*>::iterator
				iter = _game->getSavedGame()->getRegions()->begin();
				iter != _game->getSavedGame()->getRegions()->end();
				++iter)
			score[entry] += (*iter)->getActivityXcom().at(invertedEntry) - (*iter)->getActivityAlien().at(invertedEntry);

		if (_financeToggles.at(0))
		{
			if (income[entry] > upperLimit)
				upperLimit = income[entry];

			if (income[entry] < lowerLimit)
				lowerLimit = income[entry];
		}
		
		if (_financeToggles.at(2))
		{
			if (maintenance[entry] > upperLimit)
				upperLimit = maintenance[entry];

			if (maintenance[entry] < lowerLimit)
				lowerLimit = maintenance[entry];
		}

		if (_financeToggles.at(3))
		{
			if (balance[entry] > upperLimit)
				upperLimit = balance[entry];

			if (balance[entry] < lowerLimit)
				lowerLimit = balance[entry];
		}

		if (_financeToggles.at(4))
		{
			if (score[entry] > upperLimit)
				upperLimit = score[entry];

			if (score[entry] < lowerLimit)
				lowerLimit = score[entry];
		}
	}

	expenditure[0] = balance[1] - balance[0];
	if (expenditure[0] < 0) expenditure[0] = 0;

	if (_financeToggles.at(1)
			&& expenditure[0] > upperLimit)
	{
		upperLimit = expenditure[0];
	}

	for (size_t
			entry = 1;
			entry < _game->getSavedGame()->getFundsList().size();
			++entry)
	{
		expenditure[entry] = ((balance[entry + 1] + income[entry]) - maintenance[entry]) - balance[entry];
		if (expenditure[entry] < 0) expenditure[entry] = 0;

		if (_financeToggles.at(1)
				&& expenditure[entry] > upperLimit)
		{
			upperLimit = expenditure[entry];
		}
	}

	double range = static_cast<double>(upperLimit - lowerLimit);
	double low = static_cast<double>(lowerLimit);

	int test = 200;
	int grid = 9;

	while (range > static_cast<double>(test * grid))
	{
		test *= 2;
	}

	lowerLimit = 0;
	upperLimit = test * grid;

	if (low < 0.0)
	{
		while (low < static_cast<double>(lowerLimit))
		{
			lowerLimit -= test;
			upperLimit -= test;
		}
	}

	// toggle screens
	for (int
			button = 0;
			button < 5;
			++button)
	{
		_financeLines.at(button)->setVisible(_financeToggles.at(button));
		_financeLines.at(button)->clear();
	}

	range = static_cast<double>(upperLimit - lowerLimit);
	// figure out how many units to the pixel, then plot the points for the graph and connect the dots.
	double units = range / 126.0;

	for (int
			button = 0;
			button < 5;
			++button)
	{
		std::vector<Sint16> newLineVector;

		for (int
				iter = 0;
				iter < 12;
				++iter)
		{
			int x = 312 - (iter * 17);
			int y = 175 + static_cast<int>(static_cast<double>(lowerLimit) / units);
			int reduction = 0;

			switch (button)
			{
				case 0:
					reduction = static_cast<int>(static_cast<double>(income[iter]) / units);
				break;
				case 1:
					reduction = static_cast<int>(static_cast<double>(expenditure[iter]) / units);
				break;
				case 2:
					reduction = static_cast<int>(static_cast<double>(maintenance[iter]) / units);
				break;
				case 3:
					reduction = static_cast<int>(static_cast<double>(balance[iter]) / units);
				break;
				case 4:
					reduction = static_cast<int>(static_cast<double>(score[iter]) / units);
				break;
			}

			y -= reduction;
			newLineVector.push_back(y);

			int offset = button %2? 8: 0;

			if (newLineVector.size() > 1)
				_financeLines.at(button)->drawLine(
						x,
						y,
						x + 17,
						newLineVector.at(newLineVector.size() - 2),
						Palette::blockOffset((button / 2) + 1) + offset);
		}
	}

	updateScale(static_cast<double>(lowerLimit), static_cast<double>(upperLimit));

	_txtFactor->setVisible(true);
}

/**
 * 'Shift' the buttons to display only GRAPH_MAX_BUTTONS - reset their state from toggles
 */
void GraphsState::shiftButtons(Action* action)
{
	// only if active 'screen' is other than finance
	if (_finance) return;

	// select the data we'll processing - regions or countries
	if (_country)
	{
		// too few countries? - return
		if (_countryToggles.size() <= GRAPH_MAX_BUTTONS)
			return;
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			scrollButtons(_countryToggles, _btnCountries, _btnCountriesOffset, - 1);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			scrollButtons(_countryToggles, _btnCountries, _btnCountriesOffset, 1);
	}
	else
	{
		// too few regions? - return
		if (_regionToggles.size() <= GRAPH_MAX_BUTTONS)
			return;
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			scrollButtons(_regionToggles, _btnRegions, _btnRegionsOffset, - 1);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			scrollButtons(_regionToggles, _btnRegions, _btnRegionsOffset, 1);
	}
}

/**
 *
 */
void GraphsState::scrollButtons(
		std::vector<GraphBtnInfo*>& toggles,
		std::vector<ToggleTextButton*>& buttons,
		unsigned int& offset,
		int step)
{
	// minus one, 'cause we'll already added the TOTAL button to toggles
	if (step + static_cast<int>(offset) < 0
		|| static_cast<int>(offset) + step + GRAPH_MAX_BUTTONS >= toggles.size() - 1)
	{
		return;
	}

	// set the next offset - cheaper to do it from starters
	offset += static_cast<unsigned int>(step);
	unsigned int i = 0;

	std::vector<ToggleTextButton*>::iterator iterb = buttons.begin();
	for (std::vector<GraphBtnInfo*>::iterator
			itert = toggles.begin();
			itert != toggles.end();
			++itert, ++i)
	{
		if (i < offset)
			continue;
		else if (i < offset + GRAPH_MAX_BUTTONS)
			updateButton(*itert, *iterb++);
		else
			return;
	}
}

/**
 *
 */
void GraphsState::updateButton(GraphBtnInfo* from, ToggleTextButton* to)
{
	to->setText(from->_name);
	to->setInvertColor(from->_color);
	to->setPressed(from->_pushed);
}

}
