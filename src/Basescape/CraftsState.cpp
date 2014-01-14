/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CraftsState.h"

#include <sstream>

#include "CraftInfoState.h"

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


namespace OpenXcom
{

/**
 * Initializes all the elements in the Equip Craft screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
CraftsState::CraftsState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(300, 17, 10, 8);

	_txtBase	= new Text(294, 17, 16, 25);

	_txtName	= new Text(102, 9, 16, 49);
	_txtStatus	= new Text(76, 9, 118, 49);
	_txtWeapon	= new Text(46, 17, 194, 41);
	_txtCrew	= new Text(38, 9, 240, 49);
	_txtHwp		= new Text(23, 9, 268, 49);

	_lstCrafts	= new TextList(285, 112, 16, 59);

	_btnOk		= new TextButton(288, 16, 16, 177);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(3)),
				Palette::backPos,
				16);

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
					(SDLKey)Options::getInt("keyCancel"));

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
	_txtWeapon->setWordWrap(true);

	_txtCrew->setColor(Palette::blockOffset(15)+1);
	_txtCrew->setText(tr("STR_CREW"));

	_txtHwp->setColor(Palette::blockOffset(15)+1);
	_txtHwp->setText(tr("STR_HWPS"));

	_lstCrafts->setColor(Palette::blockOffset(13)+10);
	_lstCrafts->setArrowColor(Palette::blockOffset(15)+1);
	_lstCrafts->setColumns(5, 94, 76, 46, 28, 23);
	_lstCrafts->setSelectable(true);
	_lstCrafts->setBackground(_window);
	_lstCrafts->setMargin(8);
	_lstCrafts->onMouseClick((ActionHandler)& CraftsState::lstCraftsClick);
}

/**
 *
 */
CraftsState::~CraftsState()
{
}

/**
 * The soldier names can change
 * after going into other screens.
 */
void CraftsState::init()
{
	_lstCrafts->clearList();

	int r = 0;

	for (std::vector<Craft*>::iterator
			i = _base->getCrafts()->begin();
			i != _base->getCrafts()->end();
			++i)
	{
		std::wstring status = getAltStatus(*i);

		std::wstringstream
			ss,
			ss2,
			ss3;
		ss << (*i)->getNumWeapons() << "/" << (*i)->getRules()->getWeapons();
		ss2 << (*i)->getNumSoldiers();
		ss3 << (*i)->getNumVehicles();

		_lstCrafts->addRow(
						5,
						(*i)->getName(_game->getLanguage()).c_str(),
//						tr((*i)->getStatus()).c_str(),
						status.c_str(),
						ss.str().c_str(),
						ss2.str().c_str(),
						ss3.str().c_str());

		_lstCrafts->setCellColor(r, 1, _cellColor);
//		if ((*j)->getStatus() == "STR_READY")
//			_lstCrafts->setCellColor(row, 1, Palette::blockOffset(8)+10);
//		colorStatusCell();
		_lstCrafts->setCellHighContrast(
									r,
									1,
									true);

		r++;
	}
}

/**
 * kL. A more descriptive state of the Craft.
 */
std::wstring CraftsState::getAltStatus(Craft* craft)
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

		return tr(stat);
	}

	std::wstring status;

/*	Waypoint* wayPt = dynamic_cast<Waypoint*>(craft->getDestination());
	if (wayPt != 0)
		status = tr("STR_INTERCEPTING_UFO").arg(wayPt->getId());
	else */
	if (craft->getLowFuel())
	{
//		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
		status = tr("STR_LOW_FUEL");
		_cellColor = Palette::blockOffset(1)+4;
	}
	else if (craft->getDestination() == 0)
	{
		status = tr("STR_PATROLLING");
		_cellColor = Palette::blockOffset(8)+4;
	}
	else if (craft->getDestination() == dynamic_cast<Target*>(craft->getBase()))
	{
//		status = tr("STR_RETURNING_TO_BASE");
		status = tr("STR_BASE");
		_cellColor = Palette::blockOffset(5)+4;
	}
	else
	{
		Ufo* ufo = dynamic_cast<Ufo*>(craft->getDestination());
		if (ufo != 0)
		{
			if (craft->isInDogfight()) // chase
			{
//				status = tr("STR_TAILING_UFO");
				status = tr("STR_UFO_").arg(ufo->getId());
				_cellColor = Palette::blockOffset(2)+2;
			}
			else if (ufo->getStatus() == Ufo::FLYING) // intercept
			{
//				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
				status = tr("STR_UFO_").arg(ufo->getId());
				_cellColor = Palette::blockOffset(2)+4;
			}
			else // landed
			{
//				status = tr("STR_DESTINATION_UC_")
//							.arg(ufo->getName(_game->getLanguage()));
				status = ufo->getName(_game->getLanguage());
				_cellColor = Palette::blockOffset(2)+6;
			}
		}
		else // crashed,terrorSite,alienBase
		{
//			status = tr("STR_DESTINATION_UC_")
//						.arg(craft->getDestination()->getName(_game->getLanguage()));
			status = craft->getDestination()->getName(_game->getLanguage());
			_cellColor = Palette::blockOffset(2)+8;
		}
	}

	return status;
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftsState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Shows the selected craft's info.
 * @param action Pointer to an action.
 */
void CraftsState::lstCraftsClick(Action*)
{
	if (_base->getCrafts()->at(_lstCrafts->getSelectedRow())->getStatus() != "STR_OUT")
	{
		_game->pushState(new CraftInfoState(
										_game,
										_base,
										_lstCrafts->getSelectedRow()));
	}
}

}
