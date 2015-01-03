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

#include "FundingState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"

#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Funding screen.
 */
FundingState::FundingState()
{
	_screen = false;

	_window			= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_txtTitle		= new Text(300, 17, 10, 9);

	_txtCountry		= new Text(102, 9, 16, 25);
	_txtFunding		= new Text(60, 9, 118, 25);
	_txtChange		= new Text(60, 9, 178, 25);
	_txtScore		= new Text(60, 9, 238, 25);

	_lstCountries	= new TextList(277, 121, 24, 34);
	_lstTotal		= new TextList(285, 17, 16, 157);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("fundingWindow")->getElement("palette")->color); //0

	add(_window, "window", "fundingWindow");
	add(_txtTitle, "text1", "fundingWindow");
	add(_txtCountry, "text2", "fundingWindow");
	add(_txtFunding, "text2", "fundingWindow");
	add(_txtChange, "text2", "fundingWindow");
	add(_txtScore, "text2", "fundingWindow");
	add(_lstCountries, "list", "fundingWindow");
	add(_lstTotal, "text2", "fundingWindow");
	add(_btnOk, "button", "fundingWindow");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

//	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& FundingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& FundingState::btnOkClick,
					Options::keyGeoFunding);

//	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_INTERNATIONAL_RELATIONS"));

//	_txtCountry->setColor(Palette::blockOffset(8)+5);
	_txtCountry->setText(tr("STR_COUNTRY"));

//	_txtFunding->setColor(Palette::blockOffset(8)+5);
	_txtFunding->setText(tr("STR_FUNDING"));

//	_txtChange->setColor(Palette::blockOffset(8)+5);
	_txtChange->setText(tr("STR_CHANGE"));

//	_txtScore->setColor(Palette::blockOffset(8)+5);
	_txtScore->setText(tr("STR_SCORE"));

//	_lstCountries->setColor(Palette::blockOffset(15)-1);
//	_lstCountries->setSecondaryColor(Palette::blockOffset(8)+10);
//	_lstCountries->setSelectable();
	_lstCountries->setColumns(6, 94, 60, 6, 54, 6, 54);
	_lstCountries->setDot();
	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		std::wostringstream
			ss1,
			ss2,
			ss3,
			ss4,
			ss5;

		const std::vector<int>
			funds = (*i)->getFunding(),
			actX = (*i)->getActivityXcom(),
			actA = (*i)->getActivityAlien();

		ss1 << L'\x01' << Text::formatFunding(funds.at(funds.size() - 1)) << L'\x01';

		if (funds.size() > 1)
		{
			int change = funds.back() - funds.at(funds.size() - 2);
			if (change > 0)
				ss2 << L'\x01' << L'+' << L'\x01';
			else if (change < 0)
			{
				ss2 << L'\x01' << L'-' << L'\x01';
				change = -change;
			}
			else
				ss2 << L' ' << L' ';

			ss3 << L'\x01' << Text::formatFunding(change) << L'\x01';
		}
		else
		{
			ss2 << L' ' << L' ';
			ss3 << Text::formatFunding(0);
		}

		int score = actX.at(actX.size() - 1) - actA.at(actA.size() - 1);
		if (score > -1)
			ss4 << L' ' << L' ';
		else
		{
			ss4 << L'\x01' << L'-' << L'\x01';
			score = -score;
		}

		ss5 << L'\x01' << score << L'\x01';

		_lstCountries->addRow(
							6,
							tr((*i)->getRules()->getType()).c_str(),
							ss1.str().c_str(),
							ss2.str().c_str(),
							ss3.str().c_str(),
							ss4.str().c_str(),
							ss5.str().c_str());
	}


	const int gross = _game->getSavedGame()->getCountryFunding();
	int net = gross;

	for (std::vector<Base*>::const_iterator
		i = _game->getSavedGame()->getBases()->begin();
		i != _game->getSavedGame()->getBases()->end();
		++i)
	{
		net -= (*i)->getMonthlyMaintenace();
	}

	std::wostringstream
		wostr1,
		wostr2;

	wostr1 << L' ' << L' ';

	if (net > -1)
		wostr2 << L' ' << L' ';
	else
	{
		wostr2 << L'-';
		net = -net;
	}

	_lstTotal->setColor(Palette::blockOffset(8)+5);
	_lstTotal->setColumns(4, 122, 92, 6, 54);
	_lstTotal->setMargin();
	_lstTotal->setDot();
	_lstTotal->addRow(
					4,
					"",
					tr("STR_TOTAL_GROSS_UC").c_str(),
					wostr1.str().c_str(),
					Text::formatFunding(gross).c_str());
	_lstTotal->addRow(
					4,
					"",
					tr("STR_TOTAL_NET_UC").c_str(),
					wostr2.str().c_str(),
					Text::formatFunding(net).c_str());
}

/**
 * dTor.
 */
FundingState::~FundingState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void FundingState::btnOkClick(Action*)
{
	_game->popState();
}

}
