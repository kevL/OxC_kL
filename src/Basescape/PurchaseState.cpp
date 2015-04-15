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
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
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
 * @param base - pointer to the Base to get info from
 */
PurchaseState::PurchaseState(Base* base)
	:
		_base(base),
		_sel(0),
		_itemOffset(0),
		_totalCost(0),
		_persQty(0),
		_craftQty(0),
		_storeSize(0.)
{
	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(310, 17, 5, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);
	_txtSpaceUsed	= new Text(85, 9, 219, 9);

	_txtFunds		= new Text(140, 9, 16, 24);
	_txtPurchases	= new Text(140, 9, 160, 24);

/*	_txtSpaceUsed = new Text(150, 9, 160, 34);
	_txtItem = new Text(140, 9, 10, Options::storageLimitsEnforced? 44:33);
	_txtCost = new Text(102, 9, 152, Options::storageLimitsEnforced? 44:33);
	_txtQuantity = new Text(60, 9, 256, Options::storageLimitsEnforced? 44:33);
	_lstItems = new TextList(287, Options::storageLimitsEnforced? 112:120, 8, Options::storageLimitsEnforced? 55:44); */

	_txtItem		= new Text(30, 9, 16, 33);
//	_txtSpaceUsed	= new Text(85, 9, 70, 33);
	_txtCost		= new Text(102, 9, 166, 33);
	_txtQuantity	= new Text(48, 9, 267, 33);

	_lstItems		= new TextList(285, 129, 16, 44);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	std::string pal = "PAL_BASESCAPE";
	int bgHue = 0; // brown by default in ufo palette
	const Element* const element = _game->getRuleset()->getInterface("buyMenu")->getElement("palette");
	if (element != NULL)
	{
		if (element->TFTDMode == true)
			pal = "PAL_GEOSCAPE";

		if (element->color != std::numeric_limits<int>::max())
			bgHue = element->color;
	}
	setPalette(pal, bgHue);

	_ammoColor = static_cast<Uint8>(_game->getRuleset()->getInterface("buyMenu")->getElement("ammoColor")->color);

	add(_window,		"window",	"buyMenu");
	add(_txtTitle,		"text",		"buyMenu");
	add(_txtBaseLabel,	"text",		"buyMenu");
	add(_txtFunds,		"text",		"buyMenu");
	add(_txtPurchases,	"text",		"buyMenu");
	add(_txtItem,		"text",		"buyMenu");
	add(_txtSpaceUsed,	"text",		"buyMenu");
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
	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES")
						.arg(Text::formatFunding(_totalCost)));

	_txtItem->setText(tr("STR_ITEM"));

	_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);
	_txtSpaceUsed->setAlign(ALIGN_RIGHT);
	std::wostringstream ss;
	ss << _base->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtSpaceUsed->setText(ss.str());

	_txtCost->setText(tr("STR_COST_PER_UNIT_UC"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstItems->setArrowColumn(227, ARROW_VERTICAL);
	_lstItems->setColumns(4, 142, 55, 46, 32);
	_lstItems->setSelectable();
	_lstItems->setBackground(_window);
	_lstItems->setMargin();
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstItems->onMousePress((ActionHandler)& PurchaseState::lstItemsMousePress); // mousewheel
	_lstItems->onLeftArrowPress((ActionHandler)& PurchaseState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& PurchaseState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& PurchaseState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& PurchaseState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& PurchaseState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& PurchaseState::lstItemsRightArrowClick);

	std::wostringstream
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
	_quantities.push_back(0);

	ss2 << _base->getTotalScientists();
	_lstItems->addRow(
					4,
					tr("STR_SCIENTIST").c_str(),
					Text::formatFunding(_game->getRuleset()->getScientistCost() * 2).c_str(),
					ss2.str().c_str(),
					L"0");
	_quantities.push_back(0);

	ss3 << _base->getTotalEngineers();
	_lstItems->addRow(
					4,
					tr("STR_ENGINEER").c_str(),
					Text::formatFunding(_game->getRuleset()->getEngineerCost() * 2).c_str(),
					ss3.str().c_str(),
					L"0");
	_quantities.push_back(0);

	_itemOffset = 3;


	// Add craft-types to purchase list.
	const RuleCraft* crftRule;
	const std::vector<std::string>& crafts = _game->getRuleset()->getCraftsList();
	for (std::vector<std::string>::const_iterator
			i = crafts.begin();
			i != crafts.end();
			++i)
	{
		ss4.str(L"");

		crftRule = _game->getRuleset()->getCraft(*i);
		if (crftRule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(crftRule->getRequirements()) == true)
		{
			_quantities.push_back(0);
			_crafts.push_back(*i);

			++_itemOffset;

			int crafts = 0;
			for (std::vector<Craft*>::const_iterator
					j = _base->getCrafts()->begin();
					j != _base->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getType() == *i)
					++crafts;
			}

			ss4 << crafts;
			_lstItems->addRow(
							4,
							tr(*i).c_str(),
							Text::formatFunding(crftRule->getBuyCost()).c_str(),
							ss4.str().c_str(),
							L"0");
		}
	}


	std::vector<std::string> items = _game->getRuleset()->getItemsList();

	// Add craft Weapon-types to purchase list.
	const RuleCraftWeapon* cwRule;
	const RuleItem
		* itRule,
		* laRule,
		* clRule,
		* amRule;
	std::string st;
	std::wstring wst;
	int clipSize;

	const std::vector<std::string>& cwList = _game->getRuleset()->getCraftWeaponsList();
	for (std::vector<std::string>::const_iterator
			i = cwList.begin();
			i != cwList.end();
			++i)
	{
		ss5.str(L"");
		ss6.str(L"");

		// Special handling for treating craft weapons as items
		cwRule = _game->getRuleset()->getCraftWeapon(*i);
		laRule = _game->getRuleset()->getItem(cwRule->getLauncherItem());
		st = laRule->getType();

		if (laRule->getBuyCost() > 0
			&& isExcluded(st) == false)
		{
			_quantities.push_back(0);
			_items.push_back(st);

			int tQty = _base->getItems()->getItem(st);
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getType() == TRANSFER_ITEM
					&& (*j)->getItems() == st)
				{
					tQty += (*j)->getQuantity();
				}
			}

			wst = tr(st);

			clipSize = cwRule->getAmmoMax();
			if (clipSize > 0)
				wst += (L" (" + Text::formatNumber(clipSize) + L")");

			ss5 << tQty;
			_lstItems->addRow(
							4,
							wst.c_str(),
							Text::formatFunding(laRule->getBuyCost()).c_str(),
							ss5.str().c_str(),
							L"0");

			for (std::vector<std::string>::const_iterator
					j = items.begin();
					j != items.end();
					++j)
			{
				if (*j == st)
				{
					items.erase(j);
					break;
				}
			}
		}

		// Handle craft weapon ammo.
		clRule = _game->getRuleset()->getItem(cwRule->getClipItem());
		st = clRule->getType();

		if (clRule->getBuyCost() > 0 // clRule != NULL &&
			&& isExcluded(st) == false)
		{
			_quantities.push_back(0);
			_items.push_back(st);

			int tQty = _base->getItems()->getItem(st);
			for (std::vector<Transfer*>::const_iterator
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getType() == TRANSFER_ITEM
					&& (*j)->getItems() == st)
				{
					tQty += (*j)->getQuantity();
				}
			}

			wst = tr(st);

			clipSize = clRule->getClipSize();
			if (clipSize > 1)
				wst += (L"s (" + Text::formatNumber(clipSize) + L")");

			ss6 << tQty;
			wst.insert(0, L"  ");
			_lstItems->addRow(
							4,
							wst.c_str(),
							Text::formatFunding(clRule->getBuyCost()).c_str(),
							ss6.str().c_str(),
							L"0");
			_lstItems->setRowColor(
								_quantities.size() - 1,
								_ammoColor);

			for (std::vector<std::string>::const_iterator
					j = items.begin();
					j != items.end();
					++j)
			{
				if (*j == st)
				{
					items.erase(j);
					break;
				}
			}
		}
	}


	for (std::vector<std::string>::const_iterator // add items to purchase list.
			i = items.begin();
			i != items.end();
			++i)
	{
		itRule = _game->getRuleset()->getItem(*i);
		//Log(LOG_INFO) << (*i) << " list# " << itRule->getListOrder(); // Prints listOrder to LOG.

		if (itRule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(itRule->getRequirements()) == true
			&& isExcluded(*i) == false)
		{
			ss7.str(L"");
			st = itRule->getType();

			_quantities.push_back(0);
			_items.push_back(*i);

			int totalQty = _base->getItems()->getItem(*i);

			for (std::vector<Transfer*>::const_iterator // add transfer items
					j = _base->getTransfers()->begin();
					j != _base->getTransfers()->end();
					++j)
			{
				if ((*j)->getItems() == st)
					totalQty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // add craft items & vehicles & vehicle ammo
					j = _base->getCrafts()->begin();
					j != _base->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::const_iterator // ha, map is my bitch!!
							k = (*j)->getItems()->getContents()->begin();
							k != (*j)->getItems()->getContents()->end();
							++k)
					{
						if (k->first == st)
							totalQty += k->second;
					}
				}

				if ((*j)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							k = (*j)->getVehicles()->begin();
							k != (*j)->getVehicles()->end();
							++k)
					{
						if ((*k)->getRules()->getType() == st)
							++totalQty;

						if ((*k)->getAmmo() != 255)
						{
							amRule = _game->getRuleset()->getItem(
									 _game->getRuleset()->getItem((*k)->getRules()->getType())
									 ->getCompatibleAmmo()->front());

							if (amRule->getType() == st)
								totalQty += (*k)->getAmmo();
						}
					}
				}
			}

			ss7 << totalQty;
			wst = tr(*i);

			if (itRule->getBattleType() == BT_AMMO			// #2, weapon clips & HWP rounds
//					|| (itRule->getBattleType() == BT_NONE	// #0, craft weapon rounds - ^HANDLED ABOVE^^
//						&& itRule->getClipSize() > 0))
				&& itRule->getType() != "STR_ELERIUM_115")
			{
				if (itRule->getType().substr(0,8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					clipSize = itRule->getClipSize();
					if (clipSize > 1)
						wst += (L" (" + Text::formatNumber(clipSize) + L")");
				}
				wst.insert(0, L"  ");

				_lstItems->addRow(
								4,
								wst.c_str(),
								Text::formatFunding(itRule->getBuyCost()).c_str(),
								ss7.str().c_str(),
								L"0");
				_lstItems->setRowColor(
								_quantities.size() - 1,
								Palette::blockOffset(15)+6);
			}
			else
			{
                if (itRule->isFixed() == true // tank w/ Ordnance.
					&& itRule->getCompatibleAmmo()->empty() == false)
                {
					amRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					clipSize = amRule->getClipSize();
					if (clipSize > 0)
						wst += (L" (" + Text::formatNumber(clipSize) + L")");
                }

				_lstItems->addRow(
								4,
								wst.c_str(),
								Text::formatFunding(itRule->getBuyCost()).c_str(),
								ss7.str().c_str(),
								L"0");
			}
		}
	}


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
 * Returns whether the item is excluded in the options file.
 * @param item - reference an item to look up
 * @return, true if the item is excluded in the options file
 */
bool PurchaseState::isExcluded(const std::string& item)
{
	for (std::vector<std::string>::const_iterator
			i = Options::purchaseExclusions.begin();
			i != Options::purchaseExclusions.end();
			++i)
	{
		if (*i == item)
			return true;
	}

	return false;
}

/**
 * Purchases the selected items.
 * @param action - pointer to an Action
 */
void PurchaseState::btnOkClick(Action*)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _totalCost);
	_base->setCashSpent(_totalCost);

	for (size_t
			i = 0;
			i < _quantities.size();
			++i)
	{
		if (_quantities[i] > 0)
		{
			if (i == 0) // Buy soldiers
			{
				for (int
						j = 0;
						j < _quantities[i];
						++j)
				{
					Transfer* const transfer = new Transfer(_game->getRuleset()->getPersonnelTime());
					transfer->setSoldier(_game->getRuleset()->genSoldier(_game->getSavedGame()));

					_base->getTransfers()->push_back(transfer);
				}
			}
			else if (i == 1) // Buy scientists
			{
				Transfer* const transfer = new Transfer(_game->getRuleset()->getPersonnelTime());
				transfer->setScientists(_quantities[i]);

				_base->getTransfers()->push_back(transfer);
			}
			else if (i == 2) // Buy engineers
			{
				Transfer* const transfer = new Transfer(_game->getRuleset()->getPersonnelTime());
				transfer->setEngineers(_quantities[i]);

				_base->getTransfers()->push_back(transfer);
			}
			else if (i > 2 // Buy crafts
				&& i < 3 + _crafts.size())
			{
				for (int
						j = 0;
						j < _quantities[i];
						j++)
				{
					RuleCraft* const crftRule = _game->getRuleset()->getCraft(_crafts[i - 3]);
					Transfer* const transfer = new Transfer(crftRule->getTransferTime());
					Craft* const craft = new Craft(
											crftRule,
											_base,
											_game->getSavedGame()->getId(_crafts[i - 3]));
					craft->setStatus("STR_REFUELLING");
					transfer->setCraft(craft);

					_base->getTransfers()->push_back(transfer);
				}
			}
			else // Buy items
			{
				const RuleItem* const itRule = _game->getRuleset()->getItem(_items[i - 3 - _crafts.size()]);
				Transfer* const transfer = new Transfer(itRule->getTransferTime());
				transfer->setItems(
								_items[i - 3 - _crafts.size()],
								_quantities[i]);

				_base->getTransfers()->push_back(transfer);
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
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 */
/*void PurchaseState::lstItemsMousePress(Action* action)
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

/**
 * Gets the price of the currently selected item.
 * @return, the price of the currently selected item
 */
int PurchaseState::getPrice()
{
	if (_sel == 0)											// Soldier cost
		return _game->getRuleset()->getSoldierCost() * 2;
	else if (_sel == 1)										// Scientist cost
		return _game->getRuleset()->getScientistCost() * 2;
	else if (_sel == 2)										// Engineer cost
		return _game->getRuleset()->getEngineerCost() * 2;
	else if (_sel > 2										// Craft cost
		&& _sel < 3 + _crafts.size())
	{
		return _game->getRuleset()->getCraft(_crafts[_sel - 3])->getBuyCost();
	}
	else													// Item cost
		return _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])->getBuyCost();
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
 * @param change - how many to add
 */
void PurchaseState::increaseByValue(int change)
{
	if (change < 1)
		return;

	std::wstring errorMessage;
	if (_totalCost + getPrice() > _game->getSavedGame()->getFunds())
		errorMessage = tr("STR_NOT_ENOUGH_MONEY");

	else if (_sel < 3
		&& _persQty + 1 > _base->getAvailableQuarters() - _base->getUsedQuarters())
	{
		errorMessage = tr("STR_NOT_ENOUGH_LIVING_SPACE");
	}
	else if (_sel > 2
		&& _sel < 3 + _crafts.size()
		&& _craftQty + 1 > _base->getAvailableHangars() - _base->getUsedHangars())
	{
		errorMessage = tr("STR_NO_FREE_HANGARS_FOR_PURCHASE");
	}
	else if (_sel >= 3 + _crafts.size()
		&& _storeSize + _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])->getSize()
			> static_cast<double>(_base->getAvailableStores()) - _base->getUsedStores() + 0.05)
	{
		errorMessage = tr("STR_NOT_ENOUGH_STORE_SPACE");
	}
	else
	{
		const int maxByMoney = (static_cast<int>(_game->getSavedGame()->getFunds()) - _totalCost) / getPrice(); // note: (int)cast renders int64_t useless.
		change = std::min(
						change,
						maxByMoney);

		if (_sel < 3) // Personnel count
		{
			const int maxByQuarters = _base->getAvailableQuarters() - _base->getUsedQuarters() - _persQty;
			change = std::min(
							change,
							maxByQuarters);
			_persQty += change;
		}
		else if (_sel > 2
			&& _sel < 3 + _crafts.size()) // Craft count
		{
			const int maxByHangars = _base->getAvailableHangars() - _base->getUsedHangars() - _craftQty;
			change = std::min(
							change,
							maxByHangars);
			_craftQty += change;
		}
		else //if (_sel >= 3 + _crafts.size()) // Item count
		{
			const RuleItem* const rule = _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()]);

			const double
				storesPerItem = rule->getSize(),
				availStores = static_cast<double>(_base->getAvailableStores()) - _base->getUsedStores() - _storeSize;
			double qtyItemsCanBuy = std::numeric_limits<double>::max();

			if (AreSame(storesPerItem, 0.) == false)
				qtyItemsCanBuy = (availStores + 0.05) / storesPerItem;

			change = std::min(
							change,
							static_cast<int>(qtyItemsCanBuy));
			_storeSize += static_cast<double>(change) * storesPerItem;
		}

		_quantities[_sel] += change;
		_totalCost += getPrice() * change;

		updateItemStrings();
		return;
	}

	_timerInc->stop();

	RuleInterface* menuInterface = _game->getRuleset()->getInterface("buyMenu");
	_game->pushState(new ErrorMessageState(
										errorMessage,
										_palette,
										menuInterface->getElement("errorMessage")->color,
										"BACK13.SCR",
										menuInterface->getElement("errorPalette")->color));
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
 * @param change - how many to subtract
 */
void PurchaseState::decreaseByValue(int change)
{
	if (change < 1
		|| _quantities[_sel] < 1)
	{
		return;
	}

	change = std::min(
					change,
					_quantities[_sel]);

	if (_sel < 3) // Personnel count
		_persQty -= change;
	else if (_sel > 2 // Craft count
		&& _sel < 3 + _crafts.size())
	{
		_craftQty -= change;
	}
	else // Item count
		_storeSize -= _game->getRuleset()->getItem(_items[_sel - 3 - _crafts.size()])->getSize() * static_cast<double>(change);


	_quantities[_sel] -= change;
	_totalCost -= getPrice() * change;

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void PurchaseState::updateItemStrings()
{
	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES")
							.arg(Text::formatFunding(_totalCost)));

	std::wostringstream
		ss1,
		ss2;

	ss1 << _quantities[_sel];
	_lstItems->setCellText(
						_sel,
						3,
						ss1.str());

	if (_quantities[_sel] > 0)
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());

		if (_sel > _itemOffset)
		{
			const RuleItem* const rule = _game->getRuleset()->getItem(_items[_sel - _itemOffset]);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, _ammoColor);
			}
		}
	}

	ss2 << _base->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		ss2 << "(";
		if (_storeSize > 0.) ss2 << "+";
		ss2 << std::fixed << std::setprecision(1) << _storeSize << ")";
	}
	_txtSpaceUsed->setText(ss2.str());

	_btnOk->setVisible(_totalCost > 0);
}

}
