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

#include "PurchaseState.h"

//#include <cfloat>
//#include <climits>
//#include <cmath>
//#include <iomanip>
//#include <sstream>
//#include "../fmath.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Purchase/Hire screen.
 * @param base - pointer to the Base to get info from
 */
PurchaseState::PurchaseState(Base* const base)
	:
		_base(base),
		_sel(0),
		_costTotal(0),
		_qtyPersonnel(0),
		_qtyCraft(0),
		_storeSize(0.)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(310, 17,   5, 9);
	_txtBaseLabel	= new Text( 80,  9,  16, 9);
	_txtStorage		= new Text( 85,  9, 219, 9);

	_txtFunds		= new Text(140, 9,  16, 24);
	_txtPurchases	= new Text(140, 9, 160, 24);

/*	_txtStorage = new Text(150, 9, 160, 34);
	_txtItem = new Text(140, 9, 10, Options::storageLimitsEnforced? 44:33);
	_txtCost = new Text(102, 9, 152, Options::storageLimitsEnforced? 44:33);
	_txtQuantity = new Text(60, 9, 256, Options::storageLimitsEnforced? 44:33);
	_lstItems = new TextList(287, Options::storageLimitsEnforced? 112:120, 8, Options::storageLimitsEnforced? 55:44); */

	_txtItem		= new Text( 30, 9,  16, 33);
	_txtCost		= new Text(102, 9, 166, 33);
	_txtQuantity	= new Text( 48, 9, 267, 33);

	_lstItems		= new TextList(285, 129, 16, 44);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("buyMenu");

	_colorAmmo = static_cast<Uint8>(_game->getRuleset()->getInterface("buyMenu")->getElement("ammoColor")->color);

	add(_window,		"window",	"buyMenu");
	add(_txtTitle,		"text",		"buyMenu");
	add(_txtBaseLabel,	"text",		"buyMenu");
	add(_txtFunds,		"text",		"buyMenu");
	add(_txtPurchases,	"text",		"buyMenu");
	add(_txtItem,		"text",		"buyMenu");
	add(_txtStorage,	"text",		"buyMenu");
	add(_txtCost,		"text",		"buyMenu");
	add(_txtQuantity,	"text",		"buyMenu");
	add(_lstItems,		"list",		"buyMenu");
	add(_btnCancel,		"button",	"buyMenu");
	add(_btnOk,			"button",	"buyMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& PurchaseState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PurchaseState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PurchaseState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& PurchaseState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& PurchaseState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PURCHASE_HIRE_PERSONNEL"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtFunds->setSecondaryColor(Palette::blockOffset(13));
	_txtFunds->setText(tr("STR_CURRENT_FUNDS_")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES_")
						.arg(Text::formatFunding(_costTotal)));

	_txtItem->setText(tr("STR_ITEM"));

	_txtStorage->setVisible(Options::storageLimitsEnforced);
	_txtStorage->setAlign(ALIGN_RIGHT);
	_txtStorage->setColor(WHITE);
	std::wostringstream woststr;
	woststr << _base->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtStorage->setText(woststr.str());

	_txtCost->setText(tr("STR_COST_PER_UNIT_UC"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstItems->setArrowColumn(227, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142,55,46,32);
	_lstItems->setSelectable();
	_lstItems->setBackground(_window);
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstItems->onMousePress((ActionHandler)& PurchaseState::lstItemsMousePress); // mousewheel
	_lstItems->onLeftArrowPress((ActionHandler)& PurchaseState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& PurchaseState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& PurchaseState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& PurchaseState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& PurchaseState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& PurchaseState::lstItemsRightArrowClick);

	const std::vector<std::string>& soldierList = _game->getRuleset()->getSoldiersList();
	for (std::vector<std::string>::const_iterator
			i = soldierList.begin();
			i != soldierList.end();
			++i)
	{
		_orderQty.push_back(0);
		_soldiers.push_back(*i);
		_lstItems->addRow(
					4,
					tr(*i).c_str(),
					Text::formatFunding(_game->getRuleset()->getSoldier(*i)->getBuyCost()).c_str(),
					Text::intWide(_base->getSoldierCount(*i)).c_str(),
					L"0");
	}

	_orderQty.push_back(0);
	_lstItems->addRow(
					4,
					tr("STR_SCIENTIST").c_str(),
					Text::formatFunding(_game->getRuleset()->getScientistCost() * 2).c_str(),
					Text::intWide(_base->getTotalScientists()).c_str(),
					L"0");

	_orderQty.push_back(0);
	_lstItems->addRow(
					4,
					tr("STR_ENGINEER").c_str(),
					Text::formatFunding(_game->getRuleset()->getEngineerCost() * 2).c_str(),
					Text::intWide(_base->getTotalEngineers()).c_str(),
					L"0");


	// Add craft-types to purchase list.
	const RuleCraft* crftRule;
	const std::vector<std::string>& craftList = _game->getRuleset()->getCraftsList();
	for (std::vector<std::string>::const_iterator
			i = craftList.begin();
			i != craftList.end();
			++i)
	{
		crftRule = _game->getRuleset()->getCraft(*i);
		if (crftRule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(crftRule->getRequirements()) == true)
		{
			_orderQty.push_back(0);
			_crafts.push_back(*i);
			_lstItems->addRow(
						4,
						tr(*i).c_str(),
						Text::formatFunding(crftRule->getBuyCost()).c_str(),
						Text::intWide(_base->getCraftCount(*i)).c_str(),
						L"0");
		}
	}


//	const std::vector<std::string>& itemList = _game->getRuleset()->getItemsList();
	std::vector<std::string> purchaseList = _game->getRuleset()->getItemsList(); // Copy, need to remove entries; note that SellState has a more elegant way of handling this ->

	const RuleCraftWeapon* cwRule;
	const RuleItem
		* itRule,
		* laRule,
		* clRule,
		* amRule;
	std::string type;
	std::wstring wst;
	int clipSize;

	// Add craft Weapon-types to purchase list.
	const std::vector<std::string>& cwList = _game->getRuleset()->getCraftWeaponsList();
	for (std::vector<std::string>::const_iterator
			i = cwList.begin();
			i != cwList.end();
			++i)
	{
		cwRule = _game->getRuleset()->getCraftWeapon(*i);
		laRule = _game->getRuleset()->getItem(cwRule->getLauncherItem());
		type = laRule->getType();

		if (laRule->getBuyCost() != 0) // + isResearched
		{
			_orderQty.push_back(0);
			_items.push_back(type);

			int qty = _base->getStorageItems()->getItemQty(type);
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferType() == PST_ITEM
					&& (*j)->getTransferItems() == type)
				{
					qty += (*j)->getQuantity();
				}
			}

			wst = tr(type);

			clipSize = cwRule->getAmmoMax();
			if (clipSize != 0)
				wst += (L" (" + Text::intWide(clipSize) + L")");

			_lstItems->addRow(
							4,
							wst.c_str(),
							Text::formatFunding(laRule->getBuyCost()).c_str(),
							Text::intWide(qty).c_str(),
							L"0");

			for (std::vector<std::string>::const_iterator
					j = purchaseList.begin();
					j != purchaseList.end();
					++j)
			{
				if (*j == type)
				{
					purchaseList.erase(j);
					break;
				}
			}
		}

		// Handle craft weapon ammo.
		clRule = _game->getRuleset()->getItem(cwRule->getClipItem());
		type = clRule->getType();

		if (clRule->getBuyCost() != 0) // clRule != NULL && // + isResearched
		{
			_orderQty.push_back(0);
			_items.push_back(type);

			int qty = _base->getStorageItems()->getItemQty(type);
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferType() == PST_ITEM
					&& (*j)->getTransferItems() == type)
				{
					qty += (*j)->getQuantity();
				}
			}

			wst = tr(type);

			clipSize = clRule->getClipSize();
			if (clipSize > 1)
				wst += (L"s (" + Text::intWide(clipSize) + L")");

			wst.insert(0, L"  ");
			_lstItems->addRow(
							4,
							wst.c_str(),
							Text::formatFunding(clRule->getBuyCost()).c_str(),
							Text::intWide(qty).c_str(),
							L"0");
			_lstItems->setRowColor(_orderQty.size() - 1, _colorAmmo);

			for (std::vector<std::string>::const_iterator
					j = purchaseList.begin();
					j != purchaseList.end();
					++j)
			{
				if (*j == type)
				{
					purchaseList.erase(j);
					break;
				}
			}
		}
	}


	for (std::vector<std::string>::const_iterator // add items to purchase list.
			i = purchaseList.begin();
			i != purchaseList.end();
			++i)
	{
		itRule = _game->getRuleset()->getItem(*i);
		//Log(LOG_INFO) << (*i) << " list# " << itRule->getListOrder(); // Prints listOrder to LOG.

		if (itRule->getBuyCost() != 0)
//			&& _game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
		{
			_orderQty.push_back(0);
			_items.push_back(*i);

			int qty = _base->getStorageItems()->getItemQty(*i);
			type = itRule->getType();

			for (std::vector<Transfer*>::const_iterator // add transfer items
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferItems() == type)
					qty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // add craft items & vehicles & vehicle ammo
					j = _base->getCrafts()->begin();
					j != _base->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getSoldiers() != 0) // is transport craft
				{
					for (std::map<std::string, int>::const_iterator
							k = (*j)->getCraftItems()->getContents()->begin();
							k != (*j)->getCraftItems()->getContents()->end();
							++k)
					{
						if (k->first == type)
							qty += k->second;
					}
				}

				if ((*j)->getRules()->getVehicles() != 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							k = (*j)->getVehicles()->begin();
							k != (*j)->getVehicles()->end();
							++k)
					{
						if ((*k)->getRules()->getType() == type)
							++qty;

						if ((*k)->getAmmo() != 255)
						{
							amRule = _game->getRuleset()->getItem(
									 _game->getRuleset()->getItem((*k)->getRules()->getType())
									 ->getCompatibleAmmo()->front());

							if (amRule->getType() == type)
								qty += (*k)->getAmmo();
						}
					}
				}
			}

			wst = tr(*i);

			if (itRule->getBattleType() == BT_AMMO			// weapon clips & HWP rounds
//					|| (itRule->getBattleType() == BT_NONE	// craft weapon rounds - ^HANDLED ABOVE^^
//						&& itRule->getClipSize() != 0))
				&& itRule->getType() != _game->getRuleset()->getAlienFuelType())
			{
				if (itRule->getType().substr(0,8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					clipSize = itRule->getClipSize();
					if (clipSize > 1)
						wst += (L" (" + Text::intWide(clipSize) + L")");
				}
				wst.insert(0, L"  ");

				_lstItems->addRow(
								4,
								wst.c_str(),
								Text::formatFunding(itRule->getBuyCost()).c_str(),
								Text::intWide(qty).c_str(),
								L"0");
				_lstItems->setRowColor(_orderQty.size() - 1, _colorAmmo);
			}
			else
			{
                if (itRule->isFixed() == true // tank w/ Ordnance.
					&& itRule->getCompatibleAmmo()->empty() == false)
                {
					amRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					clipSize = amRule->getClipSize();
					if (clipSize != 0)
						wst += (L" (" + Text::intWide(clipSize) + L")");
                }

				_lstItems->addRow(
								4,
								wst.c_str(),
								Text::formatFunding(itRule->getBuyCost()).c_str(),
								Text::intWide(qty).c_str(),
								L"0");
			}
		}
	}

	_lstItems->scrollTo(_base->getRecallRow(REC_PURCHASE));

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)& PurchaseState::increase);

	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)& PurchaseState::decrease);
}

/**
 * dTor.
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

	_timerInc->think(this, NULL);
	_timerDec->think(this, NULL);
}

/**
 * Purchases the selected items.
 * @param action - pointer to an Action
 */
void PurchaseState::btnOkClick(Action*)
{
	_base->setRecallRow(REC_PURCHASE, _lstItems->getScroll());

	if (_costTotal != 0)
	{
		_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _costTotal);
		_base->setCashSpent(_costTotal);
	}

	for (size_t
			sel = 0;
			sel != _orderQty.size();
			++sel)
	{
		if (_orderQty[sel] != 0)
		{
			Transfer* transfer;

			switch (getPurchaseType(sel))
			{
				case PST_SOLDIER:
				for (int
						i = 0;
						i != _orderQty[sel];
						++i)
				{
					transfer = new Transfer(_game->getRuleset()->getPersonnelTime());
					transfer->setSoldier(_game->getRuleset()->genSoldier(
																	_game->getSavedGame(),
																	_soldiers[sel]));
					_base->getTransfers()->push_back(transfer);
				}
				break;

				case PST_SCIENTIST:
					transfer = new Transfer(_game->getRuleset()->getPersonnelTime());
					transfer->setScientists(_orderQty[sel]);
					_base->getTransfers()->push_back(transfer);
				break;

				case PST_ENGINEER:
					transfer = new Transfer(_game->getRuleset()->getPersonnelTime());
					transfer->setEngineers(_orderQty[sel]);
					_base->getTransfers()->push_back(transfer);
				break;

				case PST_CRAFT:
					for (int
							i = 0;
							i != _orderQty[sel];
							++i)
					{
						RuleCraft* const crftRule = _game->getRuleset()->getCraft(_crafts[getCraftIndex(sel)]);
						transfer = new Transfer(crftRule->getTransferTime());
						Craft* const craft = new Craft(
													crftRule,
													_base,
													_game->getSavedGame()->getCanonicalId(_crafts[getCraftIndex(sel)]));
						craft->setCraftStatus("STR_REFUELLING");
						transfer->setCraft(craft);
						_base->getTransfers()->push_back(transfer);
					}
				break;

				case PST_ITEM:
				{
					const RuleItem* const itRule = _game->getRuleset()->getItem(_items[getItemIndex(sel)]);
					transfer = new Transfer(itRule->getTransferTime());
					transfer->setTransferItems(
										_items[getItemIndex(sel)],
										_orderQty[sel]);
					_base->getTransfers()->push_back(transfer);
				}
			}
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void PurchaseState::btnCancelClick(Action*)
{
	_base->setRecallRow(REC_PURCHASE, _lstItems->getScroll());
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void PurchaseState::lstItemsLeftArrowPress(Action* action)
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
void PurchaseState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the item by one on left-click - to max on right-click.
 * @param action - pointer to an Action
 */
void PurchaseState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		increaseByValue(std::numeric_limits<int>::max());
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if ((SDL_GetModState() & KMOD_CTRL) != 0)
			increaseByValue(10);
		else
			increaseByValue(1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action - pointer to an Action
 */
void PurchaseState::lstItemsRightArrowPress(Action* action)
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
void PurchaseState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the item by one on left-click - to 0 on right-click.
 * @param action - pointer to an Action
 */
void PurchaseState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		decreaseByValue(std::numeric_limits<int>::max());
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if ((SDL_GetModState() & KMOD_CTRL) != 0)
			decreaseByValue(10);
		else
			decreaseByValue(1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Gets the price of the currently selected item.
 * @return, the price of the currently selected item
 */
int PurchaseState::getPrice() // private.
{
	switch (getPurchaseType(_sel))
	{
		case PST_SOLDIER:
			return _game->getRuleset()->getSoldier(_soldiers[_sel])->getBuyCost();

		case PST_SCIENTIST:
			return _game->getRuleset()->getScientistCost() * 2;

		case PST_ENGINEER:
			return _game->getRuleset()->getEngineerCost() * 2;

		case PST_CRAFT:
			return _game->getRuleset()->getCraft(_crafts[getCraftIndex(_sel)])->getBuyCost();

		case PST_ITEM:
			return _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getBuyCost();
	}

	return 0;
}

/**
 * Increases the quantity of the selected item to buy by one.
 */
void PurchaseState::increase()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		increaseByValue(10);
	else
		increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to buy.
 * @param qtyDelta - how many to add
 */
void PurchaseState::increaseByValue(int qtyDelta)
{
	if (qtyDelta < 1)
		return;

	std::wstring error;

	if (_costTotal + getPrice() > _game->getSavedGame()->getFunds())
		error = tr("STR_NOT_ENOUGH_MONEY");
	else
	{
		switch (getPurchaseType(_sel))
		{
			case PST_SOLDIER:
			case PST_SCIENTIST:
			case PST_ENGINEER:
				if (_qtyPersonnel + 1 > _base->getAvailableQuarters() - _base->getUsedQuarters())
					error = tr("STR_NOT_ENOUGH_LIVING_SPACE");
			break;

			case PST_CRAFT:
				if (_qtyCraft + 1 > _base->getAvailableHangars() - _base->getUsedHangars())
					error = tr("STR_NO_FREE_HANGARS_FOR_PURCHASE");
			break;

			case PST_ITEM:
				if (_storeSize + _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize()
					> static_cast<double>(_base->getAvailableStores()) - _base->getUsedStores() + 0.05)
				{
					error = tr("STR_NOT_ENOUGH_STORE_SPACE");
				}
		}
	}

	if (error.empty() == false)
	{
		_timerInc->stop();

		const RuleInterface* const uiRule = _game->getRuleset()->getInterface("buyMenu");
		_game->pushState(new ErrorMessageState(
											error,
											_palette,
											uiRule->getElement("errorMessage")->color,
											"BACK13.SCR",
											uiRule->getElement("errorPalette")->color));
	}
	else
	{
		qtyDelta = std::min(
						qtyDelta,
						(static_cast<int>(_game->getSavedGame()->getFunds()) - _costTotal) / getPrice()); // note: (int)cast renders int64_t useless.

		switch (getPurchaseType(_sel))
		{
			case PST_SOLDIER:
			case PST_SCIENTIST:
			case PST_ENGINEER:
				qtyDelta = std::min(
								qtyDelta,
								_base->getAvailableQuarters() - _base->getUsedQuarters() - _qtyPersonnel);
				_qtyPersonnel += qtyDelta;
			break;

			case PST_CRAFT:
				qtyDelta = std::min(
								qtyDelta,
								_base->getAvailableHangars() - _base->getUsedHangars() - _qtyCraft);
				_qtyCraft += qtyDelta;
			break;

			case PST_ITEM:
			{
				const double storesPerItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize();
				double qtyAllowed;

				if (AreSame(storesPerItem, 0.) == false)
					qtyAllowed = (static_cast<double>(_base->getAvailableStores()) - _base->getUsedStores() - _storeSize + 0.05) / storesPerItem;
				else
					qtyAllowed = std::numeric_limits<double>::max();

				qtyDelta = std::min(
								qtyDelta,
								static_cast<int>(qtyAllowed));
				_storeSize += static_cast<double>(qtyDelta) * storesPerItem;
			}
		}

		_orderQty[_sel] += qtyDelta;
		_costTotal += getPrice() * qtyDelta;

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

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		decreaseByValue(10);
	else
		decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to buy.
 * @param qtyDelta - how many to subtract
 */
void PurchaseState::decreaseByValue(int qtyDelta)
{
	if (qtyDelta < 1 || _orderQty[_sel] < 1)
		return;

	qtyDelta = std::min(qtyDelta, _orderQty[_sel]);

	switch (getPurchaseType(_sel))
	{
		case PST_SOLDIER:
		case PST_SCIENTIST:
		case PST_ENGINEER:
			_qtyPersonnel -= qtyDelta;
		break;

		case PST_CRAFT:
			_qtyCraft -= qtyDelta;
		break;

		case PST_ITEM:
			_storeSize -= _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize() * static_cast<double>(qtyDelta);
	}

	_orderQty[_sel] -= qtyDelta;
	_costTotal -= getPrice() * qtyDelta;

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void PurchaseState::updateItemStrings() // private.
{
	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES_")
							.arg(Text::formatFunding(_costTotal)));

	_lstItems->setCellText(_sel, 3, Text::intWide(_orderQty[_sel]));

	if (_orderQty[_sel] > 0)
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());

		if (getPurchaseType(_sel) == PST_ITEM)
		{
			const RuleItem* const itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			if (itRule->getBattleType() == BT_AMMO		// ammo for weapon or hwp
				|| (itRule->getBattleType() == BT_NONE	// ammo for craft armament
					&& itRule->getClipSize() != 0))
			{
				_lstItems->setRowColor(_sel, _colorAmmo);
			}
		}
	}

	std::wostringstream woststr;
	woststr << _base->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		woststr << L" ";
		if (_storeSize > 0.) woststr << L"+";
		woststr << std::fixed << std::setprecision(1) << _storeSize;
	}
	_txtStorage->setText(woststr.str());

	_btnOk->setVisible(_costTotal != 0);
}

/**
 * Gets the purchase type.
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType PurchaseState::getPurchaseType(size_t sel) const // private.
{
	size_t rowCutoff = _soldiers.size();

	if (sel < rowCutoff)
		return PST_SOLDIER;

	if (sel < (rowCutoff += 1))
		return PST_SCIENTIST;

	if (sel < (rowCutoff += 1))
		return PST_ENGINEER;

	if (sel < (rowCutoff + _crafts.size()))
		return PST_CRAFT;

	return PST_ITEM;
}

/**
 * Gets the index of selected item.
 * @param sel - currently selected item
 * @return, current index
 */
size_t PurchaseState::getItemIndex(size_t sel) const // private.
{
	return sel - _soldiers.size() - _crafts.size() - 2;
}

/**
 * Gets the index of selected craft.
 * @param sel - currently selected craft
 * @return, current index
 */
size_t PurchaseState::getCraftIndex(size_t sel) const // private.
{
	return sel - _soldiers.size() - 2;
}

/*
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 *
void PurchaseState::lstItemsMousePress(Action* action)
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
			increaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (static_cast<int>(action->getAbsoluteXMouse()) >= _lstItems->getArrowsLeftEdge()
			&& static_cast<int>(action->getAbsoluteXMouse()) <= _lstItems->getArrowsRightEdge())
		{
			decreaseByValue(Options::changeValueByMouseWheel);
		}
	}
} */

}
