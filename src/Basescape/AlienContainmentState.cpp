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
		_fishFood(0),
		_totalSpace(base->getAvailableContainment()),
		_usedSpace(base->getUsedContainment())
{
	_window			= new Window(this, 320,200);
	_txtTitle		= new Text(300, 17, 10, 10);
	_txtBaseLabel	= new Text(80, 9, 16, 10);

	_txtSpace		= new Text(144, 9, 16, 30);
	_txtResearch	= new Text(144, 9, 154, 30);

	_txtItem		= new Text(138, 9, 16, 40);
	_txtLiveAliens	= new Text(50, 9, 154, 40);
	_txtDeadAliens	= new Text(50, 9, 204, 40);
	_txtInResearch	= new Text(47, 9, 254, 40);
//	_lstAliens->setColumns(4, 130, 50, 50, 47);

	_lstAliens		= new TextList(285, 121, 16, 50);

	_btnCancel		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("manageContainment");

	add(_window,		"window",	"manageContainment");
	add(_txtTitle,		"text",		"manageContainment");
	add(_txtBaseLabel,	"text",		"manageContainment");
	add(_txtSpace,		"text",		"manageContainment");
	add(_txtResearch,	"text",		"manageContainment");
	add(_txtItem,		"text",		"manageContainment");
	add(_txtLiveAliens,	"text",		"manageContainment");
	add(_txtDeadAliens,	"text",		"manageContainment");
	add(_txtInResearch,	"text",		"manageContainment");
	add(_lstAliens,		"list",		"manageContainment");
	add(_btnCancel,		"button",	"manageContainment");
	add(_btnOk,			"button",	"manageContainment");

	centerAllSurfaces();


	_overCrowded = Options::storageLimitsEnforced
				&& _totalSpace < _usedSpace;

	std::string st;
	if (_origin == OPT_BATTLESCAPE)
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));
		st = "STR_EXECUTE";
	}
	else
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));
		st = "STR_REMOVE_SELECTED";
	}
	_btnOk->setText(tr(st));
	_btnOk->onMouseClick((ActionHandler)& AlienContainmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnOkClick,
					Options::keyOk);
	_btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& AlienContainmentState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AlienContainmentState::btnCancelClick,
					Options::keyCancel);
	if (_overCrowded == true)
		_btnCancel->setVisible(false);

	_txtTitle->setText(tr("STR_MANAGE_CONTAINMENT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_FREE_")
						.arg(_usedSpace)
						.arg(_totalSpace - _usedSpace));

	_txtResearch->setText(tr("STR_INTERROGATION_")
							.arg(_base->getInterrogatedAliens()));

	_txtItem->setText(tr("STR_ALIEN"));
	_txtLiveAliens->setText(tr("STR_LIVE_ALIENS"));
	_txtDeadAliens->setText(tr("STR_DEAD_ALIENS"));
	_txtInResearch->setText(tr("STR_RESEARCH"));

	_lstAliens->setColumns(4, 130, 50, 50, 47);
	_lstAliens->setArrowColumn(158, ARROW_HORIZONTAL);
	_lstAliens->setBackground(_window);
	_lstAliens->setSelectable();
	_lstAliens->setMargin();
//	_lstAliens->setAllowScrollOnArrowButtons(!_allowChangeListValuesByMouseWheel);
//	_lstAliens->onMousePress((ActionHandler)& AlienContainmentState::lstItemsMousePress);
	_lstAliens->onLeftArrowPress((ActionHandler)& AlienContainmentState::lstItemsLeftArrowPress);
	_lstAliens->onLeftArrowRelease((ActionHandler)& AlienContainmentState::lstItemsLeftArrowRelease);
	_lstAliens->onLeftArrowClick((ActionHandler)& AlienContainmentState::lstItemsLeftArrowClick);
	_lstAliens->onRightArrowPress((ActionHandler)& AlienContainmentState::lstItemsRightArrowPress);
	_lstAliens->onRightArrowRelease((ActionHandler)& AlienContainmentState::lstItemsRightArrowRelease);
	_lstAliens->onRightArrowClick((ActionHandler)& AlienContainmentState::lstItemsRightArrowClick);

	const RuleItem* itRule;
	const RuleResearch* rpRule;
	int qtyAliens;
	size_t row = 0;
	Uint8 color;

	std::vector<std::string> baseProjects;
	for (std::vector<ResearchProject*>::const_iterator
			i = _base->getResearch().begin();
			i != _base->getResearch().end();
			++i)
	{
		rpRule = (*i)->getRules();
		itRule = _game->getRuleset()->getItem(rpRule->getName());
		if (itRule != NULL
			&& itRule->isAlien() == true)
		{
			baseProjects.push_back(rpRule->getName());
		}
	}

	if (baseProjects.empty() == false)
		_txtInResearch->setVisible();
	else
		_txtInResearch->setVisible(false);

	const std::vector<std::string>& itemList = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		if (_game->getRuleset()->getItem(*i)->isAlien() == true)	// it's a live alien...
		{
			qtyAliens = _base->getItems()->getItemQty(*i);			// get Qty of each aLien-type at this base
			if (qtyAliens != 0)
			{
				_qtys.push_back(0);		// put it in the _qtys<vector> as (int)
				_aliens.push_back(*i);	// put its name in the _aliens<vector> as (string)

				std::wostringstream woststr;
				woststr << qtyAliens;

				std::wstring rpQty;
				std::vector<std::string>::const_iterator j = std::find(
																	baseProjects.begin(),
																	baseProjects.end(),
																	*i);
				if (j != baseProjects.end())
				{
					rpQty = tr("STR_YES");
					baseProjects.erase(j);
				}

				_lstAliens->addRow( // show its name on the list.
								4,
								tr(*i).c_str(),
								woststr.str().c_str(),
								L"0",
								rpQty.c_str());

				itRule = _game->getRuleset()->getItem(*i);
				if (_game->getSavedGame()->isResearched(itRule->getType()) == false)
					color = Palette::blockOffset(13)+5; // yellow
				else
					color = _lstAliens->getColor();

				_lstAliens->setRowColor(
									row++,
									color);
			}
		}
	}

	for (std::vector<std::string>::const_iterator // add research aLiens that are not in Containment.
			i = baseProjects.begin();
			i != baseProjects.end();
			++i)
	{
		_qtys.push_back(0);
		_aliens.push_back(*i);

		_lstAliens->addRow(
						4,
						tr(*i).c_str(),
						L"0",
						L"0",
						tr("STR_YES").c_str());
		_lstAliens->setRowColor(
							_qtys.size() - 1,
							_lstAliens->getSecondaryColor());
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
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		increaseByValue(std::numeric_limits<int>::max());
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
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);

		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		decreaseByValue(std::numeric_limits<int>::max());
}

/**
 * Gets the quantity of the currently selected alien on the base.
 * @return, quantity of alien
 */
int AlienContainmentState::getQuantity()
{
	return _base->getItems()->getItemQty(_aliens[_sel]);
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
 * Increases the quantity of the selected alien to exterminate.
 * @param change - how much to add
 */
void AlienContainmentState::increaseByValue(int change)
{
	if (change > 0)
	{
		const int qtyType = getQuantity() - _qtys[_sel];
		if (qtyType > 0)
		{
			change = std::min(
							change,
							qtyType);
			_qtys[_sel] += change;
			_fishFood += change;

			update();
		}
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
 * Decreases the quantity of the selected alien to exterminate.
 * @param change - how much to remove
 */
void AlienContainmentState::decreaseByValue(int change)
{
	if (change > 0
		&& _qtys[_sel] > 0)
	{
		change = std::min(
						change,
						_qtys[_sel]);
		_qtys[_sel] -= change;
		_fishFood -= change;

		update();
	}
}

/**
 * Updates the row (quantity & color) of the selected aLien species.
 * Also determines if the OK button should be in/visible.
 */
void AlienContainmentState::update() // private.
{
	std::wostringstream
		woststr1,
		woststr2;

	const int qty = getQuantity() - _qtys[_sel];
	woststr1 << qty;
	woststr2 << _qtys[_sel];

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

	_lstAliens->setCellText(_sel, 1, woststr1.str()); // qty still in Containment
	_lstAliens->setCellText(_sel, 2, woststr2.str()); // qty to torture
	_lstAliens->setRowColor(_sel, color);


	const int freeSpace = _totalSpace - _usedSpace + _fishFood;
	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_FREE_")
						.arg(_usedSpace - _fishFood)
						.arg(freeSpace));

	_btnCancel->setVisible(_overCrowded == false);
	_btnOk->setVisible(_fishFood > 0
					 && freeSpace > -1);
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

}
