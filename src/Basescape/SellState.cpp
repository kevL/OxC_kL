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

#include "SellState.h"

//#include <climits>
//#include <cmath>
//#include <iomanip>
//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param base		- pointer to the Base to get info from
 * @param origin	- game section that originated this state (default OPT_GEOSCAPE)
 */
SellState::SellState(
		Base* base,
		OptionsOrigin origin)
	:
		_base(base),
		_origin(origin),
		_sel(0),
		_rowOffset(0),
		_total(0),
		_hasSci(0),
		_hasEng(0),
		_spaceChange(0.)
{
	bool overfull = Options::storageLimitsEnforced == true
				 && _base->storesOverfull() == true;

	_window			= new Window(this, 320,200);

	_txtTitle		= new Text(310, 17, 5, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);
	_txtSpaceUsed	= new Text(85, 9, 219, 9);

	_txtFunds		= new Text(140, 9, 16, 24);
	_txtSales		= new Text(140, 9, 160, 24);

	_txtItem		= new Text(30, 9, 16, 33);
	_txtQuantity	= new Text(54, 9, 166, 33);
	_txtSell		= new Text(20, 9, 226, 33);
	_txtValue		= new Text(40, 9, 248, 33);

	_lstItems		= new TextList(285, 129, 16, 44);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("sellMenu");

	_ammoColor = static_cast<Uint8>(_game->getRuleset()->getInterface("sellMenu")->getElement("ammoColor")->color);
	_colorArtefact = Palette::blockOffset(13)+5;

	add(_window,		"window",	"sellMenu");
	add(_txtTitle,		"text",		"sellMenu");
	add(_txtBaseLabel,	"text",		"sellMenu");
	add(_txtFunds,		"text",		"sellMenu");
	add(_txtSales,		"text",		"sellMenu");
	add(_txtItem,		"text",		"sellMenu");
	add(_txtSpaceUsed,	"text",		"sellMenu");
	add(_txtQuantity,	"text",		"sellMenu");
	add(_txtSell,		"text",		"sellMenu");
	add(_txtValue,		"text",		"sellMenu");
	add(_lstItems,		"list",		"sellMenu");
	add(_btnCancel,		"button",	"sellMenu");
	add(_btnOk,			"button",	"sellMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)& SellState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SellState::btnOkClick,
					Options::keyOk);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& SellState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SellState::btnCancelClick,
					Options::keyCancel);
	if (overfull == true)
		_btnCancel->setVisible(false);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtSales->setText(tr("STR_VALUE_OF_SALES")
						.arg(Text::formatFunding(_total)));

	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);
	_txtSpaceUsed->setAlign(ALIGN_RIGHT);
	std::wostringstream woststr;
	woststr << _base->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtSpaceUsed->setText(woststr.str());

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSell->setText(tr("STR_SELL_SACK"));

	_txtValue->setText(tr("STR_VALUE"));

	_lstItems->setBackground(_window);
	_lstItems->setArrowColumn(182, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142, 60, 22, 53);
	_lstItems->setSelectable();
	_lstItems->setMargin();
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstItems->onMousePress((ActionHandler)& SellState::lstItemsMousePress);
	_lstItems->onLeftArrowPress((ActionHandler)& SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& SellState::lstItemsRightArrowClick);

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == NULL)
		{
			_sellQty.push_back(0);
			_soldiers.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName().c_str(),
							L"1",
							L"0",
							Text::formatFunding(0).c_str());
			++_rowOffset;
		}
	}

	for (std::vector<Craft*>::const_iterator
			i = _base->getCrafts()->begin();
			i != _base->getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT")
		{
			_sellQty.push_back(0);
			_crafts.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName(_game->getLanguage()).c_str(),
							L"1",
							L"0",
							Text::formatFunding((*i)->getRules()->getSellCost()).c_str());
			++_rowOffset;
		}
	}

	if (_base->getScientists() > 0)
	{
		_hasSci = 1;
		_sellQty.push_back(0);

		std::wostringstream woststr;
		woststr << _base->getScientists();
		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						woststr.str().c_str(),
						L"0",
						Text::formatFunding(0).c_str());
		++_rowOffset;
	}

	if (_base->getEngineers() > 0)
	{
		_hasEng = 1;
		_sellQty.push_back(0);

		std::wostringstream woststr;
		woststr << _base->getEngineers();
		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						woststr.str().c_str(),
						L"0",
						Text::formatFunding(0).c_str());
		++_rowOffset;
	}


	const SavedGame* const gameSave = _game->getSavedGame();
	const Ruleset* const rules = _game->getRuleset();
	const RuleItem
		* itRule,
		* laRule,
		* clRule;
	const RuleCraftWeapon* cwRule;

	const std::vector<std::string>& itemList = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		int qty = _base->getItems()->getItem(*i);

/*		if (Options::storageLimitsEnforced == true
			&& origin == OPT_BATTLESCAPE)
		{
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getItems() == *i)
					qty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator
					j = _base->getCrafts()->begin();
					j != _base->getCrafts()->end();
					++j)
			{
				qty += (*j)->getItems()->getItem(*i);
			}
		} */

		if (qty > 0
			&& (Options::canSellLiveAliens == true
				|| rules->getItem(*i)->isAlien() == false))
		{
			_sellQty.push_back(0);
			_items.push_back(*i);

			std::wstring item = tr(*i);

			itRule = rules->getItem(*i);
			//Log(LOG_INFO) << (*i) << " list# " << itRule->getListOrder(); // Prints listOrder to LOG.

			bool craftOrdnance = false;
			const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					j = cwList.begin();
					j != cwList.end()
						&& craftOrdnance == false;
					++j)
			{
				// Special handling for treating craft weapons as items
				cwRule = rules->getCraftWeapon(*j);

				laRule = rules->getItem(cwRule->getLauncherItem());
				clRule = rules->getItem(cwRule->getClipItem());

				if (laRule == itRule)
				{
					craftOrdnance = true;

					const int clipSize = cwRule->getAmmoMax(); // Launcher
					if (clipSize > 0)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
				}
				else if (clRule == itRule)
				{
					craftOrdnance = true;

					const int clipSize = clRule->getClipSize(); // launcher Ammo
					if (clipSize > 1)
						item += (L"s (" + Text::formatNumber(clipSize) + L")");
				}
			}

			Uint8 color;

			if ((itRule->getBattleType() == BT_AMMO			// #2 - weapon clips & HWP rounds
					|| (itRule->getBattleType() == BT_NONE	// #0 - craft weapon rounds
						&& itRule->getClipSize() > 0))
				&& itRule->getType() != "STR_ELERIUM_115")
			{
				if (itRule->getBattleType() == BT_AMMO
					&& itRule->getType().substr(0,8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					const int clipSize = itRule->getClipSize();
					if (clipSize > 1)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
				}
				item.insert(0, L"  ");

				color = _ammoColor;
			}
			else
			{
                if (itRule->isFixed() == true // tank w/ Ordnance.
					&& itRule->getCompatibleAmmo()->empty() == false)
                {
					const RuleItem* const ammoRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					const int clipSize = ammoRule->getClipSize();
					if (clipSize > 0)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
                }

				color = _lstItems->getColor();
			}

			if (gameSave->isResearched(itRule->getType()) == false				// not researched or research exempt
				&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(*i)->isAlien() == true						// or is an alien
					|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
					|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
				&& craftOrdnance == false)										// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = _colorArtefact;
			}

			std::wostringstream woststr;
			woststr << qty;

			_lstItems->addRow(
							4,
							item.c_str(),
							woststr.str().c_str(),
							L"0",
							Text::formatFunding(itRule->getSellCost()).c_str());
			_lstItems->setRowColor(
								_sellQty.size() - 1,
								color);
		}
	}


	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)& SellState::increase);

	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)& SellState::decrease);
}

/**
 * dTor.
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

	_timerInc->think(this, NULL);
	_timerDec->think(this, NULL);
}

/**
 * Sells the selected items.
 * @param action - pointer to an Action
 */
void SellState::btnOkClick(Action*)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _total);
	_base->setCashIncome(_total);

	for (size_t
			i = 0;
			i != _sellQty.size();
			++i)
	{
		if (_sellQty[i] > 0)
		{
			switch (getType(i))
			{
				case SELL_SOLDIER:
					for (std::vector<Soldier*>::const_iterator
							j = _base->getSoldiers()->begin();
							j != _base->getSoldiers()->end();
							++j)
					{
						if (*j == _soldiers[i])
						{
							if ((*j)->getArmor()->getStoreItem() != "STR_NONE")
								_base->getItems()->addItem((*j)->getArmor()->getStoreItem());

							_base->getSoldiers()->erase(j);
							break;
						}
					}

					delete _soldiers[i];
				break;

				case SELL_CRAFT:
				{
					Craft* const craft = _crafts[getCraftIndex(i)];

					for (std::vector<CraftWeapon*>::const_iterator // remove weapons from craft
							j = craft->getWeapons()->begin();
							j != craft->getWeapons()->end();
							++j)
					{
						if (*j != NULL)
						{
							_base->getItems()->addItem((*j)->getRules()->getLauncherItem());
							_base->getItems()->addItem(
													(*j)->getRules()->getClipItem(),
													(*j)->getClipsLoaded(_game->getRuleset()));
						}
					}

					for (std::map<std::string, int>::const_iterator // remove items from craft
							j = craft->getItems()->getContents()->begin();
							j != craft->getItems()->getContents()->end();
							++j)
					{
						_base->getItems()->addItem(
												j->first,
												j->second);
					}

					for (std::vector<Vehicle*>::const_iterator
							j = craft->getVehicles()->begin();
							j != craft->getVehicles()->end();
							++j)
					{
						_base->getItems()->addItem((*j)->getRules()->getType());

						const RuleItem* const ammoRule = _game->getRuleset()->getItem((*j)->getRules()->getCompatibleAmmo()->front());
						_base->getItems()->addItem(
												ammoRule->getType(),
												(*j)->getAmmo());

						delete *j;
//						craft->getVehicles()->erase(j);
					}

					for (std::vector<Soldier*>::const_iterator // remove soldiers from craft
							j = _base->getSoldiers()->begin();
							j != _base->getSoldiers()->end();
							++j)
					{
						if ((*j)->getCraft() == craft)
							(*j)->setCraft(NULL);
					}

					for (std::vector<BaseFacility*>::const_iterator // clear craft from hangar
							j = _base->getFacilities()->begin();
							j != _base->getFacilities()->end();
							++j)
					{
						if ((*j)->getCraft() == craft)
						{
							(*j)->setCraft(NULL);
							break;
						}
					}

					for (std::vector<Craft*>::const_iterator // remove craft
							j = _base->getCrafts()->begin();
							j != _base->getCrafts()->end();
							++j)
					{
						if (*j == craft)
						{
							_base->getCrafts()->erase(j);
							break;
						}
					}
					delete craft;
				}
				break;

				case SELL_SCIENTIST:
					_base->setScientists(_base->getScientists() - _sellQty[i]);
				break;

				case SELL_ENGINEER:
					_base->setEngineers(_base->getEngineers() - _sellQty[i]);
				break;

				case SELL_ITEM:
/*					if (_base->getItems()->getItem(_items[getItemIndex(i)]) < _sellQty[i])
					{
						const std::string item = _items[getItemIndex(i)];
						int toRemove = _sellQty[i] - _base->getItems()->getItem(item);

						_base->getItems()->removeItem( // remove all of said items from base
													item,
													std::numeric_limits<int>::max());

						// if you still need to remove any, remove them from the crafts first, and keep a running tally
						for (std::vector<Craft*>::const_iterator
								j = _base->getCrafts()->begin();
								j != _base->getCrafts()->end()
									&& toRemove != 0;
								++j)
						{
							if ((*j)->getItems()->getItem(item) < toRemove)
							{
								toRemove -= (*j)->getItems()->getItem(item);

								(*j)->getItems()->removeItem(
														item,
														std::numeric_limits<int>::max());
							}
							else
							{
								(*j)->getItems()->removeItem(
														item,
														toRemove);
								toRemove = 0;
							}
						}

						// if there are STILL any left to remove, take them from the transfers, and if necessary, delete it.
						for (std::vector<Transfer*>::const_iterator
								j = _base->getTransfers()->begin();
								j != _base->getTransfers()->end()
									&& toRemove != 0;
								)
						{
							if ((*j)->getItems() == item)
							{
								if ((*j)->getQuantity() <= toRemove)
								{
									toRemove -= (*j)->getQuantity();

									delete *j;
									j = _base->getTransfers()->erase(j);
								}
								else
								{
									(*j)->setItems(
												(*j)->getItems(),
												(*j)->getQuantity() - toRemove);
									toRemove = 0;
								}
							}
							else
								++j;
						}
					}
					else */
					_base->getItems()->removeItem(
												_items[getItemIndex(i)],
												_sellQty[i]);
			}
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SellState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsLeftArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerInc->isRunning() == false)
	{
		_timerInc->start();
	}
}

/**
 * Stops increasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the selected item; by one on left-click, to max on right-click.
 * @param action - pointer to an Action
 */
void SellState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		changeByValue(std::numeric_limits<int>::max(), 1);
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if ((SDL_GetModState() & KMOD_CTRL) != 0)
			changeByValue(10,1);
		else
			changeByValue(1,1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsRightArrowPress(Action* action)
{
	_sel = _lstItems->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerDec->isRunning() == false)
	{
		_timerDec->start();
	}
}

/**
 * Stops decreasing the item.
 * @param action - pointer to an Action
 */
void SellState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the selected item; by one on left-click, to 0 on right-click.
 * @param action - pointer to an Action
 */
void SellState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		changeByValue(std::numeric_limits<int>::max(), -1);
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if ((SDL_GetModState() & KMOD_CTRL) != 0)
			changeByValue(10,-1);
		else
			changeByValue(1,-1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Gets the price of the currently selected item.
 * @return, price of the selected item
 */
int SellState::getPrice() // private.
{
	switch (getType(_sel))
	{
		case SELL_ITEM:
			return _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSellCost();

		case SELL_CRAFT:
			return _crafts[getCraftIndex(_sel)]->getRules()->getSellCost();
	}

	return 0;
}

/**
 * Gets the quantity of the currently selected item on the base.
 * @return, quantity of selected item on the base
 */
int SellState::getQuantity() // private.
{
	int qty = 0;

	switch (getType(_sel))
	{
		case SELL_SOLDIER: // soldiers/crafts are sacked/sold singly.
		case SELL_CRAFT:
			return 1;

		case SELL_SCIENTIST:
			return _base->getScientists();

		case SELL_ENGINEER:
			return _base->getEngineers();

		case SELL_ITEM:
			qty = _base->getItems()->getItem(_items[getItemIndex(_sel)]);

			if (Options::storageLimitsEnforced == true
				&& _origin == OPT_BATTLESCAPE)
			{
				for (std::vector<Transfer*>::const_iterator
						j = _base->getTransfers()->begin();
						j != _base->getTransfers()->end();
						++j)
				{
					if ((*j)->getItems() == _items[getItemIndex(_sel)])
						qty += (*j)->getQuantity();
				}

				for (std::vector<Craft*>::const_iterator
						j = _base->getCrafts()->begin();
						j != _base->getCrafts()->end();
						++j)
				{
					qty += (*j)->getItems()->getItem(_items[getItemIndex(_sel)]);
				}
			}

			return qty;
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

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		changeByValue(10,1);
	else
		changeByValue(1,1);
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(80);
	_timerDec->setInterval(80);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		changeByValue(10,-1);
	else
		changeByValue(1,-1);
}

/**
 * Increases or decreases the quantity of the selected item to sell.
 * @param change	- how much to add or remove
 * @param dir		- direction to change; +1 to increase or -1 to decrease
 */
void SellState::changeByValue(
		int change,
		int dir)
{
	if (change < 1)
		return;

	if (dir > 0)
	{
		if (_sellQty[_sel] >= getQuantity())
			return;

		change = std::min(
						change,
						getQuantity() - _sellQty[_sel]);
	}
	else
	{
		if (_sellQty[_sel] < 1)
			return;

		change = std::min(
						change,
						_sellQty[_sel]);
	}

	_sellQty[_sel] += change * dir;
	_total += getPrice() * change * dir;

	const RuleItem* itRule;
	Craft* craft;
	double space = 0.;

	switch (getType(_sel)) // Calculate the change in storage space.
	{
		case SELL_SOLDIER:
			if (_soldiers[_sel]->getArmor()->getStoreItem() != "STR_NONE")
			{
				itRule = _game->getRuleset()->getItem(_soldiers[_sel]->getArmor()->getStoreItem());
				_spaceChange += static_cast<double>(dir) * itRule->getSize();
			}
		break;

		case SELL_CRAFT:
			craft = _crafts[getCraftIndex(_sel)];
			for (std::vector<CraftWeapon*>::const_iterator
					i = craft->getWeapons()->begin();
					i != craft->getWeapons()->end();
					++i)
			{
				if (*i != NULL) // kL, Cheers
				{
					itRule = _game->getRuleset()->getItem((*i)->getRules()->getLauncherItem());
					space += itRule->getSize();

					itRule = _game->getRuleset()->getItem((*i)->getRules()->getClipItem());
					if (itRule != NULL)
						space += static_cast<double>((*i)->getClipsLoaded(_game->getRuleset())) * itRule->getSize();
				}
			}
			_spaceChange += static_cast<double>(dir) * space;
		break;

		case SELL_ITEM:
			itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			_spaceChange -= static_cast<double>(dir * change) * itRule->getSize();
//		break;

//		case SELL_ENGINEER:
//		case SELL_SCIENTIST:
//		default:
//		break;
	}

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::updateItemStrings() // private.
{
	std::wostringstream
		woststr1,
		woststr2,
		woststr3;

	woststr1 << getQuantity() - _sellQty[_sel];
	_lstItems->setCellText(_sel, 1, woststr1.str());

	woststr2 << _sellQty[_sel];
	_lstItems->setCellText(_sel, 2, woststr2.str());

	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatFunding(_total)));


	Uint8 color = _lstItems->getColor();

	if (_sellQty[_sel] > 0)
		color = _lstItems->getSecondaryColor();
	else
	{
		if (_sel > _rowOffset)
		{
			const Ruleset* const rules = _game->getRuleset();
			const RuleItem* const itRule = rules->getItem(_items[_sel - _rowOffset]);

			bool craftOrdnance = false;
			const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					i = cwList.begin();
					i != cwList.end()
						&& craftOrdnance == false;
					++i)
			{
				const RuleCraftWeapon* const cwRule = rules->getCraftWeapon(*i);
				if (itRule == rules->getItem(cwRule->getLauncherItem())
					|| itRule == rules->getItem(cwRule->getClipItem()))
				{
					craftOrdnance = true;
				}
			}

			const SavedGame* const savedGame = _game->getSavedGame();
			if (savedGame->isResearched(itRule->getType()) == false				// not researched or is research exempt
				&& (savedGame->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(itRule->getType())->isAlien() == true			// or is an alien
					|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
					|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
				&& craftOrdnance == false)										// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = _colorArtefact;
			}
			else if (itRule->getBattleType() == BT_AMMO
				|| (itRule->getBattleType() == BT_NONE
					&& itRule->getClipSize() > 0))
			{
				color = _ammoColor;
			}
		}
	}

	_lstItems->setRowColor(
						_sel,
						color);


	bool okBtn = false;

	if (_total > 0)
		okBtn = true;
	else // or craft, soldier, scientist, engineer.
	{
		for (size_t
				i = 0;
				i != _sellQty.size()
					&& okBtn == false;
				++i)
		{
			if (_sellQty[i] > 0)
			{
				switch (getType(i))
				{
					case SELL_CRAFT:
					case SELL_SOLDIER:
					case SELL_SCIENTIST:
					case SELL_ENGINEER:
						okBtn = true;
				}
			}
		}
	}

	woststr3 << _base->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	if (std::abs(_spaceChange) > 0.05)
	{
		woststr3 << "(";
		if (_spaceChange > 0.) woststr3 << "+";
		woststr3 << std::fixed << std::setprecision(1) << _spaceChange << ")";
	}
	_txtSpaceUsed->setText(woststr3.str());

	if (Options::storageLimitsEnforced == true)
		okBtn = okBtn
			&& _base->storesOverfull(_spaceChange) == false;

	_btnOk->setVisible(okBtn);
}

/**
 * Gets the Type of the selected item.
 * @param sel - currently selected item
 * @return, the type of the selected item
 */
SellType SellState::getType(const size_t sel) const // private.
{
	size_t cutoff = _soldiers.size();

	if (sel < cutoff)
		return SELL_SOLDIER;

	if (sel < (cutoff += _crafts.size()))
		return SELL_CRAFT;

	if (sel < (cutoff += _hasSci))
		return SELL_SCIENTIST;

	if (sel < (cutoff + _hasEng))
		return SELL_ENGINEER;

	return SELL_ITEM;
}

/**
 * Gets the index of the selected item.
 * @param sel - currently selected item
 * @return, index of the selected item
 */
size_t SellState::getItemIndex(const size_t sel) const // private.
{
	return sel
		 - _soldiers.size()
		 - _crafts.size()
		 - _hasSci
		 - _hasEng;
}

/**
 * Gets the index of selected craft.
 * @param sel - selected craft
 * @return, index of the selected craft
 */
size_t SellState::getCraftIndex(const size_t sel) const // private.
{
	return sel - _soldiers.size();
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 */
/*void SellState::lstItemsMousePress(Action* action)
{
	if (Options::changeValueByMouseWheel < 1)
		return;

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
} */

}
