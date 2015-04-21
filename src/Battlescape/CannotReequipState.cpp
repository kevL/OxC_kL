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

#include "CannotReequipState.h"

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

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Cannot Reequip screen.
 * @param missingItems - vector of ReequipStat items still missing
 */
CannotReequipState::CannotReequipState(std::vector<ReequipStat> missingItems)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 69, 10, 9);

	_txtItem		= new Text(162, 9, 16, 77);
	_txtQuantity	= new Text(46, 9, 178, 77);
	_txtCraft		= new Text(80, 9, 224, 77);

	_lstItems		= new TextList(285, 89, 16, 87);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("cannotReequip");

	add(_window,		"window",	"cannotReequip");
	add(_txtTitle,		"heading",	"cannotReequip");
	add(_txtItem,		"text",		"cannotReequip");
	add(_txtQuantity,	"text",		"cannotReequip");
	add(_txtCraft,		"text",		"cannotReequip");
	add(_lstItems,		"list",		"cannotReequip");
	add(_btnOk,			"button",	"cannotReequip");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CannotReequipState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CannotReequipState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CannotReequipState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setText(tr("STR_NOT_ENOUGH_EQUIPMENT_TO_FULLY_RE_EQUIP_SQUAD"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtCraft->setText(tr("STR_CRAFT"));


	_lstItems->setColumns(3, 154, 46, 80);
	_lstItems->setBackground(_window);
	_lstItems->setSelectable();
	_lstItems->setMargin();

	for (std::vector<ReequipStat>::const_iterator
			i = missingItems.begin();
			i != missingItems.end();
			++i)
	{
		std::wostringstream ss;
		ss << i->qty;
		_lstItems->addRow(
						3,
						tr(i->item).c_str(),
						ss.str().c_str(),
						i->craft.c_str());
	}
}

/**
 * dTor.
 */
CannotReequipState::~CannotReequipState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CannotReequipState::btnOkClick(Action*)
{
	_game->popState();
}

}
