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

#include "PurchaseState.h"

#include <climits>
#include <cmath>
#include <sstream>

#include "../aresame.h"

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

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Purchase/Hire screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
PurchaseState::PurchaseState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base),
		_crafts(),
		_items(),
		_qtys(),
		_sel(0),
		_total(0),
		_pQty(0),
		_cQty(0),
		_iQty(0.f),
		_itemOffset(0)
{
	_changeValueByMouseWheel = Options::getInt("changeValueByMouseWheel");
	_allowChangeListValuesByMouseWheel =
							Options::getBool("allowChangeListValuesByMouseWheel")
								&& _changeValueByMouseWheel;


	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(310, 17, 5, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);

	_txtFunds		= new Text(140, 9, 16, 24);
	_txtPurchases	= new Text(140, 9, 160, 24);

	_txtItem		= new Text(140, 9, 16, 33);
	_txtCost		= new Text(102, 9, 166, 33);
	_txtQuantity	= new Text(60, 9, 267, 33);

	_lstItems		= new TextList(285, 128, 16, 44);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtFunds);
	add(_txtPurchases);
	add(_txtItem);
	add(_txtCost);
	add(_txtQuantity);
	add(_lstItems);
	add(_btnCancel);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& PurchaseState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PurchaseState::btnOkClick,
					(SDLKey)Options::getInt("keyOk"));
	_btnOk->setVisible(false);

	_btnCancel->setColor(Palette::blockOffset(13)+10);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& PurchaseState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& PurchaseState::btnCancelClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PURCHASE_HIRE_PERSONNEL"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtFunds->setColor(Palette::blockOffset(13)+10);
	_txtFunds->setSecondaryColor(Palette::blockOffset(13));
	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtPurchases->setColor(Palette::blockOffset(13)+10);
	_txtPurchases->setSecondaryColor(Palette::blockOffset(13));
	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES")
						.arg(Text::formatFunding(_total)));

	_txtItem->setColor(Palette::blockOffset(13)+10);
	_txtItem->setText(tr("STR_ITEM"));

	_txtCost->setColor(Palette::blockOffset(13)+10);
	_txtCost->setText(tr("STR_COST_PER_UNIT_UC"));

	_txtQuantity->setColor(Palette::blockOffset(13)+10);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstItems->setColor(Palette::blockOffset(13)+10);
	_lstItems->setArrowColumn(227, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142, 55, 46, 32);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(8);
	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstItems->onLeftArrowPress((ActionHandler)& PurchaseState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& PurchaseState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& PurchaseState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& PurchaseState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& PurchaseState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& PurchaseState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)& PurchaseState::lstItemsMousePress);

	std::wstringstream
		ss1,
		ss2,
		ss3,
		ss4,
		ss5,
		ss6,
		ss7;

	ss1 << _base->getTotalSoldiers();
	_lstItems->addRow(
					4,
					tr("STR_SOLDIER").c_str(),
					Text::formatFunding(_game->getRuleset()->getSoldierCost() * 2).c_str(),
					ss1.str().c_str(),
					L"0");
	_qtys.push_back(0);

	ss2 << _base->getTotalScientists();
	_lstItems->addRow(
					4,
					tr("STR_SCIENTIST").c_str(),
					Text::formatFunding(_game->getRuleset()->getScientistCost() * 2).c_str(),
					ss2.str().c_str(),
					L"0");
	_qtys.push_back(0);

	ss3 << _base->getTotalEngineers();
	_lstItems->addRow(
					4,
					tr("STR_ENGINEER").c_str(),
					Text::formatFunding(_game->getRuleset()->getEngineerCost() * 2).c_str(),
					ss3.str().c_str(),
					L"0");
	_qtys.push_back(0);

	_itemOffset = 3;

	// Add craft-types to purchase list.
	const std::vector<std::string>& crafts = _game->getRuleset()->getCraftsList();
	for (std::vector<std::string>::const_iterator
			i = crafts.begin();
			i != crafts.end();
			++i)
	{
		ss4.str(L"");

		RuleCraft* rule = _game->getRuleset()->getCraft(*i);

		if (rule->getBuyCost() > 0
			&& _game->getSavedGame()->isResearched(rule->getRequirements()))
		{
			_qtys.push_back(0);
			_crafts.push_back(*i);

			++_itemOffset;

			int crafts = 0;
			for (std::vector<Craft*>::iterator
					c = _base->getCrafts()->begin();
					c != _base->getCrafts()->end();
					++c)
			{
				if ((*c)->getRules()->getType() == *i)
					crafts++;
			}

			ss4 << crafts;
			_lstItems->addRow(
							4,
							tr(*i).c_str(),
							Text::formatFunding(rule->getBuyCost()).c_str(),
							ss4.str().c_str(),
							L"0");
		}
	}


	std::vector<std::string> items = _game->getRuleset()->getItemsList();

	// Add craft Weapon-types to purchase list.
	// WB-removed: 140124 begin:
	const std::vector<std::string>& cweapons = _game->getRuleset()->getCraftWeaponsList();
	for (std::vector<std::string>::const_iterator
			i = cweapons.begin();
			i != cweapons.end();
			++i)
	{
		ss5.str(L"");
		ss6.str(L"");

		// Special handling for treating craft weapons as items
		RuleCraftWeapon* rule = _game->getRuleset()->getCraftWeapon(*i);
		RuleItem* launcher = _game->getRuleset()->getItem(rule->getLauncherItem()); // kL

		if (launcher != 0
			&& launcher->getBuyCost() > 0
			&& !isExcluded(launcher->getType()))
		{
			_qtys.push_back(0);
			_items.push_back(launcher->getType());

			std::wstring item = tr(launcher->getType());
			// kL_begin: Add qty of craft weapons in transit to Purchase screen stores.
			int tQty = _base->getItems()->getItem(launcher->getType()); // Gets the item type. Each item has a unique type.
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				std::wstring trItem = (*j)->getName(_game->getLanguage());
				if (item == trItem)
					tQty += (*j)->getQuantity();
			} // kL_end.

//kL			ss5 << _base->getItems()->getItem(launcher->getType());
			ss5 << tQty; // kL
			_lstItems->addRow(
							4,
							item.c_str(),
							Text::formatFunding(launcher->getBuyCost()).c_str(),
							ss5.str().c_str(),
							L"0");

			for (std::vector<std::string>::iterator
					j = items.begin();
					j != items.end();
					++j)
			{
				if (*j == launcher->getType())
				{
					items.erase(j);

					break;
				}
			}
		}

		// Handle craft weapon ammo.
		RuleItem* clip = _game->getRuleset()->getItem(rule->getClipItem()); // kL

		if (clip != 0
			&& clip->getBuyCost() > 0
			&& !isExcluded(clip->getType()))
		{
			_qtys.push_back(0);
			_items.push_back(clip->getType());

			std::wstring item = tr(clip->getType());
			// kL_begin: Add qty of clips in transit to Purchase screen stores.
			int tQty = _base->getItems()->getItem(clip->getType()); // Gets the item type. Each item has a unique type.
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				std::wstring trItem = (*j)->getName(_game->getLanguage());
				if (item == trItem)
					tQty += (*j)->getQuantity();
			} // kL_end.

//kL			ss6 << _base->getItems()->getItem(clip->getType());
			ss6 << tQty; // kL

			item.insert(0, L"  ");
			_lstItems->addRow(
							4,
							item.c_str(),
							Text::formatFunding(clip->getBuyCost()).c_str(),
							ss6.str().c_str(),
							L"0");
			_lstItems->setRowColor(_qtys.size() - 1, Palette::blockOffset(15)+6); // kL

			for (std::vector<std::string>::iterator
					j = items.begin();
					j != items.end();
					++j)
			{
				if (*j == clip->getType())
				{
					items.erase(j);

					break;
				}
			}
		}
	} // end WB-removed: 140124.


	// Add items to purchase list.
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		ss7.str(L"");

		RuleItem* rule = _game->getRuleset()->getItem(*i);

		if (rule->getBuyCost() > 0
			&& !isExcluded(*i))
		{
			_qtys.push_back(0);
			_items.push_back(*i);

			std::wstring item = tr(*i);

			// kL_begin:
			int tQty = _base->getItems()->getItem(*i); // Returns the quantity of an item in the container.

			// Add qty of items in transit to Purchase screen stock.
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				std::wstring trItem = (*j)->getName(_game->getLanguage());
//				std::wstring trItem = tr(*j);
				if (item == trItem)
					tQty += (*j)->getQuantity();
			}

			// Add qty of items & vehicles on transport craft to Purchase screen stock.
			for (std::vector<Craft*>::const_iterator
					c = _base->getCrafts()->begin();
					c != _base->getCrafts()->end();
					++c)
			{
				if ((*c)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::iterator
							t = (*c)->getItems()->getContents()->begin();
							t != (*c)->getItems()->getContents()->end();
							++t) // ha, map is my bitch!!
					{
						std::wstring ti = tr(t->first);
						if (ti == item)
							tQty += t->second;
					}
				}

				if ((*c)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							v = (*c)->getVehicles()->begin();
							v != (*c)->getVehicles()->end();
							++v)
					{
						std::wstring tv = tr((*v)->getRules()->getType());
						if (item == tv)
							tQty++;

						if ((*v)->getAmmo() != 255)
						{
							RuleItem* tankRule = _game->getRuleset()->getItem((*v)->getRules()->getType());
							RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());
							std::wstring tv_a = tr(ammoRule->getType());

							if (item == tv_a)
								tQty += (*v)->getAmmo();
						}
					}
				}
			} // kL_end.

			ss7 << tQty; // kL

			if (rule->getBattleType() == BT_AMMO
				 || (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				item.insert(0, L"  ");

				_lstItems->addRow(
								4,
								item.c_str(),
								Text::formatFunding(rule->getBuyCost()).c_str(),
								ss7.str().c_str(),
								L"0");
				_lstItems->setRowColor(_qtys.size() - 1, Palette::blockOffset(15)+6);
			}
			else
				_lstItems->addRow(
								4,
								item.c_str(),
								Text::formatFunding(rule->getBuyCost()).c_str(),
								ss7.str().c_str(),
								L"0");
		}
	}

	_timerInc = new Timer(280);
	_timerInc->onTimer((StateHandler)& PurchaseState::increase);

	_timerDec = new Timer(280);
	_timerDec->onTimer((StateHandler)& PurchaseState::decrease);
}

/**
 *
 */
PurchaseState::~PurchaseState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void PurchaseState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Returns whether the item is excluded in the options file.
 * @param item Item to look up.
 * @return True if the item is excluded in the options file.
 */
bool PurchaseState::isExcluded(std::string item)
{
	std::vector<std::string> excludes = Options::getPurchaseExclusions();

	bool exclude = false;
	for (std::vector<std::string>::const_iterator
			s = excludes.begin();
			s != excludes.end();
			++s)
	{
		if (item == *s)
		{
			exclude = true;

			break;
		}
	}

	return exclude;
}

/**
 * Purchases the selected items.
 * @param action Pointer to an action.
 */
void PurchaseState::btnOkClick(Action*)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _total);
	_base->setCashSpent(_total); // kL

	for (unsigned int
			i = 0;
			i < _qtys.size();
			++i)
	{
		if (_qtys[i] > 0)
		{
			if (i == 0) // Buy soldiers
			{
				for (int
						s = 0;
						s < _qtys[i];
						s++)
				{
					Transfer* t = new Transfer(_game->getRuleset()->getPersonnelTime());
					t->setSoldier(new Soldier(
											_game->getRuleset()->getSoldier("XCOM"),
											_game->getRuleset()->getArmor("STR_NONE_UC"),
											&_game->getRuleset()->getPools(),
											_game->getSavedGame()->getId("STR_SOLDIER")));

					_base->getTransfers()->push_back(t);
				}
			}
			else if (i == 1) // Buy scientists
			{
				Transfer* t = new Transfer(_game->getRuleset()->getPersonnelTime());
				t->setScientists(_qtys[i]);

				_base->getTransfers()->push_back(t);
			}
			else if (i == 2) // Buy engineers
			{
				Transfer* t = new Transfer(_game->getRuleset()->getPersonnelTime());
				t->setEngineers(_qtys[i]);

				_base->getTransfers()->push_back(t);
			}
			else if (i >= 3
				&& i < 3 + _crafts.size()) // Buy crafts
			{
				for (int
						c = 0;
						c < _qtys[i];
						c++)
				{
					RuleCraft* rc = _game->getRuleset()->getCraft(_crafts[i - 3]);
					Transfer* t = new Transfer(rc->getTransferTime());
					Craft* craft = new Craft(rc, _base, _game->getSavedGame()->getId(_crafts[i - 3]));
					craft->setStatus("STR_REFUELLING");
					t->setCraft(craft);

					_base->getTransfers()->push_back(t);
				}
			}
			else // Buy items
			{
				RuleItem* ri = _game->getRuleset()->getItem(_items[i - 3 - _crafts.size()]);
				Transfer* t = new Transfer(ri->getTransferTime());
				t->setItems(_items[i - 3 - _crafts.size()], _qtys[i]);

				_base->getTransfers()->push_back(t);
			}
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void PurchaseState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsLeftArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& !_timerInc->isRunning())
	{
		_timerInc->start();
	}
}

/**
 * Stops increasing the item.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the item by one on left-click,
 * to max on right-click.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		increaseByValue(INT_MAX);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);

		_timerInc->setInterval(280);
		_timerDec->setInterval(280);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsRightArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& !_timerDec->isRunning())
	{
		_timerDec->start();
	}
}

/**
 * Stops decreasing the item.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the item by one on left-click,
 * to 0 on right-click.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		decreaseByValue(INT_MAX);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);

		_timerInc->setInterval(280);
		_timerDec->setInterval(280);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void PurchaseState::lstItemsMousePress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (_allowChangeListValuesByMouseWheel
			&& action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			increaseByValue(_changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (_allowChangeListValuesByMouseWheel
			&& action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			decreaseByValue(_changeValueByMouseWheel);
		}
	}
}

/**
 * Gets the price of the currently selected item.
 * @return Price of the currently selected item.
 */
int PurchaseState::getPrice()
{
	if (_sel == 0) // Soldier cost
	{
		return _game->getRuleset()->getSoldierCost() * 2;
	}
	else if (_sel == 1) // Scientist cost
	{
		return _game->getRuleset()->getScientistCost() * 2;
	}
	else if (_sel == 2) // Engineer cost
	{
		return _game->getRuleset()->getEngineerCost() * 2;
	}
	else if (_sel >= 3 && _sel < 3 + _crafts.size()) // Craft cost
	{
		return _game->getRuleset()->getCraft(_crafts[_sel - 3])->getBuyCost();
	}
	else // Item cost
	{
		return _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])->getBuyCost();
	}
}

/**
 * Increases the quantity of the selected item to buy by one.
 */
void PurchaseState::increase()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to buy.
 * @param change How many we want to add.
 */
void PurchaseState::increaseByValue(int change)
{
	if (change < 1) return;

	if (_total + getPrice() > _game->getSavedGame()->getFunds())
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NOT_ENOUGH_MONEY",
											Palette::blockOffset(15)+1,
											"BACK13.SCR",
											0));
	}
	else if (_sel <= 2
		&& _pQty + 1 > _base->getAvailableQuarters() - _base->getUsedQuarters())
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NOT_ENOUGH_LIVING_SPACE",
											Palette::blockOffset(15)+1,
											"BACK13.SCR",
											0));
	}
	else if (_sel >= 3
		&& _sel < 3 + _crafts.size()
		&& _cQty + 1 > _base->getAvailableHangars() - _base->getUsedHangars())
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NO_FREE_HANGARS_FOR_PURCHASE",
											Palette::blockOffset(15)+1,
											"BACK13.SCR",
											0));
	}
	else if (_sel >= 3 + _crafts.size()
		&& _iQty + _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])->getSize()
				> _base->getAvailableStores() - _base->getUsedStores())
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NOT_ENOUGH_STORE_SPACE",
											Palette::blockOffset(15)+1,
											"BACK13.SCR",
											0));
	}
	else
	{
		int maxByMoney = (_game->getSavedGame()->getFunds() - _total) / getPrice();
		change = std::min(maxByMoney, change);

		if (_sel <= 2) // Personnel count
		{
			int maxByQuarters = _base->getAvailableQuarters() - _base->getUsedQuarters() - _pQty;
			change = std::min(maxByQuarters, change);
			_pQty += change;
		}
		else if (_sel >= 3 && _sel < 3 + _crafts.size()) // Craft count
		{
			int maxByHangars = _base->getAvailableHangars() - _base->getUsedHangars() - _cQty;
			change = std::min(maxByHangars, change);
			_cQty += change;
		}
		else if (_sel >= 3 + _crafts.size()) // Item count
		{
			float storesNeededPerItem = _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])->getSize();
			float freeStores = static_cast<float>(_base->getAvailableStores() - _base->getUsedStores()) - _iQty;
			int maxByStores;
			if (AreSame(storesNeededPerItem, 0.f))
		        maxByStores = INT_MAX;
			else
				maxByStores = static_cast<int>(floor(freeStores / storesNeededPerItem));

			change = std::min(maxByStores, change);
			_iQty += (static_cast<float>(change) * storesNeededPerItem);
		}

		_qtys[_sel] += change;
		_total += getPrice() * change;

		updateItemStrings();
	}
}

/**
 * Decreases the quantity of the selected item to buy by one.
 */
void PurchaseState::decrease()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to buy.
 * @param change how many we want to subtract.
 */
void PurchaseState::decreaseByValue(int change)
{
	if (change < 1
		|| _qtys[_sel] < 1) return;

	change = std::min(_qtys[_sel], change);

	if (_sel <= 2) // Personnel count
	{
		_pQty -= change;
	}
	else if (_sel >= 3 && _sel < 3 + _crafts.size()) // Craft count
	{
		_cQty -= change;
	}
	else // Item count
	{
		_iQty -= _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])
										->getSize() * static_cast<float>(change);
	}

	_qtys[_sel] -= change;
	_total -= getPrice() * change;

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void PurchaseState::updateItemStrings()
{
	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES")
							.arg(Text::formatFunding(_total)));

	std::wstringstream ss;
	ss << _qtys[_sel];
	_lstItems->setCellText(_sel, 3, ss.str());

	if (_qtys[_sel] > 0)
		_lstItems->setRowColor(_sel, Palette::blockOffset(13));
	else
	{
		_lstItems->setRowColor(_sel, Palette::blockOffset(13)+10);

		if (_sel > _itemOffset)
		{
			RuleItem* rule = _game->getRuleset()->getItem(_items[_sel - _itemOffset]);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, Palette::blockOffset(15)+6);
			}
		}
	}

	if (_total > 0)
		_btnOk->setVisible(true);
	else
		_btnOk->setVisible(false);
}

}
