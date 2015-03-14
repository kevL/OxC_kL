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

#include "CraftsState.h"

//#include <sstream>

#include "CraftInfoState.h"
#include "SellState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Geoscape/Globe.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Crafts screen.
 * @param base - pointer to the Base to get info from
 */
CraftsState::CraftsState(Base* base)
	:
		_base(base)
{
	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(300, 17, 10, 8);

	_txtBase	= new Text(294, 17, 16, 25);

	_txtName	= new Text(102, 9, 16, 49);
	_txtStatus	= new Text(76, 9, 118, 49);

	_txtWeapons	= new Text(50, 27, 235, 33);
//	_txtWeapon	= new Text(44, 17, 194, 49);
//	_txtCrew	= new Text(27, 9, 230, 49);
//	_txtHwp		= new Text(24, 9, 257, 49);

	_lstCrafts	= new TextList(298, 113, 16, 59);
//	_lstCrafts	= new TextList(285, 81, 16, 66);

	_btnOk		= new TextButton(288, 16, 16, 177);

	setPalette(
			"PAL_BASESCAPE",
			_game->getRuleset()->getInterface("craftSelect")->getElement("palette")->color);

	add(_window,		"window",	"craftSelect");
	add(_txtTitle,		"text",		"craftSelect");
	add(_txtBase,		"text",		"craftSelect");
	add(_txtName,		"text",		"craftSelect");
	add(_txtStatus,		"text",		"craftSelect");
	add(_txtWeapons,	"text",		"craftSelect");
//	add(_txtWeapon,		"text",		"craftSelect");
//	add(_txtCrew,		"text",		"craftSelect");
//	add(_txtHwp,		"text",		"craftSelect");
	add(_lstCrafts,		"list",		"craftSelect");
	add(_btnOk,			"button",	"craftSelect");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftsState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_INTERCEPTION_CRAFT"));

	_txtBase->setBig();
	_txtBase->setText(tr("STR_BASE_").arg(_base->getName()));
//	_txtBase->setText(_base->getName(_game->getLanguage()));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtStatus->setText(tr("STR_STATUS"));

	_txtWeapons->setText(tr("STR_WEAPONS_CREW_HWPS"));

/*	_txtWeapon->setText(tr("STR_WEAPON_SYSTEMS"));
	_txtCrew->setText(tr("STR_CREW"));
	_txtHwp->setText(tr("STR_HWPS")); */

	_lstCrafts->setArrowColumn(275, ARROW_VERTICAL);
	_lstCrafts->setColumns(5, 91, 120, 25, 15, 15);
	_lstCrafts->setBackground(_window);
	_lstCrafts->setSelectable();
	_lstCrafts->setMargin();
	_lstCrafts->onMousePress((ActionHandler)& CraftsState::lstCraftsPress);
	_lstCrafts->onLeftArrowClick((ActionHandler)& CraftsState::lstLeftArrowClick);
	_lstCrafts->onRightArrowClick((ActionHandler)& CraftsState::lstRightArrowClick);
}

/**
 * dTor.
 */
CraftsState::~CraftsState()
{}

/**
 * The soldier names can change after going into other screens.
 */
void CraftsState::init()
{
	State::init();

	_lstCrafts->clearList();

	const RuleCraft* craftRule;

	size_t row = 0;
	for (std::vector<Craft*>::const_iterator
			i = _base->getCrafts()->begin();
			i != _base->getCrafts()->end();
			++i,
				++row)
	{
		std::wostringstream
			ss1,
			ss2,
			ss3;

		craftRule = (*i)->getRules();

		if (craftRule->getWeapons() > 0)
			ss1 << (*i)->getNumWeapons() << L"/" << craftRule->getWeapons();
		else
			ss1 << L"-";

		if (craftRule->getSoldiers() > 0)
			ss2 << (*i)->getNumSoldiers();
		else
			ss2 << L"-";

		if (craftRule->getVehicles() > 0)
			ss3 << (*i)->getNumVehicles();
		else
			ss3 << L"-";

		std::wstring status = getAltStatus(*i);
		_lstCrafts->addRow(
						5,
						(*i)->getName(_game->getLanguage()).c_str(),
						status.c_str(),
						ss1.str().c_str(),
						ss2.str().c_str(),
						ss3.str().c_str());

		_lstCrafts->setCellColor(
								row,
								1,
								_cellColor,
								true);
	}

	_lstCrafts->draw();
}

/**
 * A more descriptive status of these Crafts.
 * @param craft - pointer to Craft in question
 * @return, status string
 */
std::wstring CraftsState::getAltStatus(Craft* const craft)
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

		_cellColor = LAVENDER;

		stat.push_back('_');

		bool delayed;
		const int hours = craft->getDowntime(delayed);
		std::wstring wst = formatTime(
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
		_cellColor = BLUE;
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

		_cellColor = YELLOW;
	}

	return status;
}

/**
 * Formats a duration in hours into a day & hour string.
 * @param total		- time in hours
 * @param delayed	- true to add '+' for lack of materiel
 * @return, day & hour
 */
std::wstring CraftsState::formatTime(
		const int total,
		const bool delayed)
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
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftsState::btnOkClick(Action*)
{
	_game->popState();

	if (_game->getSavedGame()->getMonthsPassed() > -1
		&& Options::storageLimitsEnforced == true
		&& _base->storesOverfull() == true)
	{
		_game->pushState(new SellState(_base));
		_game->pushState(new ErrorMessageState(
											tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
											_palette,
											_game->getRuleset()->getInterface("craftSelect")->getElement("errorMessage")->color,
											"BACK01.SCR",
											_game->getRuleset()->getInterface("craftSelect")->getElement("errorPalette")->color));
	}
}

/**
 * LMB shows the selected craft's info.
 * RMB pops out of Basescape and centers craft on Geoscape.
 * @param action - pointer to an Action
 */
void CraftsState::lstCraftsPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= _lstCrafts->getArrowsLeftEdge()
		&& mx < _lstCrafts->getArrowsRightEdge())
	{
		return;
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_base->getCrafts()->at(_lstCrafts->getSelectedRow())->getStatus() != "STR_OUT")
			_game->pushState(new CraftInfoState(
											_base,
											_lstCrafts->getSelectedRow()));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		const Craft* const craft = _base->getCrafts()->at(_lstCrafts->getSelectedRow());
		_game->getSavedGame()->setGlobeLongitude(craft->getLongitude());
		_game->getSavedGame()->setGlobeLatitude(craft->getLatitude());

		kL_reCenter = true;

		_game->popState(); // close Crafts window.
		_game->popState(); // close Basescape view.
	}
}

/**
 * Reorders a craft - moves craft-slot up.
 * @param action - pointer to an Action
 */
void CraftsState::lstLeftArrowClick(Action* action)
{
	const int row = _lstCrafts->getSelectedRow();
	if (row > 0)
	{
		Craft* const craft = _base->getCrafts()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getCrafts()->at(row) = _base->getCrafts()->at(row - 1);
			_base->getCrafts()->at(row - 1) = craft;

			if (row != static_cast<int>(_lstCrafts->getScroll()))
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse()
						- static_cast<int>(8. * action->getYScale())));
			}
			else
				_lstCrafts->scrollUp(false);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->getCrafts()->erase(_base->getCrafts()->begin() + row);
			_base->getCrafts()->insert(
									_base->getCrafts()->begin(),
									craft);
		}

		init();
	}
}

/**
 * Reorders a craft - moves craft-slot down.
 * @param action - pointer to an Action
 */
void CraftsState::lstRightArrowClick(Action* action)
{
	const size_t
		numCrafts = _base->getCrafts()->size(),
		row = _lstCrafts->getSelectedRow();

	if (numCrafts > 0
		&& numCrafts <= std::numeric_limits<size_t>::max()
		&& row < numCrafts - 1)
	{
		Craft* const craft = _base->getCrafts()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getCrafts()->at(row) = _base->getCrafts()->at(row + 1);
			_base->getCrafts()->at(row + 1) = craft;

			if (row != _lstCrafts->getVisibleRows() - 1 + _lstCrafts->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse()
						+ static_cast<int>(8. * action->getYScale())));
			}
			else
				_lstCrafts->scrollDown(false);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->getCrafts()->erase(_base->getCrafts()->begin() + row);
			_base->getCrafts()->insert(
									_base->getCrafts()->end(),
									craft);
		}

		init();
	}
}

}
