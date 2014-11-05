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

#include "InterceptState.h"

#include <sstream>

#include "ConfirmDestinationState.h"
#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"

#include "../Basescape/BasescapeState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Intercept window.
 * @param globe		- pointer to the geoscape Globe
 * @param base		- pointer to Base to show contained crafts (default NULL to show all crafts)
 * @param target	- pointer to Target to intercept (default NULL to ask user for target)
 * @param geo		- pointer to GeoscapeState (default NULL)
 */
InterceptState::InterceptState(
		Globe* globe,
		Base* base,
		Target* target,
		GeoscapeState* geo)
	:
		_globe(globe),
		_base(base),
		_target(target),
		_geo(geo)
{
	_screen = false;

	_window		= new Window(
							this,
							320,
							144,
							0,
							30,
							POPUP_HORIZONTAL);
	_txtBase		= new Text(288, 17, 16, 41); // might do getRegion in here also.

	_txtCraft		= new Text(86, 9, 16, 56);
	_txtStatus		= new Text(53, 9, 115, 56);
	_txtWeapons		= new Text(85, 9, 213, 56);

	_lstCrafts		= new TextList(285, 81, 16, 66);

	_btnGotoBase	= new TextButton(142, 16, 16, 151);
	_btnCancel		= new TextButton(142, 16, 162, 151);

	setPalette("PAL_GEOSCAPE", 4);

	add(_window);
	add(_txtBase);
	add(_txtCraft);
	add(_txtStatus);
	add(_txtWeapons);
	add(_lstCrafts);
	add(_btnGotoBase);
	add(_btnCancel);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnGotoBase->setColor(Palette::blockOffset(8)+5);
	_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	_btnGotoBase->onMouseClick((ActionHandler)& InterceptState::btnGotoBaseClick);
	_btnGotoBase->setVisible(_base != 0);

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& InterceptState::btnCancelClick);
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						Options::keyCancel);
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						Options::keyGeoIntercept);

	_txtCraft->setColor(Palette::blockOffset(8)+5);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtStatus->setColor(Palette::blockOffset(8)+5);
	_txtStatus->setText(tr("STR_STATUS"));

	_txtBase->setColor(Palette::blockOffset(8)+5);
	_txtBase->setBig();
	_txtBase->setText(tr("STR_INTERCEPT"));

	_txtWeapons->setColor(Palette::blockOffset(8)+5);
	_txtWeapons->setText(tr("STR_WEAPONS_CREW_HWPS"));

	_lstCrafts->setColor(Palette::blockOffset(15)-1);
	_lstCrafts->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstCrafts->setColumns(5, 91, 126, 25, 15, 15);
	_lstCrafts->setSelectable();
	_lstCrafts->setMargin();
	_lstCrafts->setBackground(_window);
	_lstCrafts->onMouseClick((ActionHandler)& InterceptState::lstCraftsLeftClick);
	_lstCrafts->onMouseClick(
					(ActionHandler)& InterceptState::lstCraftsRightClick,
					SDL_BUTTON_RIGHT);
	_lstCrafts->onMouseOver((ActionHandler)& InterceptState::lstCraftsMouseOver);
	_lstCrafts->onMouseOut((ActionHandler)& InterceptState::lstCraftsMouseOut);


	RuleCraft* rule;

	size_t row = 0;
	for (std::vector<Base*>::iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (_base != NULL
			&& *i != _base)
		{
			continue;
		}

		for (std::vector<Craft*>::iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			_bases.push_back((*i)->getName().c_str());
			_crafts.push_back(*j);

			std::wostringstream
				ss1,
				ss2,
				ss3;

			rule = (*j)->getRules();

			if (rule->getWeapons() > 0)
				ss1 << (*j)->getNumWeapons() << L"/" << rule->getWeapons();
			else
				ss1 << L"-";

			if (rule->getSoldiers() > 0)
				ss2 << (*j)->getNumSoldiers();
			else
				ss2 << L"-";

			if (rule->getVehicles() > 0)
				ss3 << (*j)->getNumVehicles();
			else
				ss3 << L"-";

			std::wstring status = getAltStatus(*j);
			_lstCrafts->addRow(
							5,
							(*j)->getName(_game->getLanguage()).c_str(),
							status.c_str(),
							ss1.str().c_str(),
							ss2.str().c_str(),
							ss3.str().c_str());

			_lstCrafts->setCellHighContrast(row, 1);
			_lstCrafts->setCellColor(
									row,
									1,
									_cellColor);

			row++;
		}
	}

	if (_base)
		_txtBase->setText(_base->getName());
}

/**
 * dTor.
 */
InterceptState::~InterceptState()
{
}

/**
 * A more descriptive state of the Crafts.
 * @param craft - pointer to Craft in question
 * @return, status string
 */
std::wstring InterceptState::getAltStatus(Craft* const craft)
{
	std::string stat = craft->getStatus();
	if (stat != "STR_OUT")
	{
		if (stat == "STR_READY")
		{
			_cellColor = Palette::blockOffset(2); // green (7)-2

			return tr(stat);
		}
		else if (stat == "STR_REFUELLING")
		{
			stat = "STR_REFUELLING_";
			_cellColor = Palette::blockOffset(10)+2; // slate gray
		}
		else if (stat == "STR_REARMING")
		{
			stat = "STR_REARMING_";
			_cellColor = Palette::blockOffset(10)+4; // slate gray
		}
		else if (stat == "STR_REPAIRS")
		{
			stat = "STR_REPAIRS_";
			_cellColor = Palette::blockOffset(10)+6; // slate gray
		}

		bool delayed;
		const int hours = craft->getDowntime(delayed);
		std::wstring wstr = formatTime(
									hours,
									delayed);

		return tr(stat).arg(wstr);
	}

	std::wstring status;

/*	Waypoint* wayPt = dynamic_cast<Waypoint*>(craft->getDestination());
	if (wayPt != NULL)
		status = tr("STR_INTERCEPTING_UFO").arg(wayPt->getId());
	else */

	if (craft->getLowFuel() == true)
	{
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
		_cellColor = Palette::blockOffset(9)+4; // brown
	}
	else if (craft->getMissionComplete() == true)
	{
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
		_cellColor = Palette::blockOffset(9)+6; // brown
	}
	else if (craft->getDestination() == dynamic_cast<Target*>(craft->getBase()))
	{
		status = tr("STR_RETURNING_TO_BASE");
		_cellColor = Palette::blockOffset(9)+2; // brown
	}
	else if (craft->getDestination() == NULL)
	{
		status = tr("STR_PATROLLING");
		_cellColor = Palette::blockOffset(4); // olive green // blue(5)+3
	}
	else
	{
		Ufo* ufo = dynamic_cast<Ufo*>(craft->getDestination());
		if (ufo != NULL)
		{
			if (craft->isInDogfight() == true) // chase UFO
			{
				status = tr("STR_TAILING_UFO").arg(ufo->getId());
				_cellColor = Palette::blockOffset(11); // purple
			}
			else if (ufo->getStatus() == Ufo::FLYING) // intercept UFO
			{
				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
				_cellColor = Palette::blockOffset(11)+1; // purple
			}
			else // landed UFO
			{
				status = tr("STR_DESTINATION_UC_")
							.arg(ufo->getName(_game->getLanguage()));
				_cellColor = Palette::blockOffset(11)+2; // purple
			}
		}
		else // crashed UFO, terrorSite, alienBase, or wayPoint
		{
			status = tr("STR_DESTINATION_UC_")
						.arg(craft->getDestination()->getName(_game->getLanguage()));
			_cellColor = Palette::blockOffset(9); // brown //(11)+3; purple
		}
	}

	return status;
}

/**
 * Formats a duration in hours into a day & hour string.
 * @param total		- time in hours
 * @param delayed	- true to add '+' for lack of materiel
 * @return, day & hour
 */
std::wstring InterceptState::formatTime(
		const int total,
		const bool delayed)
{
	std::wostringstream ss;
	const int
		days = total / 24,
		hours = total %24;

	if (days > 0)
	{
		ss << tr("STR_DAY", days);

		if (hours > 0)
			ss << L" ";
	}

	if (hours > 0)
		ss << tr("STR_HOUR", hours);

	if (delayed == true)
		ss << L" +";

	return ss.str();
}

/**
 * Closes the window.
 * @param action - pointer to an action
 */
void InterceptState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the base for the respective craft.
 * @param action - pointer to an action
 */
void InterceptState::btnGotoBaseClick(Action*)
{
	_geo->timerReset();

	_game->popState();
	_game->pushState(new BasescapeState(
									_base,
									_globe));
}

/**
 * Pick a target for the selected craft.
 * @param action - pointer to an action
 */
void InterceptState::lstCraftsLeftClick(Action*)
{
	Craft* craft = _crafts[_lstCrafts->getSelectedRow()];
	_game->pushState(new GeoscapeCraftState(
										craft,
										_globe,
										NULL,
										_geo,
										true));
}

/**
 * Centers on the selected craft.
 * @param action - pointer to an action
 */
void InterceptState::lstCraftsRightClick(Action*)
{
	_game->popState();

	Craft* craft = _crafts[_lstCrafts->getSelectedRow()];
	_globe->center(
				craft->getLongitude(),
				craft->getLatitude());
}

/**
 * Shows Base label.
 * @param action - pointer to an action
 */
void InterceptState::lstCraftsMouseOver(Action*)
{
	if (_base)
		return;

	std::wstring wsBase;

	int sel = _lstCrafts->getSelectedRow();
	if (static_cast<size_t>(sel) < _bases.size())
		wsBase = _bases[sel];
	else
		wsBase = tr("STR_INTERCEPT");

	_txtBase->setText(wsBase);
}

/**
 * Hides Base label.
 * @param action - pointer to an action
 */
void InterceptState::lstCraftsMouseOut(Action*)
{
	if (_base)
		return;

	_txtBase->setText(tr("STR_INTERCEPT"));
}

}
