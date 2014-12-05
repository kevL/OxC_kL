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

#include "UfoDetectedState.h"

#include "GeoscapeState.h"
#include "Globe.h"
#include "InterceptState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Ufo Detected window.
 * @param ufo			- pointer to a UFO to get info from
 * @param state			- pointer to GeoscapeState
 * @param detected		- true if the UFO has just been detected
 * @param hyper			- true if the UFO has been hyperdetected
 * @param contact		- true if radar contact is established (default true)
 * @param hyperBases	- pointer to a vector of pointers to Bases that hyperdetected UFO (default NULL)
 */
UfoDetectedState::UfoDetectedState(
		Ufo* ufo,
		GeoscapeState* state,
		bool detected,
		bool hyper,
		bool contact,
		std::vector<Base*>* hyperBases)
	:
		_ufo(ufo),
		_state(state)
{
	state->getGlobe()->rotateStop();

	if (_ufo->getId() == 0) // generate UFO-ID
		_ufo->setId(_game->getSavedGame()->getId("STR_UFO"));

	if (_ufo->getAltitude() == "STR_GROUND"
		&& _ufo->getLandId() == 0)
	{
		_ufo->setLandId(_game->getSavedGame()->getId("STR_LANDING_SITE"));
	}

	_screen = false;

	if (hyper == true)
	{
		_window			= new Window(this, 224, 170, 16, 10, POPUP_BOTH);

		_txtHyperwave	= new Text(216, 16, 20, 45);
		_lstInfo2		= new TextList(192, 33, 32, 98);
		_txtBases		= new Text(100, 41, 32, 135);
	}
	else
		_window			= new Window(this, 224, 120, 16, 48, POPUP_BOTH);

	_txtUfo			= new Text(90, 16, 26, 56);
	_txtDetected	= new Text(80, 9, 32, 73);
	_lstInfo		= new TextList(192, 33, 32, 85);
	_btnCentre		= new TextButton(192, 16, 32, 124);
	_btnIntercept	= new TextButton(88, 16, 32, 144);
	_btnCancel		= new TextButton(88, 16, 136, 144);

	_txtRegion		= new Text(114, 9, 116, 56);
	_txtTexture		= new Text(114, 9, 116, 66);

	if (hyper == true)
	{
		_txtUfo->setY(19);
		_txtDetected->setY(36);
		_lstInfo->setY(60);
		_btnCentre->setY(135);
		_btnIntercept->setY(155);
		_btnCancel->setY(155);

		_txtRegion->setY(19);
		_txtTexture->setY(29);

		if (contact == false)
		{
			_btnCentre->setX(136);
			_btnCentre->setWidth(88);
		}

		setPalette("PAL_GEOSCAPE", 2);
	}
	else
		setPalette("PAL_GEOSCAPE", 7);

	add(_window);
	add(_txtUfo);
	add(_txtDetected);
	add(_lstInfo);
	add(_btnCentre);
	add(_btnIntercept);
	add(_btnCancel);

	add(_txtRegion);
	add(_txtTexture);

	if (hyper == true)
	{
		add(_txtHyperwave);
		add(_lstInfo2);
		add(_txtBases);
	}

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_txtUfo->setColor(Palette::blockOffset(8)+5);
	_txtUfo->setBig();
	_txtUfo->setText(_ufo->getName(_game->getLanguage()));

	if (detected == true)
	{
		_txtDetected->setColor(Palette::blockOffset(8)+5);
		_txtDetected->setText(tr("STR_DETECTED"));
	}
	else
		_txtDetected->setVisible(false);


	_txtRegion->setColor(Palette::blockOffset(8)+10);
	_txtRegion->setAlign(ALIGN_RIGHT);
	std::wostringstream ss;
	const double
		lon = _ufo->getLongitude(),
		lat = _ufo->getLatitude();

	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										lon,
										lat) == true)
		{
			ss << tr((*i)->getRules()->getType());
			break;
		}
	}

	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(
										lon,
										lat) == true)
		{
			ss << L"> " << tr((*i)->getRules()->getType());
			break;
		}
	}
	_txtRegion->setText(ss.str());


	_lstInfo->setColor(Palette::blockOffset(8)+5);
	_lstInfo->setColumns(2, 80, 112);
	_lstInfo->setDot();
	_lstInfo->addRow(
					2,
					tr("STR_SIZE_UC").c_str(),
					tr(_ufo->getRules()->getSize()).c_str());
	_lstInfo->setCellColor(0, 1, Palette::blockOffset(8)+10);

	std::string alt = _ufo->getAltitude();
	if (alt == "STR_GROUND")
		alt = "STR_GROUNDED";
	_lstInfo->addRow(
					2,
					tr("STR_ALTITUDE").c_str(),
					tr(alt).c_str());
	_lstInfo->setCellColor(1, 1, Palette::blockOffset(8)+10);

	std::string heading = _ufo->getDirection();
	if (_ufo->getStatus() != Ufo::FLYING)
		heading = "STR_UNKNOWN";
	_lstInfo->addRow(
					2,
					tr("STR_HEADING").c_str(),
					tr(heading).c_str());
	_lstInfo->setCellColor(2, 1, Palette::blockOffset(8)+10);

	_lstInfo->addRow(
					2,
					tr("STR_SPEED").c_str(),
					Text::formatNumber(_ufo->getSpeed()).c_str());
	_lstInfo->setCellColor(3, 1, Palette::blockOffset(8)+10);

	_btnIntercept->setColor(Palette::blockOffset(8)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& UfoDetectedState::btnInterceptClick);
	_btnIntercept->setVisible(contact);

	_btnCentre->setColor(Palette::blockOffset(8)+5);
	_btnCentre->setText(tr("STR_CENTER_ON_UFO_TIME_5_SECONDS"));
	_btnCentre->onMouseClick((ActionHandler)& UfoDetectedState::btnCentreClick);
	_btnCentre->onKeyboardPress(
					(ActionHandler)& UfoDetectedState::btnCentreClick,
					Options::keyOk);

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& UfoDetectedState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& UfoDetectedState::btnCancelClick,
					Options::keyCancel);

	if (ufo->getStatus() == Ufo::CRASHED
		|| ufo->getStatus() == Ufo::LANDED)
	{
		// IMPORTANT: This does not return the actual battleField terrain; that is done
		// in ConfirmLandingState. This is merely an indicator .... cf. DogfightState
		int
			texture,
			shade;
		state->getGlobe()->getPolygonTextureAndShade( // look up polygon's texture
												_ufo->getLongitude(),
												_ufo->getLatitude(),
												&texture,
												&shade);
		std::string terrain;

		switch (texture)
		{
			case 0:		terrain = "FOREST";	break;
			case 1:		terrain = "CULTA";	break;
			case 2:		terrain = "CULTA";	break;
			case 3:		terrain = "FOREST";	break;
			case 4:		terrain = "POLAR";	break;
			case 5:		terrain = "MOUNT";	break;
			case 6:		terrain = "JUNGLE";	break;
			case 7:		terrain = "DESERT";	break;
			case 8:		terrain = "DESERT";	break;
			case 9:		terrain = "POLAR";	break;
			case 10:	terrain = "URBAN";	break;
			case 11:	terrain = "POLAR";	break;
			case 12:	terrain = "POLAR";	break;

			default:
				terrain = "WATER"; // tex = -1
		}

		std::wostringstream ss;
		ss << tr(terrain);
		ss << L"> shade " << shade;

		_txtTexture->setColor(Palette::blockOffset(8)+10);
		_txtTexture->setSecondaryColor(Palette::blockOffset(8)+5);
		_txtTexture->setAlign(ALIGN_RIGHT);
		_txtTexture->setText(ss.str());
	}
	else
		_txtTexture->setVisible(false);

	if (hyper == true)
	{
		_txtHyperwave->setColor(Palette::blockOffset(8)+5);
		_txtHyperwave->setAlign(ALIGN_CENTER);
		_txtHyperwave->setWordWrap();
		_txtHyperwave->setText(tr("STR_HYPER_WAVE_TRANSMISSIONS_ARE_DECODED"));

		_lstInfo2->setColor(Palette::blockOffset(8)+5);
		_lstInfo2->setColumns(2, 80, 112);
		_lstInfo2->setDot();
		_lstInfo2->addRow(
						2,
						tr("STR_CRAFT_TYPE").c_str(),
						tr(_ufo->getRules()->getType()).c_str());
		_lstInfo2->setCellColor(0, 1, Palette::blockOffset(8)+10);
		_lstInfo2->addRow(
						2,
						tr("STR_RACE").c_str(),
						tr(_ufo->getAlienRace()).c_str());
		_lstInfo2->setCellColor(1, 1, Palette::blockOffset(8)+10);
		_lstInfo2->addRow(
						2,
						tr("STR_MISSION").c_str(),
						tr(_ufo->getMissionType()).c_str());
		_lstInfo2->setCellColor(2, 1, Palette::blockOffset(8)+10);
		_lstInfo2->addRow(
						2,
						tr("STR_ZONE").c_str(),
						tr(_ufo->getMission()->getRegion()).c_str());
		_lstInfo2->setCellColor(3, 1, Palette::blockOffset(8)+10);

		if (contact == false
			&& hyperBases != NULL) // safety.
		{
			_txtBases->setColor(Palette::blockOffset(8)+5);

			std::wostringstream bases;
			for (std::vector<Base*>::const_iterator
					i = hyperBases->begin();
					i != hyperBases->end();
					++i)
			{
				bases << (*i)->getName(_game->getLanguage()) << L"\n";
			}
			_txtBases->setText(bases.str());
		}
	}
}
/*		if		(texture == 0)	terrain = "FOREST"; // these could get expanded/redef'd. in future
		else if (texture == 1)	terrain = "CULTA";
		else if (texture == 2)	terrain = "CULTA";
		else if (texture == 3)	terrain = "FOREST";
		else if (texture == 4)	terrain = "POLAR";
		else if (texture == 5)	terrain = "MOUNT";
		else if (texture == 6)	terrain = "JUNGLE";
		else if (texture == 7)	terrain = "DESERT";
		else if (texture == 8)	terrain = "DESERT";
		else if (texture == 9)	terrain = "POLAR";
		else if (texture == 10)	terrain = "URBAN";
		else if (texture == 11)	terrain = "POLAR";
		else if (texture == 12)	terrain = "POLAR";
		else
			terrain = "WATER"; // tex = -1 */

/*
		// the first terrain found for proper Globe-texture is chosen

		RuleTerrain* terrainRule = NULL;

		const std::vector<std::string>& terrains = _game->getRuleset()->getTerrainList();
		for (std::vector<std::string>::const_iterator
				i = terrains.begin();
				i != terrains.end()
					&& terrain.empty() == true;
				++i)
		{
			//Log(LOG_INFO) << ". cycle terrains";
			terrainRule = _game->getRuleset()->getTerrain(*i);
			for (std::vector<int>::const_iterator
					j = terrainRule->getTextures()->begin();
					j != terrainRule->getTextures()->end()
						&& terrain.empty() == true;
					++j)
			{
				//Log(LOG_INFO) << ". . cycle textures";
				if (*j == texture
					&& (terrainRule->getHemisphere() == 0
						|| (terrainRule->getHemisphere() < 0
							&& _ufo->getLatitude() < 0.)
						|| (terrainRule->getHemisphere() > 0
							&& _ufo->getLatitude() >= 0.)))
				{
					//Log(LOG_INFO) << ". . . terrainRule-texture MATCH found: tex = " << texture";
					terrain = terrainRule->getName();
				}
			}
		}
		//Log(LOG_INFO) << ". terrain = " << terrain;

		if (terrain.empty() == true)
			terrain = "WATER";
		else if (terrain == "JUNGLE"
			|| terrain == "FORESTMOUNT"
			|| terrain == "MUJUNGLE")
		{
			terrain = "FOREST";
		}
		else if (terrain == "CULTAFARMA"
			|| terrain == "CULTAFARMB")
		{
			terrain = "CULTA";
		}
		else if (terrain == "DESERTMOUNT"
			|| terrain == "ATLANTDESERT")
		{
			terrain = "DESERT";
		}
		else if (terrain == "POLARMOUNT")
			terrain = "POLAR";
//		else if (terrain == "COMRCURBAN"	// these are Terror sites only:
//			|| terrain == "DAWNURBANA"		// ie. not referenced by any of the Globe's polygon textures.
//			|| terrain == "DAWNURBANB"		// but NOTE: if UFO lands directly on a city,
//			|| terrain == "INDUSTRIALURBAN"	// one of these should pop via ConfirmLandingState
//			|| terrain == "MADURBAN"
//			|| terrain == "NATIVEURBAN"
//			|| terrain == "PORTURBAN"
//			|| terrain == "RAILYARDURBAN")
//		{
//			terrain = "URBAN";
//		} */

/**
 * dTor.
 */
UfoDetectedState::~UfoDetectedState()
{
}

/**
 * Pick a craft to intercept the UFO.
 * @param action - pointer to an Action
 */
void UfoDetectedState::btnInterceptClick(Action*)
{
	_state->timerReset();
//	_state->getGlobe()->center(_ufo->getLongitude(), _ufo->getLatitude());

	_game->popState();
	_game->pushState(new InterceptState(
									_state->getGlobe(),
									NULL,
//									_ufo,
									_state));
}

/**
 * Centers the globe on the UFO and returns to the previous screen.
 * @param action - pointer to an Action
 */
void UfoDetectedState::btnCentreClick(Action*)
{
	_state->timerReset();
	_state->getGlobe()->center(
							_ufo->getLongitude(),
							_ufo->getLatitude());

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void UfoDetectedState::btnCancelClick(Action*)
{
	_state->timerReset();
	_game->popState();
}

}
