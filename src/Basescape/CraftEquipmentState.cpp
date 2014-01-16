/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CraftEquipmentState.h"

#include <algorithm>
#include <climits>
#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

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
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftEquipmentState::CraftEquipmentState(
		Game* game,
		Base* base,
		size_t craft)
	:
		State(game),
		_sel(0),
		_base(base),
		_craft(craft)
{
	_changeValueByMouseWheel = Options::getInt("changeValueByMouseWheel");
	_allowChangeListValuesByMouseWheel = Options::getBool("allowChangeListValuesByMouseWheel")
											&& _changeValueByMouseWheel;

	Craft* c = _base->getCrafts()->at(_craft);
	bool craftHasCrew = c->getNumSoldiers() > 0;
	bool newBattle = game->getSavedGame()->getMonthsPassed() == -1;


	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtUsed		= new Text(110, 9, 16, 25);
	_txtAvailable	= new Text(110, 9, 160, 25);

	_txtItem		= new Text(144, 9, 16, 33);
	_txtStores		= new Text(50, 9, 171, 33);
	_txtCraft		= new Text(50, 9, 256, 33);
//kL	_txtCrew		= new Text(71, 9, 244, 24);

	_lstEquipment	= new TextList(285, 128, 16, 42);

	_btnClear		= new TextButton(94, 16, 16, 177);
	_btnInventory	= new TextButton(94, 16, 113, 177);
//kL	_btnOk			= new TextButton((craftHasCrew || newBattle) ? 148 : 288, 16, (craftHasCrew || newBattle) ? 164 : 16, 176);
	_btnOk			= new TextButton(94, 16, 210, 177);


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_1")->getColors());
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(2)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtAvailable);
	add(_txtUsed);
	add(_txtItem);
	add(_txtStores);
	add(_txtCraft);
//kL	add(_txtCrew);
	add(_lstEquipment);
	add(_btnClear);
	add(_btnInventory);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnClear->setColor(Palette::blockOffset(15)+1);
	_btnClear->setText(tr("STR_UNLOAD"));
	_btnClear->onMouseClick((ActionHandler)& CraftEquipmentState::btnClearClick);
//kL	_btnClear->setVisible(newBattle);

	_btnInventory->setColor(Palette::blockOffset(15) + 1);
	_btnInventory->setText(tr("STR_INVENTORY"));
	_btnInventory->onMouseClick((ActionHandler)& CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(craftHasCrew && !newBattle);
//	_btnInventory->setVisible(craftHasCrew || newBattle); // kL

	_btnOk->setColor(Palette::blockOffset(15)+1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftEquipmentState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(15)+1);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtBaseLabel->setColor(Palette::blockOffset(15)+1);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setColor(Palette::blockOffset(15)+1);
	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setColor(Palette::blockOffset(15)+1);
	_txtStores->setText(tr("STR_STORES"));

	_txtCraft->setColor(Palette::blockOffset(15)+1);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtAvailable->setColor(Palette::blockOffset(15)+1);
	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));

	_txtUsed->setColor(Palette::blockOffset(15)+1);
	_txtUsed->setSecondaryColor(Palette::blockOffset(13));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));

//kL	_txtCrew->setColor(Palette::blockOffset(15)+1);
//kL	_txtCrew->setSecondaryColor(Palette::blockOffset(13));
//kL	std::wstringstream ss3;
//kL	ss3 << tr("STR_SOLDIERS_UC") << "> " << L'\x01'<< c->getNumSoldiers();
//kL	_txtCrew->setText(ss3.str());
//	_txtCrew->setText(tr("STR_SOLDIERS_UC").arg(c->getNumSoldiers())); // kL

	_lstEquipment->setColor(Palette::blockOffset(13)+10);
	_lstEquipment->setArrowColor(Palette::blockOffset(15)+1);
	_lstEquipment->setArrowColumn(188, ARROW_HORIZONTAL);
	_lstEquipment->setColumns(3, 147, 85, 41);
	_lstEquipment->setSelectable(true);
	_lstEquipment->setBackground(_window);
	_lstEquipment->setMargin(8);
	_lstEquipment->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);

	_lstEquipment->onLeftArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowPress);
	_lstEquipment->onLeftArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowRelease);
	_lstEquipment->onLeftArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentLeftArrowClick);
	_lstEquipment->onRightArrowPress((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowPress);
	_lstEquipment->onRightArrowRelease((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowRelease);
	_lstEquipment->onRightArrowClick((ActionHandler)& CraftEquipmentState::lstEquipmentRightArrowClick);
	_lstEquipment->onMousePress((ActionHandler)& CraftEquipmentState::lstEquipmentMousePress);

	int row = 0;
	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		// CHEAP HACK TO HIDE HWP AMMO
		if ((*i).substr(0, 8) == "STR_HWP_")
			continue;

		RuleItem* rule = _game->getRuleset()->getItem(*i);

		int cQty = 0;
		if (rule->isFixed())
		{
			cQty = c->getVehicleCount(*i);
		}
		else
		{
			cQty = c->getItems()->getItem(*i);
		}

		if (rule->getBigSprite() > -1
			&& rule->getBattleType() != BT_NONE
			&& rule->getBattleType() != BT_CORPSE
			&& _game->getSavedGame()->isResearched(rule->getRequirements())
			&& (_base->getItems()->getItem(*i) > 0 || cQty > 0))
		{
			_items.push_back(*i);
			std::wstringstream ss, ss2;

			if (_game->getSavedGame()->getMonthsPassed() > -1)
			{
				ss << _base->getItems()->getItem(*i);
			}
			else
			{
				ss << "-";
			}

			ss2 << cQty;

			std::wstring s = tr(*i);
			if (rule->getBattleType() == BT_AMMO)
			{
				s.insert(0, L"  ");
			}

			_lstEquipment->addRow(3, s.c_str(), ss.str().c_str(), ss2.str().c_str());

			Uint8 color;

			if (cQty == 0)
			{
				if (rule->getBattleType() == BT_AMMO)
				{
					color = Palette::blockOffset(15)+6;
				}
				else
				{
					color = Palette::blockOffset(13)+10;
				}
			}
			else
			{
				color = Palette::blockOffset(13);
			}

			_lstEquipment->setRowColor(row, color);

			++row;
		}
	}

	_timerLeft = new Timer(280);
	_timerLeft->onTimer((StateHandler)& CraftEquipmentState::moveLeft);

	_timerRight = new Timer(280);
	_timerRight->onTimer((StateHandler)& CraftEquipmentState::moveRight);
}

/**
 *
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
	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_1")->getColors());
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(2)),
				Palette::backPos,
				16);

	_game->getSavedGame()->setBattleGame(0);

	Craft* c = _base->getCrafts()->at(_craft);
	c->setInBattlescape(false);
}

/**
 * Runs the arrow timers.
 */
void CraftEquipmentState::think()
{
	State::think();

	_timerLeft->think(this, 0);
	_timerRight->think(this, 0);
}


/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Starts moving the item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& !_timerLeft->isRunning())
	{
		_timerLeft->start();
	}
}

/**
 * Stops moving the item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLeft->stop();
	}
}

/**
 * Moves all the items to the base on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		moveLeftByValue(INT_MAX);
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValue(1);

		_timerRight->setInterval(280);
		_timerLeft->setInterval(280);
	}
}

/**
 * Starts moving the item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowPress(Action* action)
{
	_sel = _lstEquipment->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& !_timerRight->isRunning())
	{
		_timerRight->start();
	}
}

/**
 * Stops moving the item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerRight->stop();
	}
}

/**
 * Moves all the items (as much as possible) to the craft on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		moveRightByValue(INT_MAX);
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValue(1);

		_timerRight->setInterval(280);
		_timerLeft->setInterval(280);
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

		if (_allowChangeListValuesByMouseWheel
			&& action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveRightByValue(_changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerRight->stop();
		_timerLeft->stop();

		if (_allowChangeListValuesByMouseWheel
			&& action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveLeftByValue(_changeValueByMouseWheel);
		}
	}
}

/**
 * Updates the displayed quantities of the selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	Craft* c = _base->getCrafts()->at(_craft);
	RuleItem* itemRule = _game->getRuleset()->getItem(_items[_sel]);

	int cQty = 0;
	if (itemRule->isFixed())
	{
		cQty = c->getVehicleCount(_items[_sel]);
	}
	else
	{
		cQty = c->getItems()->getItem(_items[_sel]);
	}


	std::wstringstream
		ss,
		ss2;

	if (_game->getSavedGame()->getMonthsPassed() > -1)
		ss << _base->getItems()->getItem(_items[_sel]);
	else
		ss << "-";

	ss2 << cQty;


	Uint8 color;

	if (cQty == 0)
	{
//kL		RuleItem* rule = _game->getRuleset()->getItem(_items[_sel]);

//kL		if (rule->getBattleType() == BT_AMMO)
		if (itemRule->getBattleType() == BT_AMMO)
		{
			color = Palette::blockOffset(15)+6;
		}
		else
		{
			color = Palette::blockOffset(13)+10;
		}
	}
	else
	{
		color = Palette::blockOffset(13);
	}

	_lstEquipment->setRowColor(_sel, color);
	_lstEquipment->setCellText(_sel, 1, ss.str());
	_lstEquipment->setCellText(_sel, 2, ss2.str());

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
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
 * @param change, Item difference.
 */
void CraftEquipmentState::moveLeftByValue(int change)
{
	if (change < 1) return;


	Craft* c = _base->getCrafts()->at(_craft);
	RuleItem* itemRule = _game->getRuleset()->getItem(_items[_sel]);

	int cQty = 0;

	if (itemRule->isFixed())
	{
		cQty = c->getVehicleCount(_items[_sel]);
	}
	else
	{
		cQty = c->getItems()->getItem(_items[_sel]);
	}

	if (cQty < 1) return;


	change = std::min(cQty, change);

	if (itemRule->isFixed()) // Convert vehicle to item
	{
		if (!itemRule->getCompatibleAmmo()->empty())
		{
			// First we remove all vehicles because we want to redistribute the ammo
			RuleItem* ammo = _game->getRuleset()->getItem(itemRule->getCompatibleAmmo()->front());

			for (std::vector<Vehicle*>::iterator
					i = c->getVehicles()->begin();
					i != c->getVehicles()->end();
					)
			{
				if ((*i)->getRules() == itemRule)
				{
					_base->getItems()->addItem(ammo->getType(), (*i)->getAmmo());

					delete *i;
					i = c->getVehicles()->erase(i);
				}
				else ++i;
			}

			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				_base->getItems()->addItem(_items[_sel], cQty);
			}

			// And now reAdd the count we want to keep in the craft (and redistribute the ammo among them)
			if (cQty > change) moveRightByValue(cQty - change);
		}
		else
		{
			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				_base->getItems()->addItem(_items[_sel], change);
			}

			for (std::vector<Vehicle*>::iterator
					i = c->getVehicles()->begin();
					i != c->getVehicles()->end();
					)
			{
				if ((*i)->getRules() == itemRule)
				{
					delete *i;
					i = c->getVehicles()->erase(i);

					if (--change < 1)
						break;
				}
				else ++i;
			}
		}
	}
	else
	{
		c->getItems()->removeItem(_items[_sel], change);

		if (_game->getSavedGame()->getMonthsPassed() == -1)
		{
			Options::setInt("NewBattle_" + _items[_sel], Options::getInt("NewBattle_" +_items[_sel]) - change);
		}
		else
		{
			_base->getItems()->addItem(_items[_sel], change);
		}
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
 * @param change, Item difference.
 */
void CraftEquipmentState::moveRightByValue(int change)
{
	if (change < 1) return;


	Craft* c = _base->getCrafts()->at(_craft);
	RuleItem* itemRule = _game->getRuleset()->getItem(_items[_sel]);

	int bQty = _base->getItems()->getItem(_items[_sel]);

	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		if (change == INT_MAX)
		{
			change = 10;
		}

		bQty = change;
	}

	if (bQty < 1) return;


	change = std::min(bQty, change);

	if (itemRule->isFixed()) // Do we need to convert item to vehicle?
	{
		int size = 4;
		Unit* tankRule = _game->getRuleset()->getUnit(itemRule->getType());
		if (tankRule)
		{
			size = _game->getRuleset()->getArmor(tankRule->getArmor())->getSize();
			size *= size;
		}
/*		if (_game->getRuleset()->getUnit(itemRule->getType()))
		{
			size = _game->getRuleset()->getArmor(_game->getRuleset()->getUnit(itemRule->getType())->getArmor())->getSize();
			size *= size;
		} */

		// Check if there's enough room
		int room = std::min(
						c->getRules()->getVehicles() - c->getNumVehicles(),
						c->getSpaceAvailable() / size);
		if (room > 0)
		{
			change = std::min(room, change);

			if (!itemRule->getCompatibleAmmo()->empty()) // if there is compatible ammo
			{
				// We want to redistribute all the available ammo among the vehicles,
				// so first we note the total number of vehicles we want in the craft
				int oldVehiclesCount = c->getVehicleCount(_items[_sel]);
				int newVehiclesCount = oldVehiclesCount + change;
				// ...and we move back all of this vehicle-type to the base.
				if (oldVehiclesCount > 0)
					moveLeftByValue(INT_MAX);

				// And now let's see if we can add the total number of vehicles.
				RuleItem* ammo = _game->getRuleset()->getItem(itemRule->getCompatibleAmmo()->front()); // here's the ammo

				int baQty = _base->getItems()->getItem(ammo->getType()); // Ammo Quantity for this vehicle-type on the base

				int canBeAdded = std::min(newVehiclesCount, baQty);
				if (canBeAdded > 0)
				{
					int newAmmoPerVehicle = std::min(baQty / canBeAdded, ammo->getClipSize());
					int remainder = 0;

					if (ammo->getClipSize() > newAmmoPerVehicle)
						remainder = baQty - (canBeAdded * newAmmoPerVehicle);

					int newAmmo;
					for (int
							i = 0;
							i < canBeAdded;
							++i)
					{
						newAmmo = newAmmoPerVehicle;

						if (i < remainder) ++newAmmo;

						if (_game->getSavedGame()->getMonthsPassed() != -1)
						{
							_base->getItems()->removeItem(ammo->getType(), newAmmo);
							_base->getItems()->removeItem(_items[_sel]);
						}
						else
						{
							newAmmo = ammo->getClipSize();
						}

						c->getVehicles()->push_back(new Vehicle(
															itemRule,
															newAmmo,
//															static_cast<int>(sqrt(static_cast<double>(size))))); // kL
															size));
					}
				}

				if (oldVehiclesCount >= canBeAdded)
				{
					// So we haven't managed to increase the count of vehicles because of the ammo
					_timerRight->stop();

					LocalizedText msg(tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP").arg(tr(ammo->getType())));
					_game->pushState(new ErrorMessageState(
														_game,
														msg,
														Palette::blockOffset(15)+1, "BACK04.SCR", 2));
				}
			}
			else // no ammo required.
			{
				for (int
						i = 0;
						i < change;
						++i)
				{
					c->getVehicles()->push_back(new Vehicle(
														itemRule,
														itemRule->getClipSize(),
//														static_cast<int>(sqrt(static_cast<double>(size))))); // kL
														size));

					if (_game->getSavedGame()->getMonthsPassed() != -1)
					{
						_base->getItems()->removeItem(_items[_sel]);
					}
				}
			}
		}
	}
	else
	{
		c->getItems()->addItem(_items[_sel],change);

		if (_game->getSavedGame()->getMonthsPassed() == -1)
		{
				Options::setInt("NewBattle_" + _items[_sel], Options::getInt("NewBattle_" + _items[_sel]) + change);
		}
		else
		{
			_base->getItems()->removeItem(_items[_sel],change);
		}
	}

	updateQuantity();
}

/**
 * Empties the contents of the craft, moving all of the items back to the base.
 */
void CraftEquipmentState::btnClearClick(Action*)
{
	for (
			_sel = 0;
			_sel != _items.size();
			++_sel)
	{
		moveLeftByValue(INT_MAX);
	}
}

/**
* Displays the inventory screen for the soldiers inside the craft.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnInventoryClick(Action*)
{
	Log(LOG_INFO) << "\nCraftEquipmentState::btnInventoryClick()";

	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_4")->getColors());

//	delete _game->getSavedGame()->getSavedBattle(); // kL, didn't work
//	_game->getSavedGame()->setBattleGame(0);		// kL, didn't work
		// kL_note: EquipCraft->Inventory for soldiers refuses to create after a mission.
		// Try fix


	SavedBattleGame* bgame = new SavedBattleGame();
	Log(LOG_INFO) << ". bgame = " << bgame;
	_game->getSavedGame()->setBattleGame(bgame);

	BattlescapeGenerator bgen = BattlescapeGenerator(_game);
	Log(LOG_INFO) << ". bgen = " << &bgen;
	Craft* craft = _base->getCrafts()->at(_craft);
	Log(LOG_INFO) << ". craft = " << craft;
	bgen.runInventory(craft);

	_game->pushState(new InventoryState(_game, false, 0));
	Log(LOG_INFO) << "CraftEquipmentState::btnInventoryClick() EXIT";
}

}
