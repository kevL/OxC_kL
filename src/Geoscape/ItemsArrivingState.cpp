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

#include "ItemsArrivingState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "GeoscapeState.h"
#include "../Engine/Options.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Items Arriving window.
 * @param game Pointer to the core game.
 * @param state Pointer to the Geoscape state.
 */
ItemsArrivingState::ItemsArrivingState(Game* game, GeoscapeState* state)
	:
		State(game),
		_state(state)
{
	_screen = false;

	_window			= new Window(this, 320, 184, 0, 8, POPUP_BOTH);

	_txtTitle		= new Text(310, 17, 5, 17);

	_txtItem		= new Text(144, 9, 16, 34);
	_txtQuantity	= new Text(52, 9, 168, 34);
	_txtDestination	= new Text(92, 9, 220, 34);

	_lstTransfers	= new TextList(286, 120, 16, 46);

	_btnOk			= new TextButton(134, 16, 16, 169);
	_btnOk5Secs		= new TextButton(134, 16, 170, 169);


	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)), Palette::backPos, 16);

	add(_window);
	add(_btnOk);
	add(_btnOk5Secs);
	add(_txtTitle);
	add(_txtItem);
	add(_txtQuantity);
	add(_txtDestination);
	add(_lstTransfers);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ItemsArrivingState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ItemsArrivingState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_btnOk5Secs->setColor(Palette::blockOffset(8)+5);
	_btnOk5Secs->setText(tr("STR_OK_5_SECS"));
	_btnOk5Secs->onMouseClick((ActionHandler)&ItemsArrivingState::btnOk5SecsClick);
	_btnOk5Secs->onKeyboardPress((ActionHandler)&ItemsArrivingState::btnOk5SecsClick, (SDLKey)Options::getInt("keyOk"));

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_ITEMS_ARRIVING"));

	_txtItem->setColor(Palette::blockOffset(8)+5);
	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setColor(Palette::blockOffset(8)+5);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtDestination->setColor(Palette::blockOffset(8)+5);
	_txtDestination->setText(tr("STR_DESTINATION_UC"));

	_lstTransfers->setColor(Palette::blockOffset(8)+10);
	_lstTransfers->setArrowColor(Palette::blockOffset(8)+5);
	_lstTransfers->setColumns(3, 144, 53, 92);
	_lstTransfers->setSelectable(true);
	_lstTransfers->setBackground(_window);
	_lstTransfers->setMargin(8);

	for (std::vector<Base*>::iterator i = _game->getSavedGame()->getBases()->begin(); i != _game->getSavedGame()->getBases()->end(); ++i)
	{
		for (std::vector<Transfer*>::iterator j = (*i)->getTransfers()->begin(); j != (*i)->getTransfers()->end();)
		{
			if ((*j)->getHours() == 0)
			{
				if ((*j)->getType() == TRANSFER_ITEM
					&& _game->getRuleset()->getItem((*j)->getItems())->getBattleType() == BT_NONE)
				{
					for (std::vector<Craft*>::iterator c = (*i)->getCrafts()->begin(); c != (*i)->getCrafts()->end(); ++c)
					{
						if ((*c)->getStatus() != "STR_READY")
							continue;

						for (std::vector<CraftWeapon*>::iterator w = (*c)->getWeapons()->begin(); w != (*c)->getWeapons()->end(); ++w)
						{
							if (*w != 0
								&& (*w)->getAmmo() < (*w)->getRules()->getAmmoMax())
							{
								(*w)->setRearming(true);
								(*c)->setStatus("STR_REARMING");
							}
						}
					}
				}

				std::wstringstream ss;
				ss << (*j)->getQuantity();
				_lstTransfers->addRow(3, (*j)->getName(_game->getLanguage()).c_str(), ss.str().c_str(), (*i)->getName().c_str());

				delete *j;
				j = (*i)->getTransfers()->erase(j);
			}
			else
			{
				++j;
			}
		}
	}
}

/**
 *
 */
ItemsArrivingState::~ItemsArrivingState()
{
}

/**
 * Resets the palette.
 */
void ItemsArrivingState::init()
{
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)), Palette::backPos, 16);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ItemsArrivingState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Reduces the speed to 5 Secs and returns to the previous screen.
 * @param action Pointer to an action.
 */
void ItemsArrivingState::btnOk5SecsClick(Action*)
{
	_state->timerReset();
	_game->popState();
}

}
