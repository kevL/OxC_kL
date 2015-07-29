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

#include "ResearchInfoState.h"

//#include <limits>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ResearchProject screen.
 * @param base - pointer to the Base to get info from
 * @param rule - pointer to a RuleResearch which will be used to create a new ResearchProject
 */
ResearchInfoState::ResearchInfoState(
		Base* base,
		RuleResearch* rule)
	:
		_base(base),
		_rule(rule),
		_project(new ResearchProject( // time = 65 to 130%
								rule,
								(rule->getCost() * RNG::generate(65,130)) / 100))
{
	buildUi();
}

/**
 * Initializes all the elements in the ResearchProject screen.
 * @param base		- pointer to the Base to get info from
 * @param project	- pointer to a ResearchProject to modify
 */
ResearchInfoState::ResearchInfoState(
		Base* base,
		ResearchProject* project)
	:
		_base(base),
		_project(project),
		_rule(NULL)
{
	buildUi();
}

/**
 * Cleans up the ResearchInfo state.
 */
ResearchInfoState::~ResearchInfoState()
{
	delete _timerMore;
	delete _timerLess;
}

/**
 * Builds dialog.
 */
void ResearchInfoState::buildUi()
{
	_screen = false;

	_window					= new Window(this, 230, 140, 45, 30);
	_txtTitle				= new Text(198, 16, 61, 40);

	_txtAvailableScientist	= new Text(198, 9, 61, 60);
	_txtAvailableSpace		= new Text(198, 9, 61, 70);
	_txtAllocatedScientist	= new Text(198, 16, 61, 80);

	_txtMore				= new Text(134, 16, 93, 100);
	_txtLess				= new Text(134, 16, 93, 120);
	_btnMore				= new ArrowButton(ARROW_BIG_UP, 13, 14, 205, 100);
	_btnLess				= new ArrowButton(ARROW_BIG_DOWN, 13, 14, 205, 120);

	_btnCancel				= new TextButton(95, 16, 61, 144);
	_btnOk					= new TextButton(95, 16, 164, 144);

//	_srfScientists			= new InteractiveSurface(230, 140, 45, 30);

	setInterface("allocateResearch");

	add(_window,				"window",	"allocateResearch");
	add(_txtTitle,				"text",		"allocateResearch");
	add(_txtAvailableScientist,	"text",		"allocateResearch");
	add(_txtAvailableSpace,		"text",		"allocateResearch");
	add(_txtAllocatedScientist,	"text",		"allocateResearch");
	add(_txtMore,				"text",		"allocateResearch");
	add(_txtLess,				"text",		"allocateResearch");
	add(_btnMore,				"button1",	"allocateResearch");
	add(_btnLess,				"button1",	"allocateResearch");
	add(_btnCancel,				"button2",	"allocateResearch");
	add(_btnOk,					"button2",	"allocateResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setBig();
	if (_rule != NULL)
		_txtTitle->setText(tr(_rule->getName()));
	else
		_txtTitle->setText(tr(_project->getRules()->getName()));

	_txtAllocatedScientist->setBig();

	_txtMore->setText(tr("STR_INCREASE"));
	_txtMore->setBig();

	_txtLess->setText(tr("STR_DECREASE"));
	_txtLess->setBig();

	if (_rule != NULL)
	{
		_base->addResearch(_project);

		if (_rule->needItem() == true
			&& (Options::spendResearchedItems == true
				|| _game->getRuleset()->getUnit(_rule->getName()) != NULL))
		{
			_base->getItems()->removeItem(_rule->getName());
		}
	}

	setAssignedScientist();

	_btnMore->onMousePress((ActionHandler)& ResearchInfoState::morePress);
	_btnMore->onMouseRelease((ActionHandler)& ResearchInfoState::moreRelease);
	_btnMore->onMouseClick((ActionHandler)& ResearchInfoState::moreClick, 0);

	_btnLess->onMousePress((ActionHandler)& ResearchInfoState::lessPress);
	_btnLess->onMouseRelease((ActionHandler)& ResearchInfoState::lessRelease);
	_btnLess->onMouseClick((ActionHandler)& ResearchInfoState::lessClick, 0);

	_timerMore = new Timer(250);
	_timerMore->onTimer((StateHandler)& ResearchInfoState::moreSci);

	_timerLess = new Timer(250);
	_timerLess->onTimer((StateHandler)& ResearchInfoState::lessSci);

	if (_rule != NULL)
	{
		_btnOk->setText(tr("STR_START_PROJECT"));

		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onKeyboardPress(
						(ActionHandler)& ResearchInfoState::btnCancelClick,
						Options::keyCancel);
	}
	else
	{
		_btnCancel->setText(tr("STR_CANCEL_PROJECT"));

		_btnOk->setText(tr("STR_OK"));
		_btnOk->onKeyboardPress(
						(ActionHandler)& ResearchInfoState::btnOkClick,
						Options::keyCancel);
	}
	_btnCancel->onMouseClick((ActionHandler)& ResearchInfoState::btnCancelClick);

	_btnOk->onMouseClick((ActionHandler)& ResearchInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchInfoState::btnOkClick,
					Options::keyOk);
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchInfoState::btnOkClick(Action*)
{
	_project->setOffline(false);
	_game->popState();
}

/**
 * Returns to the previous screen and removes the current project from the active
 * research list.
 * @param action - pointer to an Action
 */
void ResearchInfoState::btnCancelClick(Action*)
{
	const RuleResearch* resRule;
	if (_rule != NULL)
		resRule = _rule;
	else
		resRule = _project->getRules();

	if (resRule->needItem() == true
		&& (Options::spendResearchedItems == true
			|| _game->getRuleset()->getUnit(resRule->getName()) != NULL))
	{
		_base->getItems()->addItem(resRule->getName());
	}

	_base->removeResearch(
						_project,
						false,
						_rule == NULL);

	_game->popState();
}

/**
 * Updates count of assigned/free scientists and available lab space.
 */
void ResearchInfoState::setAssignedScientist()
{
	_txtAvailableScientist->setText(tr("STR_SCIENTISTS_AVAILABLE_UC_")
									.arg(_base->getScientists()));
	_txtAvailableSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE_UC_")
									.arg(_base->getFreeLaboratories()));
	_txtAllocatedScientist->setText(tr("STR_SCIENTISTS_ALLOCATED_")
									.arg(_project->getAssigned()));
}

/**
 * Increases or decreases the scientists according the mouse-wheel used.
 * @param action - pointer to an Action
 */
/* void ResearchInfoState::handleWheel(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		moreByValue(Options::changeValueByMouseWheel);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		lessByValue(Options::changeValueByMouseWheel);
} */

/**
 * Starts the timeMore timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::morePress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerMore->start();
}

/**
 * Stops the timeMore timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::moreRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerMore->setInterval(250);
		_timerMore->stop();
	}
}

/**
 * Allocates scientists to the current project;
 * one scientist on left-click, all scientists on right-click.
 * @param action - pointer to an Action
 */
void ResearchInfoState::moreClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		moreByValue(std::numeric_limits<int>::max());
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		moreByValue(getQty());
}

/**
 * Starts the timeLess timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::lessPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLess->start();
}

/**
 * Stops the timeLess timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::lessRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLess->setInterval(250);
		_timerLess->stop();
	}
}

/**
 * Removes scientists from the current project;
 * one scientist on left-click, all scientists on right-click.
 * @param action - pointer to an Action
 */
void ResearchInfoState::lessClick(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		lessByValue(std::numeric_limits<int>::max());
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		lessByValue(getQty());
}

/**
 * Adds one scientist to the project if possible.
 */
void ResearchInfoState::moreSci()
{
	_timerMore->setInterval(80);
	moreByValue(getQty());
}

/**
 * Adds the given number of scientists to the project if possible.
 * @param change Number of scientists to add.
 */
void ResearchInfoState::moreByValue(int change)
{
	if (change < 1)
		return;

	const int
		freeScientists = _base->getScientists(),
		freeSpaceLab = _base->getFreeLaboratories();

	if (freeScientists > 0
		&& freeSpaceLab > 0)
	{
		change = std::min(
						change,
						std::min(
								freeScientists,
								freeSpaceLab));
		_project->setAssigned(_project->getAssigned() + change);
		_base->setScientists(_base->getScientists() - change);

		setAssignedScientist();
	}
}

/**
 * Removes one scientist from the project if possible.
 */
void ResearchInfoState::lessSci()
{
	_timerLess->setInterval(80);
	lessByValue(getQty());
}

/**
 * Removes the given number of scientists from the project if possible.
 * @param change Number of scientists to subtract.
 */
void ResearchInfoState::lessByValue(int change)
{
	if (change < 1)
		return;

	const int assigned = _project->getAssigned();
	if (assigned > 0)
	{
		change = std::min(
						change,
						assigned);
		_project->setAssigned(assigned - change);
		_base->setScientists(_base->getScientists() + change);

		setAssignedScientist();
	}
}

/**
 * Gets quantity to change by.
 * @note what were these guys smokin'
 * @return, 10 if CTRL is pressed else 1
 */
int ResearchInfoState::getQty() const
{
	if ((SDL_GetModState() & KMOD_CTRL) == 0)
		return 1;

	return 10;
}

/**
 * Runs state functionality every cycle (used to update the timer).
 */
void ResearchInfoState::think()
{
	State::think();

	_timerLess->think(this, NULL);
	_timerMore->think(this, NULL);
}

}
