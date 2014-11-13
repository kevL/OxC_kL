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

#include "CraftEquipmentState.h"

//#include <algorithm>
//#include <climits>
//#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

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
 * @param base	- pointer to the Base to get info from
 * @param craft	- ID of the selected craft
 */
CraftEquipmentState::CraftEquipmentState(
		Base* base,
		size_t craftID)
	:
		_sel(0),
		_craftID(craftID),
		_base(base)
{
	Craft* const craft = _base->getCrafts()->at(_craftID);

	const bool
		hasCrew = craft->getNumSoldiers() > 0,
		newBattle = _game->getSavedGame()->getMonthsPassed() == -1;

	_window			= new Window(this, 320, 200, 0, 0);
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
//	_btnOk			= new TextButton((hasCrew || newBattle) ? 148 : 288, 16, (hasCrew || newBattle) ? 164 : 16, 176);
	_btnOk			= new TextButton(94, 16, 210, 177);

	setPalette("PAL_BASESCAPE", 2);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtSpace);
	add(_txtLoad);
	add(_txtItem);
	add(_txtStores);
	add(_txtCraft);
	add(_lstEquipment);
	add(_btnClear);
	add(_btnInventory);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnClear->setColor(Palette::blockOffset(15)+1);
	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick((ActionHandler)& CraftEquipmentState::btnClearClick);
//	_btnClear->setVisible(newBattle);

	_btnInventory->setColor(Palette::blockOffset(15)+1);
	_btnInventory->setText(tr("STR_INVENTORY"));
	_btnInventory->onMouseClick((ActionHandler)& CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(hasCrew && newBattle == false);
//	_btnInventory->setVisible(hasCrew || newBattle); // kL

	_btnOk->setColor(Palette::blockOffset(15)+1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(15)+1);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(craft->getName(_game->getLanguage())));

	_txtBaseLabel->setColor(Palette::blockOffset(15)+1);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setColor(Palette::blockOffset(15)+1);
	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setColor(Palette::blockOffset(15)+1);
	_txtStores->setText(tr("STR_STORES"));

	_txtCraft->setColor(Palette::blockOffset(15)+1);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtSpace->setColor(Palette::blockOffset(15)+1);
	_txtSpace->setSecondaryColor(Palette::blockOffset(13));
	_txtSpace->setText(tr("STR_SPACE_USED_FREE_")
					.arg(craft->getSpaceUsed())
					.arg(craft->getSpaceAvailable()));

	_txtLoad->setColor(Palette::blockOffset(15)+1);
	_txtLoad->setSecondaryColor(Palette::blockOffset(13));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
					.arg(craft->getLoadCapacity())
					.arg(craft->getLoadCapacity() - craft->getLoadCurrent()));

	_lstEquipment->setColor(Palette::blockOffset(13)+10);
	_lstEquipment->setArrowColor(Palette::blockOffset(15)+1);
	_lstEquipment->setArrowColumn(189, ARROW_HORIZONTAL);
	_lstEquipment->setColumns(3, 147, 85, 41);
	_lstEquipment->setSelectable();
	_lstEquipment->setBackground(_window);
	_lstEquipment->setMargin();
//	_lstEquipment->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);

	_lstEquipment->onLeftArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowPress);
	_lstEquipment->onLeftArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowRelease);
	_lstEquipment->onLeftArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowClick);
	_lstEquipment->onRightArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowPress);
	_lstEquipment->onRightArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowRelease);
	_lstEquipment->onRightArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowClick);
	_lstEquipment->onMousePress((ActionHandler)& CraftEquipmentState::lstEquipmentMousePress);


	size_t row = 0;

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		RuleItem* const rule = _game->getRuleset()->getItem(*i);

		int craftQty = 0;
		if (rule->isFixed() == true)
			craftQty = craft->getVehicleCount(*i);
		else
			craftQty = craft->getItems()->getItem(*i);

		if (rule->getBigSprite() > -1
			&& rule->getBattleType() != BT_NONE
			&& rule->getBattleType() != BT_CORPSE
			&& _game->getSavedGame()->isResearched(rule->getRequirements()) == true
			&& (_base->getItems()->getItem(*i) > 0
				|| craftQty > 0))
		{
			_items.push_back(*i);

			std::wostringstream
				ss1,
				ss2;

			if (_game->getSavedGame()->getMonthsPassed() > -1)
				ss1 << _base->getItems()->getItem(*i);
			else
				ss1 << "-";

			ss2 << craftQty;

			std::wstring item = tr(*i);
			if (rule->getBattleType() == BT_AMMO) // weapon clips
			{
				const int clipSize = rule->getClipSize();
				if (clipSize > 1)
					item += (L" (" + Text::formatNumber(clipSize) + L")");

				item.insert(0, L"  ");
			}
			else if (rule->isFixed() // tank w/ Ordnance.
				&& rule->getCompatibleAmmo()->empty() == false)
			{
				const RuleItem* const ammoRule = _game->getRuleset()->getItem(rule->getCompatibleAmmo()->front());
				const int clipSize = ammoRule->getClipSize();
				if (clipSize > 0)
					item += (L" (" + Text::formatNumber(clipSize) + L")");
			}

			_lstEquipment->addRow(
								3,
								item.c_str(),
								ss1.str().c_str(),
								ss2.str().c_str());

			Uint8 color;
			if (craftQty == 0)
			{
				if (rule->getBattleType() == BT_AMMO)
					color = Palette::blockOffset(15)+6;
				else
					color = Palette::blockOffset(13)+10;
			}
			else
				color = Palette::blockOffset(13);

			_lstEquipment->setRowColor(row, color);

			row++;
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
* Resets the savegame when coming back from the inventory.
*/
void CraftEquipmentState::init()
{
	State::init();

	_game->getSavedGame()->setBattleGame(NULL);

	Craft* const craft = _base->getCrafts()->at(_craftID);
	craft->setInBattlescape(false);

	// Restore system colors
	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
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
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moveLeftByValue(std::numeric_limits<int>::max());

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValue(1);

		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
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
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moveRightByValue(std::numeric_limits<int>::max());

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValue(1);

		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action, Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentMousePress(Action* action)
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
}

/**
 * Updates the displayed quantities of the selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	const RuleItem* const itemRule = _game->getRuleset()->getItem(_items[_sel]);
	Craft* const craft = _base->getCrafts()->at(_craftID);

	int cQty = 0;
	if (itemRule->isFixed() == true)
		cQty = craft->getVehicleCount(_items[_sel]);
	else
		cQty = craft->getItems()->getItem(_items[_sel]);

	std::wostringstream
		ss1,
		ss2;

	if (_game->getSavedGame()->getMonthsPassed() > -1)
		ss1 << _base->getItems()->getItem(_items[_sel]);
	else
		ss1 << "-";

	ss2 << cQty;

	Uint8 color;
	if (cQty == 0)
	{
		if (itemRule->getBattleType() == BT_AMMO)
			color = Palette::blockOffset(15)+6;
		else
			color = Palette::blockOffset(13)+10;
	}
	else
		color = Palette::blockOffset(13);

	_lstEquipment->setRowColor(_sel, color);
	_lstEquipment->setCellText(_sel, 1, ss1.str());
	_lstEquipment->setCellText(_sel, 2, ss2.str());

	_txtSpace->setText(tr("STR_SPACE_USED_FREE_")
					.arg(craft->getSpaceUsed())
					.arg(craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
					.arg(craft->getLoadCapacity())
					.arg(craft->getLoadCapacity() - craft->getLoadCurrent()));
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
	if (change < 1)
		return;

	RuleItem* const rule = _game->getRuleset()->getItem(_items[_sel]);
	Craft* const craft = _base->getCrafts()->at(_craftID);

	int craftQty = 0;
	if (rule->isFixed() == true)
		craftQty = craft->getVehicleCount(_items[_sel]);
	else
		craftQty = craft->getItems()->getItem(_items[_sel]);

	if (craftQty == 0)
		return;


	change = std::min(craftQty, change);

	if (rule->isFixed() == true) // convert vehicle to item
	{
		if (rule->getCompatibleAmmo()->empty() == false)
		{
			// first remove all vehicles to redistribute the ammo
			const RuleItem* const ammoRule = _game->getRuleset()->getItem(rule->getCompatibleAmmo()->front());

			for (std::vector<Vehicle*>::const_iterator
					i = craft->getVehicles()->begin();
					i != craft->getVehicles()->end();
					)
			{
				if ((*i)->getRules() == rule)
				{
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
				if ((*i)->getRules() == rule)
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
	if (change < 1)
		return;

	int baseQty = _base->getItems()->getItem(_items[_sel]);
	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		if (change == std::numeric_limits<int>::max())
			change = 10;

		baseQty = change;
	}

	if (baseQty < 1)
		return;


	change = std::min(
					change,
					baseQty);

	RuleItem* const rule = _game->getRuleset()->getItem(_items[_sel]);
	Craft* const craft = _base->getCrafts()->at(_craftID);

	if (rule->isFixed() == true) // load vehicle, convert item to a vehicle
	{
		const Unit* const tankRule = _game->getRuleset()->getUnit(rule->getType());
		const int
			unitSize = _game->getRuleset()->getArmor(tankRule->getArmor())->getSize()
					 * _game->getRuleset()->getArmor(tankRule->getArmor())->getSize(),
			spaceAvailable = std::min(
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

			if (rule->getCompatibleAmmo()->empty() == false)
			{
				const RuleItem* const ammoRule = _game->getRuleset()->getItem(rule->getCompatibleAmmo()->front());
				const int rounds = ammoRule->getClipSize();

				if (_game->getSavedGame()->getMonthsPassed() == -1)
					baseQty = 1;
				else
					baseQty = _base->getItems()->getItem(ammoRule->getType()) / rounds;

				change = std::min( // maximum number of Vehicles, w/ full Ammo.
								change,
								baseQty);

				if (change > 0)
				{
					for (int
							i = 0;
							i < change;
							++i)
					{
						if (_game->getSavedGame()->getMonthsPassed() != -1)
						{
							_base->getItems()->removeItem(
														ammoRule->getType(),
														rounds);
							_base->getItems()->removeItem(_items[_sel]);
						}

						craft->getVehicles()->push_back(new Vehicle(
																rule,
																rounds,
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
						i < change;
						++i)
				{
					if (_game->getSavedGame()->getMonthsPassed() != -1)
						_base->getItems()->removeItem(_items[_sel]);

					craft->getVehicles()->push_back(new Vehicle(
															rule,
															rule->getClipSize(),
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
							craft->getRules()->getMaxItems()));

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

/**
 * Empties the contents of the craft, moving all of the items back to the base.
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
	SavedBattleGame* battle = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(battle);
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);

	Craft* craft = _base->getCrafts()->at(_craftID);
	bgen.runInventory(craft);

	// Set system colors
	_game->getCursor()->setColor(Palette::blockOffset(9));
	_game->getFpsCounter()->setColor(Palette::blockOffset(9));

	_game->getScreen()->clear();

	_game->pushState(new InventoryState(
									false,
									NULL));
}

}
