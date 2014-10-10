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

#include "PrimeGrenadeState.h"

#include <cmath>
#include <sstream>

#include "Inventory.h" // kL

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleInterface.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Prime Grenade window.
 * @param action			- pointer to the BattleAction (BattlescapeGame.h)
 * @param inInventoryView	- true if called from inventory
 * @param grenade			- pointer to associated grenade
 * @param inventory			- pointer to Inventory
 */
PrimeGrenadeState::PrimeGrenadeState(
		BattleAction* action,
		bool inInventoryView,
		BattleItem* grenade,
		Inventory* inventory) // kL_add.
	:
		_action(action),
		_inInventoryView(inInventoryView),
		_grenade(grenade),
		_inventory(inventory) // kL_add.
{
	_screen = false;

	_fraTop		= new Frame(192, 27, 65, 37);
	_txtTitle	= new Text(192, 18, 65, 43);

	_srfBG		= new Surface(192, 93, 65, 45);

	_txtTurn0	= new Text(190, 18, 66, 67); // kL
	_isfBtn0	= new InteractiveSurface(190, 22, 66, 65); // kL

	int
		x = 67,
		y = 92;

	for (int
			i = 0;
			i < 16;
			++i)
	{
		_isfBtn[i] = new InteractiveSurface(
											22,
											22,
											x - 1 + ((i %8) * 24),
											y - 4 + ((i / 8) * 24));
		_txtTurn[i] = new Text(
							20,
							20,
							x + (((i %8) * 24)),
							y + ((i / 8) * 24) - 3);
	}

	if (inInventoryView)
		setPalette("PAL_BATTLESCAPE");
	else
		_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	Element* grenadeBackground = _game->getRuleset()->getInterface("battlescape")->getElement("grenadeBackground");

/*	SDL_Rect square;
	square.x = 0;
	square.y = 0;
	square.w = _srfBG->getWidth();
	square.h = _srfBG->getHeight();
	_srfBG->drawRect(&square, Palette::blockOffset(6)+9); */
	add(_srfBG);
	_srfBG->drawRect(0, 0, _srfBG->getWidth(), _srfBG->getHeight(), grenadeBackground->color); //Palette::blockOffset(6)+9);

	add(_fraTop, "grenadeMenu", "battlescape");
//	_fraTop->setColor(Palette::blockOffset(6)+3);
//kL	_fraTop->setBackground(Palette::blockOffset(6)+12);
	_fraTop->setBackground(Palette::blockOffset(8)+4); // kL
	_fraTop->setThickness(3);
	_fraTop->setHighContrast();

	add(_txtTitle, "grenadeMenu", "battlescape");
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SET_TIMER"));
//	_txtTitle->setColor(Palette::blockOffset(1)-1);
	_txtTitle->setHighContrast();

	// kL_begin:
	add(_isfBtn0);
	_isfBtn0->onMouseClick((ActionHandler)& PrimeGrenadeState::btnClick);

/*	square.x = 0; // dark border
	square.y = 0;
	square.w = _isfBtn0->getWidth();
	square.h = _isfBtn0->getHeight();
	_isfBtn0->drawRect(&square, Palette::blockOffset(0)+15); */
	_isfBtn0->drawRect(0, 0, _isfBtn0->getWidth(), _isfBtn0->getHeight(), Palette::blockOffset(0)+15);

/*	square.x = 1; // inside fill
	square.y = 1;
	square.w = _isfBtn0->getWidth() - 2;
	square.h = _isfBtn0->getHeight() - 2;
	_isfBtn0->drawRect(&square, Palette::blockOffset(6)+12); */
	_isfBtn0->drawRect(1, 1, _isfBtn0->getWidth() - 2, _isfBtn0->getHeight() - 2, Palette::blockOffset(6)+12);

	add(_txtTurn0);
	std::wostringstream ss0;
	ss0 << 0;
	_txtTurn0->setText(ss0.str());
	_txtTurn0->setBig();
	_txtTurn0->setColor(Palette::blockOffset(1)-1);
	_txtTurn0->setHighContrast();
	_txtTurn0->setAlign(ALIGN_CENTER);
	_txtTurn0->setVerticalAlign(ALIGN_MIDDLE);
	// kL_end.

	for (int
			i = 0;
			i < 16;
			++i)
	{
		add(_isfBtn[i]);
		_isfBtn[i]->onMouseClick((ActionHandler)& PrimeGrenadeState::btnClick);

		SDL_Rect square;

		square.x = 0; // dark border
		square.y = 0;
		square.w = _isfBtn[i]->getWidth();
		square.h = _isfBtn[i]->getHeight();
		_isfBtn[i]->drawRect(&square, grenadeBackground->border); //Palette::blockOffset(0)+15);

		square.x++; // inside fill
		square.y++;
		square.w -= 2;
		square.h -= 2;
		_isfBtn[i]->drawRect(&square, grenadeBackground->color2); //Palette::blockOffset(6)+12);

		add(_txtTurn[i], "grenadeMenu", "battlescape");
		std::wostringstream ss;
		ss << i + 1;
		_txtTurn[i]->setText(ss.str());
//		_txtTurn[i]->setBig();
//		_txtTurn[i]->setColor(Palette::blockOffset(1)-1);
		_txtTurn[i]->setHighContrast();
		_txtTurn[i]->setAlign(ALIGN_CENTER);
		_txtTurn[i]->setVerticalAlign(ALIGN_MIDDLE);
	}

	centerAllSurfaces();
	lowerAllSurfaces();
}

/**
 * Deletes the Prime Grenade window object.
 */
PrimeGrenadeState::~PrimeGrenadeState()
{
}

/**
 * Closes the window on right-click.
 * @param action - pointer to an action
 */
void PrimeGrenadeState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONUP
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (!_inInventoryView)
			_action->value = -1;

		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action - pointer to an action
 */
void PrimeGrenadeState::btnClick(Action* action)
{
	int btnID = -1;

	if (action->getDetails()->type == SDL_MOUSEBUTTONUP
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (!_inInventoryView)
			_action->value = -1;

		_game->popState();

		return;
	}

	if (action->getSender() == _isfBtn0)
		btnID = 0;
	else
	{
		for (int // got to find out which button was pressed
				i = 0;
				i < 16
					&& btnID == -1;
				++i)
		{
			if (action->getSender() == _isfBtn[i])
				btnID = i + 1;
		}
	}

	if (btnID != -1)
	{
		if (_inInventoryView)
		{
			_grenade->setFuseTimer(btnID);
			_inventory->setPrimeGrenade(btnID); // kL
		}
		else
			_action->value = btnID;

		_game->popState(); // get rid of the Set Timer menu

		if (!_inInventoryView)
			_game->popState(); // get rid of the Action menu.
	}
}

}
