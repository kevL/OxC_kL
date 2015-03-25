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

#include "AlienContainmentState.h"

//#include <sstream>
//#include <climits>
//#include <cmath>

#include "SellState.h"

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

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleResearch.h"

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
 */
AlienContainmentState::AlienContainmentState(
		Base* base,
		OptionsOrigin origin)
	:
		_base(base),
		_origin(origin),
		_sel(0),
		_fishFood(0)
{
	_overCrowded = Options::storageLimitsEnforced
				&& _base->getAvailableContainment() < _base->getUsedContainment();

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

	setPalette(
			"PAL_BASESCAPE",
			_game->getRuleset()->getInterface("manageContainment")->getElement("palette")->color);

	add(_window,		"window",	"manageContainment");
	add(_txtTitle,		"text",		"manageContainment");
	add(_txtBaseLabel,	"text",		"manageContainment");
	add(_txtAvailable,	"text",		"manageContainment");
	add(_txtUsed,		"text",		"manageContainment");
	add(_txtItem,		"text",		"manageContainment");
	add(_txtLiveAliens,	"text",		"manageContainment");
	add(_txtDeadAliens,	"text",		"manageContainment");
	add(_lstAliens,		"list",		"manageContainment");
	add(_btnCancel,		"button",	"manageContainment");
	add(_btnOk,			"button",	"manageContainment");

	centerAllSurfaces();


	std::string st;
	if (origin == OPT_BATTLESCAPE)
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));
		st = "STR_EXECUTE";
	}
	else
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));
		st = "STR_REMOVE_SELECTED";
	}

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& AlienContainmentState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnCancelClick,
					Options::keyCancel);
	if (_overCrowded == true)
		_btnCancel->setVisible(false);

	_btnOk->setText(tr(st));
	_btnOk->onMouseClick((ActionHandler)& AlienContainmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnOkClick,
					Options::keyOk);
	_btnOk->setVisible(false);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MANAGE_CONTAINMENT"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setText(tr("STR_ALIEN"));

	_txtLiveAliens->setText(tr("STR_LIVE_ALIENS"));

	_txtDeadAliens->setText(tr("STR_DEAD_ALIENS"));

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE")
							.arg(_base->getAvailableContainment() - _base->getUsedContainment()));

	_txtUsed->setText(tr("STR_SPACE_USED")
						.arg(_base->getUsedContainment()));

	_lstAliens->setArrowColumn(178, ARROW_HORIZONTAL);
	_lstAliens->setColumns(3, 140, 66, 56);
	_lstAliens->setSelectable();
	_lstAliens->setBackground(_window);
	_lstAliens->setMargin();
//	_lstAliens->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstAliens->onMousePress((ActionHandler)& AlienContainmentState::lstItemsMousePress);
	_lstAliens->onLeftArrowPress((ActionHandler)& AlienContainmentState::lstItemsLeftArrowPress);
	_lstAliens->onLeftArrowRelease((ActionHandler)& AlienContainmentState::lstItemsLeftArrowRelease);
	_lstAliens->onLeftArrowClick((ActionHandler)& AlienContainmentState::lstItemsLeftArrowClick);
	_lstAliens->onRightArrowPress((ActionHandler)& AlienContainmentState::lstItemsRightArrowPress);
	_lstAliens->onRightArrowRelease((ActionHandler)& AlienContainmentState::lstItemsRightArrowRelease);
	_lstAliens->onRightArrowClick((ActionHandler)& AlienContainmentState::lstItemsRightArrowClick);


	int qty;
	const RuleItem* itRule;
	Uint8 color;
	size_t row = 0;

	const std::vector<std::string>& itemList = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		qty = _base->getItems()->getItem(*i);						// get Qty of each item at this base
		if (qty > 0													// if item exists at this base
			&& _game->getRuleset()->getItem(*i)->isAlien() == true)	// and it's a live alien...
		{
			_qtys.push_back(0);		// put it in the _qtys<vector> as (int)
			_aliens.push_back(*i);	// put its name in the _aliens<vector> as (string)

			std::wostringstream ss;
			ss << qty;
			_lstAliens->addRow( // show its name on the list.
							3,
							tr(*i).c_str(),
							ss.str().c_str(),
							L"0");

			itRule = _game->getRuleset()->getItem(*i);
			if (_game->getSavedGame()->isResearched(itRule->getType()) == false)
				color = Palette::blockOffset(13)+5; // yellow
			else
				color = _lstAliens->getColor();

			_lstAliens->setRowColor(row, color);
			++row;
		}
	}

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)& AlienContainmentState::increase);

	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)& AlienContainmentState::decrease);
}

/**
 * dTor.
 */
AlienContainmentState::~AlienContainmentState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void AlienContainmentState::think()
{
	State::think();

	_timerInc->think(this, NULL);
	_timerDec->think(this, NULL);
}

/**
 * Deals with the selected aliens.
 * @param action - pointer to an Action
 */
void AlienContainmentState::btnOkClick(Action*)
{
	for (size_t
			i = 0;
			i != _qtys.size();
			++i)
	{
		if (_qtys[i] > 0)
		{
			_base->getItems()->removeItem(
									_aliens[i],
									_qtys[i]);

			_base->getItems()->addItem(
									_game->getRuleset()->getArmor(
															_game->getRuleset()->getUnit(_aliens[i])->getArmor())
														->getCorpseGeoscape(),
									_qtys[i]);
		}
	}

	_game->popState();

	if (Options::storageLimitsEnforced == true
		&& _base->storesOverfull() == true)
	{
		_game->pushState(new SellState(
									_base,
									_origin));

		std::string st;
		if (_origin == OPT_BATTLESCAPE)
			st = "BACK01.SCR";
		else
			st = "BACK13.SCR";

		const int color = _game->getRuleset()->getInterface("manageContainment")->getElement("errorMessage")->color;

		_game->pushState(new ErrorMessageState(
											tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
											_palette,
											color,
											st,
											color));
 	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void AlienContainmentState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Starts increasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsRightArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerInc->isRunning() == false)
	{
		_timerInc->start();
	}
}

/**
 * Stops increasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsRightArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerInc->stop();
}

/**
 * Increases the selected alien count;
 * by one on left-click, to max on right-click.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsRightArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		increaseByValue(std::numeric_limits<int>::max());

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsLeftArrowPress(Action* action)
{
	_sel = _lstAliens->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _timerDec->isRunning() == false)
	{
		_timerDec->start();
	}
}

/**
 * Stops decreasing the alien count.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsLeftArrowRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerDec->stop();
}

/**
 * Decreases the selected alien count;
 * by one on left-click, to 0 on right-click.
 * @param action - pointer to an Action
 */
void AlienContainmentState::lstItemsLeftArrowClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		decreaseByValue(std::numeric_limits<int>::max());

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 */
/*void AlienContainmentState::lstItemsMousePress(Action* action)
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
} */

/**
 * Gets the quantity of the currently selected alien on the base.
 * @return, quantity of selected alien on the base
 */
int AlienContainmentState::getQuantity()
{
	return _base->getItems()->getItem(_aliens[_sel]);
}

/**
 * Increases the quantity of the selected alien to exterminate by one.
 */
void AlienContainmentState::increase()
{
	_timerDec->setInterval(80);
	_timerInc->setInterval(80);

	increaseByValue(1);
}

/**
 * Increases the quantity of the selected alien to exterminate by "change".
 * @param change - how much to add
 */
void AlienContainmentState::increaseByValue(int change)
{
	if (change < 1)
		return;

	const int qty = getQuantity() - _qtys[_sel];
	if (qty > 0)
	{
		change = std::min(
						change,
						qty);
		_qtys[_sel] += change;
		_fishFood += change;

		updateStrings();
	}
}

/**
 * Decreases the quantity of the selected alien to exterminate by one.
 */
void AlienContainmentState::decrease()
{
	_timerInc->setInterval(80);
	_timerDec->setInterval(80);

	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected alien to exterminate by "change".
 * @param change - how much to remove
 */
void AlienContainmentState::decreaseByValue(int change)
{
	if (change < 1)
		return;

	if (_qtys[_sel] > 0)
	{
		change = std::min(_qtys[_sel], change);
		_qtys[_sel] -= change;
		_fishFood -= change;

		updateStrings();
	}
}

/**
 * Updates the row (quantity & color) of the selected aLien species.
 * Also determines if the OK button should be in/visible.
 */
void AlienContainmentState::updateStrings()
{
	std::wostringstream
		ss,
		ss2;

	const int qty = getQuantity() - _qtys[_sel];
	ss << qty;
	ss2 << _qtys[_sel];

	Uint8 color;
	if (_qtys[_sel] != 0)
		color = _lstAliens->getSecondaryColor();
	else
	{
		const RuleItem* const itRule = _game->getRuleset()->getItem(_aliens[_sel]);
		if (_game->getSavedGame()->isResearched(itRule->getType()) == false)
			color = Palette::blockOffset(13)+5; // yellow
		else
			color = _lstAliens->getColor();
	}

	_lstAliens->setRowColor(_sel, color);
	_lstAliens->setCellText(_sel, 1, ss.str());  // # still in Containment
	_lstAliens->setCellText(_sel, 2, ss2.str()); // # to torture


	const int
		freeSpace = _base->getAvailableContainment() - _base->getUsedContainment() + _fishFood,
		aliens = _base->getUsedContainment() - _fishFood;

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(freeSpace));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(aliens));

	_btnCancel->setVisible(_overCrowded == false);
	_btnOk->setVisible(_fishFood > 0
					   && freeSpace > -1);
}

}
