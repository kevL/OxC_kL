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

#include "ManageAlienContainmentState.h"

#include <sstream>
#include <climits>
#include <cmath>

#include "SellState.h"

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

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Manage Alien Containment screen.
 * @param base		- pointer to the base to get info from
 * @param origin	- game section that originated this state
 * @param allowHelp	- true to get researchHelp from executed aLiens
 */
ManageAlienContainmentState::ManageAlienContainmentState(
		Base* base,
		OptionsOrigin origin,
		bool allowHelp) // kL_add.
	:
		_base(base),
		_origin(origin),
		_sel(0),
		_aliensSold(0),
//kL	_researchAliens(0)
		_allowHelp(allowHelp) // kL
{
	_overCrowded = Options::storageLimitsEnforced
				&& _base->getAvailableContainment() < _base->getUsedContainment();

/*kL
	for (std::vector<ResearchProject*>::const_iterator
			i = _base->getResearch().begin();
			i != _base->getResearch().end();
			++i)
	{
		const RuleResearch* research = (*i)->getRules();
		if (_game->getRuleset()->getUnit(research->getName()))
		{
			++_researchAliens;
		}
	} */

	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 10);
	_txtBaseLabel	= new Text(80, 9, 16, 10);

	_txtUsed		= new Text(144, 9, 16, 30);
	_txtAvailable	= new Text(144, 9, 160, 30);

	_txtItem		= new Text(144, 9, 16, 40);
	_txtLiveAliens	= new Text(62, 9, 164, 40);
	_txtDeadAliens	= new Text(66, 9, 230, 40);

	_lstAliens		= new TextList(285, 121, 16, 50);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);
//	_btnOk			= new TextButton(_overCrowded? 288: 148, 16, _overCrowded? 16: 8, 177);


	if (origin == OPT_BATTLESCAPE)
	{
		setPalette("PAL_GEOSCAPE", 0);

		_color  = Palette::blockOffset(15)-1;
		_color2 = Palette::blockOffset(8)+10;
	}
	else
	{
		setPalette("PAL_BASESCAPE", 1);

		_color  = Palette::blockOffset(13)+10;
		_color2 = Palette::blockOffset(13);
	}

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtAvailable);
	add(_txtUsed);
	add(_txtItem);
	add(_txtLiveAliens);
	add(_txtDeadAliens);
	add(_lstAliens);
	add(_btnOk);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(_color);
	if (origin == OPT_BATTLESCAPE)
		_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));
	else
		_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnOk->setColor(_color);
	_btnOk->setText(tr("STR_REMOVE_SELECTED"));
	_btnOk->onMouseClick((ActionHandler)& ManageAlienContainmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManageAlienContainmentState::btnOkClick,
					Options::keyOk);

	_btnCancel->setColor(_color);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& ManageAlienContainmentState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& ManageAlienContainmentState::btnCancelClick,
					Options::keyCancel);

	_btnOk->setVisible(false);
	if (_overCrowded)
		_btnCancel->setVisible(false);

	if (origin == OPT_BATTLESCAPE)
		_txtTitle->setColor(Palette::blockOffset(8)+5);
	else
		_txtTitle->setColor(_color);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MANAGE_CONTAINMENT"));

	if (origin == OPT_BATTLESCAPE)
		_txtBaseLabel->setColor(Palette::blockOffset(8)+5);
	else
		_txtBaseLabel->setColor(_color);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setColor(_color);
	_txtItem->setText(tr("STR_ALIEN"));

	_txtLiveAliens->setColor(_color);
	_txtLiveAliens->setText(tr("STR_LIVE_ALIENS"));
//kL	_txtLiveAliens->setWordWrap();
//kL	_txtLiveAliens->setVerticalAlign(ALIGN_BOTTOM);

	_txtDeadAliens->setColor(_color);
	_txtDeadAliens->setText(tr("STR_DEAD_ALIENS"));
//kL	_txtDeadAliens->setWordWrap();
//kL	_txtDeadAliens->setVerticalAlign(ALIGN_BOTTOM);

	_txtAvailable->setColor(_color);
	_txtAvailable->setSecondaryColor(_color2);
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE")
							.arg(_base->getAvailableContainment() - _base->getUsedContainment()));

	_txtUsed->setColor(_color);
	_txtUsed->setSecondaryColor(_color2);
	_txtUsed->setText(tr("STR_SPACE_USED")
						.arg(_base->getUsedContainment())); //kL - _researchAliens));

	_lstAliens->setColor(_color2);
	_lstAliens->setArrowColor(_color);
	_lstAliens->setArrowColumn(178, ARROW_HORIZONTAL);
	_lstAliens->setColumns(3, 140, 66, 56);
	_lstAliens->setSelectable();
	_lstAliens->setBackground(_window);
	_lstAliens->setMargin();
//	_lstAliens->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
	_lstAliens->onLeftArrowPress((ActionHandler)& ManageAlienContainmentState::lstItemsLeftArrowPress);
	_lstAliens->onLeftArrowRelease((ActionHandler)& ManageAlienContainmentState::lstItemsLeftArrowRelease);
	_lstAliens->onLeftArrowClick((ActionHandler)& ManageAlienContainmentState::lstItemsLeftArrowClick);
	_lstAliens->onRightArrowPress((ActionHandler)& ManageAlienContainmentState::lstItemsRightArrowPress);
	_lstAliens->onRightArrowRelease((ActionHandler)& ManageAlienContainmentState::lstItemsRightArrowRelease);
	_lstAliens->onRightArrowClick((ActionHandler)& ManageAlienContainmentState::lstItemsRightArrowClick);
	_lstAliens->onMousePress((ActionHandler)& ManageAlienContainmentState::lstItemsMousePress);

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		int qty = _base->getItems()->getItem(*i);				// get Qty of each item at this base
		if (qty > 0												// if item exists at this base
			&& _game->getRuleset()->getItem(*i)->getAlien())	// and it's a live alien...
		{
			_qtys.push_back(0);									// put it in the _qtys<vector> as (int)
			_aliens.push_back(*i);								// put its name in the _aliens<vector> as (string)

			std::wostringstream ss;
			ss << qty;
			_lstAliens->addRow(									// show its name on the list.
							3,
							tr(*i).c_str(),
							ss.str().c_str(),
							L"0");
		}
	}

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&ManageAlienContainmentState::increase);

	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&ManageAlienContainmentState::decrease);
}

/**
 * dTor.
 */
ManageAlienContainmentState::~ManageAlienContainmentState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void ManageAlienContainmentState::think()
{
	State::think();

	_timerInc->think(this, NULL);
	_timerDec->think(this, NULL);
}

/**
 * Deals with the selected aliens.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::btnOkClick(Action*)
{
	for (size_t // iterate as many times as there are aliens queued.
			i = 0;
			i < _qtys.size();
			++i)
	{
		if (_qtys[i] > 0) // if _qtys<vector>(int) at position[i]
		{
			if (_allowHelp) // kL_begin:
			{
				for (int
						j = 0;
						j <	_qtys[i];
						++j)
				{
					_base->researchHelp(_aliens[i]);
				}
			} // kL_end.

			// remove the aliens
			_base->getItems()->removeItem(
										_aliens[i],
										_qtys[i]);

			if (Options::canSellLiveAliens)
			{
				_game->getSavedGame()->setFunds(
											_game->getSavedGame()->getFunds()
											+ _game->getRuleset()->getItem(_aliens[i])->getSellCost()
											* _qtys[i]);
			}
			else // add the corpses
			{
				_base->getItems()->addItem(
										_game->getRuleset()->getArmor(
																_game->getRuleset()->getUnit(_aliens[i])->getArmor()
															)->getCorpseGeoscape(),
										_qtys[i]);
			}
		}
	}

	_game->popState();

	if (Options::storageLimitsEnforced
		&& _base->storesOverfull())
	{
		_game->pushState(new SellState(
									_base,
									_origin));

		if (_origin == OPT_BATTLESCAPE)
			_game->pushState(new ErrorMessageState(
												tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
												_palette,
												Palette::blockOffset(8)+5,
												"BACK01.SCR",
												0));
		else
			_game->pushState(new ErrorMessageState(
												tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												6));
 	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts increasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsRightArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& !_timerInc->isRunning())
	{
		_timerInc->start();
	}
}

/**
 * Stops increasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected alien count;
 * by one on left-click, to max on right-click.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsRightArrowClick(Action* action)
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
 * Starts decreasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsLeftArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& !_timerDec->isRunning())
	{
		_timerDec->start();
	}
}

/**
 * Stops decreasing the alien count.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected alien count;
 * by one on left-click, to 0 on right-click.
 * @param action Pointer to an action.
 */
void ManageAlienContainmentState::lstItemsLeftArrowClick(Action* action)
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
void ManageAlienContainmentState::lstItemsMousePress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (action->getAbsoluteXMouse() >= _lstAliens->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstAliens->getArrowsRightEdge())
		{
			increaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();

		if (action->getAbsoluteXMouse() >= _lstAliens->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstAliens->getArrowsRightEdge())
		{
			decreaseByValue(Options::changeValueByMouseWheel);
		}
	}
}

/**
 * Gets the quantity of the currently selected alien on the base.
 * @return Quantity of selected alien on the base.
 */
int ManageAlienContainmentState::getQuantity()
{
	return _base->getItems()->getItem(_aliens[_sel]);
}

/**
 * Increases the quantity of the selected alien to exterminate by one.
 */
void ManageAlienContainmentState::increase()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	increaseByValue(1);
}

/**
 * Increases the quantity of the selected alien to exterminate by "change".
 * @param change How much we want to add.
 */
void ManageAlienContainmentState::increaseByValue(int change)
{
	int qty = getQuantity() - _qtys[_sel];
	if (change <= 0 || qty <= 0)
		return;

	change = std::min(qty, change);
	_qtys[_sel] += change;
	_aliensSold += change;

	updateStrings();
}

/**
 * Decreases the quantity of the selected alien to exterminate by one.
 */
void ManageAlienContainmentState::decrease()
{
	_timerInc->setInterval(80);
	_timerDec->setInterval(80);

	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected alien to exterminate by "change".
 * @param change How much we want to remove.
 */
void ManageAlienContainmentState::decreaseByValue(int change)
{
	if (change <= 0 || _qtys[_sel] <= 0)
		return;

	change = std::min(_qtys[_sel], change);
	_qtys[_sel] -= change;
	_aliensSold -= change;

	updateStrings();
}

/**
 * Updates the quantity-strings of the selected alien.
 */
void ManageAlienContainmentState::updateStrings()
{
	std::wostringstream
		ss,
		ss2;

	int qty = getQuantity() - _qtys[_sel];
	ss << qty;
	ss2 << _qtys[_sel];

	_lstAliens->setRowColor(_sel, (qty != 0)? _color2: _color);
	_lstAliens->setCellText(_sel, 1, ss.str());		// # in Containment
	_lstAliens->setCellText(_sel, 2, ss2.str());	// # to torture


	int aliens = _base->getUsedContainment() - _aliensSold; //kL - _researchAliens;
	int spaces = _base->getAvailableContainment() - _base->getUsedContainment() + _aliensSold;

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(spaces));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(aliens));


//kL	bool enoughSpaceToExit = Options::storageLimitsEnforced? spaces > -1: true;

//kL	_btnOk->setVisible(enoughSpaceToExit);
	_btnCancel->setVisible(!_overCrowded);
	_btnOk->setVisible(_aliensSold > 0			// kL
							&& spaces > -1);	// kL
}

}
