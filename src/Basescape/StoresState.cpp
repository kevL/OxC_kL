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

#include "StoresState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleItem.h"
#include "../Savegame/ItemContainer.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Stores window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
StoresState::StoresState(Game* game, Base* base)
	:
		State(game),
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);

//kL	_txtTitle		= new Text(310, 16, 5, 8);
	_txtTitle		= new Text(300, 16, 10, 9);			// kL

//kL	_txtItem		= new Text(142, 8, 10, 32);
//kL	_txtQuantity	= new Text(88, 8, 152, 32);
//kL	_txtSpaceUsed	= new Text(74, 8, 240, 32);
	_txtItem		= new Text(142, 9, 16, 25);			// kL
	_txtQuantity	= new Text(88, 9, 178, 25);			// kL
	_txtSpaceUsed	= new Text(74, 9, 270, 25);			// kL

	_lstStores		= new TextList(288, 136, 8, 36);

//kL	_btnOk			= new TextButton(300, 16, 10, 176);
	_btnOk			= new TextButton(288, 16, 16, 177);		// kL


	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)), Palette::backPos, 16);

	add(_window);
	add(_btnOk);
	add(_txtTitle);
	add(_txtItem);
	add(_txtQuantity);
	add(_txtSpaceUsed);
	add(_lstStores);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& StoresState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)& StoresState::btnOkClick, (SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress((ActionHandler)& StoresState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
//kL	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_STORES"));

	_txtItem->setColor(Palette::blockOffset(13)+10);
	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setColor(Palette::blockOffset(13)+10);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSpaceUsed->setColor(Palette::blockOffset(13)+10);
//kL	_txtSpaceUsed->setText(tr("STR_SPACE_USED_UC"));
	_txtSpaceUsed->setText(tr("STR_VOLUME"));		// kL

	_lstStores->setColor(Palette::blockOffset(13)+10);
	_lstStores->setColumns(3, 162, 92, 32);
	_lstStores->setSelectable(true);
	_lstStores->setBackground(_window);
//kL	_lstStores->setMargin(2);
	_lstStores->setMargin(8);		// kL

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		int qty = _base->getItems()->getItem(*i);
		if (qty > 0)
		{
			RuleItem* rule = _game->getRuleset()->getItem(*i);

			std::wstringstream ss, ss2;
			ss << qty;
			ss2 << qty * rule->getSize();
			_lstStores->addRow(3, tr(*i).c_str(), ss.str().c_str(), ss2.str().c_str());
		}
	}
}

/**
 *
 */
StoresState::~StoresState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StoresState::btnOkClick(Action* )
{
	_game->popState();
}

}
