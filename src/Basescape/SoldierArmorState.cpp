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

#include "SoldierArmorState.h"

#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Armor window.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierArmorState::SoldierArmorState(
		Base* base,
		size_t soldier)
	:
		_base(base),
		_soldier(soldier)
{
	_screen = false;

	_window			= new Window(this, 192, 120, 64, 40, POPUP_BOTH);

//kL	_txtTitle		= new Text(182, 9, 69, 48);
	_txtSoldier		= new Text(182, 9, 69, 55);

	_txtType		= new Text(102, 9, 84, 72);
	_txtQuantity	= new Text(42, 9, 194, 72);

	_lstArmor		= new TextList(168, 40, 76, 88);

	_btnCancel		= new TextButton(152, 16, 84, 136);

	setPalette("PAL_BASESCAPE", 4);

	add(_window);
//kL	add(_txtTitle);
	add(_txtSoldier);
	add(_txtType);
	add(_txtQuantity);
	add(_lstArmor);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnCancel->setColor(Palette::blockOffset(13)+5);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& SoldierArmorState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SoldierArmorState::btnCancelClick,
					Options::keyCancel);

	Soldier* s = _base->getSoldiers()->at(_soldier);
/*kL
	_txtTitle->setColor(Palette::blockOffset(13)+5);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_ARMOR_FOR_SOLDIER").arg(s->getName())); */

	_txtSoldier->setColor(Palette::blockOffset(13)+5);
	_txtSoldier->setAlign(ALIGN_CENTER);
	_txtSoldier->setText(s->getName(true));

	_txtType->setColor(Palette::blockOffset(13)+5);
	_txtType->setText(tr("STR_TYPE"));

	_txtQuantity->setColor(Palette::blockOffset(13)+5);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstArmor->setColor(Palette::blockOffset(13));
	_lstArmor->setArrowColor(Palette::blockOffset(13)+5);
	_lstArmor->setColumns(2, 110, 42);
	_lstArmor->setSelectable(true);
	_lstArmor->setBackground(_window);
	_lstArmor->setMargin(8);

	const std::vector<std::string>& armors = _game->getRuleset()->getArmorsList();
	for (std::vector<std::string>::const_iterator
			i = armors.begin();
			i != armors.end();
			++i)
	{
		Armor* armor = _game->getRuleset()->getArmor(*i);
		if (_base->getItems()->getItem(armor->getStoreItem()) > 0)
		{
			_armors.push_back(armor);

			std::wostringstream ss;
			if (_game->getSavedGame()->getMonthsPassed() > -1)
				ss << _base->getItems()->getItem(armor->getStoreItem());
			else
				ss << "-";

			_lstArmor->addRow(
							2,
							tr(armor->getType()).c_str(),
							ss.str().c_str());
		}
		else if (armor->getStoreItem() == "STR_NONE")
		{
			_armors.push_back(armor);

			_lstArmor->addRow(
							1,
							tr(armor->getType()).c_str());
		}
	}

	_lstArmor->onMouseClick((ActionHandler)& SoldierArmorState::lstArmorClick);
}

/**
 * dTor.
 */
SoldierArmorState::~SoldierArmorState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierArmorState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Equips the armor on the soldier and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierArmorState::lstArmorClick(Action*)
{
	Soldier* soldier = _base->getSoldiers()->at(_soldier);
	if (_game->getSavedGame()->getMonthsPassed() != -1)
	{
		if (soldier->getArmor()->getStoreItem() != "STR_NONE")
			_base->getItems()->addItem(soldier->getArmor()->getStoreItem());

		if (_armors[_lstArmor->getSelectedRow()]->getStoreItem() != "STR_NONE")
			_base->getItems()->removeItem(_armors[_lstArmor->getSelectedRow()]->getStoreItem());
	}

	soldier->setArmor(_armors[_lstArmor->getSelectedRow()]);
//	SavedGame* save;
//	save = _game->getSavedGame();
//	save->setLastSelectedArmor(_armors[_lstArmor->getSelectedRow()]->getType());

	_game->popState();
}

}
