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
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
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
		size_t craftId)
	:
		_base(base),
		_craftId(craftId),
		_craft(base->getCrafts()->at(craftId))
{
	if (_game->getSavedGame()->getMonthsPassed() != -1)
		_window		= new Window(this, 320, 200, 0,0, POPUP_BOTH);
	else
		_window		= new Window(this, 320, 200);

	_edtCraft		= new TextEdit(this, 160, 16, 80, 10);
	_txtBaseLabel	= new Text(80, 9,  16, 10);
	_txtStatus		= new Text(80, 9, 224, 10);

	_txtFuel		= new Text(80, 17,  16, 28);
	_txtRadar		= new Text(80,  9, 120, 28);
	_txtDamage		= new Text(80, 17, 226, 28);

	_btnW1			= new TextButton(24, 32,  16, 48);
	_btnW2			= new TextButton(24, 32, 282, 48);
	_txtW1Name		= new Text(78,  9,  46, 48);
	_txtW2Name		= new Text(78,  9, 204, 48);
	_txtW1Ammo		= new Text(60, 25,  46, 64);
	_txtW2Ammo		= new Text(60, 25, 204, 64);

	_txtKills		= new Text(60, 9, 246, 83);

	_btnCrew		= new TextButton( 64, 16, 16,  96);
	_btnEquip		= new TextButton( 64, 16, 16, 120);
	_btnArmor		= new TextButton( 64, 16, 16, 144);
	_btnInventory	= new TextButton(220, 16, 84, 144);

	_sprite			= new Surface( 32, 38, 144,  50);
	_weapon1		= new Surface( 15, 17, 121,  63);
	_weapon2		= new Surface( 15, 17, 184,  63);
	_crew			= new Surface(220, 18,  85,  95);
	_equip			= new Surface(220, 18,  85, 119);

	_txtCost		= new Text(150, 9, 24, 165);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("craftInfo");

	add(_window,		"window",	"craftInfo");
	add(_edtCraft,		"text1",	"craftInfo");
	add(_txtBaseLabel,	"text1",	"craftInfo");
	add(_txtStatus,		"text2",	"craftInfo");
	add(_txtFuel,		"text2",	"craftInfo");
	add(_txtRadar,		"text2",	"craftInfo");
	add(_txtDamage,		"text2",	"craftInfo");
	add(_btnW1,			"button",	"craftInfo");
	add(_btnW2,			"button",	"craftInfo");
	add(_txtW1Name,		"text2",	"craftInfo");
	add(_txtW2Name,		"text2",	"craftInfo");
	add(_txtW1Ammo,		"text2",	"craftInfo");
	add(_txtW2Ammo,		"text2",	"craftInfo");
	add(_txtKills,		"text2",	"craftInfo");
	add(_btnCrew,		"button",	"craftInfo");
	add(_btnEquip,		"button",	"craftInfo");
	add(_btnArmor,		"button",	"craftInfo");
	add(_btnInventory,	"button",	"craftInfo");
	add(_sprite);
	add(_weapon1);
	add(_weapon2);
	add(_crew);
	add(_equip);
	add(_txtCost,		"text1",	"craftInfo");
	add(_btnOk,			"button",	"craftInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_edtCraft->setBig();
	_edtCraft->setAlign(ALIGN_CENTER);
	_edtCraft->onChange((ActionHandler)& CraftInfoState::edtCraftChange);

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));
	_txtStatus->setAlign(ALIGN_RIGHT);
	_txtRadar->setAlign(ALIGN_CENTER);
	_txtDamage->setAlign(ALIGN_RIGHT);

	_btnW1->setText(L"1");
	_btnW1->onMouseClick((ActionHandler)& CraftInfoState::btnW1Click);

	_btnW2->setText(L"2");
	_btnW2->onMouseClick((ActionHandler)& CraftInfoState::btnW2Click);

	if (_craft->getRules()->getWeapons() != 0)
	{
		_txtKills->setText(tr("STR_KILLS_LC_").arg(_craft->getKills()));
		_txtKills->setAlign(ALIGN_RIGHT);
	}

	_btnCrew->setText(tr("STR_CREW"));
	_btnCrew->onMouseClick((ActionHandler)& CraftInfoState::btnCrewClick);

	_btnEquip->setText(tr("STR_EQUIPMENT_UC"));
	_btnEquip->onMouseClick((ActionHandler)& CraftInfoState::btnEquipClick);

	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)& CraftInfoState::btnArmorClick);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick((ActionHandler)& CraftInfoState::btnInventoryClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftInfoState::btnOkClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
CraftInfoState::~CraftInfoState()
{}

/**
 * The craft's info can change after going into other screens.
 */
void CraftInfoState::init()
{
	State::init();

	// Reset stuff when coming back from pre-battle Inventory.
	if (_game->getSavedGame()->getSavedBattle() != NULL)
	{
		_game->getSavedGame()->setBattleGame(NULL);
		_craft->setInBattlescape(false);
	}


	const bool tacBattle = _game->getSavedGame()->getMonthsPassed() == -1;
	_btnInventory->setVisible(_craft->getNumSoldiers() > 0
							  && tacBattle == false);

	_edtCraft->setText(_craft->getName(_game->getLanguage()));
	if (tacBattle == true)
		_txtStatus->setText(L"");
	else
		_txtStatus->setText(tr(_craft->getStatus()));


	const RuleCraft* const crRule = _craft->getRules();

	int hours;
	std::wostringstream
		woststr1, // fuel
		woststr2; // hull

	if (tacBattle == true)
		_craft->setFuel(crRule->getMaxFuel()); // top up Craft for insta-Battle mode.

	woststr1 << tr("STR_FUEL").arg(Text::formatPercentage(_craft->getFuelPercentage()));
	if (crRule->getMaxFuel() - _craft->getFuel() > 0)
	{
		hours = static_cast<int>(std::ceil(
				static_cast<double>(crRule->getMaxFuel() - _craft->getFuel()) / static_cast<double>(crRule->getRefuelRate())
				/ 2.)); // refuel every half-hour.
		woststr1 << formatTime(
						hours,
						_craft->getWarning() == CW_CANTREFUEL);
	}
	_txtFuel->setText(woststr1.str());

	_txtRadar->setText(tr("STR_RADAR_RANGE")
						.arg(Text::formatNumber(crRule->getRadarRange())));

	woststr2 << tr("STR_HULL_").arg(Text::formatPercentage(100 - _craft->getDamagePercent()));
	if (_craft->getDamage() > 0)
	{
		hours = static_cast<int>(std::ceil(
				static_cast<double>(_craft->getDamage()) / static_cast<double>(crRule->getRepairRate())
				/ 2.)); // repair every half-hour.
		woststr2 << formatTime(
							hours,
							false); // ... unless item is required to repair Craft.
	}
	_txtDamage->setText(woststr2.str());


	SurfaceSet* const baseBits = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	const int sprite = crRule->getSprite() + 33;
	baseBits->getFrame(sprite)->setX(0); // BaseView::draw() changes x&y
	baseBits->getFrame(sprite)->setY(0);
	baseBits->getFrame(sprite)->blit(_sprite);

	Surface* bit;
	const int icon_width = 10;

	if (crRule->getSoldiers() > 0)
	{
		_crew->clear();
		_equip->clear();

		int x = 0;

		bit = baseBits->getFrame(38); // soldier graphic
		for (int
				i = 0;
				i != _craft->getNumSoldiers();
				++i,
					x += icon_width)
		{
			bit->setX(x);
			bit->blit(_crew);
		}

		bit = baseBits->getFrame(40); // vehicle graphic
		bit->setY(2);
		x = 0;
		for (int
				i = 0;
				i != _craft->getNumVehicles();
				++i,
					x += icon_width)
		{
			bit->setX(x);
			bit->blit(_equip);
		}

		bit = baseBits->getFrame(39); // equip't gra'hic
		bit->setY(0);

		const int
			totalIcons = ((_equip->getWidth() - x) + (icon_width - 1)) / icon_width,
			loadCap = _craft->getLoadCapacity() - (_craft->getSpaceUsed() * 10),
			qtyIcons = ((totalIcons * _craft->getNumEquipment()) + (loadCap - 1)) / loadCap;

		for (int
				i = 0;
				i != qtyIcons;
				++i,
					x += icon_width)
		{
			bit->setX(x);
			bit->blit(_equip);
		}

		calcCost();
	}
	else
	{
		_crew->setVisible(false);
		_equip->setVisible(false);
		_btnCrew->setVisible(false);
		_btnEquip->setVisible(false);
		_btnArmor->setVisible(false);
		_txtCost->setVisible(false);
	}


	const RuleCraftWeapon* cwRule;
	const CraftWeapon* cw;

	if (crRule->getWeapons() > 0)
	{
		cw = _craft->getWeapons()->at(0);
		if (cw != NULL)
		{
			cwRule = cw->getRules();

			bit = baseBits->getFrame(cwRule->getSprite() + 48);
//			bit->setX(0);
//			bit->setY(0);
			bit->blit(_weapon1);

			std::wostringstream woststr;

			woststr << L'\x01' << tr(cwRule->getType());
			_txtW1Name->setText(woststr.str());

			woststr.str(L"");

			woststr << tr("STR_AMMO_").arg(cw->getAmmo()) << L"\n\x01";
			woststr << tr("STR_MAX").arg(cwRule->getAmmoMax());
			if (cw->getAmmo() < cwRule->getAmmoMax())
			{
				hours = static_cast<int>(std::ceil( // rearm every half-hour.
						static_cast<double>(cwRule->getAmmoMax() - cw->getAmmo()) / static_cast<double>(cwRule->getRearmRate())
						/ 2.));
				woststr << formatTime(
									hours,
									cw->getCantLoad());
			}
			_txtW1Ammo->setText(woststr.str());
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

	if (crRule->getWeapons() > 1)
	{
		cw = _craft->getWeapons()->at(1);
		if (cw != NULL)
		{
			cwRule = cw->getRules();

			bit = baseBits->getFrame(cwRule->getSprite() + 48);
//			bit->setX(0);
//			bit->setY(0);
			bit->blit(_weapon2);

			std::wostringstream woststr;

			woststr << L'\x01' << tr(cwRule->getType());
			_txtW2Name->setText(woststr.str());

			woststr.str(L"");
			woststr << tr("STR_AMMO_").arg(cw->getAmmo()) << L"\n\x01";
			woststr << tr("STR_MAX").arg(cwRule->getAmmoMax());
			if (cw->getAmmo() < cwRule->getAmmoMax())
			{
				hours = static_cast<int>(std::ceil( // rearm every half-hour.
						static_cast<double>(cwRule->getAmmoMax() - cw->getAmmo()) / static_cast<double>(cwRule->getRearmRate())
						/ 2.));
				woststr << formatTime(
									hours,
									cw->getCantLoad());
			}
			_txtW2Ammo->setText(woststr.str());
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
	std::wostringstream woststr;
	const int
		dys = total / 24,
		hrs = total % 24;

	woststr << L"\n(";

	if (dys != 0)
	{
		woststr << tr("STR_DAY", dys);

		if (hrs != 0)
			woststr << L" ";
	}

	if (hrs != 0)
		woststr << tr("STR_HOUR", hrs);

	if (delayed == true)
		woststr << L" +";

	woststr << L")";

	return woststr.str();
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
										_craftId,
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
										_craftId,
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
										_craftId));
}

/**
 * Goes to the Select Equipment screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnEquipClick(Action*)
{
	_game->pushState(new CraftEquipmentState(
										_base,
										_craftId));
}

/**
 * Goes to the Select Armor screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnArmorClick(Action*)
{
	_game->pushState(new CraftArmorState(
									_base,
									_craftId));
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

	bgen.runInventory(_craft);

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

/**
 * Sets current cost to send the Craft on a mission.
 */
void CraftInfoState::calcCost() // private.
{
	const int cost = _base->calcSoldierBonuses(_craft)
				   + _craft->getRules()->getSoldiers() * 1000;
	_txtCost->setText(tr("STR_COST_").arg(Text::formatFunding(cost)));
}

}
