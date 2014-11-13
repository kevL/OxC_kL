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

#include "PromotionsState.h"

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

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Promotions screen.
 */
PromotionsState::PromotionsState()
{
	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(300, 17, 10, 13);

	_txtName		= new Text(114, 9, 16, 32);
	_txtRank		= new Text(70, 9, 150, 32);
	_txtBase		= new Text(80, 9, 220, 32);

	_lstSoldiers	= new TextList(285, 129, 16, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_txtName);
	add(_txtRank);
	add(_txtBase);
	add(_lstSoldiers);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& PromotionsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PromotionsState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PromotionsState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setText(tr("STR_PROMOTIONS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtName->setColor(Palette::blockOffset(15)-1);
	_txtName->setText(tr("STR_NAME"));

	_txtRank->setColor(Palette::blockOffset(15)-1);
	_txtRank->setText(tr("STR_NEW_RANK"));

	_txtBase->setColor(Palette::blockOffset(15)-1);
	_txtBase->setText(tr("STR_BASE"));

	_lstSoldiers->setColor(Palette::blockOffset(8)+10);
	_lstSoldiers->setColumns(3, 126, 70, 81);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin();

	for (std::vector<Base*>::iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Soldier*>::iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			if ((*j)->isPromoted())
			{
				_lstSoldiers->addRow(
									3,
									(*j)->getName().c_str(),
									tr((*j)->getRankString()).c_str(),
									(*i)->getName().c_str());
			}
		}
	}
}

/**
 * dTor.
 */
PromotionsState::~PromotionsState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void PromotionsState::btnOkClick(Action*)
{
	_game->popState();
}

}
