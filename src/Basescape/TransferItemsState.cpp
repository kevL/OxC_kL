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

#include "TransferItemsState.h"

#include <cfloat>
#include <climits>
#include <cmath>
#include <sstream>

#include "../fmath.h"

#include "TransferConfirmState.h"

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
		Base* baseFrom,
		Base* baseTo)
	:
		_baseQty(),
		_destQty(),
		_transferQty(),

		_baseFrom(baseFrom),
		_baseTo(baseTo),

		_soldiers(),
		_crafts(),
		_items(),
		_sel(0),
		_offset(0),
		_total(0),

		_pQty(0),
		_cQty(0),
		_aQty(0),
		_iQty(0.0),

		_hasSci(0),
		_hasEng(0),

		_distance(0.0)
{
	_window					= new Window(this, 320, 200, 0, 0);
	_txtTitle				= new Text(300, 16, 10, 9);
	_txtBaseFrom			= new Text(80, 9, 16, 9);
	_txtBaseTo				= new Text(80, 9, 224, 9);

	_txtItem				= new Text(128, 9, 16, 24);
	_txtQuantity			= new Text(35, 9, 160, 24);
	_txtAmountTransfer		= new Text(46, 9, 200, 24);
	_txtAmountDestination	= new Text(62, 9, 247, 24);

	_lstItems				= new TextList(285, 136, 16, 35);

	_btnCancel				= new TextButton(134, 16, 16, 177);
	_btnOk					= new TextButton(134, 16, 170, 177);

	setPalette("PAL_BASESCAPE", 0);

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
					Options::keyOk);
	_btnOk->setVisible(false);

	_btnCancel->setColor(Palette::blockOffset(15)+6);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& TransferItemsState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferItemsState::btnCancelClick,
					Options::keyCancel);

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
//	_lstItems->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstItems->onLeftArrowPress((ActionHandler)& TransferItemsState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)& TransferItemsState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)& TransferItemsState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)& TransferItemsState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)& TransferItemsState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)& TransferItemsState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)& TransferItemsState::lstItemsMousePress);

	_distance = getDistance();

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)& TransferItemsState::increase);

	_timerDec = new Timer(250);
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
 * Re-initialize the Transfer menu.
 * Called when cancelling TransferConfirmState.
 */
void TransferItemsState::init()
{
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
	_iQty = 0;
	_hasSci = 0;
	_hasEng = 0;
	_offset = 0;


	_btnOk->setVisible(false);

	// from above, cTor.
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

			// kL_begin:
			std::wostringstream woStr;
			woStr << (*i)->getName();

			if ((*i)->getWoundRecovery() > 0)
				woStr << L" (" << (*i)->getWoundRecovery() << L" dy)";
			// kL_end.

			_lstItems->addRow(
							4,
//kL						(*i)->getName().c_str(),
							woStr.str().c_str(), // kL
							L"1",
							L"0",
							L"0");

			++_offset;
		}
	}

	for (std::vector<Craft*>::iterator
			i = _baseFrom->getCrafts()->begin();
			i != _baseFrom->getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT"
			|| (Options::canTransferCraftsWhileAirborne
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

			++_offset;
		}
	}

//kL	if (_baseFrom->getAvailableScientists() > 0)
	if (_baseFrom->getScientists() > 0) // kL
	{
//kL	_baseQty.push_back(_baseFrom->getAvailableScientists());
		_baseQty.push_back(_baseFrom->getScientists()); // kL
		_transferQty.push_back(0);
//kL	_destQty.push_back(_baseTo->getAvailableScientists());
		_destQty.push_back(_baseTo->getTotalScientists()); // kL
		_hasSci = 1;

		std::wostringstream
			ss1,
			ss2;
//kL	ss1 << _baseFrom->getAvailableScientists();
//kL	ss2 << _baseTo->getAvailableScientists();
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

		++_offset;
	}

//kL	if (_baseFrom->getAvailableEngineers() > 0)
	if (_baseFrom->getEngineers() > 0) // kL
	{
//kL	_baseQty.push_back(_baseFrom->getAvailableEngineers());
		_baseQty.push_back(_baseFrom->getEngineers()); // kL
		_transferQty.push_back(0);
//kL	_destQty.push_back(_baseTo->getAvailableEngineers());
		_destQty.push_back(_baseTo->getTotalEngineers()); // kL
		_hasEng = 1;

		std::wostringstream
			ss1,
			ss2;
//kL	ss1 << _baseFrom->getAvailableEngineers();
//kL	ss2 << _baseTo->getAvailableEngineers();
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

		++_offset;
	}

	SavedGame* save = _game->getSavedGame();
	Ruleset* rules = _game->getRuleset();
	RuleItem
		* itemRule,
		* launcher,
		* clip;
	RuleCraftWeapon* cwRule;

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
//kL		_destQty.push_back(_baseTo->getItems()->getItem(*i));
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
					tQty += (*j)->getQuantity();
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
						std::wstring tranTank = tr((*v)->getRules()->getType());
						if (tranTank == item)
							tQty++;

						if ((*v)->getAmmo() != 255)
						{
							RuleItem* tankRule = _game->getRuleset()->getItem((*v)->getRules()->getType());
							RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());

							std::wstring tranTank_a = tr(ammoRule->getType());
							if (tranTank_a == item)
								tQty += (*v)->getAmmo();
						}
					}
				}
			}

			_destQty.push_back(tQty);
			// kL_end.

			// kL_begin:
			itemRule = rules->getItem(*i);

			bool craftOrdnance = false;
			const std::vector<std::string>& craftWeaps = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					j = craftWeaps.begin();
					j != craftWeaps.end()
						&& craftOrdnance == false;
					++j)
			{
				// Special handling for treating craft weapons as items
				cwRule = rules->getCraftWeapon(*j);

				launcher = rules->getItem(cwRule->getLauncherItem());
				clip = rules->getItem(cwRule->getClipItem());

				if (launcher == itemRule)
				{
					craftOrdnance = true;

					int clipSize = cwRule->getAmmoMax(); // Launcher
					if (clipSize > 0)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
				}
				else if (clip == itemRule)
				{
					craftOrdnance = true;

					int clipSize = clip->getClipSize(); // launcher Ammo
					if (clipSize > 1)
						item = item + L"s (" + Text::formatNumber(clipSize) + L")";
				}
			}

			Uint8 color = Palette::blockOffset(13)+10; // blue
			if (!save->isResearched(itemRule->getType())				// not researched
				&& (!save->isResearched(itemRule->getRequirements())	// and has requirements to use but not been researched
					|| rules->getItem(*i)->getAlien()					// or is an alien
					|| itemRule->getBattleType() == BT_CORPSE				// or is a corpse
					|| itemRule->getBattleType() == BT_NONE)				// or is not a battlefield item
				&& craftOrdnance == false)							// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = Palette::blockOffset(13)+5; // yellow
			}
			// kL_end.

			std::wostringstream
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

//			RuleItem* rule = _game->getRuleset()->getItem(*i);
			if (itemRule->getBattleType() == BT_AMMO
				|| (itemRule->getBattleType() == BT_NONE
					&& itemRule->getClipSize() > 0))
			{
				if (itemRule->getBattleType() == BT_AMMO
					&& itemRule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					int clipSize = itemRule->getClipSize();
					if (clipSize > 1)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
				}

				item.insert(0, L"  ");

				_lstItems->addRow(
								4,
								item.c_str(),
								ss1.str().c_str(),
								L"0",
								ss2.str().c_str());

				if (color != Palette::blockOffset(13)+5) // yellow
					color = Palette::blockOffset(15)+6; // purple
			}
			else
			{
                if (itemRule->isFixed() // tank w/ Ordnance.
					&& !itemRule->getCompatibleAmmo()->empty())
                {
					clip = _game->getRuleset()->getItem(itemRule->getCompatibleAmmo()->front());
					int clipSize = clip->getClipSize();
					if (clipSize > 0)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
                }

				_lstItems->addRow(
								4,
								item.c_str(),
								ss1.str().c_str(),
								L"0",
								ss2.str().c_str());
			}

			_lstItems->setRowColor(_baseQty.size() - 1, color);
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

	for (size_t
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
							craft->setBase(_baseTo, false);

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
		_timerInc->stop();
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

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
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
		_timerDec->stop();
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

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
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
}

/**
 * Gets the transfer cost of the currently selected item.
 * @return, Transfer cost
 */
int TransferItemsState::getCost() const
{
	// kL_note: All prices increased tenfold.
	double cost = 0.0;

	if (TRANSFER_ITEM == getType(_sel)) // Item cost
	{
		RuleItem* rule = _game->getRuleset()->getItem(_items[_sel - _offset]);
		if (rule->getType() == "STR_ALIEN_ALLOYS")
			cost = 0.1;
		else if (rule->getType() == "STR_ELERIUM_115")
			cost = 1.0;
		else if (rule->getAlien())
			cost = 200.0;
		else
			cost = 10.0;
	}
	else if (TRANSFER_CRAFT == getType(_sel)) // Craft cost
		cost = 1000.0;
	else // Personnel cost
		cost = 100.0;

	return static_cast<int>(ceil(_distance * cost));
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
	if (change < 1
		|| _transferQty[_sel] >= getQuantity())
	{
		return;
	}

	const enum TransferType selType = getType(_sel);

	if ((selType == TRANSFER_SOLDIER // Check Living Quarters
			|| selType == TRANSFER_SCIENTIST
			|| selType == TRANSFER_ENGINEER)
		&& _pQty + 1 > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
	{
		_timerInc->stop();
		_game->pushState(new ErrorMessageState(
											tr("STR_NO_FREE_ACCOMODATION"),
											_palette,
											Palette::blockOffset(15)+1,
											"BACK13.SCR",
											0));
		return;
	}
	else if (selType == TRANSFER_CRAFT)
	{
		Craft* craft = _crafts[_sel - _soldiers.size()];

		if (_cQty + 1 > _baseTo->getAvailableHangars() - _baseTo->getUsedHangars())
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												tr("STR_NO_FREE_HANGARS_FOR_TRANSFER"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												0));
			return;
		}

		if (_pQty + craft->getNumSoldiers() > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												tr("STR_NO_FREE_ACCOMODATION_CREW"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												0));
			return;
		}

		if (Options::storageLimitsEnforced
			&& _baseTo->storesOverfull(_iQty + craft->getItems()->getTotalSize(_game->getRuleset())))
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												tr("STR_NOT_ENOUGH_STORE_SPACE_FOR_CRAFT"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												0));
			return;
		}
		// kL_note: What about ITEMS on board craft
	}


	RuleItem* selItem = NULL;
	if (selType == TRANSFER_ITEM)
		selItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);

	if (selType == TRANSFER_ITEM)
	{
		if (!selItem->getAlien()
			&& _baseTo->storesOverfull(selItem->getSize() + _iQty))
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												tr("STR_NOT_ENOUGH_STORE_SPACE"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												0));
			return;
		}
		else if (selItem->getAlien()
			&& (Options::storageLimitsEnforced * _aQty) + 1
					> _baseTo->getAvailableContainment() - (Options::storageLimitsEnforced * _baseTo->getUsedContainment()))
		{
			_timerInc->stop();
			_game->pushState(new ErrorMessageState(
												tr("STR_NO_ALIEN_CONTAINMENT_FOR_TRANSFER"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												0));
			return;
		}
	}
	// errors Done.


	if (selType == TRANSFER_SOLDIER // Personnel count
		|| selType == TRANSFER_SCIENTIST
		|| selType == TRANSFER_ENGINEER)
	{
		int freeQuarters = _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters() - _pQty;
		change = std::min(
						std::min(
								freeQuarters,
								getQuantity() - _transferQty[_sel]),
						change);
		_pQty += change;
		_baseQty[_sel] -= change;
		_destQty[_sel] += change;
		_transferQty[_sel] += change;
		_total += getCost() * change;
	}
	else if (selType == TRANSFER_CRAFT) // Craft count
	{
		Craft* craft = _crafts[_sel - _soldiers.size()];
		_cQty++;
		_pQty += craft->getNumSoldiers();
		_iQty += craft->getItems()->getTotalSize(_game->getRuleset());

		_baseQty[_sel]--;
		_destQty[_sel]++;
		_transferQty[_sel]++;

		if (!Options::canTransferCraftsWhileAirborne
			|| craft->getStatus() != "STR_OUT")
		{
			_total += getCost();
		}
	}
	else if (selType == TRANSFER_ITEM)
	{
		if (!selItem->getAlien()) // Item count
		{
			double
				storesNeededPerItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)])->getSize(),
				freeStores = static_cast<double>(_baseTo->getAvailableStores()) - _baseTo->getUsedStores() - _iQty,
				freeStoresForItem = DBL_MAX;

			if (!AreSame(storesNeededPerItem, 0.0))
				freeStoresForItem = (freeStores + 0.05) / storesNeededPerItem;

			change = std::min(
							std::min(
									static_cast<int>(freeStoresForItem),
									getQuantity() - _transferQty[_sel]),
							change);
			_iQty += static_cast<double>(change) * storesNeededPerItem;
			_baseQty[_sel] -= change;
			_destQty[_sel] += change;
			_transferQty[_sel] += change;
			_total += getCost() * change;
		}
		else if (selItem->getAlien()) // Live Alien count
		{
			int freeContainment = INT_MAX;
			if (Options::storageLimitsEnforced)
				freeContainment = _baseTo->getAvailableContainment() - _baseTo->getUsedContainment() - _aQty;

			change = std::min(
							std::min(
									freeContainment,
									getQuantity() - _transferQty[_sel]),
							change);
			_aQty += change;
			_baseQty[_sel] -= change;
			_destQty[_sel] += change;
			_transferQty[_sel] += change;
			_total += getCost() * change;
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

	Craft* craft = NULL;

	const enum TransferType selType = getType(_sel);

	if (selType == TRANSFER_SOLDIER
		|| selType == TRANSFER_SCIENTIST
		|| selType == TRANSFER_ENGINEER) // Personnel count
	{
		_pQty -= change;
	}
	else if (selType == TRANSFER_CRAFT) // Craft count
	{
		craft = _crafts[_sel - _soldiers.size()];

		_cQty--;
		_pQty -= craft->getNumSoldiers();
		_iQty -= craft->getItems()->getTotalSize(_game->getRuleset());
	}
	else if (selType == TRANSFER_ITEM) // Item count
	{
		const RuleItem* selItem = _game->getRuleset()->getItem(_items[getItemIndex(_sel)]);
		if (!selItem->getAlien())
			_iQty -= selItem->getSize() * static_cast<double>(change);
		else
			_aQty -= change;
	}

	_baseQty[_sel] += change;
	_destQty[_sel] -= change;
	_transferQty[_sel] -= change;

	if (!Options::canTransferCraftsWhileAirborne
		|| craft == NULL
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
	std::wostringstream
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
		_lstItems->setRowColor(_sel, Palette::blockOffset(13)+10);

		if (_sel > _offset)
		{
			RuleItem* rule = _game->getRuleset()->getItem(_items[_sel - _offset]);
			if (rule->getBattleType() == BT_AMMO
				|| (rule->getBattleType() == BT_NONE
					&& rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, Palette::blockOffset(15)+6);
			}
		}
	}

	_btnOk->setVisible(_total > 0);
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
enum TransferType TransferItemsState::getType(size_t selected) const
{
	size_t max = _soldiers.size();

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
size_t TransferItemsState::getItemIndex(size_t selected) const
{
	return selected - _soldiers.size() - _crafts.size() - _hasSci - _hasEng;
}

}
