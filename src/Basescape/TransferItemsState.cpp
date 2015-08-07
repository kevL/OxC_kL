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

#include "TransferItemsState.h"

//#include <cfloat>
//#include <climits>
//#include <cmath>
//#include <sstream>

//#include "../fmath.h"

#include "TransferConfirmState.h"

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
#include "../Ruleset/RuleItem.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Transfer screen.
 * @param baseFrom	- pointer to the source Base
 * @param baseTo	- pointer to the destination Base
 */
TransferItemsState::TransferItemsState(
		Base* baseFrom,
		Base* baseTo)
	:
		_baseFrom(baseFrom),
		_baseTo(baseTo),

		_sel(0),
		_rowOffset(0),
		_totalCost(0),

		_persQty(0),
		_craftQty(0),
		_alienQty(0),
		_storeSize(0.),

		_hasSci(0),
		_hasEng(0),

		_distance(0.),

		_reset(true),
		_rowDefault(0)
{
	_window				= new Window(this, 320, 200);
	_txtTitle			= new Text(300, 16, 10, 9);
	_txtBaseFrom		= new Text(80, 9, 16, 9);
	_txtBaseTo			= new Text(80, 9, 224, 9);

	_txtSpaceFrom		= new Text(85, 9, 16, 18);
	_txtSpaceTo			= new Text(85, 9, 224, 18);

	_txtItem			= new Text(128, 9, 16, 29);
	_txtQuantity		= new Text(35, 9, 160, 29);
//	_txtAmountTransfer	= new Text(46, 9, 200, 29);
	_txtAtDestination	= new Text(62, 9, 247, 29);

	_lstItems			= new TextList(285, 137, 16, 39);

	_btnCancel			= new TextButton(134, 16, 16, 177);
	_btnOk				= new TextButton(134, 16, 170, 177);

	setInterface("transferMenu");

	_ammoColor = static_cast<Uint8>(_game->getRuleset()->getInterface("transferMenu")->getElement("ammoColor")->color);
	_colorArtefact = Palette::blockOffset(13)+5; // yellow

	add(_window,			"window",	"transferMenu");
	add(_txtTitle,			"text",		"transferMenu");
	add(_txtBaseFrom,		"text",		"transferMenu");
	add(_txtBaseTo,			"text",		"transferMenu");
	add(_txtSpaceFrom,		"text",		"transferMenu");
	add(_txtSpaceTo,		"text",		"transferMenu");
	add(_txtItem,			"text",		"transferMenu");
	add(_txtQuantity,		"text",		"transferMenu");
//	add(_txtAmountTransfer,	"text",		"transferMenu");
	add(_txtAtDestination,	"text",		"transferMenu");
	add(_lstItems,			"list",		"transferMenu");
	add(_btnCancel,			"button",	"transferMenu");
	add(_btnOk,				"button",	"transferMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_TRANSFER"));
	_btnOk->onMouseClick((ActionHandler)& TransferItemsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnOkClick,
					Options::keyOk);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& TransferItemsState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER"));

	_txtBaseFrom->setText(_baseFrom->getName());

	_txtBaseTo->setAlign(ALIGN_RIGHT);
	_txtBaseTo->setText(_baseTo->getName());

	_txtSpaceFrom->setAlign(ALIGN_RIGHT);

	_txtSpaceTo->setAlign(ALIGN_LEFT);

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

//	_txtAmountTransfer->setText(tr("STR_AMOUNT_TO_TRANSFER"));

	_txtAtDestination->setText(tr("STR_AMOUNT_AT_DESTINATION"));

	_lstItems->setBackground(_window);
	_lstItems->setArrowColumn(172, ARROW_VERTICAL);
	_lstItems->setColumns(4, 136, 56, 31, 20);
	_lstItems->setSelectable();
	_lstItems->setMargin();
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstItems->onMousePress((ActionHandler)& TransferItemsState::lstItemsMousePress);
	_lstItems->onLeftArrowPress((ActionHandler)& TransferItemsState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& TransferItemsState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& TransferItemsState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& TransferItemsState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& TransferItemsState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& TransferItemsState::lstItemsRightArrowClick);

	_distance = getDistance();


	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)& TransferItemsState::increase);

	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)& TransferItemsState::decrease);
}

/**
 * dTor.
 */
TransferItemsState::~TransferItemsState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Initialize the Transfer menu.
 * @note Also called when cancelling TransferConfirmState.
 */
void TransferItemsState::init()
{
	if (_reset == false)
	{
		_reset = true;

		std::wostringstream
			woststr1,
			woststr2;

		woststr1 << _baseFrom->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _baseFrom->getUsedStores();
		if (std::abs(_storeSize) > 0.05)
		{
			woststr1 << "(";
			if (-(_storeSize) > 0.) woststr1 << "+";
			woststr1 << std::fixed << std::setprecision(1) << -(_storeSize) << ")";
		}
		_txtSpaceFrom->setText(woststr1.str());

		woststr2 << _baseTo->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _baseTo->getUsedStores();
		if (std::abs(_storeSize) > 0.05)
		{
			woststr2 << "(";
			if (_storeSize > 0.) woststr2 << "+";
			woststr2 << std::fixed << std::setprecision(1) << _storeSize << ")";
		}
		_txtSpaceTo->setText(woststr2.str());

		return;
	}

	_lstItems->clearList();

	_baseQty.clear();
	_destQty.clear();
	_transferQty.clear();

	_soldiers.clear();
	_crafts.clear();
	_items.clear();

	_sel =
	_rowOffset =
	_hasSci =
	_hasEng = 0;
	_totalCost =
	_persQty =
	_craftQty =
	_alienQty = 0;
	_storeSize = 0.;


	_btnOk->setVisible(false);

	for (std::vector<Soldier*>::const_iterator
			i = _baseFrom->getSoldiers()->begin();
			i != _baseFrom->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == NULL)
		{
			_baseQty.push_back(1);
			_transferQty.push_back(0);
			_destQty.push_back(0);
			_soldiers.push_back(*i);

			std::wostringstream woststr;
			woststr << (*i)->getName();

			if ((*i)->getRecovery() > 0)
				woststr << L" (" << (*i)->getRecovery() << L" dy)";

			_lstItems->addRow(
							4,
							woststr.str().c_str(),
							L"1",
							L"0",
							L"0");
			++_rowOffset;
		}
	}

	for (std::vector<Craft*>::const_iterator
			i = _baseFrom->getCrafts()->begin();
			i != _baseFrom->getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT"
			|| (Options::canTransferCraftsWhileAirborne == true
				&& (*i)->getFuel() >= (*i)->getFuelLimit(_baseTo)))
		{
			_baseQty.push_back(1);
			_transferQty.push_back(0);
			_destQty.push_back(0);
			_crafts.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName(_game->getLanguage()).c_str(),
							L"1",
							L"0",
							L"0");
			++_rowOffset;
		}
	}

	if (_baseFrom->getScientists() > 0)
	{
		_hasSci = 1;
		_baseQty.push_back(_baseFrom->getScientists());
		_transferQty.push_back(0);
		_destQty.push_back(_baseTo->getTotalScientists());

		std::wostringstream
			woststr1,
			woststr2;
		woststr1 << _baseQty.back();
		woststr2 << _destQty.back();

		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						woststr1.str().c_str(),
						L"0",
						woststr2.str().c_str());
		++_rowOffset;
	}

	if (_baseFrom->getEngineers() > 0)
	{
		_hasEng = 1;
		_baseQty.push_back(_baseFrom->getEngineers());
		_transferQty.push_back(0);
		_destQty.push_back(_baseTo->getTotalEngineers());

		std::wostringstream
			woststr1,
			woststr2;
		woststr1 << _baseQty.back();
		woststr2 << _destQty.back();

		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						woststr1.str().c_str(),
						L"0",
						woststr2.str().c_str());
		++_rowOffset;
	}


	const SavedGame* const savedGame = _game->getSavedGame();
	const Ruleset* const rules = _game->getRuleset();
	const RuleItem
		* itRule = NULL,
		* laRule = NULL,
		* clRule = NULL;
	const RuleCraftWeapon* cwRule = NULL;

	const std::vector<std::string>& itemList = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		const int baseQty = _baseFrom->getItems()->getItemQty(*i);
		if (baseQty > 0)
		{
			_items.push_back(*i);
			_baseQty.push_back(baseQty);
			_transferQty.push_back(0);

			itRule = rules->getItem(*i);
			std::string itType = itRule->getType();

			int destQty = _baseTo->getItems()->getItemQty(*i);

			for (std::vector<Transfer*>::const_iterator // add transfers
					j = _baseTo->getTransfers()->begin();
					j != _baseTo->getTransfers()->end();
					++j)
			{
				if ((*j)->getItems() == itType)
					destQty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // add items & vehicles on crafts
					j = _baseTo->getCrafts()->begin();
					j != _baseTo->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::const_iterator // add items on craft
							k = (*j)->getItems()->getContents()->begin();
							k != (*j)->getItems()->getContents()->end();
							++k)
					{
						if (k->first == itType)
							destQty += k->second;
					}
				}

				if ((*j)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator // add vehicles & vehicle ammo on craft
							k = (*j)->getVehicles()->begin();
							k != (*j)->getVehicles()->end();
							++k)
					{
						if ((*k)->getRules()->getType() == itType)
							++destQty;

						if ((*k)->getAmmo() != 255)
						{
							const RuleItem
								* const tankRule = _game->getRuleset()->getItem((*k)->getRules()->getType()),
								* const ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());

							if (ammoRule->getType() == itType)
								destQty += (*k)->getAmmo();
						}
					}
				}
			}

			_destQty.push_back(destQty);


			std::wstring item = tr(*i);

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

			std::wostringstream
				woststr1,
				woststr2;
			woststr1 << baseQty;
			woststr2 << destQty;

			if ((itRule->getBattleType() == BT_AMMO
					|| (itRule->getBattleType() == BT_NONE
						&& itRule->getClipSize() > 0))
				&& itRule->getType() != "STR_ELERIUM_115") // <- is this necessary
			{
				if (itRule->getBattleType() == BT_AMMO
					&& itRule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
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
					clRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					const int clipSize = clRule->getClipSize();
					if (clipSize > 0)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
                }

				color = _lstItems->getColor();
			}

			if (savedGame->isResearched(itRule->getType()) == false				// not researched or is research exempt
				&& (savedGame->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(*i)->isAlien() == true						// or is an alien
					|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
					|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
				&& craftOrdnance == false)										// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = _colorArtefact;
			}

			_lstItems->addRow(
							4,
							item.c_str(),
							woststr1.str().c_str(),
							L"0",
							woststr2.str().c_str());
			_lstItems->setRowColor(
							_baseQty.size() - 1,
							color);
		}
	}

/*	if (row > 0
		&& _lstSoldiers->getScroll() >= row)
	{
		_lstSoldiers->scrollTo(0);
	}
	else */
	if (_rowDefault > 0)
		_lstItems->scrollTo(_rowDefault);


	std::wostringstream
		woststr1,
		woststr2;

	woststr1 << _baseFrom->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _baseFrom->getUsedStores();
	_txtSpaceFrom->setText(woststr1.str());

	woststr2 << _baseTo->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _baseTo->getUsedStores();
	_txtSpaceTo->setText(woststr2.str());
}

/**
 * Runs the arrow timers.
 */
void TransferItemsState::think()
{
	State::think();

	_timerInc->think(this, NULL);
	_timerDec->think(this, NULL);
}

/**
 * Transfers the selected items.
 * @param action - pointer to an Action
 */
void TransferItemsState::btnOkClick(Action*)
{
	_reset = false;
	_rowDefault = _lstItems->getScroll();

	_game->pushState(new TransferConfirmState(
											_baseTo,
											this));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void TransferItemsState::btnCancelClick(Action*)
{
	_game->popState(); // pop main Transfer (this)
//	_game->popState(); // pop choose Destination
}

/**
 * Completes the transfer between bases.
 */
void TransferItemsState::completeTransfer()
{
	_reset = true;

	const int transferTime = static_cast<int>(std::floor(6. + _distance / 10.));
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _totalCost);
	_baseFrom->setCashSpent(_totalCost);

	for (size_t
			i = 0;
			i != _transferQty.size();
			++i)
	{
		if (_transferQty[i] > 0)
		{
			if (i < _soldiers.size()) // transfer soldiers
			{
				for (std::vector<Soldier*>::const_iterator
						j = _baseFrom->getSoldiers()->begin();
						j != _baseFrom->getSoldiers()->end();
						++j)
				{
					if (*j == _soldiers[i])
					{
						if ((*j)->isInPsiTraining() == true)
							(*j)->togglePsiTraining();

						Transfer* const transfer = new Transfer(transferTime);
						transfer->setSoldier(*j);
						_baseTo->getTransfers()->push_back(transfer);
						_baseFrom->getSoldiers()->erase(j);

						break;
					}
				}
			}
			else if (i >= _soldiers.size()
				&& i < _soldiers.size() + _crafts.size()) // transfer crafts w/ soldiers inside (but not items)
			{
				Craft* const craft = _crafts[i - _soldiers.size()];

/*				for (std::vector<Soldier*>::const_iterator // transfer soldiers inside craft
						j = _baseFrom->getSoldiers()->begin();
						j != _baseFrom->getSoldiers()->end();
						)
				{
					if ((*j)->getCraft() == craft)
					{
						if ((*j)->isInPsiTraining() == true)
							(*j)->togglePsiTraining();

						if (craft->getStatus() == "STR_OUT")
							_baseTo->getSoldiers()->push_back(*j);
						else
						{
							Transfer* const transfer = new Transfer(transferTime);
							transfer->setSoldier(*j);
							_baseTo->getTransfers()->push_back(transfer);
						}

						j = _baseFrom->getSoldiers()->erase(j);
					}
					else
						++j;
				} */
				if (craft->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::vector<Soldier*>::const_iterator // unload Soldiers from craft
							j = _baseFrom->getSoldiers()->begin();
							j != _baseFrom->getSoldiers()->end();
							)
					{
						if ((*j)->getCraft() == craft)
							(*j)->setCraft(NULL);
					}
				}

				for (std::vector<Craft*>::const_iterator // transfer craft
						j = _baseFrom->getCrafts()->begin();
						j != _baseFrom->getCrafts()->end();
						++j)
				{
					if (*j == craft)
					{
						if (craft->getRules()->getSoldiers() > 0) // is transport craft
						{
							for (std::map<std::string, int>::const_iterator // remove items from craft
									k = craft->getItems()->getContents()->begin();
									k != craft->getItems()->getContents()->end();
									++k)
							{
								craft->getItems()->removeItem(
															k->first,
															k->second);

								_baseFrom->getItems()->addItem(
															k->first,
															k->second);
							}
						}

						if (craft->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
						{
							for (std::vector<Vehicle*>::const_iterator // remove tanks + tankAmmo
									k = craft->getVehicles()->begin();
									k != craft->getVehicles()->end();
									++k)
							{
								_baseFrom->getItems()->addItem((*k)->getRules()->getType());

								const RuleItem* const ammoRule = _game->getRuleset()->getItem((*k)->getRules()->getCompatibleAmmo()->front());
								_baseFrom->getItems()->addItem(
															ammoRule->getType(),
															(*k)->getAmmo());

								delete *k;
								craft->getVehicles()->erase(k);
							}
						}


						if (craft->getStatus() == "STR_OUT")
						{
							_baseTo->getCrafts()->push_back(craft);
							craft->setBase(
										_baseTo,
										false);

							const bool returning = (craft->getDestination() == dynamic_cast<Target*>(craft->getBase()));

							if (craft->getFuel() <= craft->getFuelLimit(_baseTo))
							{
								craft->setLowFuel(true);
								craft->returnToBase();
							}
							else if (returning == true)
							{
								craft->setLowFuel(false);
								craft->returnToBase();
							}
						}
						else
						{
							Transfer* const transfer = new Transfer(transferTime);
							transfer->setCraft(*j);
							_baseTo->getTransfers()->push_back(transfer);
						}

						for (std::vector<BaseFacility*>::const_iterator // clear hangar
								k = _baseFrom->getFacilities()->begin();
								k != _baseFrom->getFacilities()->end();
								++k)
						{
							if ((*k)->getCraft() == *j)
							{
								(*k)->setCraft(NULL);
								break;
							}
						}

						_baseFrom->getCrafts()->erase(j);
						break;
					}
				}
			}
			else if (_baseFrom->getScientists() > 0
				&& i == _soldiers.size() + _crafts.size()) // transfer scientists
			{
				_baseFrom->setScientists(_baseFrom->getScientists() - _transferQty[i]);
				Transfer* const transfer = new Transfer(transferTime);
				transfer->setScientists(_transferQty[i]);

				_baseTo->getTransfers()->push_back(transfer);
			}
			else if (_baseFrom->getEngineers() > 0
				&& i == _soldiers.size() + _crafts.size() + _hasSci) // transfer engineers
			{
				_baseFrom->setEngineers(_baseFrom->getEngineers() - _transferQty[i]);
				Transfer* const transfer = new Transfer(transferTime);
				transfer->setEngineers(_transferQty[i]);

				_baseTo->getTransfers()->push_back(transfer);
			}
			else // transfer items
			{
				_baseFrom->getItems()->removeItem(_items[getItemIndex(i)], _transferQty[i]);
				Transfer* const transfer = new Transfer(transferTime);
				transfer->setItems(_items[getItemIndex(i)], _transferQty[i]);

				_baseTo->getTransfers()->push_back(transfer);
			}
		}
	}
}

/**
 * Starts increasing the item.
 * @param action - pointer to an Action
 */
void TransferItemsState::lstItemsLeftArrowPress(Action* action)
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
void TransferItemsState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the selected item; by one on left-click; to max on right-click.
 * @param action - pointer to an Action
 */
void TransferItemsState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		increaseByValue(std::numeric_limits<int>::max());

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
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
void TransferItemsState::lstItemsRightArrowPress(Action* action)
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
void TransferItemsState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the selected item; by one on left-click; to 0 on right-click.
 * @param action - pointer to an Action
 */
void TransferItemsState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		decreaseByValue(std::numeric_limits<int>::max());

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
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
 * Gets the transfer cost of the currently selected item.
 * @note All prices increased tenfold.
 * @return, transfer cost
 */
int TransferItemsState::getCost() const // private.
{
	double cost;

	if (TRANSFER_ITEM == getType(_sel))			// item cost
	{
		const RuleItem* const itRule = _game->getRuleset()->getItem(_items[_sel - _rowOffset]);
		if (itRule->getType() == "STR_ALIEN_ALLOYS")
			cost = 0.1;
		else if (itRule->getType() == "STR_ELERIUM_115")
			cost = 1.;
		else if (itRule->isAlien() == true)
			cost = 200.;
		else
			cost = 10.;
	}
	else if (TRANSFER_CRAFT == getType(_sel))	// craft cost
		cost = 1000.;
	else										// personnel cost
		cost = 100.;

	return static_cast<int>(std::ceil(_distance * cost));
}

/**
 * Gets the quantity of the currently selected item on the base.
 * @return, item quantity
 */
int TransferItemsState::getQuantity() const // private.
{
	switch (getType(_sel))
	{
//		case TRANSFER_SOLDIER:
//		case TRANSFER_CRAFT:		return 1;
		case TRANSFER_SCIENTIST:	return _baseFrom->getScientists();
		case TRANSFER_ENGINEER:		return _baseFrom->getEngineers();
		case TRANSFER_ITEM:			return _baseFrom->getItems()->getItemQty(_items[getItemIndex(_sel)]);
	}

	return 1;
}

/**
 * Increases the quantity of the selected item to transfer by one.
 */
void TransferItemsState::increase()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		increaseByValue(10);
	else
		increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to transfer.
 * @param change - how many to add
 */
void TransferItemsState::increaseByValue(int change)
{
	if (change < 1
		|| _transferQty[_sel] >= getQuantity())
	{
		return;
	}

	const TransferType type = getType(_sel);
	const RuleItem* itRule;
	if (type == TRANSFER_ITEM)
		itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
	else
		itRule = NULL;

	// ERRORS Start:
	std::wstring wstError;
	if ((	   type == TRANSFER_SOLDIER
			|| type == TRANSFER_SCIENTIST
			|| type == TRANSFER_ENGINEER)
		&& _persQty + 1 > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
	{
		wstError = tr("STR_NO_FREE_ACCOMMODATION");
	}
	else if (type == TRANSFER_CRAFT)
	{
//		const Craft* const craft = _crafts[_sel - _soldiers.size()];

		if (_craftQty + 1 > _baseTo->getAvailableHangars() - _baseTo->getUsedHangars())
			wstError = tr("STR_NO_FREE_HANGARS_FOR_TRANSFER");
/*		else if (_persQty + craft->getNumSoldiers() > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
			wstError = tr("STR_NO_FREE_ACCOMMODATION_CREW");
		else if (Options::storageLimitsEnforced == true
			&& _baseTo->storesOverfull(craft->getItems()->getTotalSize(_game->getRuleset()) + _storeSize))
		{
			wstError = tr("STR_NOT_ENOUGH_STORE_SPACE_FOR_CRAFT");
		} */
		// kL_note: What about TANKS + tankAmmo
	}
	else if (type == TRANSFER_ITEM)
	{
		if (itRule->isAlien() == false
			&& _baseTo->storesOverfull(itRule->getSize() + _storeSize - 0.05))
		{
			wstError = tr("STR_NOT_ENOUGH_STORE_SPACE");
		}
		else if (itRule->isAlien() == true
			&& (_baseTo->getAvailableContainment() == 0
				|| (Options::storageLimitsEnforced == true
					&& _alienQty + 1 > _baseTo->getAvailableContainment() - _baseTo->getUsedContainment())))
		{
			wstError = tr("STR_NO_ALIEN_CONTAINMENT_FOR_TRANSFER");
		}
	}

	if (wstError.empty() == false)
	{
		_reset = false;

		const RuleInterface* const uiRule = _game->getRuleset()->getInterface("transferMenu");
		_game->pushState(new ErrorMessageState(
											wstError,
											_palette,
											uiRule->getElement("errorMessage")->color,
											"BACK13.SCR",
											uiRule->getElement("errorPalette")->color));
		return;
	}
	// ERRORS Done.


	if (type == TRANSFER_SOLDIER // personnel count
		|| type == TRANSFER_SCIENTIST
		|| type == TRANSFER_ENGINEER)
	{
		change = std::min(
						change,
						std::min(
								_baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters() - _persQty,
								getQuantity() - _transferQty[_sel]));
		_persQty += change;
		_baseQty[_sel] -= change;
		_destQty[_sel] += change;
		_transferQty[_sel] += change;
		_totalCost += getCost() * change;
	}
	else if (type == TRANSFER_CRAFT) // craft count
	{
		const Craft* const craft = _crafts[_sel - _soldiers.size()];

		++_craftQty;
//		_persQty += craft->getNumSoldiers();
//		_storeSize += craft->getItems()->getTotalSize(_game->getRuleset());

		--_baseQty[_sel];
		++_destQty[_sel];
		++_transferQty[_sel];

		if (craft->getStatus() != "STR_OUT"
			|| Options::canTransferCraftsWhileAirborne == false)
		{
			_totalCost += getCost();
		}
	}
	else if (type == TRANSFER_ITEM)
	{
		if (itRule->isAlien() == false) // item count
		{
			const double
				storesPerItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize(),
				availStores = static_cast<double>(_baseTo->getAvailableStores()) - _baseTo->getUsedStores() - _storeSize;
			double
				qtyItemsCanGo = std::numeric_limits<double>::max();

			if (AreSame(storesPerItem, 0.) == false)
				qtyItemsCanGo = (availStores + 0.05) / storesPerItem;

			change = std::min(
							change,
							std::min(
									static_cast<int>(qtyItemsCanGo),
									getQuantity() - _transferQty[_sel]));
			_storeSize += static_cast<double>(change) * storesPerItem;
			_baseQty[_sel] -= change;
			_destQty[_sel] += change;
			_transferQty[_sel] += change;
			_totalCost += getCost() * change;
		}
		else // if (itRule->isAlien()) // live alien count
		{
			int freeContainment;
			if (Options::storageLimitsEnforced == true)
				freeContainment = _baseTo->getAvailableContainment() - _baseTo->getUsedContainment() - _alienQty;
			else
				freeContainment = std::numeric_limits<int>::max();

			change = std::min(
							change,
							std::min(
									freeContainment,
									getQuantity() - _transferQty[_sel]));
			_alienQty += change;
			_baseQty[_sel] -= change;
			_destQty[_sel] += change;
			_transferQty[_sel] += change;
			_totalCost += getCost() * change;
		}
	}

	updateItemStrings();
}

/**
 * Decreases the quantity of the selected item to transfer by one.
 */
void TransferItemsState::decrease()
{
	_timerInc->setInterval(80);
	_timerDec->setInterval(80);

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
		decreaseByValue(10);
	else
		decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to transfer.
 * @param change - how many to remove
 */
void TransferItemsState::decreaseByValue(int change)
{
	if (change < 1
		|| _transferQty[_sel] < 1)
	{
		return;
	}

	change = std::min(
					change,
					_transferQty[_sel]);

	const Craft* craft = NULL;

	const enum TransferType type = getType(_sel);

	if (   type == TRANSFER_SOLDIER
		|| type == TRANSFER_SCIENTIST
		|| type == TRANSFER_ENGINEER) // personnel count
	{
		_persQty -= change;
	}
	else if (type == TRANSFER_CRAFT) // craft count
	{
		craft = _crafts[_sel - _soldiers.size()];

		--_craftQty;
//		_persQty -= craft->getNumSoldiers();
//		_storeSize -= craft->getItems()->getTotalSize(_game->getRuleset());
	}
	else if (type == TRANSFER_ITEM) // item count
	{
		const RuleItem* const itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
		if (itRule->isAlien() == false)
			_storeSize -= itRule->getSize() * static_cast<double>(change);
		else
			_alienQty -= change;
	}

	_baseQty[_sel] += change;
	_destQty[_sel] -= change;
	_transferQty[_sel] -= change;

	if (craft == NULL
		|| craft->getStatus() != "STR_OUT"
		|| Options::canTransferCraftsWhileAirborne == false)
	{
		_totalCost -= getCost() * change;
	}

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void TransferItemsState::updateItemStrings() // private.
{
	std::wostringstream
		woststr1,
		woststr2,
		woststr3,
		woststr4,
		woststr5;

	woststr1 << _baseQty[_sel];
	_lstItems->setCellText(_sel, 1, woststr1.str());

	woststr2 << _transferQty[_sel];
	_lstItems->setCellText(_sel, 2, woststr2.str());

	woststr3 << _destQty[_sel];
	_lstItems->setCellText(_sel, 3, woststr3.str());

	Uint8 color = _lstItems->getColor();

	if (_transferQty[_sel] > 0)
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


	_btnOk->setVisible(_totalCost > 0);

	woststr4 << _baseFrom->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _baseFrom->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		woststr4 << "(";
		if (-(_storeSize) > 0.) woststr4 << "+";
		woststr4 << std::fixed << std::setprecision(1) << -(_storeSize) << ")";
	}
	_txtSpaceFrom->setText(woststr4.str());

	woststr5 << _baseTo->getAvailableStores() << ":" << std::fixed << std::setprecision(1) << _baseTo->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		woststr5 << "(";
		if (_storeSize > 0.) woststr5 << "+";
		woststr5 << std::fixed << std::setprecision(1) << _storeSize << ")";
	}
	_txtSpaceTo->setText(woststr5.str());
}

/**
 * Gets the total cost of the current transfer.
 * @return, total cost
 */
int TransferItemsState::getTotalCost() const
{
	return _totalCost;
}

/**
 * Gets the shortest distance between the two bases.
 * @return, distance
 */
double TransferItemsState::getDistance() const // private.
{
	const double r = 51.2; // kL_note: what's this conversion factor is it right
	double
		x[3],y[3],z[3];

	const Base* base = _baseFrom;

	for (size_t
			i = 0;
			i != 2;
			++i)
	{
		x[i] = r *  std::cos(base->getLatitude()) * std::cos(base->getLongitude());
		y[i] = r *  std::cos(base->getLatitude()) * std::sin(base->getLongitude());
		z[i] = r * -std::sin(base->getLatitude());

		base = _baseTo;
	}

	x[2] = x[1] - x[0];
	y[2] = y[1] - y[0];
	z[2] = z[1] - z[0];

	return std::sqrt((x[2] * x[2]) + (y[2] * y[2]) + (z[2] * z[2]));
}

/**
 * Gets type of selected item.
 * @param sel - the selected item
 * @return, the type of the selected item
 */
TransferType TransferItemsState::getType(const size_t sel) const // private.
{
	size_t row = _soldiers.size();

	if (sel < row)
		return TRANSFER_SOLDIER;

	if (sel < (row += _crafts.size()))
		return TRANSFER_CRAFT;

	if (sel < (row += _hasSci))
		return TRANSFER_SCIENTIST;

	if (sel < (row + _hasEng))
		return TRANSFER_ENGINEER;

	return TRANSFER_ITEM;
}

/**
 * Gets the index of the selected item.
 * @param sel - currently selected item
 * @return, index of the selected item
 */
size_t TransferItemsState::getItemIndex(const size_t sel) const // private.
{
	return sel
		 - _soldiers.size()
		 - _crafts.size()
		 - _hasSci
		 - _hasEng;
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 */
/*void TransferItemsState::lstItemsMousePress(Action* action)
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
