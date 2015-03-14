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

#include "InterceptState.h"

//#include <sstream>

#include "ConfirmDestinationState.h"
#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"

#include "../Basescape/BasescapeState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

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
// * @param target	- pointer to Target to intercept (default NULL to ask user for target)
 * @param geo		- pointer to GeoscapeState (default NULL)
 */
InterceptState::InterceptState(
		Globe* globe,
		Base* base,
//		Target* target,
		GeoscapeState* geo)
	:
		_globe(globe),
		_base(base),
//		_target(target),
		_geo(geo)
{
	_screen = false;

	_window			= new Window(
								this,
								320,
								176,
								0,
								14,
								POPUP_HORIZONTAL);
	_txtBase		= new Text(288, 17, 16, 24); // might do getRegion in here also.

	_txtCraft		= new Text(86, 9, 16, 40);
	_txtStatus		= new Text(53, 9, 115, 40);
	_txtWeapons		= new Text(50, 27, 241, 24);

	_lstCrafts		= new TextList(285, 113, 16, 50);

	_btnGotoBase	= new TextButton(142, 16, 16, 167);
	_btnCancel		= new TextButton(142, 16, 162, 167);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("geoCraftScreens")->getElement("palette")->color);

	add(_window,		"window",	"geoCraftScreens");
	add(_txtBase,		"text2",	"geoCraftScreens");
	add(_txtCraft,		"text2",	"geoCraftScreens");
	add(_txtStatus,		"text2",	"geoCraftScreens");
	add(_txtWeapons,	"text2",	"geoCraftScreens");
	add(_lstCrafts,		"text1",	"geoCraftScreens");
	add(_btnGotoBase,	"button",	"geoCraftScreens");
	add(_btnCancel,		"button",	"geoCraftScreens");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	_btnGotoBase->onMouseClick((ActionHandler)& InterceptState::btnGotoBaseClick);
	_btnGotoBase->setVisible(_base != 0);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& InterceptState::btnCancelClick);
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						Options::keyCancel);
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						Options::keyGeoIntercept);

	_txtCraft->setText(tr("STR_CRAFT"));

	_txtStatus->setText(tr("STR_STATUS"));

	_txtBase->setBig();
	_txtBase->setText(tr("STR_INTERCEPT"));

	_txtWeapons->setText(tr("STR_WEAPONS_CREW_HWPS"));

	_lstCrafts->setColumns(5, 91, 126, 25, 15, 15);
	_lstCrafts->setBackground(_window);
	_lstCrafts->setSelectable();
	_lstCrafts->setMargin();
	_lstCrafts->onMouseClick((ActionHandler)& InterceptState::lstCraftsLeftClick);
	_lstCrafts->onMouseClick(
					(ActionHandler)& InterceptState::lstCraftsRightClick,
					SDL_BUTTON_RIGHT);
	_lstCrafts->onMouseOver((ActionHandler)& InterceptState::lstCraftsMouseOver);
	_lstCrafts->onMouseOut((ActionHandler)& InterceptState::lstCraftsMouseOut);


	const RuleCraft* rule = NULL;

	size_t row = 0;
	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (_base != NULL
			&& *i != _base)
		{
			continue;
		}

		for (std::vector<Craft*>::const_iterator
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

			const std::wstring status = getAltStatus(*j);
			_lstCrafts->addRow(
							5,
							(*j)->getName(_game->getLanguage()).c_str(),
							status.c_str(),
							ss1.str().c_str(),
							ss2.str().c_str(),
							ss3.str().c_str());

			_lstCrafts->setCellColor(
									row,
									1,
									_cellColor,
									true);

			++row;
		}
	}

	if (_base != NULL)
		_txtBase->setText(_base->getName());
}

/**
 * dTor.
 */
InterceptState::~InterceptState()
{}

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
			_cellColor = GREEN;
			return tr(stat);
		}

/*		if (stat == "STR_REFUELLING")
			stat = "STR_REFUELLING_";
		else if (stat == "STR_REARMING")
			stat = "STR_REARMING_";
		else if (stat == "STR_REPAIRS")
			stat = "STR_REPAIRS_"; */

		_cellColor = SLATE;

		stat.push_back('_');

		bool delayed;
		const int hours = craft->getDowntime(delayed);
		const std::wstring wst = formatTime(
											hours,
											delayed);
		return tr(stat).arg(wst);
	}

	std::wstring status;
	if (craft->getLowFuel() == true)
	{
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
		_cellColor = BROWN;
	}
	else if (craft->getMissionComplete() == true)
	{
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
		_cellColor = BROWN;
	}
	else if (craft->getDestination() == dynamic_cast<Target*>(craft->getBase()))
	{
		status = tr("STR_RETURNING_TO_BASE");
		_cellColor = BROWN;
	}
	else if (craft->getDestination() == NULL)
	{
		status = tr("STR_PATROLLING");
		_cellColor = OLIVE;
	}
	else
	{
		const Ufo* const ufo = dynamic_cast<Ufo*>(craft->getDestination());
		if (ufo != NULL)
		{
			if (craft->isInDogfight() == true)
				status = tr("STR_TAILING_UFO").arg(ufo->getId());
			else if (ufo->getStatus() == Ufo::FLYING)
				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
			else
				status = tr("STR_DESTINATION_UC_")
							.arg(ufo->getName(_game->getLanguage()));
		}
		else
			status = tr("STR_DESTINATION_UC_")
						.arg(craft->getDestination()->getName(_game->getLanguage()));

		_cellColor = PURPLE;
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
		const bool delayed) const
{
	std::wostringstream woststr;
	woststr << L"(";

	const int
		days = total / 24,
		hours = total %24;

	if (days > 0)
	{
		woststr << tr("STR_DAY", days);

		if (hours > 0)
			woststr << L" ";
	}

	if (hours > 0)
		woststr << tr("STR_HOUR", hours);

	if (delayed == true)
		woststr << L" +";

	woststr << L")";

	return woststr.str();
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void InterceptState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the base for the respective craft.
 * @param action - pointer to an Action
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
 * Opens the craft info window.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsLeftClick(Action*)
{
	Craft* const craft = _crafts[_lstCrafts->getSelectedRow()];
	_game->pushState(new GeoscapeCraftState(
										craft,
										_globe,
										NULL,
										_geo,
										true));
}

/**
 * Centers on the selected craft.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsRightClick(Action*)
{
	_game->popState();

	const Craft* const craft = _crafts[_lstCrafts->getSelectedRow()];
	_globe->center(
				craft->getLongitude(),
				craft->getLatitude());
}

/**
 * Shows Base label.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsMouseOver(Action*)
{
	if (_base != NULL)
		return;

	std::wstring wsBase;

	const int sel = _lstCrafts->getSelectedRow();
	if (static_cast<size_t>(sel) < _bases.size())
		wsBase = _bases[sel];
	else
		wsBase = tr("STR_INTERCEPT");

	_txtBase->setText(wsBase);
}

/**
 * Hides Base label.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsMouseOut(Action*)
{
	if (_base != NULL)
		return;

	_txtBase->setText(tr("STR_INTERCEPT"));
}

}
