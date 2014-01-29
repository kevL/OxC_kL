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

#include "TransferItemsState.h"

#include <climits>
#include <cmath>
#include <sstream>

#include "TransferConfirmState.h"

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
 * Initializes all the elements in the Transfer screen.
 * @param game Pointer to the core game.
 * @param baseFrom Pointer to the source base.
 * @param baseTo Pointer to the destination base.
 */
TransferItemsState::TransferItemsState(
		Game* game,
		Base* baseFrom,
		Base* baseTo)
	:
		State(game),
		_baseQty(),
		_destQty(),
		_transferQty(),

		_baseFrom(baseFrom),
		_baseTo(baseTo),

		_soldiers(),
		_crafts(),
		_items(),
		_sel(0),
		_total(0),

		_pQty(0),
		_cQty(0),
		_aQty(0),
		_iQty(0.f),

		_hasSci(0),
		_hasEng(0),

		_distance(0.0),
		_itemOffset(0)
{
	_changeValueByMouseWheel			= Options::getInt("changeValueByMouseWheel");
	_allowChangeListValuesByMouseWheel	= Options::getBool("allowChangeListValuesByMouseWheel")
											&& _changeValueByMouseWheel;
	_containmentLimit					= Options::getBool("alienContainmentLimitEnforced");
	_canTransferCraftsWhileAirborne		= Options::getBool("canTransferCraftsWhileAirborne");


	_window					= new Window(this, 320, 200, 0, 0);
	_txtTitle				= new Text(300, 16, 10, 9);
	_txtBaseFrom			= new Text(80, 9, 16, 9);
	_txtBaseTo				= new Text(80, 9, 224, 9);

	_txtItem				= new Text(128, 9, 16, 24);
	_txtQuantity			= new Text(35, 9, 160, 24);
	_txtAmountTransfer		= new Text(60, 9, 200, 24);
	_txtAmountDestination	= new Text(62, 9, 247, 24);

	_lstItems				= new TextList(285, 136, 16, 35);

	_btnCancel				= new TextButton(134, 16, 16, 177);
	_btnOk					= new TextButton(134, 16, 170, 177);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtBaseFrom);
	add(_txtBaseTo);
	add(_txtItem);
	add(_txtQuantity);
	add(_txtAmountTransfer);
	add(_txtAmountDestination);
	add(_lstItems);
	add(_btnCancel);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_TRANSFER"));
	_btnOk->onMouseClick((ActionHandler)& TransferItemsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnOkClick,
					(SDLKey)Options::getInt("keyOk"));
	_btnOk->setVisible(false);

	_btnCancel->setColor(Palette::blockOffset(15)+6);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& TransferItemsState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnCancelClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER"));

	_txtBaseFrom->setColor(Palette::blockOffset(13)+10);
	_txtBaseFrom->setText(_baseFrom->getName());

	_txtBaseTo->setColor(Palette::blockOffset(13)+10);
	_txtBaseTo->setAlign(ALIGN_RIGHT);
	_txtBaseTo->setText(_baseTo->getName());

	_txtItem->setColor(Palette::blockOffset(13)+10);
	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setColor(Palette::blockOffset(13)+10);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtAmountTransfer->setColor(Palette::blockOffset(13)+10);
	_txtAmountTransfer->setText(tr("STR_AMOUNT_TO_TRANSFER"));
	_txtAmountTransfer->setWordWrap(true);

	_txtAmountDestination->setColor(Palette::blockOffset(13)+10);
	_txtAmountDestination->setText(tr("STR_AMOUNT_AT_DESTINATION"));
	_txtAmountDestination->setWordWrap(true);

//	_lstItems->setColor(Palette::blockOffset(15)+1);
	_lstItems->setColor(Palette::blockOffset(13)+10);
	_lstItems->setArrowColor(Palette::blockOffset(13)+10);
	_lstItems->setArrowColumn(172, ARROW_VERTICAL);
	_lstItems->setColumns(4, 136, 56, 31, 20);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(8);
	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstItems->onLeftArrowPress((ActionHandler)& TransferItemsState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& TransferItemsState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& TransferItemsState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& TransferItemsState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& TransferItemsState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& TransferItemsState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)& TransferItemsState::lstItemsMousePress);

	for (std::vector<Soldier*>::iterator
			i = _baseFrom->getSoldiers()->begin();
			i != _baseFrom->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == 0)
		{
			_baseQty.push_back(1);
			_transferQty.push_back(0);
			_destQty.push_back(0);
			_soldiers.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName().c_str(),
							L"1",
							L"0",
							L"0");

			++_itemOffset;
		}
	}

	for (std::vector<Craft*>::iterator
			i = _baseFrom->getCrafts()->begin();
			i != _baseFrom->getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT"
			|| (_canTransferCraftsWhileAirborne
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

			++_itemOffset;
		}
	}

//kL	if (_baseFrom->getAvailableScientists() > 0)
	if (_baseFrom->getScientists() > 0) // kL
	{
//kL		_baseQty.push_back(_baseFrom->getAvailableScientists());
		_baseQty.push_back(_baseFrom->getScientists()); // kL
		_transferQty.push_back(0);
//kL		_destQty.push_back(_baseTo->getAvailableScientists());
		_destQty.push_back(_baseTo->getTotalScientists()); // kL
		_hasSci = 1;

		std::wstringstream
			ss1,
			ss2;
//kL		ss1 << _baseFrom->getAvailableScientists();
//kL		ss2 << _baseTo->getAvailableScientists();
//		ss1 << _baseFrom->getScientists(); // kL
//		ss2 << _baseTo->getScientists(); // kL
		ss1 << _baseQty.back();
		ss2 << _destQty.back();

		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						ss1.str().c_str(),
						L"0",
						ss2.str().c_str());

		++_itemOffset;
	}

//kL	if (_baseFrom->getAvailableEngineers() > 0)
	if (_baseFrom->getEngineers() > 0) // kL
	{
//kL		_baseQty.push_back(_baseFrom->getAvailableEngineers());
		_baseQty.push_back(_baseFrom->getEngineers()); // kL
		_transferQty.push_back(0);
//kL		_destQty.push_back(_baseTo->getAvailableEngineers());
		_destQty.push_back(_baseTo->getTotalEngineers()); // kL
		_hasEng = 1;

		std::wstringstream
			ss1,
			ss2;
//kL		ss1 << _baseFrom->getAvailableEngineers();
//kL		ss2 << _baseTo->getAvailableEngineers();
//		ss1 << _baseFrom->getEngineers(); // kL
//		ss2 << _baseTo->getEngineers(); // kL
		ss1 << _baseQty.back();
		ss2 << _destQty.back();

		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						ss1.str().c_str(),
						L"0",
						ss2.str().c_str());

		++_itemOffset;
	}

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		int qty = _baseFrom->getItems()->getItem(*i);
		if (qty > 0)
		{
			_baseQty.push_back(qty);
			_transferQty.push_back(0);
//kL			_destQty.push_back(_baseTo->getItems()->getItem(*i));
			_items.push_back(*i);

			// kL_begin:
			std::wstring item = tr(*i);

			int tQty = _baseTo->getItems()->getItem(*i); // Returns the quantity of an item in the container.

			for (std::vector<Transfer*>::const_iterator
					j = _baseTo->getTransfers()->begin();
					j != _baseTo->getTransfers()->end();
					++j)
			{
				std::wstring trItem = (*j)->getName(_game->getLanguage());
				if (trItem == item)
				{
					tQty += (*j)->getQuantity();
				}
			}

			// Add qty of items & vehicles on transport craft to Transfers, _baseTo screen stock.
			for (std::vector<Craft*>::const_iterator
					c = _baseTo->getCrafts()->begin();
					c != _baseTo->getCrafts()->end();
					++c)
			{
				if ((*c)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::iterator
							t = (*c)->getItems()->getContents()->begin();
							t != (*c)->getItems()->getContents()->end();
							++t)
					{
						std::wstring tranItem = tr(t->first);
						if (tranItem == item)
						{
							tQty += t->second;
						}
					}
				}

				if ((*c)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							v = (*c)->getVehicles()->begin();
							v != (*c)->getVehicles()->end();
							++v)
					{
						std::wstring tranTank = tr((*v)->getRules()->getType());
						if (tranTank == item)
						{
							tQty++;
						}

						if ((*v)->getAmmo() != 255)
						{
							RuleItem* tankRule = _game->getRuleset()->getItem((*v)->getRules()->getType());
							RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());

							std::wstring tranTank_a = tr(ammoRule->getType());
							if (tranTank_a == item)
							{
								tQty += (*v)->getAmmo();
							}
						}
					}
				}
			}

			_destQty.push_back(tQty);
			// kL_end.

			std::wstringstream
				ss1,
				ss2;
			ss1 << qty;
			ss2 << tQty; // kL
//kL			ss2 << _destQty.back();
//			_lstItems->addRow( // kL
//							4,
//							item.c_str(),
//							ss1.str().c_str(),
//							L"0",
//							ss2.str().c_str());
//kL			std::wstring item = tr(*i);
			RuleItem* rule = _game->getRuleset()->getItem(*i);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				item.insert(0, L"  ");
				_lstItems->addRow(
								4,
								item.c_str(),
								ss1.str().c_str(),
								L"0",
								ss2.str().c_str());
				_lstItems->setRowColor(_baseQty.size() - 1, Palette::blockOffset(15) + 6);
			}
			else
				_lstItems->addRow(
								4,
								item.c_str(),
								ss1.str().c_str(),
								L"0",
								ss2.str().c_str());
		}
	}

	_distance = getDistance();

	_timerInc = new Timer(280);
	_timerInc->onTimer((StateHandler)& TransferItemsState::increase);

	_timerDec = new Timer(280);
	_timerDec->onTimer((StateHandler)& TransferItemsState::decrease);
}

/**
 *
 */
TransferItemsState::~TransferItemsState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Resets the palette since it's bound to change on other screens.
 */
void TransferItemsState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);
}

/**
 * Re-initialize the Transfer menu.
 * Called when cancelling TransferConfirmState.
 */
void TransferItemsState::reinit()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);

	_lstItems->clearList();

	_baseQty.clear();
	_destQty.clear();
	_transferQty.clear();

	_soldiers.clear();
	_crafts.clear();
	_items.clear();

	_sel = 0;
	_total = 0;
	_pQty = 0;
	_cQty = 0;
	_aQty = 0;
	_iQty = 0.0f;
	_hasSci = 0;
	_hasEng = 0;
	_itemOffset = 0;


	_btnOk->setVisible(false);

	for (std::vector<Soldier*>::iterator
			i = _baseFrom->getSoldiers()->begin();
			i != _baseFrom->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == 0)
		{
			_baseQty.push_back(1);
			_transferQty.push_back(0);
			_destQty.push_back(0);
			_soldiers.push_back(*i);

			_lstItems->addRow(
							4,
							(*i)->getName().c_str(),
							L"1",
							L"0",
							L"0");

			++_itemOffset;
		}
	}

	for (std::vector<Craft*>::iterator
			i = _baseFrom->getCrafts()->begin();
			i != _baseFrom->getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT"
			|| (_canTransferCraftsWhileAirborne
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

			++_itemOffset;
		}
	}

	if (_baseFrom->getScientists() > 0)
	{
		_baseQty.push_back(_baseFrom->getScientists());
		_transferQty.push_back(0);
		_destQty.push_back(_baseTo->getTotalScientists());
		_hasSci = 1;

		std::wstringstream
			ss1,
			ss2;
//kL		ss << _baseFrom->getAvailableScientists();
//kL		ss2 << _baseTo->getAvailableScientists();
		ss1 << _baseFrom->getScientists(); // kL
		ss2 << _baseTo->getScientists(); // kL

		_lstItems->addRow(
						4,
						tr("STR_SCIENTIST").c_str(),
						ss1.str().c_str(),
						L"0",
						ss2.str().c_str());

		++_itemOffset;
	}

	if (_baseFrom->getEngineers() > 0)
	{
		_baseQty.push_back(_baseFrom->getEngineers());
		_transferQty.push_back(0);
		_destQty.push_back(_baseTo->getTotalEngineers());
		_hasEng = 1;

		std::wstringstream
			ss1,
			ss2;
		ss1 << _baseFrom->getEngineers();
		ss2 << _baseTo->getEngineers();

		_lstItems->addRow(
						4,
						tr("STR_ENGINEER").c_str(),
						ss1.str().c_str(),
						L"0",
						ss2.str().c_str());

		++_itemOffset;
	}

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		int qty = _baseFrom->getItems()->getItem(*i);
		if (qty > 0)
		{
			_baseQty.push_back(qty);
			_transferQty.push_back(0);
//			_destQty.push_back(_baseTo->getItems()->getItem(*i));
			_items.push_back(*i);

			std::wstring item = tr(*i);

			int tQty = _baseTo->getItems()->getItem(*i); // Returns the quantity of an item in the container.

			for (std::vector<Transfer*>::const_iterator
					j = _baseTo->getTransfers()->begin();
					j != _baseTo->getTransfers()->end();
					++j)
			{
				std::wstring trItem = (*j)->getName(_game->getLanguage());
				if (trItem == item)
				{
					tQty += (*j)->getQuantity();
				}
			}

			// Add qty of items & vehicles on transport craft to Transfers, _baseTo screen stock.
			for (std::vector<Craft*>::const_iterator
					c = _baseTo->getCrafts()->begin();
					c != _baseTo->getCrafts()->end();
					++c)
			{
				if ((*c)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::iterator
							t = (*c)->getItems()->getContents()->begin();
							t != (*c)->getItems()->getContents()->end();
							++t)
					{
						std::wstring tranItem = tr(t->first);
						if (tranItem == item)
						{
							tQty += t->second;
						}
					}
				}

				if ((*c)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							v = (*c)->getVehicles()->begin();
							v != (*c)->getVehicles()->end();
							++v)
					{
						std::wstring tranTank = tr((*v)->getRules()->getType());
						if (tranTank == item)
						{
							tQty++;
						}

						if ((*v)->getAmmo() != 255)
						{
							RuleItem* tankRule = _game->getRuleset()->getItem((*v)->getRules()->getType());
							RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());

							std::wstring tranTank_a = tr(ammoRule->getType());
							if (tranTank_a == item)
							{
								tQty += (*v)->getAmmo();
							}
						}
					}
				}
			}

			_destQty.push_back(tQty);

			std::wstringstream
				ss1,
				ss2;
			ss1 << qty;
			ss2 << tQty;
//			_lstItems->addRow(
//							4,
//							item.c_str(),
//							ss1.str().c_str(),
//							L"0",
//							ss2.str().c_str());
			RuleItem* rule = _game->getRuleset()->getItem(*i);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				item.insert(0, L"  ");
				_lstItems->addRow(
								4,
								item.c_str(),
								ss1.str().c_str(),
								L"0",
								ss2.str().c_str());
				_lstItems->setRowColor(_baseQty.size() - 1, Palette::blockOffset(15) + 6);
			}
			else
				_lstItems->addRow(
								4,
								item.c_str(),
								ss1.str().c_str(),
								L"0",
								ss2.str().c_str());
		}
	}
}

/**
 * Runs the arrow timers.
 */
void TransferItemsState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Transfers the selected items.
 * @param action Pointer to an action.
 */
void TransferItemsState::btnOkClick(Action*)
{
	_game->pushState(new TransferConfirmState(
											_game,
											_baseTo,
											this));
}

/**
 * Completes the transfer between bases.
 */
void TransferItemsState::completeTransfer()
{
	int time = static_cast<int>(floor(6.0 + _distance / 10.0));
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _total);
	_baseFrom->setCashSpent(_total); // kL

	for (unsigned int
			i = 0;
			i < _transferQty.size();
			++i)
	{
		if (_transferQty[i] > 0)
		{
			if (i < _soldiers.size()) // Transfer soldiers
			{
				for (std::vector<Soldier*>::iterator
						s = _baseFrom->getSoldiers()->begin();
						s != _baseFrom->getSoldiers()->end();
						++s)
				{
					if (*s == _soldiers[i])
					{
						if ((*s)->isInPsiTraining())
							(*s)->setPsiTraining();

						Transfer* t = new Transfer(time);
						t->setSoldier(*s);
						_baseTo->getTransfers()->push_back(t);
						_baseFrom->getSoldiers()->erase(s);

						break;
					}
				}
			}
			else if (i >= _soldiers.size()
				&& i < _soldiers.size() + _crafts.size()) // Transfer crafts w/ soldiers inside (but not items)
			{
				Craft* craft = _crafts[i - _soldiers.size()];

				for (std::vector<Soldier*>::iterator // Transfer soldiers inside craft
						s = _baseFrom->getSoldiers()->begin();
						s != _baseFrom->getSoldiers()->end();
						)
				{
					if ((*s)->getCraft() == craft)
					{
						if ((*s)->isInPsiTraining())
							(*s)->setPsiTraining();

						if (craft->getStatus() == "STR_OUT")
							_baseTo->getSoldiers()->push_back(*s);
						else
						{
							Transfer* t = new Transfer(time);
							t->setSoldier(*s);
							_baseTo->getTransfers()->push_back(t);
						}

						s = _baseFrom->getSoldiers()->erase(s);
					}
					else
						++s;
				}

				for (std::vector<Craft*>::iterator // Transfer craft
						c = _baseFrom->getCrafts()->begin();
						c != _baseFrom->getCrafts()->end();
						++c)
				{
					if (*c == craft)
					{
						if (craft->getStatus() == "STR_OUT")
						{
							bool returning = (craft->getDestination() == (Target*)craft->getBase());
							_baseTo->getCrafts()->push_back(craft);
							craft->setBaseOnly(_baseTo);

							if (craft->getFuel() <= craft->getFuelLimit(_baseTo))
							{
								craft->setLowFuel(true);
								craft->returnToBase();
							}
							else if (returning)
							{
								craft->setLowFuel(false);
								craft->returnToBase();
							}
						}
						else
						{
							Transfer* t = new Transfer(time);
							t->setCraft(*c);
							_baseTo->getTransfers()->push_back(t);
						}

						// Clear Hangar
						for (std::vector<BaseFacility*>::iterator
								f = _baseFrom->getFacilities()->begin();
								f != _baseFrom->getFacilities()->end();
								++f)
						{
							if ((*f)->getCraft() == *c)
							{
								(*f)->setCraft(0);

								break;
							}
						}

						_baseFrom->getCrafts()->erase(c);

						break;
					}
				}
			}
//kL			else if (_baseFrom->getAvailableScientists() > 0
			else if (_baseFrom->getScientists() > 0 // kL
				&& i == _soldiers.size() + _crafts.size()) // Transfer scientists
			{
				_baseFrom->setScientists(_baseFrom->getScientists() - _transferQty[i]);
				Transfer* t = new Transfer(time);
				t->setScientists(_transferQty[i]);

				_baseTo->getTransfers()->push_back(t);
			}
//kL			else if (_baseFrom->getAvailableEngineers() > 0
			else if (_baseFrom->getEngineers() > 0 // kL
				&& i == _soldiers.size() + _crafts.size() + _hasSci) // Transfer engineers
			{
				_baseFrom->setEngineers(_baseFrom->getEngineers() - _transferQty[i]);
				Transfer* t = new Transfer(time);
				t->setEngineers(_transferQty[i]);

				_baseTo->getTransfers()->push_back(t);
			}
			else // Transfer items
			{
				_baseFrom->getItems()->removeItem(_items[getItemIndex(i)], _transferQty[i]);
				Transfer* t = new Transfer(time);
				t->setItems(_items[getItemIndex(i)], _transferQty[i]);

				_baseTo->getTransfers()->push_back(t);
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TransferItemsState::btnCancelClick(Action*)
{
	_game->setPalette( // kL, from TransferBaseState
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(4)),
				Palette::backPos,
				16);

	_game->popState(); // pop main Transfer (this)
//kL	_game->popState(); // pop choose Destination
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsLeftArrowPress(Action* action)
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
void TransferItemsState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected item;
 * by one on left-click; to max on right-click.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsLeftArrowClick(Action* action)
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
void TransferItemsState::lstItemsRightArrowPress(Action* action)
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
void TransferItemsState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected item;
 * by one on left-click; to 0 on right-click.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsRightArrowClick(Action* action)
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
void TransferItemsState::lstItemsMousePress(Action* action)
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
 * Gets the transfer cost of the currently selected item.
 * @return, Transfer cost
 */
int TransferItemsState::getCost() const
{
	//Log(LOG_INFO) << "TransferItemsState::getCost()";
	float cost = 0.f;

	if (TRANSFER_ITEM == getType(_sel)) // Item cost
	{
		RuleItem* rule = _game->getRuleset()->getItem(_items[_sel - _itemOffset]);
		if (rule->getType() == "STR_ALIEN_ALLOYS")
		{
			//Log(LOG_INFO) << ". transfer Alloys";
			cost = 0.01f;
		}
		else if (rule->getType() == "STR_ELERIUM_115")
		{
			//Log(LOG_INFO) << ". transfer Elerium";
			cost = 0.1f;
		}
		else
		{
			//Log(LOG_INFO) << ". transfer Item";
			cost = 1.f;
		}
	}
	else if (TRANSFER_CRAFT == getType(_sel)) // Craft cost
	{
		//Log(LOG_INFO) << ". transfer Craft";
		cost = 100.f;
	}
	else // Personnel cost
	{
		//Log(LOG_INFO) << ". transfer Personel";
		cost = 10.f;
	}

//	float fCost = static_cast<float>(floor(_distance * cost));
	int iCost = static_cast<int>(ceil(static_cast<float>(_distance) * cost));
	//Log(LOG_INFO) << ". . _distance = " << _distance;
	//Log(LOG_INFO) << ". . cost = " << iCost;

	return iCost;
}

/**
 * Gets the quantity of the currently selected item on the base.
 * @return Item quantity.
 */
int TransferItemsState::getQuantity() const
{
	switch (getType(_sel))
	{
		case TRANSFER_SOLDIER:
		case TRANSFER_CRAFT:		return 1;
//kL		case TRANSFER_SCIENTIST:	return _baseFrom->getAvailableScientists();
//kL		case TRANSFER_ENGINEER:		return _baseFrom->getAvailableEngineers();
		case TRANSFER_SCIENTIST:	return _baseFrom->getScientists(); // kL
		case TRANSFER_ENGINEER:		return _baseFrom->getEngineers(); // kL
		case TRANSFER_ITEM:			return _baseFrom->getItems()->getItem(_items[getItemIndex(_sel)]);
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

	increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to transfer.
 * @param change How many we want to add.
 */
void TransferItemsState::increaseByValue(int change)
{
	const enum TransferType selType = getType(_sel);
	RuleItem* selItem = NULL;

	if (TRANSFER_ITEM == selType)
		selItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);

	if (change < 1
		|| getQuantity() <= _transferQty[_sel])
	{
		return;
	}

	// Check Living Quarters
	if ((TRANSFER_SOLDIER == selType
			|| TRANSFER_SCIENTIST == selType
			|| TRANSFER_ENGINEER == selType)
		&& _pQty + 1 > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NO_FREE_ACCOMODATION",
											Palette::blockOffset(15) + 1,
											"BACK13.SCR",
											0));

		return;
	}

	if (TRANSFER_CRAFT == selType)
	{
		Craft* craft = _crafts[_sel - _soldiers.size()];

		if (_cQty + 1 > _baseTo->getAvailableHangars() - _baseTo->getUsedHangars())
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												_game,
												"STR_NO_FREE_HANGARS_FOR_TRANSFER",
												Palette::blockOffset(15) + 1,
												"BACK13.SCR",
												0));

			return;
		}

		if (_pQty + craft->getNumSoldiers() > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												_game,
												"STR_NO_FREE_ACCOMODATION_CREW",
												Palette::blockOffset(15) + 1,
												"BACK13.SCR",
												0));

			return;
		}
	}

	if (TRANSFER_ITEM == selType
		&& !selItem->getAlien()
		&& (_iQty + selItem->getSize()) > (_baseTo->getAvailableStores() - _baseTo->getUsedStores()))
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NOT_ENOUGH_STORE_SPACE",
											Palette::blockOffset(15) + 1,
											"BACK13.SCR",
											0));

		return;
	}

	if (TRANSFER_ITEM == selType
		&& selItem->getAlien()
		&& (_containmentLimit * _aQty) + 1 > _baseTo->getAvailableContainment() - (_containmentLimit * _baseTo->getUsedContainment()))
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											_game,
											"STR_NO_ALIEN_CONTAINMENT_FOR_TRANSFER",
											Palette::blockOffset(15) + 1,
											"BACK13.SCR",
											0));

		return;
	}


	if (TRANSFER_SOLDIER == selType
		|| TRANSFER_SCIENTIST == selType
		|| TRANSFER_ENGINEER == selType) // Personnel count
	{
		int freeQuarters = _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters() - _pQty;
		change = std::min(std::min(freeQuarters, getQuantity() - _transferQty[_sel]), change);
		_pQty += change;
		_baseQty[_sel] -= change;
		_destQty[_sel] += change;
		_transferQty[_sel] += change;
		_total += getCost() * change;
	}
	else if (TRANSFER_CRAFT == selType) // Craft count
	{
		Craft* craft = _crafts[_sel - _soldiers.size()];
		_cQty++;
		_pQty += craft->getNumSoldiers();
		_baseQty[_sel]--;
		_destQty[_sel]++;
		_transferQty[_sel]++;

		if (!_canTransferCraftsWhileAirborne
			|| craft->getStatus() != "STR_OUT")
		{
			_total += getCost();
		}
	}
	else if (TRANSFER_ITEM == selType
		&& !selItem->getAlien()) // Item count
	{
		float storesNeededPerItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize();
		float freeStores = static_cast<float>(_baseTo->getAvailableStores() - _baseTo->getUsedStores()) - _iQty;

		int freeStoresForItem;
		if (AreSame(storesNeededPerItem, 0.f))
		{
			freeStoresForItem = INT_MAX;
		}
		else
		{
			freeStoresForItem = static_cast<int>(floor(freeStores / storesNeededPerItem));
		}

		change = std::min(std::min(freeStoresForItem, getQuantity() - _transferQty[_sel]), change);
		_iQty += (static_cast<float>(change)) * storesNeededPerItem;
		_baseQty[_sel] -= change;
		_destQty[_sel] += change;
		_transferQty[_sel] += change;
		_total += getCost() * change;
	}
	else if (TRANSFER_ITEM == selType
		&& selItem->getAlien()) // Live Aliens count
	{
		int freeContainment = _containmentLimit? _baseTo->getAvailableContainment() - _baseTo->getUsedContainment() - _aQty: INT_MAX;
		change = std::min(std::min(freeContainment, getQuantity() - _transferQty[_sel]), change);
		_aQty += change;
		_baseQty[_sel] -= change;
		_destQty[_sel] += change;
		_transferQty[_sel] += change;
		_total += getCost() * change;
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

	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to transfer.
 * @param change How many we want to remove.
 */
void TransferItemsState::decreaseByValue(int change)
{
	if (change < 1 || _transferQty[_sel] < 1)
		return;


	change = std::min(
					_transferQty[_sel],
					change);
	Craft* craft = 0;

	const enum TransferType selType = getType(_sel);
	if (TRANSFER_SOLDIER == selType
		|| TRANSFER_SCIENTIST == selType
		|| TRANSFER_ENGINEER == selType) // Personnel count
	{
		_pQty -= change;
	}
	else if (TRANSFER_CRAFT == selType) // Craft count
	{
		craft = _crafts[_sel - _soldiers.size()];

		_cQty--;
		_pQty -= craft->getNumSoldiers();
	}
	else if (TRANSFER_ITEM == selType) // Item count
	{
		const RuleItem* selItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
		if (!selItem->getAlien())
		{
			_iQty -= selItem->getSize() * (static_cast<float>(change));
		}
		else
		{
			_aQty -= change;
		}
	}

	_baseQty[_sel] += change;
	_destQty[_sel] -= change;
	_transferQty[_sel] -= change;

	if (!_canTransferCraftsWhileAirborne
		|| craft == 0
		|| craft->getStatus() != "STR_OUT")
	{
		_total -= getCost() * change;
	}

	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void TransferItemsState::updateItemStrings()
{
	std::wstringstream
		ss1,
		ss2,
		ss3;

	ss1 << _baseQty[_sel];
	_lstItems->setCellText(_sel, 1, ss1.str());

	ss2 << _transferQty[_sel];
	_lstItems->setCellText(_sel, 2, ss2.str());

	ss3 << _destQty[_sel];
	_lstItems->setCellText(_sel, 3, ss3.str());

	if (_transferQty[_sel] > 0)
		_lstItems->setRowColor(_sel, Palette::blockOffset(13));
	else
	{
		_lstItems->setRowColor(_sel, Palette::blockOffset(13) + 10);

		if (_sel > _itemOffset)
		{
			RuleItem* rule = _game->getRuleset()->getItem(_items[_sel - _itemOffset]);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, Palette::blockOffset(15) + 6);
			}
		}
	}

	if (_total > 0)
		_btnOk->setVisible(true);
	else
		_btnOk->setVisible(false);
}

/**
 * Gets the total cost of the current transfer.
 * @return Total cost.
 */
int TransferItemsState::getTotal() const
{
	return _total;
}

/**
 * Gets the shortest distance between the two bases.
 * @return Distance.
 */
double TransferItemsState::getDistance() const
{
	double
		x[3],
		y[3],
		z[3],
		r = 51.2; // kL_note: what's this? conversion factor??? is it right??

	Base* base = _baseFrom;

	for (int
			i = 0;
			i < 2;
			++i)
	{
		x[i] = r * cos(base->getLatitude()) * cos(base->getLongitude());
		y[i] = r * cos(base->getLatitude()) * sin(base->getLongitude());
		z[i] = r * -sin(base->getLatitude());

		base = _baseTo;
	}

	x[2] = x[1] - x[0];
	y[2] = y[1] - y[0];
	z[2] = z[1] - z[0];

	return sqrt((x[2] * x[2]) + (y[2] * y[2]) + (z[2] * z[2]));
}

/**
 * Gets type of selected item.
 * @param selected, The selected item.
 * @return, The type of the selected item.
 */
enum TransferType TransferItemsState::getType(unsigned selected) const
{
	unsigned max = _soldiers.size();

	if (selected < max)
		return TRANSFER_SOLDIER;

	if (selected < (max += _crafts.size()))
		return TRANSFER_CRAFT;

	if (selected < (max += _hasSci))
		return TRANSFER_SCIENTIST;

	if (selected < (max += _hasEng))
		return TRANSFER_ENGINEER;

	return TRANSFER_ITEM;
}

/**
 * Gets the index of the selected item.
 * @param selected Currently selected item.
 * @return Index of the selected item.
 */
int TransferItemsState::getItemIndex(unsigned selected) const
{
	return selected - _soldiers.size() - _crafts.size() - _hasSci - _hasEng;
}

}
