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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SoldierMemorialState.h"

#include <iomanip>
#include <sstream>

//kL #include "SoldierInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/SavedGame.h"
//kL#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h" // kL
#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Memorial screen.
 * @param game Pointer to the core game.
 */
SoldierMemorialState::SoldierMemorialState(Game* game)
	:
		State(game)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(310, 17, 5, 9);

	_txtRecruited	= new Text(110, 9, 16, 25);
	_txtLost		= new Text(110, 9, 210, 25);

	_txtName		= new Text(132, 9, 16, 36);
	_txtRank		= new Text(70, 9, 148, 36);
	_txtDate		= new Text(86, 9, 218, 36);

	_lstSoldiers	= new TextList(285, 128, 16, 44);

	_btnOk			= new TextButton(288, 16, 16, 177);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);

//kL	_game->getResourcePack()->getMusic("GMLOSE")->play();
	_game->getResourcePack()->getMusic("GMWIN")->play(); // kL

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
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MEMORIAL"));

	_txtName->setColor(Palette::blockOffset(13)+10);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setColor(Palette::blockOffset(13)+10);
	_txtRank->setText(tr("STR_RANK"));

	_txtDate->setColor(Palette::blockOffset(13)+10);
	_txtDate->setText(tr("STR_DATE_UC"));

	int lost = _game->getSavedGame()->getDeadSoldiers()->size();
	int recruited = lost;
	for (std::vector<Base*>::iterator
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
	_lstSoldiers->setColumns(5, 124, 70, 26, 24, 32);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onMouseClick((ActionHandler)& SoldierMemorialState::lstSoldiersClick);

	//Log(LOG_INFO) << "SoldierMemorialState::SoldierMemorialState() -> getDeadSoldiers";
//kL	for (std::vector<Soldier*>::reverse_iterator
	for (std::vector<SoldierDead*>::reverse_iterator // kL
			i = _game->getSavedGame()->getDeadSoldiers()->rbegin();
			i != _game->getSavedGame()->getDeadSoldiers()->rend();
			++i)
	{
		//Log(LOG_INFO) << ". dead soldier, getSoldierDeath & addRow etc";
		SoldierDeath* death = (*i)->getDeath();

		std::wstringstream
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
 *
 */
SoldierMemorialState::~SoldierMemorialState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierMemorialState::btnOkClick(Action*)
{
	_game->popState();

	_game->getResourcePack()->getRandomMusic("GMGEO")->play();
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void SoldierMemorialState::lstSoldiersClick(Action*)
{
//	_game->pushState(new SoldierInfoState(_game, _base, _lstSoldiers->getSelectedRow()));
}

}
