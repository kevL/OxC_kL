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

#include "AlienBaseState.h"

#include <sstream>

#include "GeoscapeState.h"
#include "Globe.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the AlienBase discovered window.
 * @param base	- pointer to the AlienBase to get info from
 * @param state	- pointer to the GeoscapeState
 */
AlienBaseState::AlienBaseState(
		AlienBase* base,
		GeoscapeState* state)
	:
		_state(state),
		_base(base)
{
	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(308, 60, 6, 60);
	_btnOk		= new TextButton(50, 12, 135, 180);

	setPalette("PAL_GEOSCAPE", 3);

	add(_window);
	add(_txtTitle);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& AlienBaseState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AlienBaseState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AlienBaseState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();

	double // Check location of base
		lon = _base->getLongitude(),
		lat = _base->getLatitude();

	std::wstring region, country;
	for (std::vector<Country*>::iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(
										lon,
										lat))
		{
			country = tr((*i)->getRules()->getType());
			break;
		}
	}

	for (std::vector<Region*>::iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										lon,
										lat))
		{
			region = tr((*i)->getRules()->getType());
			break;
		}
	}


	std::wstring location;

	if (country.empty() == false)
		location = tr("STR_COUNTRIES_COMMA").arg(country).arg(region);
	else if (region.empty() == false)
		location = region;
	else
		location = tr("STR_UNKNOWN");

	_txtTitle->setText(tr("STR_XCOM_AGENTS_HAVE_LOCATED_AN_ALIEN_BASE_IN_REGION").arg(location));
}

/**
 * dTor.
 */
AlienBaseState::~AlienBaseState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void AlienBaseState::btnOkClick(Action*)
{
	_state->timerReset();

	_state->getGlobe()->center(
							_base->getLongitude(),
							_base->getLatitude());
	_game->popState();
}

}
