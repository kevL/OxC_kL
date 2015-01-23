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

#include "../Ruleset/Armor.h"
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


namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param base		- pointer to the Base to get info from
 * @param origin	- game section that originated this state
 */
SellState::SellState(
		Base* base,
		OptionsOrigin origin)
	:
		_base(base),
		_sel(0),
		_itemOffset(0),
		_total(0),
		_hasSci(0),
		_hasEng(0),
		_spaceChange(0.),
		_origin(origin)
{
	bool overfull = Options::storageLimitsEnforced == true
				 && _base->storesOverfull() == true;

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
	_txtSpaceUsed	= new Text(85, 9, 219, 9);

	_txtFunds		= new Text(140, 9, 16, 24);
	_txtSales		= new Text(140, 9, 160, 24);

	_txtItem		= new Text(30, 9, 16, 33);
//	_txtSpaceUsed	= new Text(85, 9, 70, 33);
	_txtQuantity	= new Text(54, 9, 166, 33);
	_txtSell		= new Text(20, 9, 226, 33);
	_txtValue		= new Text(40, 9, 248, 33);

	_lstItems		= new TextList(285, 129, 16, 44);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("sellMenu")->getElement("palette")->color); //0
	if (origin == OPT_BATTLESCAPE)
	{
//		setPalette("PAL_GEOSCAPE", 0);
//		_color		= Palette::blockOffset(15)-1;
//		_color2		= Palette::blockOffset(8)+10;
		_colorArtefact = Palette::blockOffset(8)+5;
//		_colorAmmo	= Palette::blockOffset(15)+6;
	}
	else
	{
		setPalette("PAL_BASESCAPE", 0);
//		_color		= Palette::blockOffset(13)+10;
//		_color2		= Palette::blockOffset(13);
		_colorArtefact = Palette::blockOffset(13)+5;
//		_colorAmmo	= Palette::blockOffset(15)+6;
	}

	_ammoColor = _game->getRuleset()->getInterface("sellMenu")->getElement("ammoColor")->color;

	add(_window, "window", "sellMenu");
	add(_txtTitle, "text", "sellMenu");
	add(_txtBaseLabel, "text", "sellMenu");
	add(_txtFunds, "text", "sellMenu");
	add(_txtSales, "text", "sellMenu");
	add(_txtItem, "text", "sellMenu");
	add(_txtSpaceUsed, "text", "sellMenu");
	add(_txtQuantity, "text", "sellMenu");
	add(_txtSell, "text", "sellMenu");
	add(_txtValue, "text", "sellMenu");
	add(_lstItems, "list", "sellMenu");
	add(_btnCancel, "button", "sellMenu");
	add(_btnOk, "button", "sellMenu");

	centerAllSurfaces();


//	_window->setColor(_color);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

//	_btnOk->setColor(_color);
	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)& SellState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SellState::btnOkClick,
					Options::keyOk);
	_btnOk->setVisible(false);

//	_btnCancel->setColor(_color);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& SellState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SellState::btnCancelClick,
					Options::keyCancel);
	if (overfull == true)
		_btnCancel->setVisible(false);

//	_txtTitle->setColor(_color);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

//	_txtBaseLabel->setColor(_color);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

//	_txtSales->setColor(_color);
//	_txtSales->setSecondaryColor(_color2);
	_txtSales->setText(tr("STR_VALUE_OF_SALES")
						.arg(Text::formatFunding(_total)));

//	_txtFunds->setColor(_color);
//	_txtFunds->setSecondaryColor(_color2);
	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

//	_txtItem->setColor(_color);
	_txtItem->setText(tr("STR_ITEM"));

//	_txtSpaceUsed->setColor(_color);
//	_txtSpaceUsed->setSecondaryColor(_color2);
	_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);
	_txtSpaceUsed->setAlign(ALIGN_RIGHT);
	std::wostringstream ss1;
	ss1 << _base->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
//	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss1.str()));
	_txtSpaceUsed->setText(ss1.str());

//	_txtQuantity->setColor(_color);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

//	_txtSell->setColor(_color);
	_txtSell->setText(tr("STR_SELL_SACK"));

//	_txtValue->setColor(_color);
	_txtValue->setText(tr("STR_VALUE"));

//	_lstItems->setColor(_color);
	_lstItems->setBackground(_window);
	_lstItems->setArrowColumn(182, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142, 60, 22, 53);
	_lstItems->setSelectable();
	_lstItems->setMargin();
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstItems->onLeftArrowPress((ActionHandler)& SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& SellState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)& SellState::lstItemsMousePress);

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == NULL)
		{
			_qtys.push_back(0);
			_soldiers.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName().c_str(),
							L"1",
							L"0",
							Text::formatFunding(0).c_str());
			++_itemOffset;
		}
	}

	for (std::vector<Craft*>::const_iterator
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

	if (_base->getScientists() > 0)
	{
		_hasSci = 1;
		_qtys.push_back(0);

		std::wostringstream ss;
		ss << _base->getScientists();
		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						ss.str().c_str(),
						L"0",
						Text::formatFunding(0).c_str());
		++_itemOffset;
	}

	if (_base->getEngineers() > 0)
	{
		_hasEng = 1;
		_qtys.push_back(0);

		std::wostringstream ss;
		ss << _base->getEngineers();
		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						ss.str().c_str(),
						L"0",
						Text::formatFunding(0).c_str());
		++_itemOffset;
	}


	const SavedGame* const save = _game->getSavedGame();
	const Ruleset* const rules = _game->getRuleset();
	const RuleItem
		* rule,
		* launchRule,
		* clipRule;
	const RuleCraftWeapon* cwRule;

	const std::vector<std::string>& items = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		int qty = _base->getItems()->getItem(*i);

		if (Options::storageLimitsEnforced == true
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
		}

		if (qty > 0
			&& (Options::canSellLiveAliens == true
				|| rules->getItem(*i)->getAlien() == false))
		{
			_qtys.push_back(0);
			_items.push_back(*i);

			std::wstring item = tr(*i);

			rule = rules->getItem(*i);
			//Log(LOG_INFO) << (*i) << " list# " << rule->getListOrder(); // Prints listOrder to LOG.

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

				launchRule = rules->getItem(cwRule->getLauncherItem());
				clipRule = rules->getItem(cwRule->getClipItem());

				if (launchRule == rule)
				{
					craftOrdnance = true;

					const int clipSize = cwRule->getAmmoMax(); // Launcher
					if (clipSize > 0)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
				}
				else if (clipRule == rule)
				{
					craftOrdnance = true;

					const int clipSize = clipRule->getClipSize(); // launcher Ammo
					if (clipSize > 1)
						item += (L"s (" + Text::formatNumber(clipSize) + L")");
				}
			}

			Uint8 color;

			if ((rule->getBattleType() == BT_AMMO			// #2 - weapon clips & HWP rounds
					|| (rule->getBattleType() == BT_NONE	// #0 - craft weapon rounds
						&& rule->getClipSize() > 0))
				&& rule->getType() != "STR_ELERIUM_115")
			{
				if (rule->getBattleType() == BT_AMMO
					&& rule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					const int clipSize = rule->getClipSize();
					if (clipSize > 1)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
				}
				item.insert(0, L"  ");

				color = _ammoColor;
			}
			else
			{
                if (rule->isFixed() == true // tank w/ Ordnance.
					&& rule->getCompatibleAmmo()->empty() == false)
                {
					const RuleItem* const ammoRule = _game->getRuleset()->getItem(rule->getCompatibleAmmo()->front());
					const int clipSize = ammoRule->getClipSize();
					if (clipSize > 0)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
                }

				color = _lstItems->getColor();
			}

			if (save->isResearched(rule->getType()) == false				// not researched
				&& (save->isResearched(rule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(*i)->getAlien() == true					// or is an alien
					|| rule->getBattleType() == BT_CORPSE						// or is a corpse
					|| rule->getBattleType() == BT_NONE)						// or is not a battlefield item
				&& craftOrdnance == false										// and is not craft ordnance
				&& rule->isResearchExempt() == false)						// and is not research exempt
			{
				// well, that was !NOT! easy.
				color = _colorArtefact;
			}

			std::wostringstream ss;
			ss << qty;

			_lstItems->addRow(
							4,
							item.c_str(),
							ss.str().c_str(),
							L"0",
							Text::formatFunding(rule->getSellCost()).c_str());
			_lstItems->setRowColor(_qtys.size() - 1, color);
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
 * Gets the index of selected craft.
 * @param selected - selected craft
 * @return, index of the selected craft
 */
size_t SellState::getCraftIndex(size_t selected) const
{
	return selected - _soldiers.size();
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
			i < _qtys.size();
			++i)
	{
		if (_qtys[i] > 0)
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
						_base->getItems()->addItem(j->first, j->second);
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
					_base->setScientists(_base->getScientists() - _qtys[i]);
				break;

				case SELL_ENGINEER:
					_base->setEngineers(_base->getEngineers() - _qtys[i]);
				break;

				case SELL_ITEM:
					if (_base->getItems()->getItem(_items[getItemIndex(i)]) < _qtys[i])
					{
						const std::string item = _items[getItemIndex(i)];
						int toRemove = _qtys[i] - _base->getItems()->getItem(item);

						_base->getItems()->removeItem( // remove all of said items from base
													item,
													std::numeric_limits<int>::max());

						// if we still need to remove any, remove them from the crafts first, and keep a running tally
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
					else
						_base->getItems()->removeItem(
													_items[getItemIndex(i)],
													_qtys[i]);
				break;
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
			changeByValue(10, 1);
		else
			changeByValue(1, 1);

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
			changeByValue(10, -1);
		else
			changeByValue(1, -1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 */
void SellState::lstItemsMousePress(Action* action)
{
/*	if (Options::changeValueByMouseWheel < 1)
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
	} */
}

/**
 * Gets the price of the currently selected item.
 * @return, price of the selected item
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
			return _crafts[getCraftIndex(_sel)]->getRules()->getSellCost();
	}

	return 0;
}

/**
 * Gets the quantity of the currently selected item on the base.
 * @return, quantity of selected item on the base
 */
int SellState::getQuantity()
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
		changeByValue(10, 1);
	else
		changeByValue(1, 1);
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(80);
	_timerDec->setInterval(80);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		changeByValue(10, -1);
	else
		changeByValue(1, -1);
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

	const RuleItem* rule;
	Craft* craft;
	double space = 0.;

	switch (getType(_sel)) // Calculate the change in storage space.
	{
		case SELL_SOLDIER:
			if (_soldiers[_sel]->getArmor()->getStoreItem() != "STR_NONE")
			{
				rule = _game->getRuleset()->getItem(_soldiers[_sel]->getArmor()->getStoreItem());
				_spaceChange += static_cast<double>(dir) * rule->getSize();
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
					rule = _game->getRuleset()->getItem((*i)->getRules()->getLauncherItem());
					space += rule->getSize();

					rule = _game->getRuleset()->getItem((*i)->getRules()->getClipItem());
					if (rule != NULL)
						space += static_cast<double>((*i)->getClipsLoaded(_game->getRuleset())) * rule->getSize();
				}
			}
			_spaceChange += static_cast<double>(dir) * space;
		break;
		case SELL_ITEM:
			rule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			_spaceChange -= static_cast<double>(dir * change) * rule->getSize();
		break;

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
void SellState::updateItemStrings()
{
	std::wostringstream
		ss1,
		ss2,
		ss3;

	ss1 << getQuantity() - _qtys[_sel];
	_lstItems->setCellText(_sel, 1, ss1.str());

	ss2 << _qtys[_sel];
	_lstItems->setCellText(_sel, 2, ss2.str());

	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatFunding(_total)));

	if (_qtys[_sel] > 0)
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	else
	{
		if (_sel > _itemOffset)
		{
			const Ruleset* const rules = _game->getRuleset();
			const RuleItem* const rule = rules->getItem(_items[_sel - _itemOffset]);

			bool craftOrdnance = false;
			const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					i = cwList.begin();
					i != cwList.end()
						&& craftOrdnance == false;
					++i)
			{
				// Special handling for treating craft weapons as items
				const RuleCraftWeapon* const cwRule = rules->getCraftWeapon(*i);
				if (rule == rules->getItem(cwRule->getLauncherItem())
					|| rule == rules->getItem(cwRule->getClipItem()))
				{
					craftOrdnance = true;
				}
			}

			const SavedGame* const save = _game->getSavedGame();
			if (save->isResearched(rule->getType()) == false				// not researched
				&& (save->isResearched(rule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(rule->getType())->getAlien() == true		// or is an alien
					|| rule->getBattleType() == BT_CORPSE						// or is a corpse
					|| rule->getBattleType() == BT_NONE)						// or is not a battlefield item
				&& craftOrdnance == false										// and is not craft ordnance
				&& rule->isResearchExempt() == false)						// and is not research exempt
			{
				// well, that was !NOT! easy.
				_lstItems->setRowColor(_sel, _colorArtefact);
			}
			else
			{
				if (rule->getBattleType() == BT_AMMO
					|| (rule->getBattleType() == BT_NONE
						&& rule->getClipSize() > 0))
				{
					_lstItems->setRowColor(_sel, _ammoColor);
				}
				else
					_lstItems->setRowColor(_sel, _lstItems->getColor());
			}
		}
		else
			_lstItems->setRowColor(_sel, _lstItems->getColor());
	}


	bool okBtn = false;

	if (_total > 0)
		okBtn = true;
	else // or craft, soldier, scientist, engineer.
	{
		for (size_t
				i = 0;
				i < _qtys.size()
					&& okBtn == false;
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
						okBtn = true;
				}
			}
		}
	}

	ss3 << _base->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	if (std::abs(_spaceChange) > 0.05)
	{
		ss3 << "(";
		if (_spaceChange > 0.) ss3 << "+";
		ss3 << std::fixed << std::setprecision(1) << _spaceChange << ")";
	}
	_txtSpaceUsed->setText(ss3.str());
//	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss3.str()));

	if (Options::storageLimitsEnforced == true)
		okBtn = okBtn
			&& _base->storesOverfull(_spaceChange) == false;

	_btnOk->setVisible(okBtn);
}

/**
 * Gets the Type of the selected item.
 * @param selected - currently selected item
 * @return, the type of the selected item
 */
enum SellType SellState::getType(size_t selected) const
{
	size_t cutoff = _soldiers.size();

	if (selected < cutoff)
		return SELL_SOLDIER;

	if (selected < (cutoff += _crafts.size()))
		return SELL_CRAFT;

	if (selected < (cutoff += _hasSci))
		return SELL_SCIENTIST;

	if (selected < (cutoff += _hasEng))
		return SELL_ENGINEER;

	return SELL_ITEM;
}

/**
 * Gets the index of the selected item.
 * @param selected - currently selected item
 * @return, index of the selected item
 */
size_t SellState::getItemIndex(size_t selected) const
{
	return selected
		 - _soldiers.size()
		 - _crafts.size()
		 - static_cast<size_t>(_hasSci)
		 - static_cast<size_t>(_hasEng);
}

}
