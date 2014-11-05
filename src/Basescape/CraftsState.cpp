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

#include "CraftsState.h"

#include <sstream>

#include "CraftInfoState.h"
#include "SellState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

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
CraftsState::CraftsState(
		Base* base)
	:
		_base(base)
{
	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(300, 17, 10, 8);

	_txtBase	= new Text(294, 17, 16, 25);

	_txtName	= new Text(102, 9, 16, 49);
	_txtStatus	= new Text(76, 9, 118, 49);
	_txtWeapon	= new Text(44, 17, 194, 49);
	_txtCrew	= new Text(27, 9, 230, 49);
	_txtHwp		= new Text(24, 9, 257, 49);

	_lstCrafts	= new TextList(288, 113, 16, 59);

	_btnOk		= new TextButton(288, 16, 16, 177);

	setPalette("PAL_BASESCAPE", 3);

	add(_window);
	add(_txtTitle);
	add(_txtBase);
	add(_txtName);
	add(_txtStatus);
	add(_txtWeapon);
	add(_txtCrew);
	add(_txtHwp);
	add(_lstCrafts);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftsState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(15)+1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_INTERCEPTION_CRAFT"));

	_txtBase->setColor(Palette::blockOffset(15)+1);
	_txtBase->setBig();
	_txtBase->setText(tr("STR_BASE_").arg(_base->getName()));
//	_txtBase->setText(_base->getName(_game->getLanguage()));

	_txtName->setColor(Palette::blockOffset(15)+1);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtStatus->setColor(Palette::blockOffset(15)+1);
	_txtStatus->setText(tr("STR_STATUS"));

	_txtWeapon->setColor(Palette::blockOffset(15)+1);
	_txtWeapon->setText(tr("STR_WEAPON_SYSTEMS"));
	_txtWeapon->setWordWrap();

	_txtCrew->setColor(Palette::blockOffset(15)+1);
	_txtCrew->setText(tr("STR_CREW"));

	_txtHwp->setColor(Palette::blockOffset(15)+1);
	_txtHwp->setText(tr("STR_HWPS"));

	_lstCrafts->setColor(Palette::blockOffset(13)+10);
	_lstCrafts->setArrowColor(Palette::blockOffset(15)+1);
	_lstCrafts->setArrowColumn(265, ARROW_VERTICAL);
	_lstCrafts->setColumns(5, 94, 76, 36, 27, 47);
	_lstCrafts->setSelectable();
	_lstCrafts->setBackground(_window);
	_lstCrafts->setMargin();
	_lstCrafts->onMousePress((ActionHandler)& CraftsState::lstCraftsPress);
	_lstCrafts->onLeftArrowClick((ActionHandler)& CraftsState::lstLeftArrowClick);
	_lstCrafts->onRightArrowClick((ActionHandler)& CraftsState::lstRightArrowClick);
}

/**
 * dTor.
 */
CraftsState::~CraftsState()
{
}

/**
 * The soldier names can change after going into other screens.
 */
void CraftsState::init()
{
	State::init();

	_lstCrafts->clearList();

	RuleCraft* rule;

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

		rule = (*i)->getRules();

		if (rule->getWeapons() > 0)
			ss1 << (*i)->getNumWeapons() << L"/" << rule->getWeapons();
		else
			ss1 << L"-";

		if (rule->getSoldiers() > 0)
			ss2 << (*i)->getNumSoldiers();
		else
			ss2 << L"-";

		if (rule->getVehicles() > 0)
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

		_lstCrafts->setCellHighContrast(row, 1);
		_lstCrafts->setCellColor(
								row,
								1,
								_cellColor);
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
			_cellColor = Palette::blockOffset(3)+2;
		else if (stat == "STR_REFUELLING")
			_cellColor = Palette::blockOffset(4)+2;
		else if (stat == "STR_REARMING")
			_cellColor = Palette::blockOffset(4)+4;
		else if (stat == "STR_REPAIRS")
			_cellColor = Palette::blockOffset(4)+6;

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
		status = tr("STR_LOW_FUEL"); // "STR_LOW_FUEL_RETURNING_TO_BASE"
		_cellColor = Palette::blockOffset(1)+4;
	}
	else if (craft->getMissionComplete() == true)
	{
		status = tr("STR_MISSION_COMPLETE"); // "STR_MISSION_COMPLETE_RETURNING_TO_BASE"
		_cellColor = Palette::blockOffset(1)+6;
	}
	else if (craft->getDestination() == dynamic_cast<Target*>(craft->getBase()))
	{
		status = tr("STR_BASE"); // "STR_RETURNING_TO_BASE"
		_cellColor = Palette::blockOffset(5)+4;
	}
	else if (craft->getDestination() == NULL)
	{
		status = tr("STR_PATROLLING");
		_cellColor = Palette::blockOffset(8)+4;
	}
	else
	{
		Ufo* ufo = dynamic_cast<Ufo*>(craft->getDestination());
		if (ufo != NULL)
		{
			if (craft->isInDogfight() == true) // chase UFO
			{
				status = tr("STR_UFO_").arg(ufo->getId()); // "STR_TAILING_UFO"
				_cellColor = Palette::blockOffset(2)+2;
			}
			else if (ufo->getStatus() == Ufo::FLYING) // intercept UFO
			{
				status = tr("STR_UFO_").arg(ufo->getId()); // "STR_INTERCEPTING_UFO"
				_cellColor = Palette::blockOffset(2)+4;
			}
			else // landed UFO
			{
//				status = tr("STR_DESTINATION_UC_").arg(ufo->getName(_game->getLanguage()));
				status = ufo->getName(_game->getLanguage());
				_cellColor = Palette::blockOffset(2)+6;
			}
		}
		else // crashed UFO, terrorSite, alienBase, or wayPoint
		{
//			status = tr("STR_DESTINATION_UC_").arg(craft->getDestination()->getName(_game->getLanguage()));
			status = craft->getDestination()->getName(_game->getLanguage());
			_cellColor = Palette::blockOffset(2)+8;
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
std::wstring CraftsState::formatTime(
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
 * Returns to the previous screen.
 * @param action - pointer to an action
 */
void CraftsState::btnOkClick(Action*)
{
	_game->popState();

	if (_game->getSavedGame()->getMonthsPassed() > -1
		&& Options::storageLimitsEnforced
		&& _base->storesOverfull())
	{
		_game->pushState(new SellState(_base));

		_game->pushState(new ErrorMessageState(
											tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
											_palette,
											Palette::blockOffset(15)+1,
											"BACK01.SCR",
											0));
	}
}

/**
 * LMB shows the selected craft's info.
 * RMB pops out of Basescape and centers craft on Geoscape.
 * @param action - pointer to an action
 */
void CraftsState::lstCraftsPress(Action* action)
{
	double mx = action->getAbsoluteXMouse();
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
		Craft* craft = _base->getCrafts()->at(_lstCrafts->getSelectedRow());
		_game->getSavedGame()->setGlobeLongitude(craft->getLongitude());
		_game->getSavedGame()->setGlobeLatitude(craft->getLatitude());

		kL_reCenter = true;

		_game->popState(); // close Crafts window.
		_game->popState(); // close Basescape view.
	}
}

/**
 * Reorders a craft - moves craft-slot up.
 * @param action - pointer to an action
 */
void CraftsState::lstLeftArrowClick(Action* action)
{
	int row = _lstCrafts->getSelectedRow();
	if (row > 0)
	{
		Craft* craft = _base->getCrafts()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getCrafts()->at(row) = _base->getCrafts()->at(row - 1);
			_base->getCrafts()->at(row - 1) = craft;

			if (row != static_cast<int>(_lstCrafts->getScroll()))
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() - static_cast<int>(8.0 * action->getYScale())));
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
 * @param action - pointer to an action
 */
void CraftsState::lstRightArrowClick(Action* action)
{
	int row = _lstCrafts->getSelectedRow();
	size_t numCrafts = _base->getCrafts()->size();

	if (numCrafts > 0
		&& numCrafts <= INT_MAX
		&& row < static_cast<int>(numCrafts) - 1)
	{
		Craft* craft = _base->getCrafts()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getCrafts()->at(row) = _base->getCrafts()->at(row + 1);
			_base->getCrafts()->at(row + 1) = craft;

			if (row != static_cast<int>(_lstCrafts->getVisibleRows() - 1 + _lstCrafts->getScroll()))
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() + static_cast<int>(8.0 * action->getYScale())));
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
