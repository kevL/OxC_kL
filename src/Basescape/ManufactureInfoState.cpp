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

#include "ManufactureInfoState.h"

#include <limits>

#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/Base.h"
#include "../Savegame/Production.h"


namespace OpenXcom
{

/**
 * Initializes all elements in the Production settings screen (new Production).
 * @param base Pointer to the base to get info from.
 * @param item The RuleManufacture to produce.
 */
ManufactureInfoState::ManufactureInfoState(
		Base* base,
		RuleManufacture* item)
	:
		_base(base),
		_item(item),
		_production(NULL)
{
	buildUi();
}

/**
 * Initializes all elements in the Production settings screen (modifying Production).
 * @param base Pointer to the base to get info from.
 * @param production The Production to modify.
 */
ManufactureInfoState::ManufactureInfoState(
		Base* base,
		Production* production)
	:
		_base(base),
		_item(NULL),
		_production(production)
{
	buildUi();
}

/**
 * kL. Cleans up the ManufactureInfo state.
 */
ManufactureInfoState::~ManufactureInfoState() // not implemented yet.
{
	delete _timerMoreEngineer;
	delete _timerLessEngineer;
	delete _timerMoreUnit;
	delete _timerLessUnit;
} // kL_end.

/**
 * Builds screen User Interface.
 */
void ManufactureInfoState::buildUi()
{
	_screen = false;

	_window					= new Window(this, 320, 170, 0, 15, POPUP_BOTH);

	_txtTitle				= new Text(280, 17, 20, 25);

	_txtTimeDescr			= new Text(30, 17, 244, 36);
	_txtTimeTotal			= new Text(30, 17, 274, 36);
	_btnSell				= new ToggleTextButton(60, 16, 244, 56);

	_txtAvailableEngineer	= new Text(100, 9, 16, 47);
	_txtAvailableSpace		= new Text(100, 9, 16, 57);

	_txtAllocatedEngineer	= new Text(84, 17, 16, 80);
	_txtAllocated			= new Text(50, 17, 100, 80);

	_txtUnitToProduce		= new Text(84, 17, 176, 80);
	_txtTodo				= new Text(50, 17, 260, 80);

	_txtEngineerUp			= new Text(100, 17, 32, 111);
	_btnEngineerUp			= new ArrowButton(ARROW_BIG_UP, 14, 14, 145, 111);
	_txtEngineerDown		= new Text(100, 17, 32, 135);
	_btnEngineerDown		= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 145, 135);

	_txtUnitUp				= new Text(100, 17, 205, 111);
	_btnUnitUp				= new ArrowButton(ARROW_BIG_UP, 14, 14, 280, 111);
	_txtUnitDown			= new Text(100, 17, 205, 135);
	_btnUnitDown			= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 280, 135);

	_btnStop				= new TextButton(135, 16, 10, 159);
	_btnOk					= new TextButton(135, 16, 175, 159);

//	_surfaceEngineers = new InteractiveSurface(160, 150, 0, 25);
//	_surfaceEngineers->onMouseClick((ActionHandler)& ManufactureInfoState::handleWheelEngineer, 0);

//	_surfaceUnits = new InteractiveSurface(160, 150, 160, 25);
//	_surfaceUnits->onMouseClick((ActionHandler)& ManufactureInfoState::handleWheelUnit, 0);

	setPalette("PAL_BASESCAPE", 6);

	add(_window);
	add(_txtTitle);
	add(_txtTimeDescr);
	add(_txtTimeTotal);
	add(_btnSell);
	add(_txtAvailableEngineer);
	add(_txtAvailableSpace);
	add(_txtAllocatedEngineer);
	add(_txtAllocated);
	add(_txtUnitToProduce);
	add(_txtTodo);
	add(_txtEngineerUp);
	add(_btnEngineerUp);
	add(_txtEngineerDown);
	add(_btnEngineerDown);
	add(_txtUnitUp);
	add(_btnUnitUp);
	add(_txtUnitDown);
	add(_btnUnitDown);
	add(_btnStop);
	add(_btnOk);

//	add(_surfaceEngineers);
//	add(_surfaceUnits);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setColor(Palette::blockOffset(15)+1);
	_txtTitle->setText(tr(_item? _item->getName(): _production->getRules()->getName()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtTimeDescr->setColor(Palette::blockOffset(15)+1);
	_txtTimeDescr->setText(tr("STR_DAYS_HOURS_LEFT"));

	_txtTimeTotal->setColor(Palette::blockOffset(13));

	_btnSell->setColor(Palette::blockOffset(15)+1);
	_btnSell->setText(tr("STR_SELL_PRODUCTION"));
	_btnSell->onMouseRelease((ActionHandler)& ManufactureInfoState::btnSellRelease);

	_txtAvailableEngineer->setColor(Palette::blockOffset(15)+1);
	_txtAvailableEngineer->setSecondaryColor(Palette::blockOffset(13));

	_txtAvailableSpace->setColor(Palette::blockOffset(15)+1);
	_txtAvailableSpace->setSecondaryColor(Palette::blockOffset(13));

	_txtAllocatedEngineer->setColor(Palette::blockOffset(15)+1);
	_txtAllocatedEngineer->setText(tr("STR_ENGINEERS__ALLOCATED"));
	_txtAllocatedEngineer->setBig();
//	_txtAllocatedEngineer->setWordWrap();
//	_txtAllocatedEngineer->setVerticalAlign(ALIGN_MIDDLE);

	_txtAllocated->setColor(Palette::blockOffset(15)+1);
	_txtAllocated->setSecondaryColor(Palette::blockOffset(13));
	_txtAllocated->setBig();

	_txtTodo->setColor(Palette::blockOffset(15)+1);
	_txtTodo->setSecondaryColor(Palette::blockOffset(13));
	_txtTodo->setBig();

	_txtUnitToProduce->setColor(Palette::blockOffset(15)+1);
	_txtUnitToProduce->setText(tr("STR_UNITS_TO_PRODUCE"));
	_txtUnitToProduce->setBig();
//	_txtUnitToProduce->setWordWrap();
//	_txtUnitToProduce->setVerticalAlign(ALIGN_MIDDLE);

	_txtEngineerUp->setColor(Palette::blockOffset(15)+1);
	_txtEngineerUp->setText(tr("STR_INCREASE_UC"));

	_txtEngineerDown->setColor(Palette::blockOffset(15)+1);
	_txtEngineerDown->setText(tr("STR_DECREASE_UC"));

	_btnEngineerUp->setColor(Palette::blockOffset(15)+1);
	_btnEngineerUp->onMousePress((ActionHandler)& ManufactureInfoState::moreEngineerPress);
	_btnEngineerUp->onMouseRelease((ActionHandler)& ManufactureInfoState::moreEngineerRelease);
	_btnEngineerUp->onMouseClick((ActionHandler)& ManufactureInfoState::moreEngineerClick, 0);

	_btnEngineerDown->setColor(Palette::blockOffset(15)+1);
	_btnEngineerDown->onMousePress((ActionHandler)& ManufactureInfoState::lessEngineerPress);
	_btnEngineerDown->onMouseRelease((ActionHandler)& ManufactureInfoState::lessEngineerRelease);
	_btnEngineerDown->onMouseClick((ActionHandler)& ManufactureInfoState::lessEngineerClick, 0);

	_txtUnitUp->setColor(Palette::blockOffset(15)+1);
	_txtUnitUp->setText(tr("STR_INCREASE_UC"));

	_txtUnitDown->setColor(Palette::blockOffset(15)+1);
	_txtUnitDown->setText(tr("STR_DECREASE_UC"));

	_btnUnitUp->setColor(Palette::blockOffset(15)+1);
	_btnUnitUp->onMousePress((ActionHandler)& ManufactureInfoState::moreUnitPress);
	_btnUnitUp->onMouseRelease((ActionHandler)& ManufactureInfoState::moreUnitRelease);
	_btnUnitUp->onMouseClick((ActionHandler)& ManufactureInfoState::moreUnitClick, 0);

	_btnUnitDown->setColor(Palette::blockOffset(15)+1);
	_btnUnitDown->onMousePress((ActionHandler)& ManufactureInfoState::lessUnitPress);
	_btnUnitDown->onMouseRelease((ActionHandler)& ManufactureInfoState::lessUnitRelease);
	_btnUnitDown->onMouseClick((ActionHandler)& ManufactureInfoState::lessUnitClick, 0);

	_btnStop->setColor(Palette::blockOffset(15)+6);
	_btnStop->setText(tr("STR_STOP_PRODUCTION"));
	_btnStop->onMouseClick((ActionHandler)& ManufactureInfoState::btnStopClick);

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ManufactureInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ManufactureInfoState::btnOkClick,
					Options::keyCancel);

	if (_production == NULL)
	{
		_btnOk->setVisible(false);

		_production = new Production(_item, 0);
		_base->addProduction(_production);
	}

	setAssignedEngineer();

	_btnSell->setPressed(_production->getSellItems());

	_timerMoreEngineer	= new Timer(250);
	_timerLessEngineer	= new Timer(250);
	_timerMoreUnit		= new Timer(250);
	_timerLessUnit		= new Timer(250);

	_timerMoreEngineer->onTimer((StateHandler)& ManufactureInfoState::onMoreEngineer);
	_timerLessEngineer->onTimer((StateHandler)& ManufactureInfoState::onLessEngineer);
	_timerMoreUnit->onTimer((StateHandler)& ManufactureInfoState::onMoreUnit);
	_timerLessUnit->onTimer((StateHandler)& ManufactureInfoState::onLessUnit);
}

/**
 * Stops this Production. Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::btnStopClick(Action*)
{
	_base->removeProduction(_production);

	exitState();
}

/**
 * Starts this Production (if new). Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::btnOkClick(Action*)
{
	if (_item)
		_production->startItem(
							_base,
							_game->getSavedGame());

//	_production->setSellItems(_btnSell->getPressed());

	exitState();
}

/**
 * Returns to the previous screen.
 */
void ManufactureInfoState::exitState()
{
	_game->popState();

	if (_item)
		_game->popState();
}

/**
 * Updates display of assigned/available engineer/workshop space.
 */
void ManufactureInfoState::setAssignedEngineer()
{
//kL	_txtAvailableEngineer->setText(tr("STR_ENGINEERS_AVAILABLE_UC").arg(_base->getAvailableEngineers()));
	_txtAvailableEngineer->setText(tr("STR_ENGINEERS_AVAILABLE_UC").arg(_base->getEngineers())); // kL
	_txtAvailableSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE_UC").arg(_base->getFreeWorkshops()));

	std::wostringstream s1;
	s1 << L"> \x01" << _production->getAssignedEngineers();
	_txtAllocated->setText(s1.str());

	std::wostringstream s2;
	s2 << L"> \x01";
	if (_production->getInfiniteAmount())
//kL	s2 << Language::utf8ToWstr("∞");
		s2 << "oo"; // kL
	else
		s2 << _production->getAmountTotal();

	_txtTodo->setText(s2.str());

	_btnOk->setVisible(_production->getAmountTotal() > 0);

	updateTimeTotal(); // kL
}

/**
 * kL. Updates the total time to complete the project.
 */
void ManufactureInfoState::updateTimeTotal() // kL
{
	std::wostringstream woStr;

	if (_production->getAssignedEngineers() > 0)
	{
		int hoursLeft;

		if (_production->getSellItems()
			|| _production->getInfiniteAmount())
		{
			hoursLeft = (_production->getAmountProduced() + 1) * _production->getRules()->getManufactureTime()
						- _production->getTimeSpent();
		}
		else
		{
			hoursLeft = _production->getAmountTotal() * _production->getRules()->getManufactureTime()
						- _production->getTimeSpent();
		}


		int engs = _production->getAssignedEngineers();
		if (!Options::canManufactureMoreItemsPerHour)
		{
			engs = std::min(
							engs,
							_production->getRules()->getManufactureTime());
		}

//		hoursLeft = static_cast<int>(
//						ceil(static_cast<double>(hoursLeft) / static_cast<double>(_production->getAssignedEngineers())));
		// ensure we round up since it takes an entire hour to manufacture any part of that hour's capacity
		hoursLeft = (hoursLeft + engs - 1) / engs;

		int daysLeft = hoursLeft / 24;
		hoursLeft %= 24;
		woStr << daysLeft << "\n" << hoursLeft;
	}
	else
		woStr << L"oo";

	_txtTimeTotal->setText(woStr.str());
}

/**
 * kL. Handler for releasing the Sell button.
 */
void ManufactureInfoState::btnSellRelease(Action* action) // kL
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_production->setSellItems(_btnSell->getPressed());

		updateTimeTotal();
	}
}

/**
 * Adds given number of engineers to the project if possible.
 * @param change How much we want to add.
 */
void ManufactureInfoState::moreEngineer(int change)
{
	if (change < 1)
		return;

//kL	int availableEngineer = _base->getAvailableEngineers();
	int availableEngineer = _base->getEngineers(); // kL
	int availableWorkSpace = _base->getFreeWorkshops();

	if (availableEngineer > 0
		&& availableWorkSpace > 0)
	{
		change = std::min(
						std::min(
								availableEngineer,
								availableWorkSpace),
						change);
		_production->setAssignedEngineers(_production->getAssignedEngineers() + change);
		_base->setEngineers(_base->getEngineers() - change);

		setAssignedEngineer();
	}
}

/**
 * Starts the timerMoreEngineer.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::moreEngineerPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerMoreEngineer->start();
}

/**
 * Stops the timerMoreEngineer.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::moreEngineerRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerMoreEngineer->setInterval(250);
		_timerMoreEngineer->stop();
	}
}

/**
 * Allocates all engineers.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::moreEngineerClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moreEngineer(std::numeric_limits<int>::max());
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		moreEngineer(1);
}

/**
 * Removes the given number of engineers from the project if possible.
 * @param change How much we want to subtract.
 */
void ManufactureInfoState::lessEngineer(int change)
{
	if (change < 1)
		return;

	int assigned = _production->getAssignedEngineers();
	if (assigned > 0)
	{
		change = std::min(assigned, change);
		_production->setAssignedEngineers(assigned-change);
		_base->setEngineers(_base->getEngineers()+change);

		setAssignedEngineer();
	}
}

/**
 * Starts the timerLessEngineer.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::lessEngineerPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLessEngineer->start();
}

/**
 * Stops the timerLessEngineer.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::lessEngineerRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLessEngineer->setInterval(250);
		_timerLessEngineer->stop();
	}
}

/**
 * Removes engineers from the production.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::lessEngineerClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		lessEngineer(std::numeric_limits<int>::max());
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		lessEngineer(1);
}

/**
 * Adds given number of units to produce to the project if possible.
 * @param change How much we want to add.
 */
void ManufactureInfoState::moreUnit(int change)
{
	if (change < 1)
		return;

	if (_production->getRules()->getCategory() == "STR_CRAFT"
		&& _base->getAvailableHangars() - _base->getUsedHangars() == 0)
	{
		_timerMoreUnit->stop();
		_game->pushState(new ErrorMessageState(
											tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"),
											_palette,
											Palette::blockOffset(15)+1,
											"BACK17.SCR",
											6));
	}
	else
	{
		int units = _production->getAmountTotal();

		change = std::min(
						std::numeric_limits<int>::max() - units,
						change);

		if (_production->getRules()->getCategory() == "STR_CRAFT")
			change = std::min(
							_base->getAvailableHangars() - _base->getUsedHangars(),
							change);

		_production->setAmountTotal(units + change);

		setAssignedEngineer();
	}
}

/**
 * Starts the timerMoreUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::moreUnitPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _production->getAmountTotal() < std::numeric_limits<int>::max())
	{
		_timerMoreUnit->start();
	}
}

/**
 * Stops the timerMoreUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::moreUnitRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerMoreUnit->setInterval(250);
		_timerMoreUnit->stop();
	}
}

/**
 * Increases the units to produce, in the case of a right-click, to infinite, and 1 on left-click.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::moreUnitClick(Action* action)
{
	if (_production->getInfiniteAmount())
		return; // We can't increase over infinite :) [cf. Cantor]

	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_production->setInfiniteAmount(true);

		setAssignedEngineer();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		moreUnit(1);
}

/**
 * Removes the given number of units to produce from the total if possible.
 * @param change - how many to subtract
 */
void ManufactureInfoState::lessUnit(int change)
{
	if (change < 1)
		return;

	int units = _production->getAmountTotal();
	change = std::min(
					units - (_production->getAmountProduced() + 1),
					change);
	_production->setAmountTotal(units - change);

	setAssignedEngineer();
}

/**
 * Starts the timerLessUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::lessUnitPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLessUnit->start();
}

/**
 * Stops the timerLessUnit.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::lessUnitRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLessUnit->setInterval(250);
		_timerLessUnit->stop();
	}
}

/**
 * Decreases the units to produce.
 * @param action A pointer to an Action.
 */
void ManufactureInfoState::lessUnitClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		|| action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_production->setInfiniteAmount(false);

		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
			|| _production->getAmountTotal() <= _production->getAmountProduced())
		{
			// so the number-to-produce is now below the produced items OR it was a right-click
			_production->setAmountTotal(_production->getAmountProduced() + 1);

			setAssignedEngineer();
		}

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			lessUnit(1);
	}
}

/**
 * Assigns one more engineer (if possible).
 */
void ManufactureInfoState::onMoreEngineer()
{
	_timerMoreEngineer->setInterval(80);
	moreEngineer(1);
}

/**
 * Removes one engineer (if possible).
 */
void ManufactureInfoState::onLessEngineer()
{
	_timerLessEngineer->setInterval(80);
	lessEngineer(1);
}

/**
 * Increases or decreases the Engineers according the mouse-wheel used.
 * @param action A pointer to an Action.
 */
/* void ManufactureInfoState::handleWheelEngineer(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		moreEngineer(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		lessEngineer(Options::changeValueByMouseWheel);
} */

/**
 * Builds one more unit.
 */
void ManufactureInfoState::onMoreUnit()
{
	_timerMoreUnit->setInterval(80);
	moreUnit(1);
}

/**
 * Builds one less unit (if possible).
 */
void ManufactureInfoState::onLessUnit()
{
	_timerLessUnit->setInterval(80);
	lessUnit(1);
}

/**
 * Increases or decreases the Units to produce according the mouse-wheel used.
 * @param action A pointer to an Action.
 */
/* void ManufactureInfoState::handleWheelUnit(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		moreUnit(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		lessUnit(Options::changeValueByMouseWheel);
} */

/**
 * Runs state functionality every cycle (used to update the timer).
 */
void ManufactureInfoState::think()
{
	State::think();

	_timerMoreEngineer->think(this, NULL);
	_timerLessEngineer->think(this, NULL);
	_timerMoreUnit->think(this, NULL);
	_timerLessUnit->think(this, NULL);
}

}
