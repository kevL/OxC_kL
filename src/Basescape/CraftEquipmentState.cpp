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

#include "CraftEquipmentState.h"

//#include <algorithm>
//#include <climits>
//#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Equipment screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftEquipmentState::CraftEquipmentState(
		Base* base,
		size_t craftId)
	:
		_base(base),
		_craftId(craftId),
		_sel(0),
		_selUnit(0)
{
	Craft* const craft = _base->getCrafts()->at(_craftId);

	const bool
		hasCrew = craft->getNumSoldiers() > 0,
		newBattle = _game->getSavedGame()->getMonthsPassed() == -1;

	_window			= new Window(this, 320, 200);

	_txtCost		= new Text(150, 9, 24, -10);

	_txtTitle		= new Text(300, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtSpace		= new Text(110, 9, 16, 26);
	_txtLoad		= new Text(110, 9, 171, 26);

	_txtItem		= new Text(144, 9, 16, 36);
	_txtStores		= new Text(50, 9, 171, 36);
	_txtCraft		= new Text(50, 9, 256, 36);

	_lstEquipment	= new TextList(285, 129, 16, 45);

	_btnClear		= new TextButton(94, 16, 16, 177);
	_btnInventory	= new TextButton(94, 16, 113, 177);
	_btnOk			= new TextButton(94, 16, 210, 177);

	setInterface("craftEquipment");

	_ammoColor = static_cast<Uint8>(_game->getRuleset()->getInterface("craftEquipment")->getElement("ammoColor")->color);

	add(_window,		"window",	"craftEquipment");
	add(_txtCost,		"text",		"craftEquipment");
	add(_txtTitle,		"text",		"craftEquipment");
	add(_txtBaseLabel,	"text",		"craftEquipment");
	add(_txtSpace,		"text",		"craftEquipment");
	add(_txtLoad,		"text",		"craftEquipment");
	add(_txtItem,		"text",		"craftEquipment");
	add(_txtStores,		"text",		"craftEquipment");
	add(_txtCraft,		"text",		"craftEquipment");
	add(_lstEquipment,	"list",		"craftEquipment");
	add(_btnClear,		"button",	"craftEquipment");
	add(_btnInventory,	"button",	"craftEquipment");
	add(_btnOk,			"button",	"craftEquipment");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick((ActionHandler)& CraftEquipmentState::btnClearClick);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick((ActionHandler)& CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(hasCrew
							  && newBattle == false);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(craft->getName(_game->getLanguage())));
	_txtTitle->setBig();

	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setText(tr("STR_ITEM"));
	_txtStores->setText(tr("STR_STORES"));
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
					.arg(craft->getNumSoldiers())
					.arg(craft->getNumVehicles())
					.arg(craft->getSpaceAvailable()));

	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
					.arg(craft->getLoadCapacity())
					.arg(craft->getLoadCapacity() - craft->getLoadCurrent()));

	_lstEquipment->setArrowColumn(189, ARROW_HORIZONTAL);
	_lstEquipment->setColumns(3, 147, 85, 41);
	_lstEquipment->setBackground(_window);
	_lstEquipment->setSelectable();
	_lstEquipment->setMargin();
//	_lstEquipment->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstEquipment->onMousePress((ActionHandler)& CraftEquipmentState::lstEquipmentMousePress);
	_lstEquipment->onLeftArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowPress);
	_lstEquipment->onLeftArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowRelease);
	_lstEquipment->onLeftArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowClick);
	_lstEquipment->onRightArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowPress);
	_lstEquipment->onRightArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowRelease);
	_lstEquipment->onRightArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowClick);


	size_t row = 0;
	std::wostringstream
		woststr1,
		woststr2;
	std::wstring wst;
	int
		craftQty,
		clipSize;
	Uint8 color;

	const std::vector<std::string>& itemList = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		const RuleItem* const itRule = _game->getRuleset()->getItem(*i);

		craftQty = 0;
		if (itRule->isFixed() == true)
			craftQty = craft->getVehicleCount(*i);
		else
			craftQty = craft->getItems()->getItemQty(*i);

		if (itRule->getBigSprite() > -1
			&& itRule->getBattleType() != BT_NONE
			&& itRule->getBattleType() != BT_CORPSE
			&& _game->getSavedGame()->isResearched(itRule->getRequirements()) == true
			&& (_base->getItems()->getItemQty(*i) > 0
				|| craftQty > 0))
		{
			_items.push_back(*i);

			woststr1.str(L"");
			woststr2.str(L"");

			if (_game->getSavedGame()->getMonthsPassed() > -1)
				woststr1 << _base->getItems()->getItemQty(*i);
			else
				woststr1 << "-";

			woststr2 << craftQty;

			wst = tr(*i);
			if (itRule->getBattleType() == BT_AMMO) // weapon clips
			{
				clipSize = itRule->getClipSize();
				if (clipSize > 1)
					wst += (L" (" + Text::formatNumber(clipSize) + L")");

				wst.insert(0, L"  ");
			}
			else if (itRule->isFixed() == true // tank w/ Ordnance.
				&& itRule->getCompatibleAmmo()->empty() == false)
			{
				const RuleItem* const amRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
				clipSize = amRule->getClipSize();
				if (clipSize > 0)
					wst += (L" (" + Text::formatNumber(clipSize) + L")");
			}

			_lstEquipment->addRow(
								3,
								wst.c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());

			if (craftQty == 0)
			{
				if (itRule->getBattleType() == BT_AMMO)
					color = _ammoColor;
				else
					color = _lstEquipment->getColor();
			}
			else
				color = _lstEquipment->getSecondaryColor();

			_lstEquipment->setRowColor(
									row++,
									color);
		}
	}

	_timerLeft = new Timer(250);
	_timerLeft->onTimer((StateHandler)& CraftEquipmentState::moveLeft);

	_timerRight = new Timer(250);
	_timerRight->onTimer((StateHandler)& CraftEquipmentState::moveRight);
}

/**
 * dTor.
 */
CraftEquipmentState::~CraftEquipmentState()
{
	delete _timerLeft;
	delete _timerRight;
}

/**
 * Resets the stuff when coming back from soldiers' inventory state.
 */
void CraftEquipmentState::init()
{
	State::init();

	calcCost();

	// Reset stuff when coming back from pre-battle Inventory.
	const SavedBattleGame* const battleSave = _game->getSavedGame()->getSavedBattle();
	if (battleSave != NULL)
	{
		_selUnit = battleSave->getSelectedUnit()->getBattleOrder();

		_game->getSavedGame()->setBattleGame(NULL);
		_base->getCrafts()->at(_craftId)->setInBattlescape(false);
	}
}

/**
 * Runs the arrow timers.
 */
void CraftEquipmentState::think()
{
	State::think();

	_timerLeft->think(this, NULL);
	_timerRight->think(this, NULL);
}


/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Starts moving the item to the base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentLeftArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerLeft->isRunning() == false)
	{
		_timerLeft->start();
	}
}

/**
 * Stops moving the item to the base.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLeft->stop();
}

/**
 * Moves all the items to the base on right-click.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValue(1);

		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moveLeftByValue(std::numeric_limits<int>::max());
}

/**
 * Starts moving the item to the craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentRightArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerRight->isRunning() == false)
	{
		_timerRight->start();
	}
}

/**
 * Stops moving the item to the craft.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerRight->stop();
}

/**
 * Moves all the items (as much as possible) to the craft on right-click.
 * @param action - pointer to an Action
 */
void CraftEquipmentState::lstEquipmentRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValue(1);

		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moveRightByValue(std::numeric_limits<int>::max());
}

/**
 * Updates the displayed quantities of the selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	const RuleItem* const itemRule = _game->getRuleset()->getItem(_items[_sel]);
	Craft* const craft = _base->getCrafts()->at(_craftId);

	int craftQty = 0;
	if (itemRule->isFixed() == true)
		craftQty = craft->getVehicleCount(_items[_sel]);
	else
		craftQty = craft->getItems()->getItemQty(_items[_sel]);

	std::wostringstream
		woststr1,
		woststr2;

	if (_game->getSavedGame()->getMonthsPassed() > -1)
		woststr1 << _base->getItems()->getItemQty(_items[_sel]);
	else
		woststr1 << "-";

	woststr2 << craftQty;

	Uint8 color;
	if (craftQty == 0)
	{
		if (itemRule->getBattleType() == BT_AMMO)
			color = _ammoColor;
		else
			color = _lstEquipment->getColor();
	}
	else
		color = _lstEquipment->getSecondaryColor();

	_lstEquipment->setRowColor(_sel, color);
	_lstEquipment->setCellText(_sel, 1, woststr1.str());
	_lstEquipment->setCellText(_sel, 2, woststr2.str());

	_txtSpace->setText(tr("STR_SPACE_CREW_HWP_FREE_")
					.arg(craft->getNumSoldiers())
					.arg(craft->getNumVehicles())
					.arg(craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
					.arg(craft->getLoadCapacity())
					.arg(craft->getLoadCapacity() - craft->getLoadCurrent()));

	calcCost();
}

/**
 * Moves the selected item to the base.
 */
void CraftEquipmentState::moveLeft()
{
	_timerLeft->setInterval(80);
	_timerRight->setInterval(80);

	moveLeftByValue(1);
}

/**
 * Moves the given number of items (selected) to the base.
 * @param change - quantity change
 */
void CraftEquipmentState::moveLeftByValue(int change)
{
	if (change > 0)
	{
		const RuleItem* const itRule = _game->getRuleset()->getItem(_items[_sel]);
		Craft* const craft = _base->getCrafts()->at(_craftId);

		int craftQty;
		if (itRule->isFixed() == true)
			craftQty = craft->getVehicleCount(_items[_sel]);
		else
			craftQty = craft->getItems()->getItemQty(_items[_sel]);

		if (craftQty == 0)
			return;


		change = std::min(
						change,
						craftQty);

		if (itRule->isFixed() == true) // convert vehicle to item
		{
			if (itRule->getCompatibleAmmo()->empty() == false)
			{
				// first remove all vehicles to redistribute the ammo
				const RuleItem* const ammoRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());

				for (std::vector<Vehicle*>::const_iterator
						i = craft->getVehicles()->begin();
						i != craft->getVehicles()->end();
						)
				{
					if ((*i)->getRules() == itRule)
					{
						if (_game->getSavedGame()->getMonthsPassed() != -1) // kL_add <-
							_base->getItems()->addItem(
													ammoRule->getType(),
													(*i)->getAmmo());

						delete *i;
						i = craft->getVehicles()->erase(i);
					}
					else
						++i;
				}

				if (_game->getSavedGame()->getMonthsPassed() != -1)
					_base->getItems()->addItem(
											_items[_sel],
											craftQty);

				// now reAdd the count to keep in the craft and distribute the ammo among them
				if (craftQty > change)
					moveRightByValue(craftQty - change);
			}
			else
			{
				if (_game->getSavedGame()->getMonthsPassed() != -1)
					_base->getItems()->addItem(
											_items[_sel],
											change);

				for (std::vector<Vehicle*>::const_iterator
						i = craft->getVehicles()->begin();
						i != craft->getVehicles()->end();
						)
				{
					if ((*i)->getRules() == itRule)
					{
						delete *i;
						i = craft->getVehicles()->erase(i);

						if (--change < 1)
							break;
					}
					else
						++i;
				}
			}
		}
		else
		{
			craft->getItems()->removeItem(
										_items[_sel],
										change);

			if (_game->getSavedGame()->getMonthsPassed() != -1)
				_base->getItems()->addItem(
										_items[_sel],
										change);
		}

		updateQuantity();
	}
}

/**
 * Moves the selected item to the craft.
 */
void CraftEquipmentState::moveRight()
{
	_timerLeft->setInterval(80);
	_timerRight->setInterval(80);

	moveRightByValue(1);
}

/**
 * Moves the given number of items (selected) to the craft.
 * @param change - quantity change
 */
void CraftEquipmentState::moveRightByValue(int change)
{
	if (change > 0)
	{
		int baseQty;
		if (_game->getSavedGame()->getMonthsPassed() == -1)
		{
			if (change == std::numeric_limits<int>::max())
				change = 10;

			baseQty = change;
		}
		else
			baseQty = _base->getItems()->getItemQty(_items[_sel]);

		if (baseQty < 1)
			return;


		change = std::min(
						change,
						baseQty);

		RuleItem* const itRule = _game->getRuleset()->getItem(_items[_sel]);
		Craft* const craft = _base->getCrafts()->at(_craftId);

		if (itRule->isFixed() == true) // load vehicle, convert item to a vehicle
		{
			int unitSize = _game->getRuleset()->getArmor(
												_game->getRuleset()->getUnit(itRule->getType())->getArmor())
											->getSize();
			unitSize *= unitSize;

			const int spaceAvailable = std::min(
											craft->getRules()->getVehicles() - craft->getNumVehicles(true),
											craft->getSpaceAvailable())
										/ unitSize;

			if (spaceAvailable > 0
				&& craft->getLoadCapacity() - craft->getLoadCurrent() >= unitSize * 10) // note: 10 is the 'load' that a single 'space' uses.
			{
				change = std::min(
								change,
								spaceAvailable);
				change = std::min(
								change,
								(craft->getLoadCapacity() - craft->getLoadCurrent()) / (unitSize * 10));

				if (itRule->getCompatibleAmmo()->empty() == false)
				{
					const RuleItem* const ammoRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					const int clipSize = ammoRule->getClipSize();

					if (_game->getSavedGame()->getMonthsPassed() == -1)
						baseQty = 1;
					else
						baseQty = _base->getItems()->getItemQty(ammoRule->getType()) / clipSize;

					change = std::min( // maximum number of Vehicles, w/ full Ammo.
									change,
									baseQty);

					if (change > 0)
					{
						for (int
								i = 0;
								i != change;
								++i)
						{
							if (_game->getSavedGame()->getMonthsPassed() != -1)
							{
								_base->getItems()->removeItem(
															ammoRule->getType(),
															clipSize);
								_base->getItems()->removeItem(_items[_sel]);
							}

							craft->getVehicles()->push_back(new Vehicle(
																	itRule,
																	clipSize,
																	unitSize));
						}
					}
					else // not enough Ammo
					{
						_timerRight->stop();
						LocalizedText msg(tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP")
											.arg(tr(ammoRule->getType())));
						_game->pushState(new ErrorMessageState(
															msg,
															_palette,
															Palette::blockOffset(15)+1,
															"BACK04.SCR",
															2));
					}
				}
				else // no Ammo required.
				{
					for (int
							i = 0;
							i != change;
							++i)
					{
						if (_game->getSavedGame()->getMonthsPassed() != -1)
							_base->getItems()->removeItem(_items[_sel]);

						craft->getVehicles()->push_back(new Vehicle(
																itRule,
																itRule->getClipSize(),
																unitSize));
					}
				}
			}
		}
		else // load item
		{
			if (craft->getRules()->getMaxItems() > 0
				&& craft->getLoadCurrent() + change > craft->getLoadCapacity())
			{
				_timerRight->stop();

				LocalizedText msg(tr(
								"STR_NO_MORE_EQUIPMENT_ALLOWED",
								craft->getLoadCapacity()));
//								craft->getRules()->getMaxItems()));

				_game->pushState(new ErrorMessageState(
													msg,
													_palette,
													Palette::blockOffset(15)+1,
													"BACK04.SCR",
													2));

				change = craft->getLoadCapacity() - craft->getLoadCurrent();
			}

			if (change > 0)
			{
				craft->getItems()->addItem(
										_items[_sel],
										change);

				if (_game->getSavedGame()->getMonthsPassed() != -1)
					_base->getItems()->removeItem(
												_items[_sel],
												change);
			}
		}

		updateQuantity();
	}
}

/**
 * Empties the contents of the Craft - moves all the items back to the Base.
* @param action - pointer to an Action
 */
void CraftEquipmentState::btnClearClick(Action*)
{
	for (
			_sel = 0;
			_sel != _items.size();
			++_sel)
	{
		moveLeftByValue(std::numeric_limits<int>::max());
	}
}

/**
* Displays the inventory screen for the soldiers inside the craft.
* @param action - pointer to an Action
*/
void CraftEquipmentState::btnInventoryClick(Action*)
{
	SavedBattleGame* const battle = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(battle);
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);

	bgen.runInventory(
					_base->getCrafts()->at(_craftId),
					NULL,
					_selUnit);

	_game->getScreen()->clear();
	_game->pushState(new InventoryState(
									false,
									NULL));
}

/**
 * Sets current cost to send the Craft on a mission.
 */
void CraftEquipmentState::calcCost() // private.
{
	const int cost = _base->calcSoldierBonuses(_base->getCrafts()->at(_craftId));
	_txtCost->setText(tr("STR_COST_").arg(Text::formatFunding(cost)));
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action, Pointer to an action.
 */
/*void CraftEquipmentState::lstEquipmentMousePress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerRight->stop();
		_timerLeft->stop();

		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveRightByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerRight->stop();
		_timerLeft->stop();

		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveLeftByValue(Options::changeValueByMouseWheel);
		}
	}
} */

}
