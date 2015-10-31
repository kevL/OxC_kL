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
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ResearchProject screen.
 * @param base		- pointer to the Base to get info from
 * @param resRule	- pointer to a RuleResearch which will be used to create a new ResearchProject
 */
ResearchInfoState::ResearchInfoState(
		Base* const base,
		const RuleResearch* const resRule)
	:
		_base(base),
		_resRule(resRule),
		_project(new ResearchProject( // time = 65 to 130%
								resRule,
								(resRule->getCost() * RNG::generate(65,130)) / 100))
{
	//Log(LOG_INFO) << "ResearchInfoState cTor w/ resRule " << resRule->getType();
	buildUi();
}

/**
 * Initializes all the elements in the ResearchProject screen.
 * @param base		- pointer to the Base to get info from
 * @param project	- pointer to a ResearchProject to modify
 */
ResearchInfoState::ResearchInfoState(
		Base* const base,
		ResearchProject* const project)
	:
		_base(base),
		_project(project),
		_resRule(NULL)
{
	//Log(LOG_INFO) << "ResearchInfoState cTor w/ project " << project->getRules()->getType();
	buildUi();
}

/**
 * Frees up memory that's not automatically cleaned on exit.
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

	_window					= new Window(this, 240, 140, 40, 30);

	_txtTitle				= new Text(198, 16, 61, 40);

	_txtAvailableScientist	= new Text(198,  9, 61, 60);
	_txtAvailableSpace		= new Text(198,  9, 61, 70);
	_txtAllocatedScientist	= new Text(198, 16, 61, 80);

//	_txtMore				= new Text(134, 16, 93, 100);
//	_txtLess				= new Text(134, 16, 93, 120);
//	_btnMore				= new ArrowButton(ARROW_BIG_UP, 13, 14, 205, 100);
//	_btnLess				= new ArrowButton(ARROW_BIG_DOWN, 13, 14, 205, 120);
	_btnMore				= new ArrowButton(ARROW_BIG_UP,   120, 16, 100, 100);
	_btnLess				= new ArrowButton(ARROW_BIG_DOWN, 120, 16, 100, 120);

	_btnCancel				= new TextButton(95, 16,  61, 144);
	_btnOk					= new TextButton(95, 16, 164, 144);

//	_srfScientists			= new InteractiveSurface(230, 140, 45, 30);

	setInterface("allocateResearch");

	add(_window,				"window",	"allocateResearch");
	add(_txtTitle,				"text",		"allocateResearch");
	add(_txtAvailableScientist,	"text",		"allocateResearch");
	add(_txtAvailableSpace,		"text",		"allocateResearch");
	add(_txtAllocatedScientist,	"text",		"allocateResearch");
//	add(_txtMore,				"text",		"allocateResearch");
//	add(_txtLess,				"text",		"allocateResearch");
	add(_btnMore,				"button1",	"allocateResearch");
	add(_btnLess,				"button1",	"allocateResearch");
	add(_btnCancel,				"button2",	"allocateResearch");
	add(_btnOk,					"button2",	"allocateResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setBig();
	if (_resRule != NULL)
		_txtTitle->setText(tr(_resRule->getType()));
	else
		_txtTitle->setText(tr(_project->getRules()->getType()));

	_txtAllocatedScientist->setBig();

//	_txtMore->setText(tr("STR_INCREASE"));
//	_txtMore->setBig();

//	_txtLess->setText(tr("STR_DECREASE"));
//	_txtLess->setBig();

	if (_resRule != NULL)
	{
		_base->addResearch(_project);

		if (_resRule->needsItem() == true
			&& (Options::spendResearchedItems == true
				|| _game->getRuleset()->getUnit(_resRule->getType()) != NULL))
		{
			_base->getStorageItems()->removeItem(_resRule->getType());
		}
	}

	updateInfo();

	_btnMore->onMousePress((ActionHandler)& ResearchInfoState::morePress);
	_btnMore->onMouseRelease((ActionHandler)& ResearchInfoState::moreRelease);
	_btnMore->onMouseClick((ActionHandler)& ResearchInfoState::moreClick, 0);

	_btnLess->onMousePress((ActionHandler)& ResearchInfoState::lessPress);
	_btnLess->onMouseRelease((ActionHandler)& ResearchInfoState::lessRelease);
	_btnLess->onMouseClick((ActionHandler)& ResearchInfoState::lessClick, 0);

	_timerMore = new Timer(Timer::SCROLL_SLOW);
	_timerMore->onTimer((StateHandler)& ResearchInfoState::moreSci);

	_timerLess = new Timer(Timer::SCROLL_SLOW);
	_timerLess->onTimer((StateHandler)& ResearchInfoState::lessSci);

	if (_resRule != NULL)
	{
		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onKeyboardPress(
						(ActionHandler)& ResearchInfoState::btnCancelClick,
						Options::keyCancel);

		_btnOk->setText(tr("STR_START_PROJECT"));
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
	_btnOk->onMouseClick((ActionHandler)& ResearchInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchInfoState::btnOkClick,
					Options::keyOkKeypad);
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
	if (_resRule != NULL)
		resRule = _resRule;
	else
		resRule = _project->getRules();

	const bool liveAlien (_game->getRuleset()->getUnit(resRule->getType()) != NULL);

	if (resRule->needsItem() == true
		&& (Options::spendResearchedItems == true || liveAlien == true))
	{
		_base->getStorageItems()->addItem(resRule->getType());
	}

	_base->removeResearch(
					_project,
					false,
					_resRule == NULL && liveAlien == false);

	_game->popState();
}

/**
 * Updates count of assigned/free scientists and available lab space.
 */
void ResearchInfoState::updateInfo()
{
	_txtAvailableScientist->setText(tr("STR_SCIENTISTS_AVAILABLE_UC_")
									.arg(_base->getScientists()));
	_txtAvailableSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE_UC_")
									.arg(_base->getFreeLaboratories()));
	_txtAllocatedScientist->setText(tr("STR_SCIENTISTS_ALLOCATED_")
									.arg(_project->getAssignedScientists()));
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
		_timerMore->setInterval(Timer::SCROLL_SLOW);
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
		_timerLess->setInterval(Timer::SCROLL_SLOW);
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
	_timerMore->setInterval(Timer::SCROLL_FAST);
	moreByValue(getQty());
}

/**
 * Adds the given number of scientists to the project if possible.
 * @param change Number of scientists to add.
 */
void ResearchInfoState::moreByValue(int change)
{
	if (change > 0)
	{
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
			_project->setAssignedScientists(_project->getAssignedScientists() + change);
			_base->setScientists(_base->getScientists() - change);

			updateInfo();
		}
	}
}

/**
 * Removes one scientist from the project if possible.
 */
void ResearchInfoState::lessSci()
{
	_timerLess->setInterval(Timer::SCROLL_FAST);
	lessByValue(getQty());
}

/**
 * Removes the given number of scientists from the project if possible.
 * @param change Number of scientists to subtract.
 */
void ResearchInfoState::lessByValue(int change)
{
	if (change > 0)
	{
		const int assigned = _project->getAssignedScientists();
		if (assigned > 0)
		{
			change = std::min(
							change,
							assigned);
			_project->setAssignedScientists(assigned - change);
			_base->setScientists(_base->getScientists() + change);

			updateInfo();
		}
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
