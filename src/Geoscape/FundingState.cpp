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

#include "FundingState.h"

#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"

#include "../Savegame/Country.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Funding screen.
 */
FundingState::FundingState()
{
	//Log(LOG_INFO) << "Create FundingState";
	_screen = false;

	_window			= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_txtTitle		= new Text(300, 17, 10, 9);

	_txtCountry		= new Text(102, 9, 16, 25);
	_txtFunding		= new Text(60, 9, 118, 25);
	_txtChange		= new Text(60, 9, 178, 25);
	_txtScore		= new Text(60, 9, 238, 25);

	_lstCountries	= new TextList(277, 129, 24, 34);
	_lstTotal		= new TextList(285, 9, 16, 165);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_txtCountry);
	add(_txtFunding);
	add(_txtChange);
	add(_txtScore);
	add(_lstCountries);
	add(_lstTotal);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)-1);
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

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_INTERNATIONAL_RELATIONS"));

	_txtCountry->setColor(Palette::blockOffset(8)+5);
	_txtCountry->setText(tr("STR_COUNTRY"));

	_txtFunding->setColor(Palette::blockOffset(8)+5);
	_txtFunding->setText(tr("STR_FUNDING"));

	_txtChange->setColor(Palette::blockOffset(8)+5);
	_txtChange->setText(tr("STR_CHANGE"));

	_txtScore->setColor(Palette::blockOffset(8)+5);
	_txtScore->setText(tr("STR_SCORE"));

	_lstCountries->setColor(Palette::blockOffset(15)-1);
	_lstCountries->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstCountries->setColumns(4, 94, 60, 60, 60);
	_lstCountries->setDot();
	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		std::wostringstream
			ss1,
			ss2,
			ss3;

		const std::vector<int>
			funds = (*i)->getFunding(),
			actX = (*i)->getActivityXcom(),
			actA = (*i)->getActivityAlien();

		ss1 << L'\x01' << Text::formatFunding(funds.at(funds.size() - 1)) << L'\x01';

		if (funds.size() > 1)
		{
			ss2 << L'\x01';
			const int change = funds.back() - funds.at(funds.size() - 2);
			if (change > 0)
				ss2 << L'+';
			ss2 << Text::formatFunding(change);
			ss2 << L'\x01';
		}
		else
			ss2 << Text::formatFunding(0);

		ss3 << actX.at(actX.size() - 1) - actA.at(actA.size() -1);

		_lstCountries->addRow(
							4,
							tr((*i)->getRules()->getType()).c_str(),
							ss1.str().c_str(),
							ss2.str().c_str(),
							ss3.str().c_str());
	}

	_lstTotal->setColor(Palette::blockOffset(8)+5);
	_lstTotal->setColumns(3, 122, 92, 60);
	_lstTotal->setMargin();
	_lstTotal->setDot();
	_lstTotal->addRow(
					3,
					"",
					tr("STR_TOTAL_UC").c_str(),
					Text::formatFunding(_game->getSavedGame()->getCountryFunding()).c_str());
	//Log(LOG_INFO) << "Create FundingState DONE";
}

/**
 * dTor.
 */
FundingState::~FundingState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void FundingState::btnOkClick(Action*)
{
	_game->popState();
}

}
