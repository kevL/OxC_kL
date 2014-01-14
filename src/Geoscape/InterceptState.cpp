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

#include "InterceptState.h"

#include <sstream>

#include "ConfirmDestinationState.h"
#include "GeoscapeCraftState.h" // kL
#include "Globe.h"
#include "SelectDestinationState.h"

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


namespace OpenXcom
{

/**
 * Initializes all the elements in the Intercept window.
 * @param game, Pointer to the core game.
 * @param globe, Pointer to the Geoscape globe.
 * @param base, Pointer to base to show contained crafts (NULL to show all crafts).
 * @param target, Pointer to target to intercept (NULL to ask user for target).
 */
InterceptState::InterceptState(
		Game* game,
		Globe* globe,
		Base* base,
		Target* target)
	:
		State(game),
		_globe(globe),
		_base(base),
		_target(target),
		_crafts(),
		_bases()
{
	_screen = false;

	_window		= new Window(
							this,
							320,
							144,
							0,
							30,
							POPUP_HORIZONTAL);
//	_txtTitle	= new Text(240, 17, 70, 40);
	_txtBase	= new Text(288, 17, 16, 41); // might do getRegion in here also.

	_txtCraft	= new Text(86, 9, 16, 64);
	_txtStatus	= new Text(53, 9, 117, 64);
	_txtWeapons	= new Text(67, 17, 243, 56);

	_lstCrafts	= new TextList(285, 72, 16, 74);

	_btnCancel	= new TextButton(288, 16, 16, 151);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(4)),
				Palette::backPos,
				16);

	add(_window);
//	add(_txtTitle);
	add(_txtBase);
	add(_txtCraft);
	add(_txtStatus);
	add(_txtWeapons);
	add(_lstCrafts);
	add(_btnCancel);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& InterceptState::btnCancelClick);
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						(SDLKey)Options::getInt("keyOk"));
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						(SDLKey)Options::getInt("keyCancel"));
	_btnCancel->onKeyboardPress(
						(ActionHandler)& InterceptState::btnCancelClick,
						(SDLKey)Options::getInt("keyGeoIntercept"));

//	_txtTitle->setColor(Palette::blockOffset(15)-1);
//	_txtTitle->setAlign(ALIGN_CENTER);
//	_txtTitle->setBig();
//	_txtTitle->setText(tr("STR_LAUNCH_INTERCEPTION"));

	_txtCraft->setColor(Palette::blockOffset(8)+5);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtStatus->setColor(Palette::blockOffset(8)+5);
	_txtStatus->setText(tr("STR_STATUS"));

	_txtBase->setColor(Palette::blockOffset(8)+5);
	_txtBase->setBig();
//	_txtBase->setText(L"");
	_txtBase->setText(tr("STR_INTERCEPT"));

	_txtWeapons->setColor(Palette::blockOffset(8)+5);
	_txtWeapons->setText(tr("STR_WEAPONS_CREW_HWPS"));

	_lstCrafts->setColor(Palette::blockOffset(15)-1);
	_lstCrafts->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstCrafts->setColumns(3, 93, 126, 50);
	_lstCrafts->setSelectable(true);
	_lstCrafts->setMargin(8);
	_lstCrafts->setBackground(_window);
	_lstCrafts->onMouseClick((ActionHandler)& InterceptState::lstCraftsLeftClick);
	_lstCrafts->onMouseClick(
					(ActionHandler)& InterceptState::lstCraftsRightClick,
					SDL_BUTTON_RIGHT);
	_lstCrafts->onMouseOver((ActionHandler)& InterceptState::lstCraftsMouseOver);
	_lstCrafts->onMouseOut((ActionHandler)& InterceptState::lstCraftsMouseOut);

	int r = 0;

	for (std::vector<Base*>::iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (_base != 0
			&& *i != _base)
		{
			continue;
		}

		for (std::vector<Craft*>::iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			_crafts.push_back(*j);

			std::wstring wsBase = (*i)->getName().c_str();
			_bases.push_back(wsBase);

			std::wstringstream ss;

			if ((*j)->getNumWeapons() > 0)
				ss << L'\x01' << (*j)->getNumWeapons() << L'\x01';
			else
				ss << 0;

			ss << "/";
			if ((*j)->getNumSoldiers() > 0)
				ss << L'\x01' << (*j)->getNumSoldiers() << L'\x01';
			else
				ss << 0;

			ss << "/";
			if ((*j)->getNumVehicles() > 0)
				ss << L'\x01' << (*j)->getNumVehicles() << L'\x01';
			else
				ss << 0;


			std::wstring status = getAltStatus(*j); // kL
			_lstCrafts->addRow(
							3,
							(*j)->getName(_game->getLanguage()).c_str(),
							status.c_str(),
							ss.str().c_str());

			_lstCrafts->setCellColor(r, 1, _cellColor);
//			if ((*j)->getStatus() == "STR_READY")
//				_lstCrafts->setCellColor(row, 1, Palette::blockOffset(8)+10);
//			colorStatusCell();
			_lstCrafts->setCellHighContrast(
										r,
										1,
										true);

			r++;
		}
	}

	if (_base)
		_txtBase->setText(_base->getName());
}

/**
 *
 */
InterceptState::~InterceptState()
{
}

/**
 * kL. A more descriptive state of the Craft.
 */
std::wstring InterceptState::getAltStatus(Craft* craft)
{
	std::string stat = craft->getStatus();
	if (stat != "STR_OUT")
	{
		if (stat == "STR_READY")
			_cellColor = Palette::blockOffset(7)-2;
		else if (stat == "STR_REFUELLING")
			_cellColor = Palette::blockOffset(10)+2;
		else if (stat == "STR_REARMING")
			_cellColor = Palette::blockOffset(10)+4;
		else if (stat == "STR_REPAIRS")
			_cellColor = Palette::blockOffset(10)+6;

		return tr(stat);
	}

	std::wstring status;

/*	Waypoint* wayPt = dynamic_cast<Waypoint*>(craft->getDestination());
	if (wayPt != 0)
		status = tr("STR_INTERCEPTING_UFO").arg(wayPt->getId());
	else */
	if (craft->getLowFuel())
	{
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
		_cellColor = Palette::blockOffset(9)+4;
	}
	else if (craft->getDestination() == 0)
	{
		status = tr("STR_PATROLLING");
		_cellColor = Palette::blockOffset(5)+3;
	}
	else if (craft->getDestination() == dynamic_cast<Target*>(craft->getBase()))
	{
		status = tr("STR_RETURNING_TO_BASE");
		_cellColor = Palette::blockOffset(9)+2;
	}
	else
	{
		Ufo* ufo = dynamic_cast<Ufo*>(craft->getDestination());
		if (ufo != 0)
		{
			if (craft->isInDogfight()) // chase
			{
				status = tr("STR_TAILING_UFO");
				_cellColor = Palette::blockOffset(11);
			}
			else if (ufo->getStatus() == Ufo::FLYING) // intercept
			{
				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
				_cellColor = Palette::blockOffset(11)+1;
			}
			else // landed
			{
				status = tr("STR_DESTINATION_UC_")
							.arg(ufo->getName(_game->getLanguage()));
				_cellColor = Palette::blockOffset(11)+2;
			}
		}
		else // crashed,terrorSite,alienBase
		{
			status = tr("STR_DESTINATION_UC_")
						.arg(craft->getDestination()->getName(_game->getLanguage()));
			_cellColor = Palette::blockOffset(11)+3;
		}
	}

	return status;
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void InterceptState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Pick a target for the selected craft.
 * @param action Pointer to an action.
 */
void InterceptState::lstCraftsLeftClick(Action*)
{
	_game->popState();

	Craft* c = _crafts[_lstCrafts->getSelectedRow()];
	_game->pushState(new GeoscapeCraftState(
										_game,
										c,
										_globe,
										0));
}

/**
 * Centers on the selected craft.
 * @param action Pointer to an action.
 */
void InterceptState::lstCraftsRightClick(Action*)
{
	_game->popState();

	Craft* c = _crafts[_lstCrafts->getSelectedRow()];
	_globe->center(
				c->getLongitude(),
				c->getLatitude());
}

// kL. These two are from SavedGameState:
/**
 * Shows Base label.
 */
void InterceptState::lstCraftsMouseOver(Action*)
{
	if (_base) return;

	std::wstring wsBase;

	int sel = _lstCrafts->getSelectedRow();
	if (sel < static_cast<int>(_bases.size()))
	{
		wsBase = _bases[sel];
	}

	_txtBase->setText(wsBase);
}

/**
 * Hides Base label.
 */
void InterceptState::lstCraftsMouseOut(Action*)
{
	if (_base) return;

//	_txtBase->setText(L"");
	_txtBase->setText(tr("STR_INTERCEPT"));
}

}
