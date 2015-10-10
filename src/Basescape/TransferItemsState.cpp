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
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

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
 * @param baseSource - pointer to the source Base
 * @param baseTarget - pointer to the target Base
 */
TransferItemsState::TransferItemsState(
		Base* const baseSource,
		Base* const baseTarget)
	:
		_baseSource(baseSource),
		_baseTarget(baseTarget),

		_sel(0),
		_costTotal(0),

		_qtyPersonnel(0),
		_qtyCraft(0),
		_qtyAlien(0),
		_storeSize(0.),

		_hasSci(0),
		_hasEng(0),

		_distance(0.),

		_resetAll(true)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 16,  10, 9);
	_txtBaseSource	= new Text( 80,  9,  16, 9);
	_txtBaseTarget	= new Text( 80,  9, 224, 9);

	_txtSpaceSource	= new Text(85, 9,  16, 18);
	_txtSpaceTarget	= new Text(85, 9, 224, 18);

	_txtItem		= new Text(128, 9,  16, 29);
	_txtQuantity	= new Text( 35, 9, 160, 29);
//	_txtTransferQty	= new Text( 46, 9, 200, 29);
	_txtQtyTarget	= new Text( 62, 9, 247, 29);

	_lstItems		= new TextList(285, 137, 16, 39);

	_btnCancel		= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("transferMenu");

	_colorAmmo = static_cast<Uint8>(_game->getRuleset()->getInterface("transferMenu")->getElement("ammoColor")->color);

	add(_window,			"window",	"transferMenu");
	add(_txtTitle,			"text",		"transferMenu");
	add(_txtBaseSource,		"text",		"transferMenu");
	add(_txtBaseTarget,		"text",		"transferMenu");
	add(_txtSpaceSource,	"text",		"transferMenu");
	add(_txtSpaceTarget,	"text",		"transferMenu");
	add(_txtItem,			"text",		"transferMenu");
	add(_txtQuantity,		"text",		"transferMenu");
//	add(_txtTransferQty,	"text",		"transferMenu");
	add(_txtQtyTarget,		"text",		"transferMenu");
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
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& TransferItemsState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER"));

	_txtBaseSource->setText(_baseSource->getName());

	_txtBaseTarget->setAlign(ALIGN_RIGHT);
	_txtBaseTarget->setText(_baseTarget->getName());

	_txtSpaceSource->setAlign(ALIGN_RIGHT);

	_txtSpaceTarget->setAlign(ALIGN_LEFT);

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

//	_txtTransferQty->setText(tr("STR_AMOUNT_TO_TRANSFER"));

	_txtQtyTarget->setText(tr("STR_AMOUNT_AT_DESTINATION"));

	_lstItems->setBackground(_window);
	_lstItems->setArrowColumn(172, ARROW_VERTICAL);
	_lstItems->setColumns(4, 136,56,31,20);
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
 * @note Also called after cancelling TransferConfirmState.
 */
void TransferItemsState::init()
{
	if (_resetAll == false) // update only the storage-space info.
	{
		_resetAll = true;

		std::wostringstream woststr;
		woststr << _baseSource->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _baseSource->getUsedStores();
		if (std::abs(_storeSize) > 0.05)
		{
			woststr << L"(";
			if (-_storeSize > 0.) woststr << L"+";
			woststr << std::fixed << std::setprecision(1) << -_storeSize << L")";
		}
		_txtSpaceSource->setText(woststr.str());

		woststr.str(L"");
		woststr << _baseTarget->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _baseTarget->getUsedStores();
		if (std::abs(_storeSize) > 0.05)
		{
			woststr << L"(";
			if (_storeSize > 0.) woststr << L"+";
			woststr << std::fixed << std::setprecision(1) << _storeSize << L")";
		}
		_txtSpaceTarget->setText(woststr.str());

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
	_hasSci =
	_hasEng = 0;
	_costTotal =
	_qtyPersonnel =
	_qtyCraft =
	_qtyAlien = 0;
	_storeSize = 0.;


	_btnOk->setVisible(false);

	for (std::vector<Soldier*>::const_iterator
			i = _baseSource->getSoldiers()->begin();
			i != _baseSource->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == NULL)
		{
			_transferQty.push_back(0);
			_baseQty.push_back(1);
			_destQty.push_back(0);
			_soldiers.push_back(*i);

			std::wostringstream woststr;
			woststr << (*i)->getName();

			if ((*i)->getRecovery() != 0)
				woststr << L" (" << (*i)->getRecovery() << L" dy)";

			_lstItems->addRow(
							4,
							woststr.str().c_str(),
							L"1",L"0",L"0");
		}
	}

	for (std::vector<Craft*>::const_iterator
			i = _baseSource->getCrafts()->begin();
			i != _baseSource->getCrafts()->end();
			++i)
	{
		if ((*i)->getCraftStatus() != "STR_OUT")
//			|| (Options::canTransferCraftsWhileAirborne == true // TODO: Don't allow transfering airborne Craft until I rework the auto-transfer of onboard Soldiers & Items & Vehicles.
//				&& (*i)->getFuel() >= (*i)->getFuelLimit(_baseTarget)))
		{
			_transferQty.push_back(0);
			_baseQty.push_back(1);
			_destQty.push_back(0);
			_crafts.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName(_game->getLanguage()).c_str(),
							L"1",L"0",L"0");
		}
	}

	if (_baseSource->getScientists() != 0)
	{
		_hasSci = 1;
		_transferQty.push_back(0);
		_baseQty.push_back(_baseSource->getScientists());
		_destQty.push_back(_baseTarget->getTotalScientists());

		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						Text::intWide(_baseQty.back()).c_str(),
						L"0",
						Text::intWide(_destQty.back()).c_str());
	}

	if (_baseSource->getEngineers() != 0)
	{
		_hasEng = 1;
		_transferQty.push_back(0);
		_baseQty.push_back(_baseSource->getEngineers());
		_destQty.push_back(_baseTarget->getTotalEngineers());

		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						Text::intWide(_baseQty.back()).c_str(),
						L"0",
						Text::intWide(_destQty.back()).c_str());
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
		const int baseQty = _baseSource->getStorageItems()->getItemQty(*i);
		if (baseQty != 0)
		{
			_transferQty.push_back(0);
			_baseQty.push_back(baseQty);
			_items.push_back(*i);

			itRule = rules->getItem(*i);
			const std::string itType = itRule->getType();

			int destQty = _baseTarget->getStorageItems()->getItemQty(*i);

			for (std::vector<Transfer*>::const_iterator // add transfers
					j = _baseTarget->getTransfers()->begin();
					j != _baseTarget->getTransfers()->end();
					++j)
			{
				if ((*j)->getTransferItems() == itType)
					destQty += (*j)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // add items & vehicles on crafts
					j = _baseTarget->getCrafts()->begin();
					j != _baseTarget->getCrafts()->end();
					++j)
			{
				if ((*j)->getRules()->getSoldiers() != 0) // is transport craft
				{
					for (std::map<std::string, int>::const_iterator // add items on craft
							k = (*j)->getCraftItems()->getContents()->begin();
							k != (*j)->getCraftItems()->getContents()->end();
							++k)
					{
						if (k->first == itType)
							destQty += k->second;
					}
				}

				if ((*j)->getRules()->getVehicles() != 0) // is transport craft capable of vehicles
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
					j != cwList.end() && craftOrdnance == false;
					++j)
			{
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

			if ((itRule->getBattleType() == BT_AMMO
					|| (itRule->getBattleType() == BT_NONE
						&& itRule->getClipSize() != 0))
				&& itRule->getType() != _game->getRuleset()->getAlienFuelType()) // <- is this necessary
			{
				if (itRule->getBattleType() == BT_AMMO
					&& itRule->getType().substr(0,8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					const int clipSize = itRule->getClipSize();
					if (clipSize > 1)
						item += (L" (" + Text::formatNumber(clipSize) + L")");
				}
				item.insert(0, L"  ");

				color = _colorAmmo;
			}
			else
			{
                if (itRule->isFixed() == true // tank w/ Ordnance.
					&& itRule->getCompatibleAmmo()->empty() == false)
                {
					clRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
					const int clipSize = clRule->getClipSize();
					if (clipSize != 0)
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
				color = YELLOW;
			}

			_lstItems->addRow(
							4,
							item.c_str(),
							Text::intWide(baseQty).c_str(),
							L"0",
							Text::intWide(destQty).c_str());
			_lstItems->setRowColor(_baseQty.size() - 1, color);
		}
	}

	_lstItems->scrollTo(_baseSource->getRecallRow(REC_TRANSFER));
	_lstItems->draw();


	std::wostringstream woststr;
	woststr << _baseSource->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _baseSource->getUsedStores();
	_txtSpaceSource->setText(woststr.str());

	woststr.str(L"");
	woststr << _baseTarget->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _baseTarget->getUsedStores();
	_txtSpaceTarget->setText(woststr.str());
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
	_resetAll = false;
	_baseSource->setRecallRow( // note that if TransferConfirmState gets cancelled this still takes effect.
						REC_TRANSFER,
						_lstItems->getScroll());

	_game->pushState(new TransferConfirmState(_baseTarget, this));
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
 * Completes the transfer between Bases.
 * @note Called from TransferConfirmState.
 */
void TransferItemsState::completeTransfer()
{
	_resetAll = true;

	const int eta = static_cast<int>(std::floor(6. + _distance / 10.));
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _costTotal);
	_baseSource->setCashSpent(_costTotal);

	for (size_t
			sel = 0;
			sel != _transferQty.size();
			++sel)
	{
		if (_transferQty[sel] != 0)
		{
			Transfer* transfer;

			switch (getTransferType(sel))
			{
				case PST_SOLDIER:
					for (std::vector<Soldier*>::const_iterator
							j = _baseSource->getSoldiers()->begin();
							j != _baseSource->getSoldiers()->end();
							++j)
					{
						if (*j == _soldiers[sel])
						{
							if ((*j)->inPsiTraining() == true)
								(*j)->togglePsiTraining();

							transfer = new Transfer(eta);
							transfer->setSoldier(*j);
							_baseTarget->getTransfers()->push_back(transfer);
							_baseSource->getSoldiers()->erase(j);

							break;
						}
					}
				break;

				case PST_CRAFT:
				{
					Craft* const craft = _crafts[getCraftIndex(sel)];
/*					for (std::vector<Soldier*>::const_iterator // transfer soldiers inside craft - TODO: Disallowed Atm.
							j = _baseSource->getSoldiers()->begin();
							j != _baseSource->getSoldiers()->end();
							)
					{
						if ((*j)->getCraft() == craft)
						{
							if ((*j)->inPsiTraining() == true)
								(*j)->togglePsiTraining();

							if (craft->getCraftStatus() == "STR_OUT")
								_baseTarget->getSoldiers()->push_back(*j);
							else
							{
								Transfer* const transfer = new Transfer(eta);
								transfer->setSoldier(*j);
								_baseTarget->getTransfers()->push_back(transfer);
							}
							j = _baseSource->getSoldiers()->erase(j);
						}
						else ++j;
					} */
					for (std::vector<Craft*>::const_iterator
							j = _baseSource->getCrafts()->begin();
							j != _baseSource->getCrafts()->end();
							++j)
					{
						if (*j == craft)
						{
							if (craft->getRules()->getSoldiers() != 0) // is transport
							{
								for (std::vector<Soldier*>::const_iterator // unload Soldiers
										k = _baseSource->getSoldiers()->begin();
										k != _baseSource->getSoldiers()->end();
										++k)
								{
									if ((*k)->getCraft() == craft)
										(*k)->setCraft(NULL);
								}

								for (std::map<std::string, int>::const_iterator // remove items
										k = craft->getCraftItems()->getContents()->begin();
										k != craft->getCraftItems()->getContents()->end();
										++k)
								{
									craft->getCraftItems()->removeItem(k->first, k->second);
									_baseSource->getStorageItems()->addItem(k->first, k->second);
								}

								if (craft->getRules()->getVehicles() != 0) // remove vehicles & ammo
								{
									for (std::vector<Vehicle*>::const_iterator
											k = craft->getVehicles()->begin();
											k != craft->getVehicles()->end();
											)
									{
										_baseSource->getStorageItems()->addItem((*k)->getRules()->getType());

										const RuleItem* const aRule = _game->getRuleset()->getItem((*k)->getRules()->getCompatibleAmmo()->front());
										_baseSource->getStorageItems()->addItem(aRule->getType(), (*k)->getAmmo());

										delete *k;
										k = craft->getVehicles()->erase(k);
									}
								}
							}

/*							if (craft->getCraftStatus() == "STR_OUT") // TODO: Disallowed Atm.
							{
								_baseTarget->getCrafts()->push_back(craft);
								craft->setBase(_baseTarget, false);

//								if (craft->getFuel() <= craft->getFuelLimit(_baseTarget)) // never runs. Craft that don't have enough fuel to reach TargetBase aren't even in the vector.
//								{
//									craft->setLowFuel();
//									craft->returnToBase();
//								}
//								else
								if (craft->getDestination() == dynamic_cast<Target*>(_baseSource))
								{
									craft->setLowFuel(false);
									craft->returnToBase();
								}
							}
							else
							{ */
							Transfer* const transfer = new Transfer(eta);
							transfer->setCraft(*j);
							_baseTarget->getTransfers()->push_back(transfer);
/*							} */

							for (std::vector<BaseFacility*>::const_iterator // clear hangar
									k = _baseSource->getFacilities()->begin();
									k != _baseSource->getFacilities()->end();
									++k)
							{
								if ((*k)->getCraft() == *j)
								{
									(*k)->setCraft(NULL);
									break;
								}
							}

							_baseSource->getCrafts()->erase(j);
							break;
						}
					}
				}
				break;

				case PST_SCIENTIST:
					_baseSource->setScientists(_baseSource->getScientists() - _transferQty[sel]);
					transfer = new Transfer(eta);
					transfer->setScientists(_transferQty[sel]);
					_baseTarget->getTransfers()->push_back(transfer);
				break;

				case PST_ENGINEER:
					_baseSource->setEngineers(_baseSource->getEngineers() - _transferQty[sel]);
					transfer = new Transfer(eta);
					transfer->setEngineers(_transferQty[sel]);
					_baseTarget->getTransfers()->push_back(transfer);
				break;

				case PST_ITEM:
					_baseSource->getStorageItems()->removeItem(
														_items[getItemIndex(sel)],
														_transferQty[sel]);
					transfer = new Transfer(eta);
					transfer->setTransferItems(
											_items[getItemIndex(sel)],
											_transferQty[sel]);
					_baseTarget->getTransfers()->push_back(transfer);
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

	switch (getTransferType(_sel))
	{
		case PST_ITEM:
		{
			const RuleItem* const itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			if (itRule->getType() == "STR_ALIEN_ALLOYS")
				cost = 0.1;
			else if (itRule->getType() == _game->getRuleset()->getAlienFuelType())
				cost = 1.;
			else if (itRule->isAlien() == true)
				cost = 200.;
			else
				cost = 10.;
		}
		break;

		case PST_CRAFT:
			cost = 1000.;
		break;

		default:
			cost = 100.;
	}

	return static_cast<int>(std::ceil(_distance * cost));
}

/**
 * Gets the quantity of the currently selected type on the base.
 * @return, type quantity
 */
int TransferItemsState::getSourceQuantity() const // private.
{
	switch (getTransferType(_sel))
	{
		case PST_ITEM:		return _baseSource->getStorageItems()->getItemQty(_items[getItemIndex(_sel)]);
		case PST_SCIENTIST:	return _baseSource->getScientists();
		case PST_ENGINEER:	return _baseSource->getEngineers();
	}

	return 1; // soldier, craft
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
 * @param qtyDelta - how many to add
 */
void TransferItemsState::increaseByValue(int qtyDelta)
{
	if (qtyDelta < 1 || _transferQty[_sel] >= getSourceQuantity())
		return;

	std::wstring wstError;
	const RuleItem* itRule;

	switch (getTransferType(_sel))
	{
		case PST_ITEM:
			itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			if (itRule->isAlien() == false
				&& _baseTarget->storesOverfull(itRule->getSize() + _storeSize - 0.05))
			{
				wstError = tr("STR_NOT_ENOUGH_STORE_SPACE");
			}
			else if (itRule->isAlien() == true
				&& (_baseTarget->getAvailableContainment() == 0
					|| (_qtyAlien + 1 > _baseTarget->getAvailableContainment() - _baseTarget->getUsedContainment()
						&& Options::storageLimitsEnforced == true)))
			{
				wstError = tr("STR_NO_ALIEN_CONTAINMENT_FOR_TRANSFER");
			}
			else if (itRule->isAlien() == false)
			{
				const double storesPerItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize();
				double qtyAllowed;

				if (AreSame(storesPerItem, 0.) == false)
					qtyAllowed = (static_cast<double>(_baseTarget->getAvailableStores()) - _baseTarget->getUsedStores() - _storeSize + 0.05)
								/ storesPerItem;
				else
					qtyAllowed = std::numeric_limits<double>::max();

				qtyDelta = std::min(
								qtyDelta,
								std::min(
										static_cast<int>(qtyAllowed),
										getSourceQuantity() - _transferQty[_sel]));
				_storeSize += static_cast<double>(qtyDelta) * storesPerItem;
				_baseQty[_sel] -= qtyDelta;
				_destQty[_sel] += qtyDelta;
				_transferQty[_sel] += qtyDelta;
				_costTotal += getCost() * qtyDelta;
			}
			else // aLien
			{
				int freeContainment;
				if (Options::storageLimitsEnforced == true)
					freeContainment = _baseTarget->getAvailableContainment() - _baseTarget->getUsedContainment() - _qtyAlien;
				else
					freeContainment = std::numeric_limits<int>::max();

				qtyDelta = std::min(
								qtyDelta,
								std::min(
										freeContainment,
										getSourceQuantity() - _transferQty[_sel]));
				_qtyAlien += qtyDelta;
				_baseQty[_sel] -= qtyDelta;
				_destQty[_sel] += qtyDelta;
				_transferQty[_sel] += qtyDelta;
				_costTotal += getCost() * qtyDelta;
			}
		break;

		case PST_CRAFT:
			if (_qtyCraft + 1 > _baseTarget->getAvailableHangars() - _baseTarget->getUsedHangars())
				wstError = tr("STR_NO_FREE_HANGARS_FOR_TRANSFER");
			else
			{
				++_qtyCraft;
				--_baseQty[_sel];
				++_destQty[_sel];
				++_transferQty[_sel];

				if (_crafts[_sel - _soldiers.size()]->getCraftStatus() != "STR_OUT"
					|| Options::canTransferCraftsWhileAirborne == false)
				{
					_costTotal += getCost();
				}
			}
		break;

		default: // soldier, scientist, engineer
			if (_qtyPersonnel + 1 > _baseTarget->getAvailableQuarters() - _baseTarget->getUsedQuarters())
				wstError = tr("STR_NO_FREE_ACCOMMODATION");
			else
			{
				qtyDelta = std::min(
								qtyDelta,
								std::min(
										_baseTarget->getAvailableQuarters() - _baseTarget->getUsedQuarters() - _qtyPersonnel,
										getSourceQuantity() - _transferQty[_sel]));
				_qtyPersonnel += qtyDelta;
				_baseQty[_sel] -= qtyDelta;
				_destQty[_sel] += qtyDelta;
				_transferQty[_sel] += qtyDelta;
				_costTotal += getCost() * qtyDelta;
			}
	}

	if (wstError.empty() == false)
	{
		_resetAll = false;
		const RuleInterface* const uiRule = _game->getRuleset()->getInterface("transferMenu");
		_game->pushState(new ErrorMessageState(
											wstError,
											_palette,
											uiRule->getElement("errorMessage")->color,
											"BACK13.SCR",
											uiRule->getElement("errorPalette")->color));
	}
	else
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
 * @param qtyDelta - how many to remove
 */
void TransferItemsState::decreaseByValue(int qtyDelta)
{
	if (qtyDelta < 1 || _transferQty[_sel] < 1)
		return;

	qtyDelta = std::min(qtyDelta, _transferQty[_sel]);

	const Craft* craft = NULL;

	switch (getTransferType(_sel))
	{
		case PST_ITEM:
		{
			const RuleItem* const itRule = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
			if (itRule->isAlien() == false)
				_storeSize -= itRule->getSize() * static_cast<double>(qtyDelta);
			else
				_qtyAlien -= qtyDelta;
		}
		break;

		case PST_CRAFT:
			craft = _crafts[_sel - _soldiers.size()];
			--_qtyCraft;
		break;

		default: // soldier, scientist, engineer
			_qtyPersonnel -= qtyDelta;
	}

	_baseQty[_sel] += qtyDelta;
	_destQty[_sel] -= qtyDelta;
	_transferQty[_sel] -= qtyDelta;

	if (Options::canTransferCraftsWhileAirborne == false
		|| craft == NULL
		|| craft->getCraftStatus() != "STR_OUT")
	{
		_costTotal -= getCost() * qtyDelta;
	}

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void TransferItemsState::updateItemStrings() // private.
{
	_lstItems->setCellText(_sel, 1, Text::intWide(_baseQty[_sel]));
	_lstItems->setCellText(_sel, 2, Text::intWide(_transferQty[_sel]));
	_lstItems->setCellText(_sel, 3, Text::intWide(_destQty[_sel]));

	Uint8 color = _lstItems->getColor();

	if (_transferQty[_sel] != 0)
		color = _lstItems->getSecondaryColor();
	else if (getTransferType(_sel) == PST_ITEM)
	{
		const Ruleset* const rules = _game->getRuleset();
		const RuleItem* const itRule = rules->getItem(_items[getItemIndex(_sel)]);

		bool craftOrdnance = false;
		const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
		for (std::vector<std::string>::const_iterator
				i = cwList.begin();
				i != cwList.end();
				++i)
		{
			const RuleCraftWeapon* const cwRule = rules->getCraftWeapon(*i);
			if (itRule == rules->getItem(cwRule->getLauncherItem())
				|| itRule == rules->getItem(cwRule->getClipItem()))
			{
				craftOrdnance = true;
				break;
			}
		}

		const SavedGame* const gameSave = _game->getSavedGame();
		if (gameSave->isResearched(itRule->getType()) == false				// not researched or is research exempt
			&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
				|| rules->getItem(itRule->getType())->isAlien() == true			// or is an alien
				|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
				|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
			&& craftOrdnance == false)										// and is not craft ordnance
		{
			// well, that was !NOT! easy.
			color = YELLOW;
		}
		else if (itRule->getBattleType() == BT_AMMO
			|| (itRule->getBattleType() == BT_NONE
				&& itRule->getClipSize() != 0))
		{
			color = _colorAmmo;
		}
	}
	_lstItems->setRowColor(_sel, color);

	std::wostringstream woststr;
	woststr << _baseSource->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _baseSource->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		woststr << L"(";
		if (-_storeSize > 0.) woststr << L"+";
		woststr << std::fixed << std::setprecision(1) << -_storeSize << L")";
	}
	_txtSpaceSource->setText(woststr.str());

	woststr.str(L"");
	woststr << _baseTarget->getAvailableStores() << L":" << std::fixed << std::setprecision(1) << _baseTarget->getUsedStores();
	if (std::abs(_storeSize) > 0.05)
	{
		woststr << L"(";
		if (_storeSize > 0.) woststr << L"+";
		woststr << std::fixed << std::setprecision(1) << _storeSize << L")";
	}
	_txtSpaceTarget->setText(woststr.str());

	_btnOk->setVisible(_costTotal != 0);
}

/**
 * Gets the total cost of the current transfer.
 * @return, total cost
 */
int TransferItemsState::getTotalCost() const
{
	return _costTotal;
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

	const Base* base = _baseSource;
	for (size_t
			i = 0;
			i != 2;
			++i)
	{
		x[i] = r *  std::cos(base->getLatitude()) * std::cos(base->getLongitude());
		y[i] = r *  std::cos(base->getLatitude()) * std::sin(base->getLongitude());
		z[i] = r * -std::sin(base->getLatitude());

		base = _baseTarget;
	}

	x[2] = x[1] - x[0];
	y[2] = y[1] - y[0];
	z[2] = z[1] - z[0];

	return std::sqrt((x[2] * x[2]) + (y[2] * y[2]) + (z[2] * z[2]));
}

/**
 * Gets the type of selected item.
 * @param sel - the selected item
 * @return, PurchaseSellTransferType (Base.h)
 */
PurchaseSellTransferType TransferItemsState::getTransferType(size_t sel) const // private.
{
	size_t rowCutoff = _soldiers.size();

	if (sel < rowCutoff)
		return PST_SOLDIER;

	if (sel < (rowCutoff += _crafts.size()))
		return PST_CRAFT;

	if (sel < (rowCutoff += _hasSci))
		return PST_SCIENTIST;

	if (sel < (rowCutoff + _hasEng))
		return PST_ENGINEER;

	return PST_ITEM;
}

/**
 * Gets the transfer-index of the currently selected item.
 * @param sel - selected item
 * @return, transfer-index
 */
size_t TransferItemsState::getItemIndex(size_t sel) const // private.
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
size_t TransferItemsState::getCraftIndex(size_t sel) const // private.
{
	return sel - _soldiers.size();
}

/*
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 *
void TransferItemsState::lstItemsMousePress(Action* action)
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
