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

//#include <algorithm>
//#include <sstream>

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

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
		Base* const base,
		size_t soldierId)
	:
		_base(base),
		_soldier(base->getSoldiers()->at(soldierId))
{
	_screen = false;

	_window			= new Window(this, 192, 157, 64, 17, POPUP_BOTH);

	_txtSoldier		= new Text(182, 9, 69, 28);

	_txtType		= new Text(102, 9,  84, 43);
	_txtQuantity	= new Text( 42, 9, 194, 43);

	_lstArmor		= new TextList(160, 89, 76, 54);

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
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SoldierArmorState::btnCancelClick,
					Options::keyOk);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SoldierArmorState::btnCancelClick,
					Options::keyOkKeypad);

	_txtSoldier->setText(_soldier->getName());
	_txtSoldier->setAlign(ALIGN_CENTER);

	_txtType->setText(tr("STR_TYPE"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstArmor->setColumns(2, 110,35);
	_lstArmor->setBackground(_window);
	_lstArmor->setSelectable();

	RuleArmor* armorRule;
	const std::vector<std::string>& armorList = _game->getRuleset()->getArmorsList();
	for (std::vector<std::string>::const_reverse_iterator
			i = armorList.rbegin();
			i != armorList.rend();
			++i)
	{
		armorRule = _game->getRuleset()->getArmor(*i);
		if (armorRule->getUnits().empty() == true
			|| std::find(
					armorRule->getUnits().begin(),
					armorRule->getUnits().end(),
					_soldier->getRules()->getType()) != armorRule->getUnits().end())
		{
			if (_base->getStorageItems()->getItemQty(armorRule->getStoreItem()) != 0)
			{
				_armors.push_back(armorRule);

				std::wostringstream woststr;
				if (_game->getSavedGame()->getMonthsPassed() != -1)
					woststr << _base->getStorageItems()->getItemQty(armorRule->getStoreItem());
				else
					woststr << L"-";

				_lstArmor->addRow(
								2,
								tr(armorRule->getType()).c_str(),
								woststr.str().c_str());
			}
			else if (armorRule->getStoreItem() == RuleArmor::NONE)
			{
				_armors.push_back(armorRule);
				_lstArmor->addRow(1, tr(armorRule->getType()).c_str());
			}
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
	if (_game->getSavedGame()->getMonthsPassed() != -1)
	{
		if (_soldier->getArmor()->getStoreItem() != RuleArmor::NONE)
			_base->getStorageItems()->addItem(_soldier->getArmor()->getStoreItem());

		if (_armors[_lstArmor->getSelectedRow()]->getStoreItem() != RuleArmor::NONE)
			_base->getStorageItems()->removeItem(_armors[_lstArmor->getSelectedRow()]->getStoreItem());
	}

	_soldier->setArmor(_armors[_lstArmor->getSelectedRow()]);
//	SavedGame* gameSave = _game->getSavedGame();
//	gameSave->setLastSelectedArmor(_armors[_lstArmor->getSelectedRow()]->getType());

	_game->popState();
}

}
