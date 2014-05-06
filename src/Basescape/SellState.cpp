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

#include "SellState.h"

#include <climits>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param origin Game section that originated this state.
 */
SellState::SellState(
		Game* game,
		Base* base,
		OptionsOrigin origin)
	:
		State(game),
		_base(base),
		_qtys(),
		_soldiers(),
		_crafts(),
		_items(),
		_sel(0),
		_itemOffset(0),
		_total(0),
		_hasSci(0),
		_hasEng(0),
		_spaceChange(0.0)
{
	bool overfull = Options::storageLimitsEnforced
					&& _base->storesOverfull();

/*	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(overfull? 288:148, 16, overfull? 16:8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtSales = new Text(150, 9, 10, 24);
	_txtFunds = new Text(150, 9, 160, 24);
	_txtSpaceUsed = new Text(150, 9, 160, 34);
	_txtItem = new Text(130, 9, 10, Options::storageLimitsEnforced? 44:33);
	_txtQuantity = new Text(54, 9, 126, Options::storageLimitsEnforced? 44:33);
	_txtSell = new Text(96, 9, 180, Options::storageLimitsEnforced? 44:33);
	_txtValue = new Text(40, 9, 260, Options::storageLimitsEnforced? 44:33);
	_lstItems = new TextList(287, Options::storageLimitsEnforced? 112:120, 8, Options::storageLimitsEnforced? 55:44); */
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(310, 17, 5, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);

	_txtFunds		= new Text(140, 9, 16, 24);
	_txtSales		= new Text(140, 9, 160, 24);

	_txtItem		= new Text(30, 9, 16, 33);
	_txtSpaceUsed	= new Text(85, 9, 70, 33);
	_txtQuantity	= new Text(54, 9, 166, 33);
	_txtSell		= new Text(20, 9, 226, 33);
	_txtValue		= new Text(40, 9, 248, 33);

	_lstItems		= new TextList(285, 128, 16, 44);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	if (origin == OPT_BATTLESCAPE)
	{
		setPalette("PAL_GEOSCAPE", 0);

		_color		= Palette::blockOffset(15)-1;
		_color2		= Palette::blockOffset(8)+10;
		_color3		= Palette::blockOffset(8)+5;
		_colorAmmo	= Palette::blockOffset(15)+6;
	}
	else
	{
		setPalette("PAL_BASESCAPE", 0);

		_color		= Palette::blockOffset(13)+10;
		_color2		= Palette::blockOffset(13);
		_color3		= Palette::blockOffset(13)+5;
		_colorAmmo	= Palette::blockOffset(15)+6;
	}

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtFunds);
	add(_txtSales);
	add(_txtItem);
	add(_txtSpaceUsed);
	add(_txtQuantity);
	add(_txtSell);
	add(_txtValue);
	add(_lstItems);
	add(_btnCancel);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(_color);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(_color);
	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)& SellState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SellState::btnOkClick,
					Options::keyOk);
	_btnOk->setVisible(false); // kL

	_btnCancel->setColor(_color);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& SellState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SellState::btnCancelClick,
					Options::keyCancel);
	if (overfull)
	{
		_btnCancel->setVisible(false);
//kL		_btnOk->setVisible(false);
	}

	_txtTitle->setColor(_color);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtBaseLabel->setColor(_color);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtSales->setColor(_color);
	_txtSales->setSecondaryColor(_color2);
	_txtSales->setText(tr("STR_VALUE_OF_SALES")
						.arg(Text::formatFunding(_total)));

	_txtFunds->setColor(_color);
	_txtFunds->setSecondaryColor(_color2);
	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtItem->setColor(_color);
	_txtItem->setText(tr("STR_ITEM"));

	_txtSpaceUsed->setColor(_color);
	_txtSpaceUsed->setSecondaryColor(_color2);
	_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);
	std::wostringstream ss1;
	ss1 << static_cast<int>(_base->getUsedStores()) << ":" << _base->getAvailableStores();
	_txtSpaceUsed->setText(ss1.str());
//kL	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss1.str()));

	_txtQuantity->setColor(_color);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSell->setColor(_color);
	_txtSell->setText(tr("STR_SELL_SACK"));

	_txtValue->setColor(_color);
	_txtValue->setText(tr("STR_VALUE"));

	_lstItems->setColor(_color);
	_lstItems->setArrowColumn(182, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142, 60, 22, 53);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(8);
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstItems->onLeftArrowPress((ActionHandler)& SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& SellState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)& SellState::lstItemsMousePress);

	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == 0)
		{
			_qtys.push_back(0);
			_soldiers.push_back(*i);
			_lstItems->addRow(
							4,
							(*i)->getName(true).c_str(),
							L"1",
							L"0",
							Text::formatFunding(0).c_str());

			++_itemOffset;
		}
	}

	for (std::vector<Craft*>::iterator
			i = _base->getCrafts()->begin();
			i != _base->getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT")
		{
			_qtys.push_back(0);
			_crafts.push_back(*i);
			_lstItems->addRow(
							4,
							(*i)->getName(_game->getLanguage()).c_str(),
							L"1",
							L"0",
							Text::formatFunding((*i)->getRules()->getSellCost()).c_str());

			++_itemOffset;
		}
	}

//kL	if (_base->getAvailableScientists() > 0)
	if (_base->getScientists() > 0) // kL
	{
		_qtys.push_back(0);
		_hasSci = 1;
		std::wostringstream ss;
//kL		ss << _base->getAvailableScientists();
		ss << _base->getScientists(); // kL
		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						ss.str().c_str(),
						L"0",
						Text::formatFunding(0).c_str());

		++_itemOffset;
	}

//kL	if (_base->getAvailableEngineers() > 0)
	if (_base->getEngineers() > 0) // kL
	{
		_qtys.push_back(0);
		_hasEng = 1;
		std::wostringstream ss;
//kL		ss << _base->getAvailableEngineers();
		ss << _base->getEngineers(); // kL
		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						ss.str().c_str(),
						L"0",
						Text::formatFunding(0).c_str());

		++_itemOffset;
	}

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		int qty = _base->getItems()->getItem(*i);
		if (qty > 0
			&& (Options::canSellLiveAliens
				|| !_game->getRuleset()->getItem(*i)->getAlien()))
		{
			_qtys.push_back(0);
			_items.push_back(*i);
			RuleItem* rule = _game->getRuleset()->getItem(*i);
			std::wostringstream ss;
			ss << qty;

			std::wstring item = tr(*i);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				item.insert(0, L"  ");
				_lstItems->addRow(
								4,
								item.c_str(),
								ss.str().c_str(),
								L"0",
								Text::formatFunding(rule->getSellCost()).c_str());
				_lstItems->setRowColor(_qtys.size() - 1, _colorAmmo);
			}
			else
			{
				_lstItems->addRow(
								4,
								item.c_str(),
								ss.str().c_str(),
								L"0",
								Text::formatFunding(rule->getSellCost()).c_str());
			}
		}
	}

	_timerInc = new Timer(280);
	_timerInc->onTimer((StateHandler)& SellState::increase);

	_timerDec = new Timer(280);
	_timerDec->onTimer((StateHandler)& SellState::decrease);
}

/**
 *
 */
SellState::~SellState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void SellState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Gets the index of selected craft.
 * @param selected Selected craft.
 * @return Index of the selected craft.
 */
int SellState::getCraftIndex(size_t selected) const
{
	return selected - _soldiers.size();
}

/**
 * Sells the selected items.
 * @param action Pointer to an action.
 */
void SellState::btnOkClick(Action*)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _total);
	_base->setCashIncome(_total); // kL

	for (size_t
			i = 0;
			i < _qtys.size();
			++i)
	{
		if (_qtys[i] > 0)
		{
			switch (getType(i))
			{
				case SELL_SOLDIER:
					for (std::vector<Soldier*>::iterator
							s = _base->getSoldiers()->begin();
							s != _base->getSoldiers()->end();
							++s)
					{
						if (*s == _soldiers[i])
						{
							if ((*s)->getArmor()->getStoreItem() != "STR_NONE")
							{
								_base->getItems()->addItem((*s)->getArmor()->getStoreItem());
							}

							_base->getSoldiers()->erase(s);

							break;
						}
					}

					delete _soldiers[i];
				break;
				case SELL_CRAFT:
				{
					Craft* craft = _crafts[getCraftIndex(i)];

					// Remove weapons from craft
					for (std::vector<CraftWeapon*>::iterator
							w = craft->getWeapons()->begin();
							w != craft->getWeapons()->end();
							++w)
					{
						if (*w != 0)
						{
							_base->getItems()->addItem((*w)->getRules()->getLauncherItem());
							_base->getItems()->addItem(
													(*w)->getRules()->getClipItem(),
													(*w)->getClipsLoaded(_game->getRuleset()));
						}
					}

					// Remove items from craft
					for (std::map<std::string, int>::iterator
							it = craft->getItems()->getContents()->begin();
							it != craft->getItems()->getContents()->end();
							++it)
					{
						_base->getItems()->addItem(it->first, it->second);
					}

					// Remove soldiers from craft
					for (std::vector<Soldier*>::iterator
							s = _base->getSoldiers()->begin();
							s != _base->getSoldiers()->end();
							++s)
					{
						if ((*s)->getCraft() == craft)
						{
							(*s)->setCraft(0);
						}
					}

					// Clear Hangar
					for (std::vector<BaseFacility*>::iterator
							f = _base->getFacilities()->begin();
							f != _base->getFacilities()->end();
							++f)
					{
						if ((*f)->getCraft() == craft)
						{
							(*f)->setCraft(0);

							break;
						}
					}

					// Remove craft
					for (std::vector<Craft*>::iterator
							c = _base->getCrafts()->begin();
							c != _base->getCrafts()->end();
							++c)
					{
						if (*c == craft)
						{
							_base->getCrafts()->erase(c);

							break;
						}
					}

					delete craft;
				}
				break;
				case SELL_SCIENTIST:
					_base->setScientists(_base->getScientists() - _qtys[i]);
				break;
				case SELL_ENGINEER:
					_base->setEngineers(_base->getEngineers() - _qtys[i]);
				break;
				case SELL_ITEM:
					_base->getItems()->removeItem(_items[getItemIndex(i)], _qtys[i]);
				break;
			}
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SellState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowPress(Action* action)
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
void SellState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected item;
 * by one on left-click, to max on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		changeByValue(INT_MAX, 1);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1, 1);

		_timerInc->setInterval(280);
		_timerDec->setInterval(280);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowPress(Action* action)
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
void SellState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected item;
 * by one on left-click, to 0 on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		changeByValue(INT_MAX, -1);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1, -1);

		_timerInc->setInterval(280);
		_timerDec->setInterval(280);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void SellState::lstItemsMousePress(Action* action)
{
	if (Options::changeValueByMouseWheel < 1)	// kL
		return;									// kL

	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteXMouse()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteXMouse()) <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, 1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteXMouse()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteXMouse()) <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, -1);
		}
	}
}

/**
 * Gets the price of the currently selected item.
 * @return Price of the selected item.
 */
int SellState::getPrice()
{
	switch (getType(_sel))
	{
		case SELL_SOLDIER: // Personnel/craft aren't worth anything
		case SELL_ENGINEER:
		case SELL_SCIENTIST:
			return 0;

		case SELL_ITEM:
			return _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSellCost();

		case SELL_CRAFT:
			Craft* craft = _crafts[getCraftIndex(_sel)];

			return craft->getRules()->getSellCost();
	}

    return 0;
}

/**
 * Gets the quantity of the currently selected item on the base.
 * @return Quantity of selected item on the base.
 */
int SellState::getQuantity()
{
	switch (getType(_sel))
	{
		case SELL_SOLDIER: // Soldiers/crafts are sold singly.
		case SELL_CRAFT:
			return 1;

		case SELL_SCIENTIST:
//kL			return _base->getAvailableScientists();
			return _base->getScientists(); // kL

		case SELL_ENGINEER:
//kL			return _base->getAvailableEngineers();
			return _base->getEngineers(); // kL

		case SELL_ITEM:
			return _base->getItems()->getItem(_items[getItemIndex(_sel)]);
	}

	return 0;
}

/**
 * Increases the quantity of the selected item to sell by one.
 */
void SellState::increase()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	changeByValue(1, 1);
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(80);
	_timerDec->setInterval(80);

	changeByValue(1, -1);
}

/**
 * Increases or decreases the quantity of the selected item to sell.
 * @param change How much we want to add or remove.
 * @param dir Direction to change, +1 to increase or -1 to decrease.
 */
void SellState::changeByValue(
		int change,
		int dir)
{
	//Log(LOG_INFO) << "changeByValue()";

	if (change < 1)
		return;

	if (dir > 0)
	{
		if (_qtys[_sel] >= getQuantity())
			return;

		change = std::min(
						getQuantity() - _qtys[_sel],
						change);
	}
	else
	{
		if (_qtys[_sel] < 1)
			return;

		change = std::min(
						_qtys[_sel],
						change);
	}

	_qtys[_sel] += change * dir;
	_total += getPrice() * change * dir;

	Craft* craft;
	RuleItem
		* armorRule,
		* itemRule,
		* weapRule,
		* ammoRule;
	double space = 0.0;

	switch (getType(_sel)) // Calculate the change in storage space.
	{
		case SELL_SOLDIER:
			if (_soldiers[_sel]->getArmor()->getStoreItem() != "STR_NONE")
			{
				armorRule = _game->getRuleset()->getItem(_soldiers[_sel]->getArmor()->getStoreItem());
				_spaceChange += static_cast<double>(dir) * armorRule->getSize();
			}
		break;
		case SELL_CRAFT:
			//Log(LOG_INFO) << ". SELL_CRAFT";

			craft = _crafts[getCraftIndex(_sel)];
			//Log(LOG_INFO) << ". craft = " << getCraftIndex(_sel);

			for (std::vector<CraftWeapon*>::iterator
					w = craft->getWeapons()->begin();
					w != craft->getWeapons()->end();
					++w)
			{
				//Log(LOG_INFO) << ". . iter";

				if (*w != 0) // kL, Cheers
				{
					weapRule = _game->getRuleset()->getItem((*w)->getRules()->getLauncherItem());
					//Log(LOG_INFO) << ". . weapRule done";

//					if (weapRule) // kL, but shouldn't beneeded.
					space += weapRule->getSize();
					//Log(LOG_INFO) << ". . space[1] = " << space;

					ammoRule = _game->getRuleset()->getItem((*w)->getRules()->getClipItem());
					//Log(LOG_INFO) << ". . ammoRule done";

					if (ammoRule)
						space += static_cast<double>((*w)->getClipsLoaded(_game->getRuleset())) * ammoRule->getSize();
					//Log(LOG_INFO) << ". . space[2] = " << space;
				}
			}
			//Log(LOG_INFO) << ". iter done";

			_spaceChange += static_cast<double>(dir) * space;
			//Log(LOG_INFO) << ". SELL_CRAFT done";
		break;
		case SELL_ITEM:
			itemRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			_spaceChange -= static_cast<double>(dir * change) * itemRule->getSize();
		break;

//		case SELL_ENGINEER:
//		case SELL_SCIENTIST:
		default:
		break;
	}

	updateItemStrings();
	//Log(LOG_INFO) << "changeByValue() EXIT";
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::updateItemStrings()
{
	//Log(LOG_INFO) << "updateItemStrings()";
	std::wostringstream
		ss,
		ss2,
		ss3;

	ss << getQuantity() - _qtys[_sel];
	_lstItems->setCellText(_sel, 1, ss.str());

	ss2 << _qtys[_sel];
	_lstItems->setCellText(_sel, 2, ss2.str());

	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatFunding(_total)));

	if (_qtys[_sel] > 0)
		_lstItems->setRowColor(_sel, _color2);
	else
	{
		_lstItems->setRowColor(_sel, _color);

		if (_sel > _itemOffset)
		{
			RuleItem* rule = _game->getRuleset()->getItem(_items[_sel - _itemOffset]);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, _colorAmmo);
			}
		}
	}

	// kL_begin:
	bool okVis = false;

	if (_total > 0)
		okVis = true;
	else // or craft, soldier, scientist, engineer.
	{
		for (size_t
				i = 0;
				i < _qtys.size()
					&& !okVis;
				++i)
		{
			if (_qtys[i] > 0)
			{
				switch (getType(static_cast<size_t>(i)))
				{
					case SELL_CRAFT:
					case SELL_SOLDIER:
					case SELL_SCIENTIST:
					case SELL_ENGINEER:
						okVis = true;
				}
			}
		}
	} // kL_end.

	ss3 << _base->getUsedStores();
	if (std::abs(_spaceChange) > 0.05)
	{
		ss3 << "(";
		if (_spaceChange > 0.0) ss3 << "+"; // kL, >.05 already established; that just re-introduced a rounding error:
//kL		if (_spaceChange > 0.05) ss3 << "+";
		ss3 << std::fixed << std::setprecision(1) << _spaceChange << ")";
	}
	ss3 << ":" << _base->getAvailableStores();
	_txtSpaceUsed->setText(ss3.str()); // kL
//kL	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss3.str()));

	if (Options::storageLimitsEnforced)
		okVis = okVis // kL
				&& !_base->storesOverfull(_spaceChange);

	_btnOk->setVisible(okVis); // kL

	//Log(LOG_INFO) << "updateItemStrings() EXIT";
}

/**
 * Gets the Type of the selected item.
 * @param selected Currently selected item.
 * @return The type of the selected item.
 */
enum SellType SellState::getType(size_t selected) const
{
	size_t max = _soldiers.size();

	if (selected < max)
		return SELL_SOLDIER;

	if (selected < (max += _crafts.size()))
		return SELL_CRAFT;

	if (selected < (max += _hasSci))
		return SELL_SCIENTIST;

	if (selected < (max += _hasEng))
		return SELL_ENGINEER;

	return SELL_ITEM;
}

/**
 * Gets the index of the selected item.
 * @param selected Currently selected item.
 * @return Index of the selected item.
 */
int SellState::getItemIndex(size_t selected) const
{
	return static_cast<int>(selected)
			- static_cast<int>(_soldiers.size())
			- static_cast<int>(_crafts.size())
			- _hasSci
			- _hasEng;
}

}
