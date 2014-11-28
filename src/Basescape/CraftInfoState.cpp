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

#include "CraftInfoState.h"

//#include <sstream>

#include "CraftArmorState.h"
#include "CraftEquipmentState.h"
#include "CraftSoldiersState.h"
#include "CraftWeaponsState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the CraftInfo screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftInfoState::CraftInfoState(
		Base* base,
		size_t craftID)
	:
		_base(base),
		_craftID(craftID)
{
	if (_game->getSavedGame()->getMonthsPassed() != -1)
		_window		= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	else
		_window		= new Window(this, 320, 200, 0, 0, POPUP_NONE);

	_edtCraft		= new TextEdit(this, 160, 16, 80, 10);
	_txtBaseLabel	= new Text(80, 9, 16, 10);
	_txtStatus		= new Text(80, 9, 224, 10);

	_txtFuel		= new Text(82, 17, 16, 28);
	_txtDamage		= new Text(82, 17, 228, 28);

	_btnW1			= new TextButton(24, 32, 16, 48);
	_btnW2			= new TextButton(24, 32, 282, 48);
	_txtW1Name		= new Text(78, 9, 46, 48);
	_txtW2Name		= new Text(78, 9, 204, 48);
	_txtW1Ammo		= new Text(60, 25, 46, 64);
	_txtW2Ammo		= new Text(60, 25, 204, 64);

	_btnCrew		= new TextButton(64, 16, 16, 96);
	_btnEquip		= new TextButton(64, 16, 16, 120);
	_btnArmor		= new TextButton(64, 16, 16, 144);
	_btnInventory	= new TextButton(64, 16, 84, 144);

	_sprite			= new Surface(32, 38, 144, 50);
	_weapon1		= new Surface(15, 17, 121, 63);
	_weapon2		= new Surface(15, 17, 184, 63);
	_crew			= new Surface(220, 18, 85, 95);
	_equip			= new Surface(220, 18, 85, 119);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_BASESCAPE", 3);

	add(_window);
	add(_edtCraft);
	add(_txtBaseLabel);
	add(_txtStatus);
	add(_txtFuel);
	add(_txtDamage);
	add(_btnW1);
	add(_btnW2);
	add(_txtW1Name);
	add(_txtW2Name);
	add(_txtW1Ammo);
	add(_txtW2Ammo);
	add(_btnCrew);
	add(_btnEquip);
	add(_btnArmor);
	add(_btnInventory);
	add(_sprite);
	add(_weapon1);
	add(_weapon2);
	add(_crew);
	add(_equip);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_edtCraft->setColor(Palette::blockOffset(13)+10);
	_edtCraft->setBig();
	_edtCraft->setAlign(ALIGN_CENTER);
	_edtCraft->onChange((ActionHandler)& CraftInfoState::edtCraftChange);

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtStatus->setColor(Palette::blockOffset(13)+10);
	_txtStatus->setAlign(ALIGN_RIGHT);

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftInfoState::btnOkClick,
					Options::keyCancel);

	_btnW1->setColor(Palette::blockOffset(13)+10);
	_btnW1->setText(L"1");
	_btnW1->onMouseClick((ActionHandler)& CraftInfoState::btnW1Click);

	_btnW2->setColor(Palette::blockOffset(13)+10);
	_btnW2->setText(L"2");
	_btnW2->onMouseClick((ActionHandler)& CraftInfoState::btnW2Click);

	_btnCrew->setColor(Palette::blockOffset(13)+10);
	_btnCrew->setText(tr("STR_CREW"));
	_btnCrew->onMouseClick((ActionHandler)& CraftInfoState::btnCrewClick);

	_btnEquip->setColor(Palette::blockOffset(13)+10);
	_btnEquip->setText(tr("STR_EQUIPMENT_UC"));
	_btnEquip->onMouseClick((ActionHandler)& CraftInfoState::btnEquipClick);

	_btnArmor->setColor(Palette::blockOffset(13)+10);
	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)& CraftInfoState::btnArmorClick);

	_btnInventory->setColor(Palette::blockOffset(13)+10);
	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick((ActionHandler)& CraftInfoState::btnInventoryClick);

	_txtDamage->setColor(Palette::blockOffset(13)+10);
	_txtDamage->setSecondaryColor(Palette::blockOffset(13));

	_txtFuel->setColor(Palette::blockOffset(13)+10);
	_txtFuel->setSecondaryColor(Palette::blockOffset(13));

	_txtW1Name->setColor(Palette::blockOffset(13)+5);

	_txtW1Ammo->setColor(Palette::blockOffset(13)+10);
	_txtW1Ammo->setSecondaryColor(Palette::blockOffset(13)+5);

	_txtW2Name->setColor(Palette::blockOffset(13)+5);

	_txtW2Ammo->setColor(Palette::blockOffset(13)+10);
	_txtW2Ammo->setSecondaryColor(Palette::blockOffset(13)+5);
}

/**
 * dTor.
 */
CraftInfoState::~CraftInfoState()
{
}

/**
 * The craft info can change after going into other screens.
 */
void CraftInfoState::init()
{
	State::init();

	_craft = _base->getCrafts()->at(_craftID);

	const bool
		hasCrew = _craft->getNumSoldiers() > 0,
		newBattle = _game->getSavedGame()->getMonthsPassed() == -1;
	_btnInventory->setVisible(hasCrew && newBattle == false);

	// Reset stuff when coming back from soldier-inventory.
	_game->getSavedGame()->setBattleGame(NULL);
	_craft->setInBattlescape(false);

	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
	// Reset_end.


	_edtCraft->setText(_craft->getName(_game->getLanguage()));

//	std::string stat = _craft->getStatus();
	_txtStatus->setText(tr(_craft->getStatus()));

	SurfaceSet* const texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	texture->getFrame(_craft->getRules()->getSprite() + 33)->setX(0);
	texture->getFrame(_craft->getRules()->getSprite() + 33)->setY(0);
	texture->getFrame(_craft->getRules()->getSprite() + 33)->blit(_sprite);


	int hours = 0;
	std::wostringstream
		ss1, // hull
		ss2; // fuel

	ss1 << tr("STR_HULL_").arg(Text::formatPercentage(100 - _craft->getDamagePercent()));
//	if (stat == "STR_REPAIRS" &&
	if (_craft->getDamage() > 0)
	{
		hours = static_cast<int>(std::ceil(
				static_cast<double>(_craft->getDamage())
					/ static_cast<double>(_craft->getRules()->getRepairRate())
				/ 2.0)); // repair every half-hour.
		ss1 << formatTime(
						hours,
						false); // ... unless item is required to repair Craft.
	}
	_txtDamage->setText(ss1.str());

	ss2 << tr("STR_FUEL").arg(Text::formatPercentage(_craft->getFuelPercentage()));
//	if (stat == "STR_REFUELLING" &&
	if (_craft->getRules()->getMaxFuel() - _craft->getFuel() > 0)
	{
		hours = static_cast<int>(std::ceil(
				static_cast<double>(_craft->getRules()->getMaxFuel() - _craft->getFuel())
					/ static_cast<double>(_craft->getRules()->getRefuelRate())
				/ 2.0)); // refuel every half-hour.
		ss2 << formatTime(
						hours,
						_craft->getWarning() == CW_CANTREFUEL);
	}
	_txtFuel->setText(ss2.str());


	if (_craft->getRules()->getSoldiers() > 0)
	{
		_crew->clear();
		_equip->clear();

		Surface* const frame1 = texture->getFrame(38);
		frame1->setY(0);
		for (int
				i = 0,
					x = 0;
				i < _craft->getNumSoldiers();
				++i,
					x += 10)
		{
			frame1->setX(x);
			frame1->blit(_crew);
		}

		Surface* const frame2 = texture->getFrame(40);
		frame2->setY(0);
		int x = 0;
		for (int
				i = 0;
				i < _craft->getNumVehicles();
				++i,
					x += 10)
		{
			frame2->setX(x);
			frame2->blit(_equip);
		}

		Surface* const frame3 = texture->getFrame(39);
		for (int
				i = 0;
				i < _craft->getNumEquipment();
				i += 4,
					x += 10)
		{
			frame3->setX(x);
			frame3->blit(_equip);
		}
	}
	else
	{
		_crew->setVisible(false);
		_equip->setVisible(false);
		_btnCrew->setVisible(false);
		_btnEquip->setVisible(false);
		_btnArmor->setVisible(false);
	}

	if (_craft->getRules()->getWeapons() > 0)
	{
		const CraftWeapon* const cw1 = _craft->getWeapons()->at(0);
		if (cw1 != NULL)
		{
			Surface* frame = texture->getFrame(cw1->getRules()->getSprite() + 48);
			frame->setX(0);
			frame->setY(0);
			frame->blit(_weapon1);

			_txtW1Name->setText(tr(cw1->getRules()->getType()));

			std::wostringstream ss;
			ss << tr("STR_AMMO_").arg(cw1->getAmmo()) << L"\n\x01";
			ss << tr("STR_MAX").arg(cw1->getRules()->getAmmoMax());
//			if (stat == "STR_REARMING" &&
			if (cw1->getAmmo() < cw1->getRules()->getAmmoMax())
			{
				hours = static_cast<int>(std::ceil(
						static_cast<double>(cw1->getRules()->getAmmoMax() - cw1->getAmmo())
							/ static_cast<double>(cw1->getRules()->getRearmRate())
						/ 2.0)); // rearm every half-hour.
				ss << formatTime(
								hours,
								cw1->getCantLoad());
			}
			_txtW1Ammo->setText(ss.str());
		}
		else
		{
			_weapon1->clear();
			_txtW1Name->setText(L"");
			_txtW1Ammo->setText(L"");
		}
	}
	else
	{
		_weapon1->setVisible(false);
		_btnW1->setVisible(false);
		_txtW1Name->setVisible(false);
		_txtW1Ammo->setVisible(false);
	}

	if (_craft->getRules()->getWeapons() > 1)
	{
		const CraftWeapon* const cw2 = _craft->getWeapons()->at(1);
		if (cw2 != NULL)
		{
			Surface* frame = texture->getFrame(cw2->getRules()->getSprite() + 48);
			frame->setX(0);
			frame->setY(0);
			frame->blit(_weapon2);

			_txtW2Name->setText(tr(cw2->getRules()->getType()));

			std::wostringstream ss;
			ss << tr("STR_AMMO_").arg(cw2->getAmmo()) << L"\n\x01";
			ss << tr("STR_MAX").arg(cw2->getRules()->getAmmoMax());
//			if (stat == "STR_REARMING" &&
			if (cw2->getAmmo() < cw2->getRules()->getAmmoMax())
			{
				hours = static_cast<int>(std::ceil(
						static_cast<double>(cw2->getRules()->getAmmoMax() - cw2->getAmmo())
							/ static_cast<double>(cw2->getRules()->getRearmRate())
						/ 2.0)); // rearm every half-hour.
				ss << formatTime(
								hours,
								cw2->getCantLoad());
			}
			_txtW2Ammo->setText(ss.str());
		}
		else
		{
			_weapon2->clear();
			_txtW2Name->setText(L"");
			_txtW2Ammo->setText(L"");
		}
	}
	else
	{
		_weapon2->setVisible(false);
		_btnW2->setVisible(false);
		_txtW2Name->setVisible(false);
		_txtW2Ammo->setVisible(false);
	}
}

/**
 * Turns an amount of time in hours into a day/hour string.
 * @param total		- time in hours
 * @param delayed	- true to add '+' (unable to rearm due to lack of materiel)
 * @return, day/hour string
 */
std::wstring CraftInfoState::formatTime(
		const int total,
		const bool delayed) const
{
	std::wostringstream ss;
	const int
		days = total / 24,
		hours = total %24;

	ss << L"\n(";

	if (days > 0)
	{
		ss << tr("STR_DAY", days);

		if (hours > 0)
			ss << L" ";
	}

	if (hours > 0)
		ss << tr("STR_HOUR", hours);

	if (delayed == true)
		ss << L" +";

	ss << L")";

	return ss.str();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the Select Armament window for the first weapon.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnW1Click(Action*)
{
	_game->pushState(new CraftWeaponsState(
										_base,
										_craftID,
										0));
}

/**
 * Goes to the Select Armament window for the second weapon.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnW2Click(Action*)
{
	_game->pushState(new CraftWeaponsState(
										_base,
										_craftID,
										1));
}

/**
 * Goes to the Select Squad screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnCrewClick(Action*)
{
	_game->pushState(new CraftSoldiersState(
										_base,
										_craftID));
}

/**
 * Goes to the Select Equipment screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnEquipClick(Action*)
{
	_game->pushState(new CraftEquipmentState(
										_base,
										_craftID));
}

/**
 * Goes to the Select Armor screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnArmorClick(Action*)
{
	_game->pushState(new CraftArmorState(
									_base,
									_craftID));
}

/**
 * Goes to the soldier-inventory screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnInventoryClick(Action*)
{
	SavedBattleGame* const battle = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(battle);
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);

	Craft* const craft = _base->getCrafts()->at(_craftID);
	bgen.runInventory(craft);

	// Set system colors
	_game->getCursor()->setColor(Palette::blockOffset(9));
	_game->getFpsCounter()->setColor(Palette::blockOffset(9));

	_game->getScreen()->clear();

	_game->pushState(new InventoryState(
									false,
									NULL));
}

/**
 * Changes the Craft name.
 * @param action - pointer to an Action
 */
void CraftInfoState::edtCraftChange(Action* action)
{
	_craft->setName(_edtCraft->getText());

	if (action->getDetails()->key.keysym.sym == SDLK_RETURN
		|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		_edtCraft->setText(_craft->getName(_game->getLanguage()));
	}
}

}
