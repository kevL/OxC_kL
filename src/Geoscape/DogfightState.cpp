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

#include "DogfightState.h"

//#include <cstdlib>
//#include <sstream>

#include "GeoscapeState.h"
#include "Globe.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h" // in PCH.
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/ImageButton.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Base.h"
#include "../Savegame/CraftWeaponProjectile.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/AlienStrategy.h"


namespace OpenXcom
{

// UFO blobs graphics ...
const int DogfightState::_ufoBlobs[8][13][13] =
{
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0 - STR_VERY_SMALL
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 3, 5, 3, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 1 - STR_SMALL
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 4, 5, 4, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 2 - STR_MEDIUM_UC
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 5, 5, 5, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 3 - STR_LARGE
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // 4 - STR_VERY_LARGE
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}, // 5 - STR_HUGE
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}
	},
	{
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}, // 6 - STR_VERY_HUGE :p
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}
	},
	{
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}, // 7 - STR_ENOURMOUS
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}
	}
};


// Projectile blobs
const int DogfightState::_projectileBlobs[4][6][3] =
{
	{
		{0, 1, 0}, // 0 - STR_STINGRAY_MISSILE
		{1, 9, 1},
		{1, 4, 1},
		{0, 3, 0},
		{0, 2, 0},
		{0, 1, 0}
	},
	{
		{1, 2, 1}, // 1 - STR_AVALANCHE_MISSILE
		{2, 9, 2},
		{2, 5, 2},
		{1, 3, 1},
		{0, 2, 0},
		{0, 1, 0}
	},
	{
		{0, 0, 0}, // 2 - STR_CANNON_ROUND
		{0, 7, 0},
		{0, 2, 0},
		{0, 1, 0},
		{0, 0, 0},
		{0, 0, 0}
	},
	{
		{2, 4, 2}, // 3 - STR_FUSION_BALL
		{4, 9, 4},
		{2, 4, 2},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}
	}
};


/**
 * Initializes all the elements in the Dogfight window.
 * @param globe	- pointer to the Globe
 * @param craft	- pointer to the Craft intercepting
 * @param ufo	- pointer to the UFO getting intercepted
 * @param geo	- pointer to GeoscapeState
 */
DogfightState::DogfightState(
		Globe* globe,
		Craft* craft,
		Ufo* ufo,
		GeoscapeState* geo)
	:
		_globe(globe),
		_craft(craft),
		_ufo(ufo),
		_geo(geo),
		_savedGame(_game->getSavedGame()),
		_diff(static_cast<int>(_game->getSavedGame()->getDifficulty())),
		_timeout(TIMEOUT),
		_dist(ENGAGE_DIST),
		_targetDist(STANDOFF_DIST),
//		_ufoHitFrame(0),
		_end(false),
		_destroyUfo(false),
		_destroyCraft(false),
		_ufoBreakingOff(false),
		_weapon1Enabled(true),
		_weapon2Enabled(true),
		_minimized(false),
		_endDogfight(false),
		_animatingHit(false),
		_ufoSize(0),
		_craftHeight(0),
		_craftHeight_pre(0),
		_currentCraftDamageColor(0), // 13
		_intercept(0),
		_interceptQty(0),
		_x(0),
		_y(0),
		_minimizedIconX(0),
		_minimizedIconY(0),
		_w1FireCountdown(0),
		_w2FireCountdown(0),
		_w1FireInterval(0),	// -1 (eg, no weapon)
		_w2FireInterval(0)	// -1
{
	_screen = false;

	_craft->setInDogfight(true);

	_window					= new Surface(160, 96, _x, _y);
	_battle					= new Surface(77, 74, _x + 3, _y + 3);
	_weapon1				= new InteractiveSurface(15, 17, _x + 4, _y + 52);
	_range1					= new Surface(21, 74, _x + 19, _y + 3);
	_weapon2				= new InteractiveSurface(15, 17, _x + 64, _y + 52);
	_range2					= new Surface(21, 74, _x + 43, _y + 3);
	_damage					= new Surface(22, 25, _x + 93, _y + 40);

	_btnMinimize			= new InteractiveSurface(12, 12, _x, _y);
	_preview				= new InteractiveSurface(160, 96, _x, _y);

//	_btnUfo					= new ImageButton(36, 17, _x + 120, _y + 52);
	_btnUfo					= new ImageButton(36, 15, _x + 83, _y + 4);
	_btnDisengage			= new ImageButton(36, 15, _x + 83, _y + 20);
/*	_btnStandoff			= new ImageButton(36, 15, _x + 83, _y + 4);
	_btnCautious			= new ImageButton(36, 15, _x + 120, _y + 4);
	_btnStandard			= new ImageButton(36, 15, _x + 83, _y + 20);
	_btnAggressive			= new ImageButton(36, 15, _x + 120, _y + 20);
	_btnDisengage			= new ImageButton(36, 15, _x + 120, _y + 36); */
	_btnCautious			= new ImageButton(36, 15, _x + 120, _y + 4);
	_btnStandard			= new ImageButton(36, 15, _x + 120, _y + 20);
	_btnAggressive			= new ImageButton(36, 15, _x + 120, _y + 36);
	_btnStandoff			= new ImageButton(36, 17, _x + 120, _y + 52);
	_mode = _btnStandoff;

	_texture				= new Surface(9, 9, _x + 147, _y + 72);

	_txtAmmo1				= new Text(16, 9, _x + 4, _y + 70);
	_txtAmmo2				= new Text(16, 9, _x + 64, _y + 70);
	_txtDistance			= new Text(40, 9, _x + 116, _y + 72);
	_txtStatus				= new Text(150, 9, _x + 4, _y + 85);
	_btnMinimizedIcon		= new InteractiveSurface(32, 20, _minimizedIconX, _minimizedIconY);
	_txtInterception		= new Text(16, 9, _minimizedIconX + 18, _minimizedIconY + 6);

	_craftDamageAnimTimer	= new Timer(500);

	setPalette("PAL_GEOSCAPE");

	add(_window);
	add(_battle);
	add(_weapon1);
	add(_range1);
	add(_weapon2);
	add(_range2);
	add(_damage);
	add(_btnMinimize);
	add(_btnUfo,			"button",			"dogfight");
	add(_btnDisengage,		"button",			"dogfight");
	add(_btnCautious,		"button",			"dogfight");
	add(_btnStandard,		"button",			"dogfight");
	add(_btnAggressive,		"button",			"dogfight");
	add(_btnStandoff,		"button",			"dogfight");
	add(_texture);
	add(_txtAmmo1,			"text",				"dogfight");
	add(_txtAmmo2,			"text",				"dogfight");
	add(_txtDistance,		"text",				"dogfight");
	add(_preview);
	add(_txtStatus,			"text",				"dogfight");
	add(_btnMinimizedIcon);
	add(_txtInterception,	"minimizedNumber",	"dogfight");

/*	Surface* graphic;
	graphic = _game->getResourcePack()->getSurface("INTERWIN.DAT");
	graphic->setX(0);
	graphic->setY(0);
	graphic->getCrop()->x = 0;
	graphic->getCrop()->y = 0;
	graphic->getCrop()->w = 160;
	graphic->getCrop()->h = 96;
	_window->drawRect(graphic->getCrop(), 15);
	graphic->blit(_window);

	_preview->drawRect(graphic->getCrop(), 15);
	graphic->getCrop()->y = 96;
	graphic->getCrop()->h = 15;
	graphic->blit(_preview);

	graphic->setY(67);
	graphic->getCrop()->y = 111;
	graphic->getCrop()->h = 29;
	graphic->blit(_preview);

	if (ufo->getRules()->getModSprite().empty())
	{
		graphic->setY(15);
		graphic->getCrop()->y = 140 + 52 * _ufo->getRules()->getSprite();
		graphic->getCrop()->h = 52;
	}
	else
	{
		graphic = _game->getResourcePack()->getSurface(ufo->getRules()->getModSprite());
		graphic->setX(0);
		graphic->setY(15);
	}
	graphic->blit(_preview); */

	// kL_begin:
	Surface* srf = _game->getResourcePack()->getSurface("INTERWIN");
	if (srf != NULL)
		srf->blit(_window);

	srf = _game->getResourcePack()->getSurface("INTERWIN_");
	if (srf != NULL)
		srf->blit(_preview);

	std::ostringstream sprite;
	sprite << "INTERWIN_" << _ufo->getRules()->getSprite();
	srf = _game->getResourcePack()->getSurface(sprite.str());
	if (srf != NULL)
	{
		srf->setY(15);
		srf->blit(_preview);
	} // kL_end.
	_preview->setVisible(false);
	_preview->onMouseClick(
					(ActionHandler)& DogfightState::previewPress,
					SDL_BUTTON_RIGHT);

	_btnMinimize->onMouseClick((ActionHandler)& DogfightState::btnMinimizeClick);

	_btnUfo->copy(_window);
	_btnUfo->onMouseClick((ActionHandler)& DogfightState::btnUfoClick);

	_btnDisengage->copy(_window);
	_btnDisengage->setGroup(&_mode);
	_btnDisengage->onMouseClick((ActionHandler)& DogfightState::btnDisengagePress);

	_btnCautious->copy(_window);
	_btnCautious->setGroup(&_mode);
	_btnCautious->onMouseClick((ActionHandler)& DogfightState::btnCautiousPress);

	_btnStandard->copy(_window);
	_btnStandard->setGroup(&_mode);
	_btnStandard->onMouseClick((ActionHandler)& DogfightState::btnStandardPress);

	_btnAggressive->copy(_window);
	_btnAggressive->setGroup(&_mode);
	_btnAggressive->onMouseClick((ActionHandler)& DogfightState::btnAggressivePress);

	_btnStandoff->copy(_window);
	_btnStandoff->setGroup(&_mode);
	_btnStandoff->onMouseClick((ActionHandler)& DogfightState::btnStandoffPress);

	srf = _game->getResourcePack()->getSurface(getTextureIcon());
	if (srf != NULL)
		srf->blit(_texture);

	_txtDistance->setText(Text::formatNumber(ENGAGE_DIST));

	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_STANDOFF"));

	SurfaceSet* const sstInticon = _game->getResourcePack()->getSurfaceSet("INTICON.PCK");

	// Create the minimized dogfight icon.
	Surface* frame = sstInticon->getFrame(_craft->getRules()->getSprite());
	frame->setX(0);
	frame->setY(0);
	frame->blit(_btnMinimizedIcon);
	_btnMinimizedIcon->onMouseClick((ActionHandler)& DogfightState::btnMinimizedIconClick);
	_btnMinimizedIcon->setVisible(false);

	// Draw correct number on the minimized dogfight icon.
	std::wostringstream ss1;
	ss1 << _craft->getInterceptionOrder();
	_txtInterception->setText(ss1.str());
	_txtInterception->setVisible(false);

	for (size_t
			i = 0;
			i != static_cast<size_t>(_craft->getRules()->getWeapons());
			++i)
	{
		const CraftWeapon* const cw = _craft->getWeapons()->at(i);
		if (cw == NULL)
			continue;

		Surface
			* weapon = NULL,
			* range = NULL;
		Text* ammo = NULL;
		int
			x1,
			x2;

		if (i == 0)
		{
			weapon = _weapon1;
			range = _range1;
			ammo = _txtAmmo1;
			x1 = 2;
			x2 = 0;
		}
		else
		{
			weapon = _weapon2;
			range = _range2;
			ammo = _txtAmmo2;
			x1 = 0;
			x2 = 18;
		}

		frame = sstInticon->getFrame(cw->getRules()->getSprite() + 5);
		frame->setX(0);
		frame->setY(0);
		frame->blit(weapon);

		std::wostringstream ss;
		ss << cw->getAmmo();
		ammo->setText(ss.str());

		const Uint8 color = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("background")->color);

		range->lock();
		const int
			rangeY = range->getHeight() - cw->getRules()->getRange(), // 1 km = 1 pixel
			connectY = 57;

		for (int
				x = x1;
				x <= x1 + 18;
				x += 2)
		{
			range->setPixelColor(
								x,
								rangeY,
								color);
		}

		int
			minY = 0,
			maxY = 0;

		if (rangeY < connectY)
		{
			minY = rangeY;
			maxY = connectY;
		}
		else if (rangeY > connectY)
		{
			minY = connectY;
			maxY = rangeY;
		}

		for (int
				y = minY;
				y <= maxY;
				++y)
		{
			range->setPixelColor(
								x1 + x2,
								y,
								color);
		}

		for (int
				x = x2;
				x <= x2 + 2;
				++x)
		{
			range->setPixelColor(
								x,
								connectY,
								color);
		}
		range->unlock();
	}

	if (!
		(_craft->getRules()->getWeapons() > 0
			&& _craft->getWeapons()->at(0) != NULL))
	{
		_weapon1->setVisible(false);
		_range1->setVisible(false);
		_txtAmmo1->setVisible(false);
	}

	if (!
		(_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL))
	{
		_weapon2->setVisible(false);
		_range2->setVisible(false);
		_txtAmmo2->setVisible(false);
	}

	// Draw damage indicator.
	frame = sstInticon->getFrame(_craft->getRules()->getSprite() + 11);
	frame->setX(0);
	frame->setY(0);
	frame->blit(_damage);

	_craftDamageAnimTimer->onTimer((StateHandler)& DogfightState::animateCraftDamage);

	if (_ufo->getEscapeCountdown() == 0) // UFO is *not* already engaged in a different dogfight/Intercept slot.
	{
		_ufo->setFireCountdown(0); // UFO is ready to Fire.

		int escape = _ufo->getRules()->getBreakOffTime();
		escape += RNG::generate(
							0,
							escape);
//							escape / 10);
		escape -= _diff * 30;
//		escape -= _diff * 10;

		if (escape < 50) escape = 50;

		_ufo->setEscapeCountdown(escape);
	}

	if (_craft->getRules()->getWeapons() > 0
		&& _craft->getWeapons()->at(0) != NULL)
	{
		_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getStandardReload();
	}

	if (_craft->getRules()->getWeapons() > 1
		&& _craft->getWeapons()->at(1) != NULL)
	{
		_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getStandardReload();
	}

	// Set UFO size - going to be moved to Ufo class to implement simultaneous dogfights.
	const std::string ufoSize = _ufo->getRules()->getSize();
	if (ufoSize.compare("STR_VERY_SMALL") == 0)
		_ufoSize = 0;
	else if (ufoSize.compare("STR_SMALL") == 0)
		_ufoSize = 1;
	else if (ufoSize.compare("STR_MEDIUM_UC") == 0)
		_ufoSize = 2;
	else if (ufoSize.compare("STR_LARGE") == 0)
		_ufoSize = 3;
	else // very large
		_ufoSize = 4;

	_color[0] = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("craftRange")->color);
	_color[1] = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("craftRange")->color2);
	_color[2] = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("radarRange")->color);
	_color[3] = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("radarRange")->color2);
	_color[4] = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("damageRange")->color);
	_color[5] = static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("damageRange")->color2);

	// Get the craft-graphic's height. Used for damage indication.
	for (int
			y = 0;
			y < _damage->getHeight();
			++y)
	{
		const Uint8 pixelColor = _damage->getPixelColor(11, y);
		const bool isCraftColor = pixelColor > _color[0] - 1
							   && pixelColor < _color[1];

		if (_craftHeight != 0
			&& isCraftColor == false)
		{
			break;
		}
		else if (isCraftColor == true)
			++_craftHeight;
		else
			++_craftHeight_pre;
	}

	drawCraftDamage();

	_weapon1->onMouseClick((ActionHandler)& DogfightState::weapon1Click);
	_weapon2->onMouseClick((ActionHandler)& DogfightState::weapon2Click);
}

/**
 * Cleans up this DogfightState.
 */
DogfightState::~DogfightState()
{
	delete _craftDamageAnimTimer;

	while (_projectiles.empty() == false)
	{
		delete _projectiles.back();
		_projectiles.pop_back();
	}

	if (_craft != NULL)
		_craft->setInDogfight(false);

	// set the ufo as "free" for the next engagement (as applicable)
	if (_ufo != NULL)
		_ufo->setInterceptionProcessed(false);
}

/**
 * Runs the higher level dogfight functionality
 */
void DogfightState::think()
{
	if (_endDogfight == false)
	{
		updateDogfight();
		_craftDamageAnimTimer->think(this, NULL);
	}

	if (_craft->getDestination() != _ufo
		|| _ufo->getStatus() == Ufo::LANDED)
	{
		endDogfight();
	}
}

/**
 * Animates interceptor damage by changing the color and redrawing the image.
 */
void DogfightState::animateCraftDamage()
{
	if (_minimized == true)
		return;

	--_currentCraftDamageColor;

	if (_currentCraftDamageColor < _color[4] - 1) // yellow
		_currentCraftDamageColor = _color[5] + 1; // black

	drawCraftDamage();
}

/**
 * Draws interceptor damage according to percentage of HPs left.
 */
void DogfightState::drawCraftDamage()
{
	const int damagePercent = _craft->getDamagePercent();
	if (damagePercent > 0)
	{
		if (_craftDamageAnimTimer->isRunning() == false)
		{
			_craftDamageAnimTimer->start();
			_currentCraftDamageColor = _color[4]; // red
		}

		int rowsToColor = static_cast<int>(std::floor(
						  static_cast<double>(_craftHeight * damagePercent) / 100.));

		if (rowsToColor == 0)
			return;
		else if (damagePercent > 99)
			++rowsToColor;

		for (int
				y = _craftHeight_pre;
				y < _craftHeight_pre + rowsToColor;
				++y)
		{
			for (int
					x = 1;
					x < 23;
					++x)
			{
				const Uint8 pixelColor = _damage->getPixelColor(x,y);

				if ((pixelColor > _color[4] - 1
						&& pixelColor < _color[5] + 2)
					|| (pixelColor > _color[0] - 1
						&& pixelColor < _color[1]))
				{
					_damage->setPixelColor(
										x,y,
										_currentCraftDamageColor);
				}
			}
		}
	}
}

/**
 * Animates the window with a palette effect.
 */
void DogfightState::animate()
{
	for (int // Animate radar waves and other stuff.
			x = 0;
			x < _window->getWidth();
			++x)
	{
		for (int
				y = 0;
				y < _window->getHeight();
				++y)
		{
			Uint8 radarPixelColor = _window->getPixelColor(x, y);
			if (radarPixelColor >= Palette::blockOffset(7)		// _color[2]
				&& radarPixelColor < Palette::blockOffset(8))	// _color[3]
			{
				++radarPixelColor;

				if (radarPixelColor >= Palette::blockOffset(8))	// _color[3]
					radarPixelColor = Palette::blockOffset(7);	// _color[2]

				_window->setPixelColor(
									x,
									y,
									radarPixelColor);
			}
		}
	}

	_battle->clear();

	if (_ufo->isDestroyed() == false)
		drawUfo();

	for (std::vector<CraftWeaponProjectile*>::const_iterator
			i = _projectiles.begin();
			i != _projectiles.end();
			++i)
	{
		drawProjectile(*i);
	}

	if (_timeout == 0) // clears text after a while
		_txtStatus->setText(L"");
	else
		--_timeout;

	bool lastHitAnimFrame = false;
	if (_animatingHit == true // animate UFO hit
		&& _ufo->getHitFrame() > 0)
	{
		_ufo->setHitFrame(_ufo->getHitFrame() - 1);

		if (_ufo->getHitFrame() == 0)
		{
			_animatingHit = false;
			lastHitAnimFrame = true;
		}
	}

	if (_ufo->isCrashed() == true // animate UFO crash landing
		&& _ufo->getHitFrame() == 0
		&& lastHitAnimFrame == false)
	{
		--_ufoSize;
	}
}

/**
 * Updates all the elements in the dogfight including ufo movement, weapons fire,
 * projectile movement, ufo escape conditions, craft and ufo destruction conditions,
 * and retaliation mission generation as applicable.
 */
void DogfightState::updateDogfight()
{
	bool finalRun = false;

	const Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	if (ufo != _ufo
		|| _craft->getLowFuel() == true
		|| (_minimized == true
			&& _ufo->isCrashed() == true))
	{
		endDogfight();	// Check if craft is not low on fuel when window minimized, and
		return;			// that craft's destination hasn't been changed when window minimized.
	}

	if (_minimized == false)
	{
		animate();

		int escapeTicks = _ufo->getEscapeCountdown();
		if (_ufo->isCrashed() == false
//			&& _ufo->isDestroyed() == false
			&& _craft->isDestroyed() == false)
		{
			if (escapeTicks > 0
				&& _ufo->getInterceptionProcessed() == false)
			{
				_ufo->setEscapeCountdown(--escapeTicks);
				_ufo->setInterceptionProcessed(true);

				if (_ufo->getFireCountdown() > 0)
					_ufo->setFireCountdown(_ufo->getFireCountdown() - 1);
			}

			if (escapeTicks == 0) // Check if UFO is breaking off.
			{
//				_ufo->setFireCountdown(0); // kL <- let cTor handle this
				_ufo->setSpeed(_ufo->getRules()->getMaxSpeed());
			}
		}
	}

	if (_ufo->getSpeed() > _craft->getRules()->getMaxSpeed()) // Crappy craft is chasing UFO.
	{
		_ufoBreakingOff = true;
		finalRun = true;
		setStatus("STR_UFO_OUTRUNNING_INTERCEPTOR");
	}
	else // UFO cannot break off, because it's crappier than the crappy craft.
	{
		_ufoBreakingOff = false;
		_craft->setSpeed(_ufo->getSpeed());
	}

	bool projectileInFlight = false;
	if (_minimized == false)
	{
		int distanceChange = 0;

		if (_ufoBreakingOff == false) // Update distance.
		{
			if (_dist < _targetDist
				&& _ufo->isCrashed() == false
				&& _craft->isDestroyed() == false)
			{
				distanceChange = 4;

				if (_dist + distanceChange > _targetDist)
					distanceChange = _targetDist - _dist;
			}
			else if (_dist > _targetDist
				&& _ufo->isCrashed() == false
				&& _craft->isDestroyed() == false)
			{
				distanceChange = -2;
			}

			// Don't let the interceptor mystically push or pull its fired projectiles.
			for (std::vector<CraftWeaponProjectile*>::const_iterator
					i = _projectiles.begin();
					i != _projectiles.end();
					++i)
			{
				if ((*i)->getGlobalType() != CWPGT_BEAM
					&& (*i)->getDirection() == D_UP)
				{
					(*i)->setPosition((*i)->getPosition() + distanceChange);
				}
			}
		}
		else
		{
			distanceChange = 4;
			// UFOs can try to outrun the missiles, don't adjust projectile positions here.
			// If UFOs ever fire anything but beams, those positions need to be adjusted here though.
		}

		_dist += distanceChange;

		std::wostringstream ss;
		ss << _dist;
		_txtDistance->setText(ss.str());

		for (std::vector<CraftWeaponProjectile*>::const_iterator // Move projectiles and check for hits.
				i = _projectiles.begin();
				i != _projectiles.end();
				++i)
		{
			CraftWeaponProjectile* const proj = *i;
			proj->moveProjectile();

			if (proj->getDirection() == D_UP) // Projectiles fired by interceptor.
			{
				if ((proj->getPosition() >= _dist // Projectile reached the UFO - determine if it's been hit.
						|| (proj->getGlobalType() == CWPGT_BEAM
							&& proj->toBeRemoved() == true))
					&& _ufo->isCrashed() == false
					&& proj->getMissed() == false)
				{
//					int ufoSize = _ufoSize;
//					if (ufoSize > 4)
//						ufoSize = 4;

					const int hitprob = proj->getAccuracy() + (_ufoSize * 5) - (_diff * 5); // Could include UFO speed here ...

					if (RNG::percent(hitprob) == true)
					{
						const int damage = RNG::generate(
													(proj->getDamage() + 1) / 2, // Round up.
													proj->getDamage());
						_ufo->setDamage(_ufo->getDamage() + damage);

						if (_ufo->isCrashed() == true)
						{
							_ufo->setShotDownByCraftId(_craft->getUniqueId());
							_ufoBreakingOff = false;
							_ufo->setSpeed(0);
						}

						if (_ufo->getHitFrame() == 0)
						{
							_animatingHit = true;
							_ufo->setHitFrame(3);
						}

						setStatus("STR_UFO_HIT");
						proj->removeProjectile();

						_game->getResourcePack()->playSoundFX(
														ResourcePack::UFO_HIT,
														true);
					}
					else // Missed.
					{
						if (proj->getGlobalType() == CWPGT_BEAM)
							proj->removeProjectile();
						else
							proj->setMissed(true);
					}
				}

				if (proj->getGlobalType() == CWPGT_MISSILE) // Check if projectile passed its maximum range.
				{
					if ((proj->getPosition() / 8) >= proj->getRange())
						proj->removeProjectile();
					else if (_ufo->isCrashed() == false)
						projectileInFlight = true;
				}
			}
			else if (proj->getDirection() == D_DOWN) // Projectiles fired by UFO.
			{
				if (proj->getGlobalType() == CWPGT_MISSILE
					|| (proj->getGlobalType() == CWPGT_BEAM
						&& proj->toBeRemoved() == true))
				{
					if (RNG::percent(proj->getAccuracy()) == true)
					{
						const int damage = RNG::generate(
													(_ufo->getRules()->getWeaponPower() + 9) / 10, // Round up.
													_ufo->getRules()->getWeaponPower());
						if (damage != 0)
						{
							_craft->setDamage(_craft->getDamage() + damage);

							drawCraftDamage();
							setStatus("STR_INTERCEPTOR_DAMAGED");
							_game->getResourcePack()->playSoundFX(
															ResourcePack::INTERCEPTOR_HIT,
															true);

							if ((_mode == _btnCautious
									&& _craft->getDamagePercent() > 60)
								|| (_mode == _btnStandard
									&& _craft->getDamagePercent() > 35))
							{
								if (_ufo->isCrashed() == false
									&& _craft->isDestroyed() == false
									&& _ufoBreakingOff == false)
								{
									_btnStandoff->releaseDogfight(); // kL_add

									_end = false;
									setStatus("STR_STANDOFF");
									_targetDist = STANDOFF_DIST;
								}
							}
						}
					}

					proj->removeProjectile();
				}
			}
		}

		for (std::vector<CraftWeaponProjectile*>::const_iterator // Remove projectiles that hit or missed their target.
				i = _projectiles.begin();
				i != _projectiles.end();
				)
		{
			if ((*i)->toBeRemoved() == true
				|| ((*i)->getMissed() == true
					&& (*i)->getPosition() < 1))
			{
				delete *i;
				i = _projectiles.erase(i);
			}
			else
				++i;
		}

		for (size_t // Handle weapons and craft distance.
				i = 0;
				i != static_cast<size_t>(_craft->getRules()->getWeapons());
				++i)
		{
			const CraftWeapon* const cw = _craft->getWeapons()->at(i);
			if (cw == NULL)
				continue;

			int wTimer;
			if (i == 0)
				wTimer = _w1FireCountdown;
			else
				wTimer = _w2FireCountdown;

			if (wTimer == 0 // Handle weapon firing.
				&& _dist <= cw->getRules()->getRange() * 8
				&& cw->getAmmo() > 0
				&& _mode != _btnStandoff
				&& _mode != _btnDisengage
				&& _ufo->isCrashed() == false
				&& _craft->isDestroyed() == false)
			{
				if (i == 0)
					fireWeapon1();
				else
					fireWeapon2();
			}
			else if (wTimer > 0)
			{
				if (i == 0)
					--_w1FireCountdown;
				else
					--_w2FireCountdown;
			}

			if (cw->getAmmo() == 0 // Handle craft distance according to option set by user and available ammo.
				&& projectileInFlight == false
				&& _craft->isDestroyed() == false)
			{
				if (_mode == _btnCautious)
					minimumDistance();
				else if (_mode == _btnStandard)
					maximumDistance();
			}
		}

		// Handle UFO firing.
/*		if (_dist <= _ufo->getRules()->getWeaponRange() * 8
			&& _ufo->isCrashed() == false
			&& _craft->isDestroyed() == false)
		{
			if (_ufo->getShootingAt() == 0)
				_ufo->setShootingAt(_intercept);

			if (_ufo->getShootingAt() == _intercept
				&& _ufo->getFireCountdown() == 0)
			{
				ufoFireWeapon();
			}
		}
		else if (_ufo->getShootingAt() == _intercept)
			_ufo->setShootingAt(0); */
		const int ufoWRange = _ufo->getRules()->getWeaponRange() * 8;

		if (_ufo->isCrashed() == true
			|| (_ufo->getShootingAt() == _intercept // this Craft out of range and/or destroyed.
				&& (_dist > ufoWRange
					|| _craft->isDestroyed() == true)))
		{
			_ufo->setShootingAt(0);
		}
		else if (_ufo->getFireCountdown() == 0 // UFO is gtg.
			&& (_ufo->getShootingAt() == 0
				|| _ufo->getShootingAt() == _intercept)
			&& _ufo->isCrashed() == false)
		{
			if (_dist <= ufoWRange
				 && _craft->isDestroyed() == false)
			{
				std::vector<int> altIntercepts; // Randomize UFO's target.
				for (std::list<DogfightState*>::const_iterator
						i = _geo->getDogfights().begin();
						i != _geo->getDogfights().end();
						++i)
				{
					if ((*i)->isMinimized() == false
						&& (*i)->getDistance() <= ufoWRange
						&& (*i)->getCraft()->isDestroyed() == false
						&& (*i)->getUfo() == _ufo)
					{
						altIntercepts.push_back((*i)->getInterceptSlot());
					}
				}

				if (altIntercepts.size() == 1) // this->craft.
				{
					_ufo->setShootingAt(_intercept);
					ufoFireWeapon();
				}
				else if (altIntercepts.size() > 1) // [ ==0 should NEVER happen.]
				{
					int shotCraft = static_cast<int>(Round(100. / static_cast<double>(altIntercepts.size())));

					if (_ufo->getShootingAt() == _intercept)
						shotCraft += 18;

					if (RNG::percent(shotCraft) == true)
					{
						_ufo->setShootingAt(_intercept);
						ufoFireWeapon();
					}
					else // This is where the magic happens, Lulzor!!
					{
						const size_t targetIntercept = static_cast<size_t>(RNG::generate(
																					0,
																					altIntercepts.size() - 1));
						for (std::list<DogfightState*>::const_iterator
								i = _geo->getDogfights().begin();
								i != _geo->getDogfights().end();
								++i)
						{
							const size_t acquiredTarget = altIntercepts.at(targetIntercept);
							if ((*i)->getInterceptSlot() == acquiredTarget)
							{
								_ufo->setShootingAt(acquiredTarget);
								break;
							}
						}
					}
				}
				else
					_ufo->setShootingAt(0); // safety.
			}
			else
				_ufo->setShootingAt(0);
		}
	}

	if (_end == true // Check when battle is over.
		&& (((_dist > ENGAGE_DIST
					|| _minimized == true)
				&& (_mode == _btnDisengage
					|| _ufoBreakingOff == true))
			|| (_timeout == 0
				&& (_ufo->isCrashed() == true
					|| _craft->isDestroyed() == true))))
	{
		if (_ufoBreakingOff == true)
		{
			_ufo->moveTarget();
			_craft->setDestination(_ufo);
		}

		if (_destroyCraft == false
			&& (_destroyUfo == true
				|| _mode == _btnDisengage))
		{
			_craft->returnToBase();
		}

		endDogfight();
	}

	if (_dist > ENGAGE_DIST
		&& _ufoBreakingOff == true)
	{
		finalRun = true;
	}

	if (_end == false)
	{
		if (_craft->isDestroyed() == true) // End dogfight if craft is destroyed.
		{
			setStatus("STR_INTERCEPTOR_DESTROYED");

			_timeout += 30;
			_game->getResourcePack()->playSoundFX(ResourcePack::INTERCEPTOR_EXPLODE);

			finalRun = true;
			_destroyCraft = true;

			_ufo->setShootingAt(0);
		}

		if (_ufo->isCrashed() == true) // End dogfight if UFO is crashed or destroyed.
		{
			_ufo->getMission()->ufoShotDown(*_ufo);

			if (RNG::percent(4 + (_diff * 4)) == true)				// Check retaliation trigger.
			{
				std::string targetRegion;							// Spawn retaliation mission.
				if (RNG::percent(50 - (_diff * 6)) == true)
					targetRegion = _ufo->getMission()->getRegion();	// Attack on UFO's mission region.
				else												// Try to find and attack the originating base.
					targetRegion = _savedGame->locateRegion(*_craft->getBase())->getRules()->getType();
																	// TODO: If the base is removed, the mission is cancelled.

				// Difference from original: No retaliation until final UFO lands (Original: Is spawned).
				if (_game->getSavedGame()->findAlienMission(
														targetRegion,
														OBJECTIVE_RETALIATION) == false)
				{
					const RuleAlienMission& rule = *_game->getRuleset()->getAlienMission("STR_ALIEN_RETALIATION");
					AlienMission* const mission = new AlienMission(
																rule,
																*_savedGame);

					mission->setId(_savedGame->getId("ALIEN_MISSIONS"));
					mission->setRegion(
									targetRegion,
									*_game->getRuleset());
					mission->setRace(_ufo->getAlienRace());
					mission->start();

					_savedGame->getAlienMissions().push_back(mission);
				}
			}

			const double
				ufoLon = _ufo->getLongitude(),
				ufoLat = _ufo->getLatitude();

			if (_ufo->isDestroyed() == true)
			{
				if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
				{
					setStatus("STR_UFO_DESTROYED");
					_game->getResourcePack()->playSoundFX(ResourcePack::UFO_EXPLODE);

					for (std::vector<Region*>::const_iterator
							i = _savedGame->getRegions()->begin();
							i != _savedGame->getRegions()->end();
							++i)
					{
						if ((*i)->getRules()->insideRegion(
														ufoLon,
														ufoLat) == true)
						{
							(*i)->addActivityXcom(_ufo->getRules()->getScore() * 2);
							(*i)->recentActivityXCOM();

							break;
						}
					}

					for (std::vector<Country*>::const_iterator
							i = _savedGame->getCountries()->begin();
							i != _savedGame->getCountries()->end();
							++i)
					{
						if ((*i)->getRules()->insideCountry(
														ufoLon,
														ufoLat) == true)
						{
							(*i)->addActivityXcom(_ufo->getRules()->getScore() * 2);
							(*i)->recentActivityXCOM();

							break;
						}
					}
				}

				_destroyUfo = true;
			}
			else
			{
				if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
				{
					setStatus("STR_UFO_CRASH_LANDS");
					_game->getResourcePack()->playSoundFX(ResourcePack::UFO_CRASH);

					for (std::vector<Region*>::const_iterator
							i = _savedGame->getRegions()->begin();
							i != _savedGame->getRegions()->end();
							++i)
					{
						if ((*i)->getRules()->insideRegion(
														ufoLon,
														ufoLat) == true)
						{
							(*i)->addActivityXcom(_ufo->getRules()->getScore());
							(*i)->recentActivityXCOM();

							break;
						}
					}

					for (std::vector<Country*>::const_iterator
							i = _savedGame->getCountries()->begin();
							i != _savedGame->getCountries()->end();
							++i)
					{
						if ((*i)->getRules()->insideCountry(
														ufoLon,
														ufoLat) == true)
						{
							(*i)->addActivityXcom(_ufo->getRules()->getScore());
							(*i)->recentActivityXCOM();

							break;
						}
					}
				}

				if (_globe->insideLand(
									ufoLon,
									ufoLat) == false)
				{
					_ufo->setStatus(Ufo::DESTROYED);
					_destroyUfo = true;
				}
				else // Set up Crash site.
				{
					_ufo->setSecondsLeft(RNG::generate(24, 96) * 3600);
					_ufo->setAltitude("STR_GROUND");

					if (_ufo->getCrashId() == 0)
						_ufo->setCrashId(_savedGame->getId("STR_CRASH_SITE"));

					_ufo->setCrashPower(_ufo->getDamagePercent()); // kL
				}
			}

			_timeout += 30;
			if (_ufo->getShotDownByCraftId() != _craft->getUniqueId())
			{
				_timeout += 50;
				_ufo->setHitFrame(3);
			}

			finalRun = true;
//			_ufoBreakingOff = false;
//			_ufo->setSpeed(0);
		}

		if (_ufo->getStatus() == Ufo::LANDED)
		{
			_timeout += 30;
			finalRun = true;
			_ufo->setShootingAt(0);
		}
	}

	if (projectileInFlight == false
		&& finalRun == true)
	{
		_end = true;
	}
}

/**
 * Fires a shot from the first weapon equipped on the Craft.
 */
void DogfightState::fireWeapon1()
{
	if (_weapon1Enabled == true)
	{
		CraftWeapon* const cw1 = _craft->getWeapons()->at(0);
		if (cw1->setAmmo(cw1->getAmmo() - 1))
		{
			_w1FireCountdown = _w1FireInterval;

			std::wostringstream wosts;
			wosts << cw1->getAmmo();
			_txtAmmo1->setText(wosts.str());

			CraftWeaponProjectile* const proj = cw1->fire();
			proj->setDirection(D_UP);
			proj->setHorizontalPosition(HP_LEFT);
			_projectiles.push_back(proj);

			_game->getResourcePack()->playSoundFX(
											cw1->getRules()->getSound(),
											true);
		}
	}
}

/**
 * Fires a shot from the second weapon equipped on the Craft.
 */
void DogfightState::fireWeapon2()
{
	if (_weapon2Enabled == true)
	{
		CraftWeapon* const cw2 = _craft->getWeapons()->at(1);
		if (cw2->setAmmo(cw2->getAmmo() - 1))
		{
			_w2FireCountdown = _w2FireInterval;

			std::wostringstream wosts;
			wosts << cw2->getAmmo();
			_txtAmmo2->setText(wosts.str());

			CraftWeaponProjectile* const proj = cw2->fire();
			proj->setDirection(D_UP);
			proj->setHorizontalPosition(HP_RIGHT);
			_projectiles.push_back(proj);

			_game->getResourcePack()->playSoundFX(
											cw2->getRules()->getSound(),
											true);
		}
	}
}

/**
 * Each time a UFO fires its cannons a new reload interval is calculated.
 */
void DogfightState::ufoFireWeapon()
{
	setStatus("STR_UFO_RETURN_FIRE");

	CraftWeaponProjectile* const proj = new CraftWeaponProjectile();
	proj->setType(CWPT_PLASMA_BEAM);
	proj->setAccuracy(60);
	proj->setDamage(_ufo->getRules()->getWeaponPower());
	proj->setDirection(D_DOWN);
	proj->setHorizontalPosition(HP_CENTER);
	proj->setPosition(_dist - (_ufo->getRules()->getRadius() / 2));
	_projectiles.push_back(proj);

	_game->getResourcePack()->playSoundFX(
									ResourcePack::UFO_FIRE,
									true);


	int reload = _ufo->getRules()->getWeaponReload();
/*	reload -= _diff * 2;
	reload += RNG::generate(
						0,
						reload); */
	reload += RNG::generate(
						0,
						reload / 2);
	reload -= _diff * 2;

	if (reload < 10) reload = 10;

	_ufo->setFireCountdown(reload);
}

/**
 * Sets the craft to the minimum distance required to fire a weapon.
 */
void DogfightState::minimumDistance()
{
	int dist = 0;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _craft->getWeapons()->begin();
			i < _craft->getWeapons()->end();
			++i)
	{
		if (*i == NULL)
			continue;

		if ((*i)->getRules()->getRange() > dist
			&& (*i)->getAmmo() > 0)
		{
			dist = (*i)->getRules()->getRange();
		}
	}

	if (dist == 0)
		_targetDist = STANDOFF_DIST;
	else
		_targetDist = dist * 8;
}

/**
 * Sets the craft to the maximum distance required to fire a weapon.
 */
void DogfightState::maximumDistance()
{
	int dist = 1000;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _craft->getWeapons()->begin();
			i < _craft->getWeapons()->end();
			++i)
	{
		if (*i == NULL)
			continue;

		if ((*i)->getRules()->getRange() < dist
			&& (*i)->getAmmo() > 0)
		{
			dist = (*i)->getRules()->getRange();
		}
	}

	if (dist == 1000)
		_targetDist = STANDOFF_DIST;
	else
		_targetDist = dist * 8;
}

/**
 * Updates the status text and restarts the text timeout counter.
 * @param status - address of status
 */
void DogfightState::setStatus(const std::string& status)
{
	_txtStatus->setText(tr(status));
	_timeout = TIMEOUT;
}

/**
 * Minimizes the dogfight window.
 * @param action - pointer to an Action
 */
void DogfightState::btnMinimizeClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		if (_dist >= STANDOFF_DIST)
		{
			setMinimized(true);

			_window->setVisible(false);
			_preview->setVisible(false);
			_btnStandoff->setVisible(false);
			_btnCautious->setVisible(false);
			_btnStandard->setVisible(false);
			_btnAggressive->setVisible(false);
			_btnDisengage->setVisible(false);
			_btnUfo->setVisible(false);
			_texture->setVisible(false);
			_btnMinimize->setVisible(false);
			_battle->setVisible(false);
			_weapon1->setVisible(false);
			_range1->setVisible(false);
			_weapon2->setVisible(false);
			_range2->setVisible(false);
			_damage->setVisible(false);
			_txtAmmo1->setVisible(false);
			_txtAmmo2->setVisible(false);
			_txtDistance->setVisible(false);
			_preview->setVisible(false);
			_txtStatus->setVisible(false);

			_btnMinimizedIcon->setVisible();
			_txtInterception->setVisible();

			// if no interception windows still open
			// zoom straight out to previous zoomlevel
			// and reset dfZoom + dfCenter
			// note: There's also something about GeoscapeState::_dfZoomIn/OutDone ...
			if (_interceptQty == _geo->minimizedDogfightsCount()
				&& _geo->getDogfightZoomOutTimer()->isRunning() == false)
			{
				if (_geo->getDogfightZoomInTimer()->isRunning() == true)
					_geo->getDogfightZoomInTimer()->stop();

				_globe->setChasingUfo();
				_geo->getDogfightZoomOutTimer()->start();
			}
		}
		else
			setStatus("STR_MINIMISE_AT_STANDOFF_RANGE_ONLY");
	}
}

/**
 * Switches to Standoff mode (maximum range).
 * @param action - pointer to an Action
 */
void DogfightState::btnStandoffPress(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		setStatus("STR_STANDOFF");
		_targetDist = STANDOFF_DIST;
	}
}

/**
 * Switches to Cautious mode (maximum weapon range).
 * @param action - pointer to an Action
 */
void DogfightState::btnCautiousPress(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		setStatus("STR_CAUTIOUS_ATTACK");

		if (_craft->getRules()->getWeapons() > 0
			&& _craft->getWeapons()->at(0) != NULL)
		{
			_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getCautiousReload();
//			_w1Timer->setInterval(static_cast<Uint32>(_craft->getWeapons()->at(0)->getRules()->getCautiousReload()) * _optionSpeed);
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getCautiousReload();
//			_w2Timer->setInterval(static_cast<Uint32>(_craft->getWeapons()->at(1)->getRules()->getCautiousReload()) * _optionSpeed);
		}

		minimumDistance();
	}
}

/**
 * Switches to Standard mode (minimum weapon range).
 * @param action - pointer to an Action
 */
void DogfightState::btnStandardPress(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		setStatus("STR_STANDARD_ATTACK");

		if (_craft->getRules()->getWeapons() > 0
			&& _craft->getWeapons()->at(0) != NULL)
		{
			_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getStandardReload();
//			_w1Timer->setInterval(static_cast<Uint32>(_craft->getWeapons()->at(0)->getRules()->getStandardReload()) * _optionSpeed);
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getStandardReload();
//			_w2Timer->setInterval(static_cast<Uint32>(_craft->getWeapons()->at(1)->getRules()->getStandardReload()) * _optionSpeed);
		}

		maximumDistance();
	}
}

/**
 * Switches to Aggressive mode (minimum range).
 * @param action - pointer to an Action
 */
void DogfightState::btnAggressivePress(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		setStatus("STR_AGGRESSIVE_ATTACK");

		if (_craft->getRules()->getWeapons() > 0
			&& _craft->getWeapons()->at(0) != NULL)
		{
			_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getAggressiveReload();
//			_w1Timer->setInterval(static_cast<Uint32>(_craft->getWeapons()->at(0)->getRules()->getAggressiveReload()) * _optionSpeed);
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getAggressiveReload();
//			_w2Timer->setInterval(static_cast<Uint32>(_craft->getWeapons()->at(1)->getRules()->getAggressiveReload()) * _optionSpeed);
		}

		_targetDist = 64;
	}
}

/**
 * Disengages from the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::btnDisengagePress(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = true;
		setStatus("STR_DISENGAGING");

		_targetDist = 800;
	}
}

/**
 * Shows a front view of the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::btnUfoClick(Action*)
{
	_preview->setVisible();

	// Disable all other buttons to prevent misclicks
	_btnStandoff->setVisible(false);
	_btnCautious->setVisible(false);
	_btnStandard->setVisible(false);
	_btnAggressive->setVisible(false);
	_btnDisengage->setVisible(false);
	_btnUfo->setVisible(false);
	_texture->setVisible(false);
	_btnMinimize->setVisible(false);
	_weapon1->setVisible(false);
	_weapon2->setVisible(false);
}

/**
 * Hides the front view of the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::previewPress(Action*)
{
	_preview->setVisible(false);

	// Reenable all other buttons to prevent misclicks Lol
	_btnStandoff->setVisible();
	_btnCautious->setVisible();
	_btnStandard->setVisible();
	_btnAggressive->setVisible();
	_btnDisengage->setVisible();
	_btnUfo->setVisible();
	_texture->setVisible();
	_btnMinimize->setVisible();
	_weapon1->setVisible();
	_weapon2->setVisible();
}

/**
 * Draws the UFO blob on the radar screen.
 * Currently works only for original sized blobs 13 x 13 pixels.
 */
void DogfightState::drawUfo()
{
	if (_ufoSize < 0
		|| _ufo->isDestroyed() == true)
	{
		return;
	}

	const int
		curUfoXpos = _battle->getWidth() / 2 - 6,
		curUfoYpos = _battle->getHeight() - (_dist / 8) - 6;

	for (int
			y = 0;
			y < 13;
			++y)
	{
		for (int
				x = 0;
				x < 13;
				++x)
		{
			Uint8 pixelOffset = static_cast<Uint8>(_ufoBlobs[_ufoSize + _ufo->getHitFrame()][y][x]);
			if (pixelOffset == 0)
				continue;
			else
			{
				if (_ufo->isCrashed() == true
					|| _ufo->getHitFrame() > 0)
				{
					pixelOffset *= 2;
				}

				const Uint8 radarPixelColor = _window->getPixelColor(
																curUfoXpos + x + 3,
																curUfoYpos + y + 3); // +3 'cause of the window frame
				Uint8 color = radarPixelColor - pixelOffset;
				if (color < 108) color = 108;

				_battle->setPixelColor(
									curUfoXpos + x,
									curUfoYpos + y,
									color);
			}
		}
	}
}

/**
 * Draws projectiles on the radar screen.
 * Depending on what type of projectile it is, its shape will be different.
 * Currently works for original sized blobs 3 x 6 pixels.
 * @param proj - pointer to CraftWeaponProjectile
 */
void DogfightState::drawProjectile(const CraftWeaponProjectile* proj)
{
	int posX = _battle->getWidth() / 2 + proj->getHorizontalPosition();
	Uint8
		color,
		offset;

	if (proj->getGlobalType() == CWPGT_MISSILE) // Draw missiles.
	{
		--posX;
		const int posY = _battle->getHeight() - proj->getPosition() / 8;
		for (int
				x = 0;
				x != 3;
				++x)
		{
			for (int
					y = 0;
					y != 6;
					++y)
			{
				offset = static_cast<Uint8>(_projectileBlobs[proj->getType()]
															[static_cast<size_t>(y)]
															[static_cast<size_t>(x)]);
				if (offset != 0)
				{
					color = _window->getPixelColor(
												posX + x + 3,
												posY + y + 3); // + 3 cause of the window frame
					color -= offset;
					if (color < 108) color = 108;

					_battle->setPixelColor(
										posX + x,
										posY + y,
										color);
				}
			}
		}
	}
	else if (proj->getGlobalType() == CWPGT_BEAM) // Draw beams.
	{
		const int
			stop = _battle->getHeight() - 2,
			start = _battle->getHeight() - (_dist / 8);
		offset = static_cast<Uint8>(proj->getState());

		for (int
				y = stop;
				y != start;
				--y)
		{
			color = _window->getPixelColor(
										posX + 3,
										y + 3);
			color -= offset;
			if (color < 108) color = 108;

			_battle->setPixelColor(
								posX,
								y,
								color);
		}
	}
}

/**
 * Toggles usage of weapon number 1.
 * @param action - pointer to an Action
 */
void DogfightState::weapon1Click(Action*)
{
	_weapon1Enabled = !_weapon1Enabled;
	recolor(0, _weapon1Enabled);
}

/**
 * Toggles usage of weapon number 2.
 * @param action - pointer to an Action
 */
void DogfightState::weapon2Click(Action*)
{
	_weapon2Enabled = !_weapon2Enabled;
	recolor(1, _weapon2Enabled);
}

/**
 * Changes the colors of craft's weapon icons, range indicators,
 * and ammo texts base on current weapon state.
 * @param weaponPod	- craft weapon to change colors of
 * @param enabled	- true if weapon is enabled
 */
void DogfightState::recolor(
		const int weaponPod,
		const bool enabled)
{
	const int
		offset1 = 25,	// 24
		offset2 = 8;	// 7
	InteractiveSurface* weapon = NULL;
	Text* ammo = NULL;
	Surface* range = NULL;

	if (weaponPod == 0)
	{
		weapon = _weapon1;
		ammo = _txtAmmo1;
		range = _range1;
	}
	else if (weaponPod == 1)
	{
		weapon = _weapon2;
		ammo = _txtAmmo2;
		range = _range2;
	}
	else
		return;

	if (enabled == true)
	{
		weapon->offset(-offset1);
		ammo->offset(-offset1);
		range->offset(-offset2);
	}
	else
	{
		weapon->offset(offset1);
		ammo->offset(offset1);
		range->offset(offset2);
	}
}

/**
 * Returns true if state is minimized.
 * @return, true if minimized
 */
bool DogfightState::isMinimized() const
{
	return _minimized;
}

/**
 * Sets the state to minimized/maximized status.
 * @param minimized - true if minimized
 */
void DogfightState::setMinimized(const bool minimized)
{
	_minimized = minimized;
}

/**
 * Maximizes the interception window.
 * @param action - pointer to an Action
 */
void DogfightState::btnMinimizedIconClick(Action*)
{
	_texture->clear();

	Surface* const srfTexture = _game->getResourcePack()->getSurface(getTextureIcon());
	if (srfTexture != NULL)
		srfTexture->blit(_texture);

	setMinimized(false);

	_window->setVisible();
	_btnStandoff->setVisible();
	_btnCautious->setVisible();
	_btnStandard->setVisible();
	_btnAggressive->setVisible();
	_btnDisengage->setVisible();
	_btnUfo->setVisible();
	_texture->setVisible();
	_btnMinimize->setVisible();
	_battle->setVisible();
	_weapon1->setVisible();
	_range1->setVisible();
	_weapon2->setVisible();
	_range2->setVisible();
	_damage->setVisible();
	_txtAmmo1->setVisible();
	_txtAmmo2->setVisible();
	_txtDistance->setVisible();
	_txtStatus->setVisible();

	_btnMinimizedIcon->setVisible(false);
	_txtInterception->setVisible(false);
	_preview->setVisible(false);

	// if any interception window is open
	// zoom in to max
	// and set dfZoom + dfCenter
	// note: There's also something about GeoscapeState::_dfZoomIn/OutDone ...
	if (_interceptQty != _geo->minimizedDogfightsCount()
		&& _geo->getDogfightZoomInTimer()->isRunning() == false)
	{
		if (_geo->getDogfightZoomOutTimer()->isRunning() == true)
			_geo->getDogfightZoomOutTimer()->stop();

		_globe->setPreDogfightZoom();
		_geo->setDogfightCoords(_craft);
		_geo->getDogfightZoomInTimer()->start();
	}
}

/**
 * Gets interception number.
 * @return, interception number
 */
size_t DogfightState::getInterceptSlot() const
{
	return _intercept;
}

/**
 * Sets interception number. Used to draw proper number when window minimized.
 * @param intercept - #ID
 */
void DogfightState::setInterceptSlot(const size_t intercept)
{
	_intercept = intercept;
}

/**
 * Sets interceptions count. Used to properly position the window.
 * @param intercepts - amount of interception windows
 */
void DogfightState::setInterceptQty(const size_t intercepts)
{
	_interceptQty = intercepts;
	calculateWindowPosition();

	moveWindow();
}

/**
 * Calculates dogfight window position according to number of active interceptions.
 */
void DogfightState::calculateWindowPosition()
{
	_minimizedIconX = 5;
	_minimizedIconY = (5 * _intercept) + (16 * (_intercept - 1));

	if (_interceptQty == 1)
	{
		_x = 80;
		_y = 52;
	}
	else if (_interceptQty == 2)
	{
		if (_intercept == 1)
		{
			_x = 80;
			_y = 0;
		}
		else // 2
		{
			_x = 80;
			_y = 200 - _window->getHeight(); // 96;
		}
	}
	else if (_interceptQty == 3)
	{
		if (_intercept == 1)
		{
			_x = 80;
			_y = 0;
		}
		else if (_intercept == 2)
		{
			_x = 0;
			_y = 200 - _window->getHeight(); // 96;
		}
		else // 3
		{
			_x = 320 - _window->getWidth(); // 160;
			_y = 200 - _window->getHeight(); // 96;
		}
	}
	else
	{
		if (_intercept == 1)
		{
			_x = 0;
			_y = 0;
		}
		else if (_intercept == 2)
		{
			_x = 320 - _window->getWidth(); // 160;
			_y = 0;
		}
		else if (_intercept == 3)
		{
			_x = 0;
			_y = 200 - _window->getHeight(); // 96;
		}
		else // 4
		{
			_x = 320 - _window->getWidth(); // 160;
			_y = 200 - _window->getHeight(); // 96;
		}
	}

	_x += _game->getScreen()->getDX();
	_y += _game->getScreen()->getDY();
}

/**
 * Relocates all dogfight window elements to calculated position.
 * This is used when multiple interceptions are running.
 */
void DogfightState::moveWindow()
{
	_window->setX(_x);
	_window->setY(_y);

	_battle->setX(_x + 3);
	_battle->setY(_y + 3);

	_weapon1->setX(_x + 4);
	_weapon1->setY(_y + 52);

	_range1->setX(_x + 19);
	_range1->setY(_y + 3);

	_weapon2->setX(_x + 64);
	_weapon2->setY(_y + 52);

	_range2->setX(_x + 43);
	_range2->setY(_y + 3);

	_damage->setX(_x + 93);
	_damage->setY(_y + 40);

	_btnMinimize->setX(_x);
	_btnMinimize->setY(_y);

	_preview->setX(_x);
	_preview->setY(_y);

	_btnUfo->setX(_x + 83);
	_btnUfo->setY(_y + 4);

	_btnDisengage->setX(_x + 83);
	_btnDisengage->setY(_y + 20);

	_btnCautious->setX(_x + 120);
	_btnCautious->setY(_y + 4);

	_btnStandard->setX(_x + 120);
	_btnStandard->setY(_y + 20);

	_btnAggressive->setX(_x + 120);
	_btnAggressive->setY(_y + 36);

	_btnStandoff->setX(_x + 120);
	_btnStandoff->setY(_y + 52);

	_texture->setX(_x + 147);
	_texture->setY(_y + 72);

	_txtAmmo1->setX(_x + 4);
	_txtAmmo1->setY(_y + 70);

	_txtAmmo2->setX(_x + 64);
	_txtAmmo2->setY(_y + 70);

	_txtDistance->setX(_x + 116);
	_txtDistance->setY(_y + 72);

	_txtStatus->setX(_x + 4);
	_txtStatus->setY(_y + 85);

	_btnMinimizedIcon->setX(_minimizedIconX);
	_btnMinimizedIcon->setY(_minimizedIconY);

	_txtInterception->setX(_minimizedIconX + 18);
	_txtInterception->setY(_minimizedIconY + 6);
}

/**
 * Checks whether the dogfight should end.
 * @return, true if the dogfight should end
 */
bool DogfightState::dogfightEnded() const
{
	return _endDogfight;
}

/**
 * Gets the UFO associated to this dogfight.
 * @return, pointer to UFO object associated with this dogfight
 */
Ufo* DogfightState::getUfo() const
{
	return _ufo;
}

/**
 * Gets pointer to the xCom Craft in this dogfight.
 * @return, pointer to Craft object associated with this dogfight
 */
Craft* DogfightState::getCraft() const
{
	return _craft;
}

/**
 * Gets the current distance between UFO and Craft.
 * @return, current distance
 */
int DogfightState::getDistance() const
{
	return _dist;
}

/**
 * Ends the dogfight.
 */
void DogfightState::endDogfight()
{
	if (_craft != NULL)
		_craft->setInDogfight(false);

	_endDogfight = true;
}

/**
 * Gets the globe texture icon to display for the interception.
 * IMPORTANT: This does not return the actual battleField terrain; that is done
 * in ConfirmLandingState. This is merely an indicator .... cf. UfoDetectedState.
 * @return, string for the icon of the texture of the globe's surface under the dogfight ha!
 */
const std::string DogfightState::getTextureIcon()
{
	int
		texture,
		shade;
	_globe->getPolygonTextureAndShade( // look up polygon's texture
									_ufo->getLongitude(),
									_ufo->getLatitude(),
									&texture,
									&shade);
	switch (texture)
	{
		case  0: return "FOREST";
		case  1: return "CULTA";
		case  2: return "CULTA";
		case  3: return "FOREST";
		case  4: return "POLAR";
		case  5: return "MOUNT";
		case  6: return "FOREST"; // JUNGLE
		case  7: return "DESERT";
		case  8: return "DESERT";
		case  9: return "POLAR";
		case 10: return "URBAN";
		case 11: return "POLAR";
		case 12: return "POLAR";
	}

	return "WATER"; // tex = -1
}

}
