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

#include "CraftWeaponsState.h"

#include <cmath>
#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Weapons window.
 * @param base		- pointer to the Base to get info from
 * @param craft		- ID of the selected craft
 * @param weapon	- ID of the selected weapon
 */
CraftWeaponsState::CraftWeaponsState(
		Base* base,
		size_t craftID,
		size_t weaponID)
	:
		_base(base),
		_craftID(craftID),
		_weaponID(weaponID)
{
	_screen = false;

	_window			= new Window(this, 220, 160, 50, 20, POPUP_BOTH);

	_txtTitle		= new Text(200, 17, 60, 32);

	_txtArmament	= new Text(98, 9, 66, 53);
	_txtQuantity	= new Text(30, 9, 164, 53);
	_txtAmmunition	= new Text(50, 9, 194, 53);

	_lstWeapons		= new TextList(204, 89, 58, 68);

	_btnCancel		= new TextButton(140, 16, 90, 156);

	setPalette("PAL_BASESCAPE", 4);

	add(_window);
	add(_txtTitle);
	add(_txtArmament);
	add(_txtQuantity);
	add(_txtAmmunition);
	add(_lstWeapons);
	add(_btnCancel);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+6);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnCancel->setColor(Palette::blockOffset(15)+6);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& CraftWeaponsState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& CraftWeaponsState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(15)+6);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_ARMAMENT"));

	_txtArmament->setColor(Palette::blockOffset(15)+6);
	_txtArmament->setText(tr("STR_ARMAMENT"));

	_txtQuantity->setColor(Palette::blockOffset(15)+6);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtAmmunition->setColor(Palette::blockOffset(15)+6);
	_txtAmmunition->setText(tr("STR_AMMUNITION_AVAILABLE"));
	_txtAmmunition->setWordWrap();

	_lstWeapons->setColor(Palette::blockOffset(13)+10);
	_lstWeapons->setArrowColor(Palette::blockOffset(15)+6);
	_lstWeapons->setColumns(3, 98, 30, 50);
	_lstWeapons->setSelectable();
	_lstWeapons->setBackground(_window);
	_lstWeapons->setMargin();
	_lstWeapons->addRow(
						1,
						tr("STR_NONE_UC").c_str());
	_weaponRules.push_back(NULL);

	const std::vector<std::string>& weapons = _game->getRuleset()->getCraftWeaponsList();
	for (std::vector<std::string>::const_iterator
			i = weapons.begin();
			i != weapons.end();
			++i)
	{
		RuleCraftWeapon* const cwRule = _game->getRuleset()->getCraftWeapon(*i);
		const RuleItem* const laRule = _game->getRuleset()->getItem(cwRule->getLauncherItem());
		//Log(LOG_INFO) << ". laRule = " << laRule->getType();

//kL	if (_base->getItems()->getItem(cwRule->getLauncherItem()) > 0)
		if (_game->getSavedGame()->isResearched(laRule->getRequirements()) == true	// requirements have been researched
			|| laRule->getRequirements().empty() == true)								// or, does not need to be researched
		{
			//Log(LOG_INFO) << ". . add craft weapon";
			_weaponRules.push_back(cwRule);

			std::wostringstream
				ss,
				ss2;

			if (_base->getItems()->getItem(cwRule->getLauncherItem()) > 0)
				ss << _base->getItems()->getItem(cwRule->getLauncherItem());
			else
				ss << L"-";

			if (cwRule->getClipItem().empty() == false)
				ss2 << _base->getItems()->getItem(cwRule->getClipItem());
			else
				ss2 << tr("STR_NOT_AVAILABLE");

			_lstWeapons->addRow(
								3,
								tr(cwRule->getType()).c_str(),
								ss.str().c_str(),
								ss2.str().c_str());
		}
	}

	_lstWeapons->onMouseClick((ActionHandler)& CraftWeaponsState::lstWeaponsClick);
}

/**
 * dTor.
 */
CraftWeaponsState::~CraftWeaponsState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftWeaponsState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Equips the weapon on the craft and returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftWeaponsState::lstWeaponsClick(Action*)
{
	bool closeState = false;

	RuleCraftWeapon* const cwRule = _weaponRules[_lstWeapons->getSelectedRow()];
	CraftWeapon* const cw = _base->getCrafts()->at(_craftID)->getWeapons()->at(_weaponID);

	if (cwRule == NULL
		&& cw != NULL)
	{
		closeState = true;

		_base->getItems()->addItem(cw->getRules()->getLauncherItem());
		_base->getItems()->addItem(
								cw->getRules()->getClipItem(),
								cw->getClipsLoaded(_game->getRuleset()));

		delete cw;
		_base->getCrafts()->at(_craftID)->getWeapons()->at(_weaponID) = NULL;
	}

	if (cwRule != NULL
		&& (cw == NULL
			|| cw->getRules() != cwRule)
		&& _base->getItems()->getItem(cwRule->getLauncherItem()) > 0)
	{
		closeState = true;

		if (cw != NULL)
		{
			_base->getItems()->addItem(cw->getRules()->getLauncherItem());
			_base->getItems()->addItem(
									cw->getRules()->getClipItem(),
									cw->getClipsLoaded(_game->getRuleset()));

			delete cw;
			_base->getCrafts()->at(_craftID)->getWeapons()->at(_weaponID) = NULL;
		}

		_base->getItems()->removeItem(cwRule->getLauncherItem());

		CraftWeapon* const sel = new CraftWeapon(
												cwRule,
												0);
		sel->setRearming(true);
		_base->getCrafts()->at(_craftID)->getWeapons()->at(_weaponID) = sel;

		_base->getCrafts()->at(_craftID)->checkup(); // kL
//		if (_base->getCrafts()->at(_craftID)->getStatus() == "STR_READY")
//			_base->getCrafts()->at(_craftID)->setStatus("STR_REARMING");
	}

	if (closeState == true)
		_game->popState();
}

}
