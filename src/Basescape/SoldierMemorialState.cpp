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

#include "SoldierMemorialState.h"

//#include <iomanip>
//#include <sstream>

#include "SoldierInfoDeadState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Memorial screen.
 */
SoldierMemorialState::SoldierMemorialState()
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(310, 17, 5, 9);

	_txtRecruited	= new Text(110, 9, 16, 25);
	_txtLost		= new Text(110, 9, 210, 25);

	_txtName		= new Text(132, 9, 16, 36);
	_txtRank		= new Text(70, 9, 148, 36);
	_txtDate		= new Text(86, 9, 218, 36);

	_lstSoldiers	= new TextList(285, 129, 16, 44);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_BASESCAPE", 7);

	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_BASE_MEMORIAL);

	add(_window);
	add(_txtTitle);
	add(_txtRecruited);
	add(_txtLost);
	add(_txtName);
	add(_txtRank);
	add(_txtDate);
	add(_lstSoldiers);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierMemorialState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierMemorialState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MEMORIAL"));

	_txtName->setColor(Palette::blockOffset(13)+10);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setColor(Palette::blockOffset(13)+10);
	_txtRank->setText(tr("STR_RANK"));

	_txtDate->setColor(Palette::blockOffset(13)+10);
	_txtDate->setText(tr("STR_DATE_DEATH"));

	const size_t lost = _game->getSavedGame()->getDeadSoldiers()->size();
	size_t recruited = lost;
	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		recruited += (*i)->getTotalSoldiers();
	}

	_txtRecruited->setColor(Palette::blockOffset(13)+10);
	_txtRecruited->setSecondaryColor(Palette::blockOffset(13));
	_txtRecruited->setText(tr("STR_SOLDIERS_RECRUITED").arg(recruited));

	_txtLost->setColor(Palette::blockOffset(13)+10);
	_txtLost->setSecondaryColor(Palette::blockOffset(13));
	_txtLost->setText(tr("STR_SOLDIERS_LOST").arg(lost));

	_lstSoldiers->setColor(Palette::blockOffset(15)+6);
	_lstSoldiers->setArrowColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setColumns(5, 124, 70, 26, 23, 33);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin();
	_lstSoldiers->onMousePress((ActionHandler)& SoldierMemorialState::lstSoldiersPress);

	//Log(LOG_INFO) << "SoldierMemorialState::SoldierMemorialState() -> getDeadSoldiers";
	for (std::vector<SoldierDead*>::const_reverse_iterator
			i = _game->getSavedGame()->getDeadSoldiers()->rbegin();
			i != _game->getSavedGame()->getDeadSoldiers()->rend();
			++i)
/*	for (std::vector<SoldierDead*>::const_iterator
			i = _game->getSavedGame()->getDeadSoldiers()->begin();
			i != _game->getSavedGame()->getDeadSoldiers()->end();
			++i) */
	{
		//Log(LOG_INFO) << ". dead soldier, getSoldierDeath & addRow etc";
		const SoldierDeath* const death = (*i)->getDeath();

		std::wostringstream
			saveDay,
			saveMonth,
			saveYear;

		saveDay << death->getTime()->getDayString(_game->getLanguage());
		saveMonth << tr(death->getTime()->getMonthString());
		saveYear << death->getTime()->getYear();

		_lstSoldiers->addRow(
							5,
							(*i)->getName().c_str(),
							tr((*i)->getRankString()).c_str(),
							saveDay.str().c_str(),
							saveMonth.str().c_str(),
							saveYear.str().c_str());
	}
}

/**
 * dTor.
 */
SoldierMemorialState::~SoldierMemorialState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierMemorialState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 900);

	_game->popState();
	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
}

/**
 * Shows the selected soldier's info.
 * @param action - pointer to an Action
 */
void SoldierMemorialState::lstSoldiersPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		const size_t row = _game->getSavedGame()->getDeadSoldiers()->size() - _lstSoldiers->getSelectedRow() - 1;
		_game->pushState(new SoldierInfoDeadState(row));
	}
}

}
