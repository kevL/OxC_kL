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

#include "SoldierArmorState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
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
 * Initializes all the elements in the Soldier Armor window.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected Soldier
 */
SoldierArmorState::SoldierArmorState(
		Base* base,
		size_t soldierId)
	:
		_base(base),
		_soldierId(soldierId)
{
	_screen = false;

	_window			= new Window(this, 192, 147, 64, 27, POPUP_BOTH);

	_txtSoldier		= new Text(182, 9, 69, 38);

	_txtType		= new Text(102, 9, 84, 53);
	_txtQuantity	= new Text(42, 9, 194, 53);

	_lstArmor		= new TextList(160, 81, 76, 64);

	_btnCancel		= new TextButton(152, 16, 84, 150);

	setInterface("soldierArmor");

	add(_window,		"window",	"soldierArmor");
	add(_txtSoldier,	"text",		"soldierArmor");
	add(_txtType,		"text",		"soldierArmor");
	add(_txtQuantity,	"text",		"soldierArmor");
	add(_lstArmor,		"list",		"soldierArmor");
	add(_btnCancel,		"button",	"soldierArmor");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& SoldierArmorState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SoldierArmorState::btnCancelClick,
					Options::keyCancel);


	_txtSoldier->setAlign(ALIGN_CENTER);
	_txtSoldier->setText(_base->getSoldiers()->at(_soldierId)->getName());

	_txtType->setText(tr("STR_TYPE"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstArmor->setBackground(_window);
	_lstArmor->setColumns(2, 110, 35);
	_lstArmor->setSelectable();
	_lstArmor->setMargin();

	RuleArmor* armorRule;
	const std::vector<std::string>& armorList = _game->getRuleset()->getArmorsList();
	for (std::vector<std::string>::const_iterator
			i = armorList.begin();
			i != armorList.end();
			++i)
	{
		armorRule = _game->getRuleset()->getArmor(*i);
		if (_base->getItems()->getItem(armorRule->getStoreItem()) > 0)
		{
			_armors.push_back(armorRule);

			std::wostringstream woststr;
			if (_game->getSavedGame()->getMonthsPassed() != -1)
				woststr << _base->getItems()->getItem(armorRule->getStoreItem());
			else
				woststr << L"-";

			_lstArmor->addRow(
							2,
							tr(armorRule->getType()).c_str(),
							woststr.str().c_str());
		}
		else if (armorRule->getStoreItem() == "STR_NONE")
		{
			_armors.push_back(armorRule);
			_lstArmor->addRow(
							1,
							tr(armorRule->getType()).c_str());
		}
	}

	_lstArmor->onMouseClick((ActionHandler)& SoldierArmorState::lstArmorClick);
}

/**
 * dTor.
 */
SoldierArmorState::~SoldierArmorState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierArmorState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Equips the armor on the soldier and returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierArmorState::lstArmorClick(Action*)
{
	Soldier* const soldier = _base->getSoldiers()->at(_soldierId);
	if (_game->getSavedGame()->getMonthsPassed() != -1)
	{
		if (soldier->getArmor()->getStoreItem() != "STR_NONE")
			_base->getItems()->addItem(soldier->getArmor()->getStoreItem());

		if (_armors[_lstArmor->getSelectedRow()]->getStoreItem() != "STR_NONE")
			_base->getItems()->removeItem(_armors[_lstArmor->getSelectedRow()]->getStoreItem());
	}

	soldier->setArmor(_armors[_lstArmor->getSelectedRow()]);
//	SavedGame* gameSave = _game->getSavedGame();
//	gameSave->setLastSelectedArmor(_armors[_lstArmor->getSelectedRow()]->getType());

	_game->popState();
}

}
