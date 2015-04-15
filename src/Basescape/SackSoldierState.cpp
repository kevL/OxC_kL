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

#include "SackSoldierState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a Sack Soldier window.
 * @param base 		- pointer to the Base to get info from
 * @param soldierId	- pointer to the soldier to sack
 */
SackSoldierState::SackSoldierState(
		Base* base,
		size_t soldierId)
	:
		_base(base),
		_soldierId(soldierId)
{
	_screen = false;

	_window		= new Window(this, 152, 80, 84, 60);
	_txtTitle	= new Text(142, 9, 89, 75);
	_txtSoldier	= new Text(142, 9, 89, 85);
	_btnCancel	= new TextButton(44, 16, 100, 115);
	_btnOk		= new TextButton(44, 16, 176, 115);

	std::string pal = "PAL_BASESCAPE";
	Uint8 color = 6; // oxide by default in ufo palette
	const Element* const element = _game->getRuleset()->getInterface("sackSoldier")->getElement("palette");
	if (element != NULL)
	{
		if (element->TFTDMode == true)
			pal = "PAL_GEOSCAPE";

		if (element->color != std::numeric_limits<int>::max())
			color = static_cast<Uint8>(element->color);
	}
	setPalette(pal, color);

	add(_window,		"window",	"sackSoldier");
	add(_txtTitle,		"text",		"sackSoldier");
	add(_txtSoldier,	"text",		"sackSoldier");
	add(_btnCancel,		"button",	"sackSoldier");
	add(_btnOk,			"button",	"sackSoldier");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SackSoldierState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SackSoldierState::btnOkClick,
					Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& SackSoldierState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SackSoldierState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SACK"));

	_txtSoldier->setAlign(ALIGN_CENTER);
	_txtSoldier->setText(_base->getSoldiers()->at(_soldierId)->getName());
}

/**
 * dTor.
 */
SackSoldierState::~SackSoldierState()
{}

/**
 * Sacks the soldier and returns to the previous screen.
 * @param action - pointer to an Action
 */
void SackSoldierState::btnOkClick(Action*)
{
	const Soldier* const soldier = _base->getSoldiers()->at(_soldierId);
	if (soldier->getArmor()->getStoreItem() != "STR_NONE")
		_base->getItems()->addItem(soldier->getArmor()->getStoreItem());

	delete soldier;
	_base->getSoldiers()->erase(_base->getSoldiers()->begin() + _soldierId);

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SackSoldierState::btnCancelClick(Action*)
{
	_game->popState();
}

}
