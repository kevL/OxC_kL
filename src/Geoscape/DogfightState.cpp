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

#include "DogfightState.h"

//#include <cstdlib>
//#include <sstream>

#include "Globe.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
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
#include "../Ruleset/Ruleset.h"
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
 * @param ufo	- pointer to the UFO being intercepted
 */
DogfightState::DogfightState(
		Globe* globe,
		Craft* craft,
		Ufo* ufo)
	:
		_globe(globe),
		_craft(craft),
		_ufo(ufo),
		_timeout(50),
		_currentDist(640),
		_targetDist(560),
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
		_currentCraftDamageColor(13),
		_interceptionNumber(0),
		_interceptionsCount(0),
		_x(0),
		_y(0),
		_minimizedIconX(0),
		_minimizedIconY(0)
{
	_screen = false;

	_craft->setInDogfight(true);
	_timeScale = 50 + Options::dogfightSpeed;

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
	_txtInterceptionNumber	= new Text(16, 9, _minimizedIconX + 18, _minimizedIconY + 6);

	_animTimer				= new Timer(Options::dogfightSpeed + 10);
	_moveTimer				= new Timer(Options::dogfightSpeed);
	_w1Timer				= new Timer(0);
	_w2Timer				= new Timer(0);
	_ufoWtimer				= new Timer(0);
	_ufoEscapeTimer			= new Timer(0);
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
	add(_btnUfo);
	add(_btnDisengage);
	add(_btnCautious);
	add(_btnStandard);
	add(_btnAggressive);
	add(_btnStandoff);
	add(_texture);
	add(_txtAmmo1);
	add(_txtAmmo2);
	add(_txtDistance);
	add(_preview);
	add(_txtStatus);
	add(_btnMinimizedIcon);
	add(_txtInterceptionNumber);

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
	Surface* srfInterwin = _game->getResourcePack()->getSurface("INTERWIN");
	if (srfInterwin != NULL)
		srfInterwin->blit(_window);

	Surface* srfPreview = _game->getResourcePack()->getSurface("INTERWIN_");
	if (srfPreview != NULL)
		srfPreview->blit(_preview);

	int iSprite = _ufo->getRules()->getSprite();
	std::ostringstream sprite;
	sprite << "INTERWIN_" << iSprite;
	srfPreview = _game->getResourcePack()->getSurface(sprite.str());
	if (srfPreview != NULL)
	{
		srfPreview->setY(15);
		srfPreview->blit(_preview);
	} // kL_end.
	_preview->setVisible(false);
	_preview->onMouseClick(
					(ActionHandler)& DogfightState::previewPress,
					SDL_BUTTON_RIGHT);

	_btnMinimize->onMouseClick((ActionHandler)& DogfightState::btnMinimizeClick);

	_btnUfo->copy(_window);
	_btnUfo->setColor(Palette::blockOffset(5)+1);
	_btnUfo->onMouseClick((ActionHandler)& DogfightState::btnUfoClick);

	_btnDisengage->copy(_window);
	_btnDisengage->setColor(Palette::blockOffset(5)+1);
	_btnDisengage->setGroup(&_mode);
	_btnDisengage->onMouseClick((ActionHandler)& DogfightState::btnDisengagePress);

	_btnCautious->copy(_window);
	_btnCautious->setColor(Palette::blockOffset(5)+1);
	_btnCautious->setGroup(&_mode);
	_btnCautious->onMouseClick((ActionHandler)& DogfightState::btnCautiousPress);

	_btnStandard->copy(_window);
	_btnStandard->setColor(Palette::blockOffset(5)+1);
	_btnStandard->setGroup(&_mode);
	_btnStandard->onMouseClick((ActionHandler)& DogfightState::btnStandardPress);

	_btnAggressive->copy(_window);
	_btnAggressive->setColor(Palette::blockOffset(5)+1);
	_btnAggressive->setGroup(&_mode);
	_btnAggressive->onMouseClick((ActionHandler)& DogfightState::btnAggressivePress);

	_btnStandoff->copy(_window);
	_btnStandoff->setColor(Palette::blockOffset(5)+1);
	_btnStandoff->setGroup(&_mode);
	_btnStandoff->onMouseClick((ActionHandler)& DogfightState::btnStandoffPress);

	Surface* srfTexture = _game->getResourcePack()->getSurface(getTextureIcon());
	if (srfTexture != NULL)
		srfTexture->blit(_texture);

	_txtAmmo1->setColor(Palette::blockOffset(5)+9);
	_txtAmmo2->setColor(Palette::blockOffset(5)+9);

	_txtDistance->setColor(Palette::blockOffset(5)+9);
	_txtDistance->setText(L"640");

	_txtStatus->setColor(Palette::blockOffset(5)+9);
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_STANDOFF"));

	SurfaceSet* sstInticon = _game->getResourcePack()->getSurfaceSet("INTICON.PCK");

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
	_txtInterceptionNumber->setColor(Palette::blockOffset(5));
	_txtInterceptionNumber->setText(ss1.str());
	_txtInterceptionNumber->setVisible(false);

	for (size_t
			i = 0;
			i < static_cast<size_t>(_craft->getRules()->getWeapons());
			++i)
	{
		CraftWeapon* w = _craft->getWeapons()->at(i);
		if (w == NULL)
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

		// Draw weapon icon
		frame = sstInticon->getFrame(w->getRules()->getSprite() + 5);
		frame->setX(0);
		frame->setY(0);
		frame->blit(weapon);

		// Draw ammo
		std::wostringstream ss;
		ss << w->getAmmo();
		ammo->setText(ss.str());

		// Draw range (1 km = 1 pixel)
		Uint8 color = Palette::blockOffset(7)-1;

		range->lock();
		int
			rangeY = range->getHeight() - w->getRules()->getRange(),
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

	_animTimer->onTimer((StateHandler)& DogfightState::animate);
	_animTimer->start();

	_moveTimer->onTimer((StateHandler)& DogfightState::moveCraft);
	_moveTimer->start();

	_w1Timer->onTimer((StateHandler)& DogfightState::fireWeapon1);
	_w2Timer->onTimer((StateHandler)& DogfightState::fireWeapon2);

	_ufoWtimer->onTimer((StateHandler)& DogfightState::ufoFireWeapon);
	Uint32 reload = static_cast<Uint32>(_ufo->getRules()->getWeaponReload());
	_ufoFireInterval = reload - (2 * static_cast<Uint32>(_game->getSavedGame()->getDifficulty()));
	reload = static_cast<Uint32>(
					static_cast<int>(_ufoFireInterval)
					+ RNG::generate(2, static_cast<int>(reload)) / 2
				* _timeScale);
	if (reload < static_cast<Uint32>(_timeScale))
		reload = static_cast<Uint32>(_timeScale);
	_ufoWtimer->setInterval(reload);

	_ufoEscapeTimer->onTimer((StateHandler)& DogfightState::ufoBreakOff);
	Uint32 ufoBreakOffInterval = static_cast<Uint32>(_ufo->getRules()->getBreakOffTime());
	ufoBreakOffInterval = static_cast<Uint32>(
								(static_cast<int>(ufoBreakOffInterval)
//								+ RNG::generate(10, static_cast<int>(ufoBreakOffInterval)) / 2
								+ RNG::generate(0, static_cast<int>(ufoBreakOffInterval)) / 10
								- (10 * static_cast<int>(_game->getSavedGame()->getDifficulty())))
							* _timeScale);
	if (ufoBreakOffInterval < static_cast<Uint32>(_timeScale) * 10)
		ufoBreakOffInterval = static_cast<Uint32>(_timeScale) * 10;
//	else
//		ufoBreakOffInterval = RNG::generate(10, ufoBreakOffInterval);
	_ufoEscapeTimer->setInterval(ufoBreakOffInterval);

	_craftDamageAnimTimer->onTimer((StateHandler)& DogfightState::animateCraftDamage);

	// Set UFO size - going to be moved to Ufo class to implement simultaneous dogfights.
	std::string ufoSize = _ufo->getRules()->getSize();
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

	// Get the craft-graphic's height. Used for damage indication.
//kL	int x = _damage->getWidth() / 2;
	for (int
			y = 0;
			y < _damage->getHeight();
			++y)
	{
		// kL_begin:
		Uint8 pixelColor = _damage->getPixelColor(11, y);
		bool isCraftColor = pixelColor >= Palette::blockOffset(10)
						&& pixelColor < Palette::blockOffset(11);

		if (_craftHeight
			&& isCraftColor == false)
		{
			break;
		}
		else if (isCraftColor)
			_craftHeight++;
		else
			_craftHeight_pre++;
		// kL_end.

/*		Uint8 pixelColor = _damage->getPixelColor(x, y);
		if (pixelColor >= Palette::blockOffset(10)
			|| pixelColor < Palette::blockOffset(11))
		{
			++_craftHeight;
		} */
	}

	drawCraftDamage();

	// Used for weapon toggling.
	_weapon1->onMouseClick((ActionHandler)& DogfightState::weapon1Click);
	_weapon2->onMouseClick((ActionHandler)& DogfightState::weapon2Click);
}

/**
 * Deletes timers.
 */
DogfightState::~DogfightState()
{
	delete _animTimer;
	delete _moveTimer;
	delete _w1Timer;
	delete _w2Timer;
	delete _ufoWtimer;
	delete _ufoEscapeTimer;
	delete _craftDamageAnimTimer;

	while (_projectiles.empty() == false)
	{
		delete _projectiles.back();
		_projectiles.pop_back();
	}

	if (_craft != NULL)
		_craft->setInDogfight(false);
}

/**
 * Runs the dogfighter timers.
 */
void DogfightState::think()
{
	if (_endDogfight == false)
	{
		_moveTimer->think(this, NULL);

		if (_endDogfight == false // check _endDogfight again, because moveTimer can change it
			&& _minimized == false)
		{
			_animTimer->think(this, NULL);
			_w1Timer->think(this, NULL);
			_w2Timer->think(this, NULL);
			_ufoWtimer->think(this, NULL);
			_ufoEscapeTimer->think(this, NULL);
			_craftDamageAnimTimer->think(this, NULL);
		}
		else if (_endDogfight == false
			&& (_craft->getDestination() != _ufo
				|| _ufo->getStatus() == Ufo::LANDED))
		{
			endDogfight();
		}
	}
}

/**
 * Animates interceptor damage by changing the color and redrawing the image.
 */
void DogfightState::animateCraftDamage()
{
	if (_minimized)
		return;

	_currentCraftDamageColor--;

//kL	if (_currentCraftDamageColor < 13)
//kL		_currentCraftDamageColor = 14;
	if (_currentCraftDamageColor < 13) // kL, 11/12 == yellow
		_currentCraftDamageColor = 15; // kL, black

	drawCraftDamage();
}

/**
 * Draws interceptor damage according to percentage of HPs left.
 */
void DogfightState::drawCraftDamage()
{
	if (_minimized)
		return;

	int damagePercent = _craft->getDamagePercent();
	if (damagePercent > 0)
	{
		if (_craftDamageAnimTimer->isRunning() == false)
			_craftDamageAnimTimer->start();

		int rowsToColor = static_cast<int>(
							floor(static_cast<double>(_craftHeight * damagePercent) / 100.0));

		if (rowsToColor == 0)
			return;
		else if (damagePercent > 99)
			rowsToColor += 1;

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
				int pixelColor = _damage->getPixelColor(x, y);

				if (pixelColor > 10
					&& pixelColor < 16)
				{
					_damage->setPixelColor(
										x,
										y,
										_currentCraftDamageColor);
				}
				else if (pixelColor >= Palette::blockOffset(10)
					&& pixelColor < Palette::blockOffset(11))
				{
					_damage->setPixelColor(
										x,
										y,
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
	if (_minimized)
		return;

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
			if (radarPixelColor >= Palette::blockOffset(7)
				&& radarPixelColor < Palette::blockOffset(8))
			{
				radarPixelColor++;

				if (radarPixelColor >= Palette::blockOffset(8))
					radarPixelColor = Palette::blockOffset(7);

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
			it = _projectiles.begin();
			it != _projectiles.end();
			++it)
	{
		drawProjectile(*it);
	}

	if (_timeout == 0) // Clears text after a while
		_txtStatus->setText(L"");
	else
		_timeout--;

	bool lastHitAnimFrame = false;
	if (_animatingHit // Animate UFO hit.
		&& _ufo->getHitFrame() > 0)
	{
		_ufo->setHitFrame(_ufo->getHitFrame() - 1);

		if (_ufo->getHitFrame() == 0)
		{
			_animatingHit = false;
			lastHitAnimFrame = true;
		}
	}

	if (_ufo->isCrashed() // Animate UFO crash landing.
		&& _ufo->getHitFrame() == 0
		&& lastHitAnimFrame == false)
	{
		_ufoSize--;
	}
}

/**
 * Moves Craft towards the UFO according to the current
 * interception mode. Handles projectile movements also.
 */
void DogfightState::moveCraft()
{
	bool finalRun = false;

	// Check if craft is not low on fuel when window minimized, and
	// Check if craft's destination hasn't been changed when window minimized.
	const Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	if (ufo != _ufo
		|| _craft->getLowFuel() == true
		|| (_minimized == true
			&& _ufo->isCrashed() == true))
	{
		endDogfight();

		return;
	}

	if (_minimized == true
		&& _ufo->getSpeed() > _craft->getSpeed())
	{
		_craft->setSpeed(_craft->getRules()->getMaxSpeed());

		if (_ufo->getSpeed() > _craft->getSpeed())
			_ufoBreakingOff = true;
	}

	if (_ufo->getSpeed() == _ufo->getRules()->getMaxSpeed()) // Check if UFO is not breaking off.
	{
		_craft->setSpeed(_craft->getRules()->getMaxSpeed());

		if (_ufo->getSpeed() > _craft->getSpeed()) // Crappy craft is chasing UFO.
		{
			_ufoBreakingOff = true;
			finalRun = true;

			setStatus("STR_UFO_OUTRUNNING_INTERCEPTOR");
		}
		else // ufo cannot break off, because it's crappier than the crappy craft.
			_ufoBreakingOff = false;
	}

	bool projectileInFlight = false;
	if (_minimized == false)
	{
		int distanceChange = 0;

		if (_ufoBreakingOff == false) // Update distance
		{
			if (_currentDist < _targetDist
				&& _ufo->isCrashed() == false
				&& _craft->isDestroyed() == false)
			{
				distanceChange = 4;

				if (_currentDist + distanceChange > _targetDist)
					distanceChange = _targetDist - _currentDist;
			}
			else if (_currentDist > _targetDist
				&& _ufo->isCrashed() == false
				&& _craft->isDestroyed() == false)
			{
				distanceChange = -2;
			}

			// don't let the interceptor mystically push or pull its fired projectiles
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

			// UFOs can try to outrun our missiles, don't adjust projectile positions here.
			// If UFOs ever fire anything but beams, those positions need to be adjusted here though.
		}

		_currentDist += distanceChange;

		std::wostringstream ss;
		ss << _currentDist;
		_txtDistance->setText(ss.str());

		// Move projectiles and check for hits.
		for (std::vector<CraftWeaponProjectile*>::const_iterator
				i = _projectiles.begin();
				i != _projectiles.end();
				++i)
		{
			CraftWeaponProjectile* const proj = *i;
			proj->moveProjectile();

			if (proj->getDirection() == D_UP) // Projectiles fired by interceptor.
			{
				// Projectile reached the UFO - determine if it's been hit.
				if ((proj->getPosition() >= _currentDist
						|| (proj->getGlobalType() == CWPGT_BEAM
							&& proj->toBeRemoved() == true))
					&& _ufo->isCrashed() == false
					&& proj->getMissed() == false)
				{
					// kL_begin:
//					int ufoSize = _ufoSize;
//					if (ufoSize > 4)
//						ufoSize = 4;

					const int
						diff = _game->getSavedGame()->getDifficulty(),
						hitchance = proj->getAccuracy() + (_ufoSize * 5) - (diff * 5); // could include UFO speed here.
					// kL_end.
//					int hitchance = ((proj->getAccuracy() * (100 + (300 / (5 - ufoSize)))) + 100) / (200 + (diff * 50)); // fuck that.
//kL				int hitchance = (proj->getAccuracy() * (100 + (300 / (5 - ufoSize))) + 100) / 200;
					//Log(LOG_INFO) << "hitchance = " << hitchance;

					if (RNG::percent(hitchance) == true)
					{
						//Log(LOG_INFO) << "hit";
						const int damage = RNG::generate(
														proj->getDamage() / 2,
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
						playSoundFX(ResourcePack::UFO_HIT, true);
/*						_game->getResourcePack()->getSound(
														"GEO.CAT",
														ResourcePack::UFO_HIT)
													->play(); */

						proj->removeProjectile();
					}
					else // Missed.
					{
						//Log(LOG_INFO) << "miss";
						if (proj->getGlobalType() == CWPGT_BEAM)
							proj->removeProjectile();
						else
							proj->setMissed(true);
					}
				}

				// Check if projectile passed its maximum range.
				if (proj->getGlobalType() == CWPGT_MISSILE)
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
													_ufo->getRules()->getWeaponPower() / 10,
													_ufo->getRules()->getWeaponPower());
						if (damage != 0)
						{
							_craft->setDamage(_craft->getDamage() + damage);
							drawCraftDamage();
							setStatus("STR_INTERCEPTOR_DAMAGED");
							playSoundFX(ResourcePack::INTERCEPTOR_HIT, true);
/*							_game->getResourcePack()->getSound(
															"GEO.CAT",
															ResourcePack::INTERCEPTOR_HIT)
														->play(); */

							if ((_mode == _btnCautious
									&& _craft->getDamagePercent() > 60)
								|| (_mode == _btnStandard
									&& _craft->getDamagePercent() > 35))
							{
								// kL_begin: taken from btnStandoffClick() below.
								if (_ufo->isCrashed() == false
									&& _craft->isDestroyed() == false
									&& _ufoBreakingOff == false)
								{
									_btnStandoff->releaseDogfight(); // kL_add

									_end = false;
									setStatus("STR_STANDOFF");
									_targetDist = STANDOFF_DIST;
								} // kL_end.
							}
						}
					}

					proj->removeProjectile();
				}
			}
		}

		// Remove projectiles that hit or missed their target.
		for (std::vector<CraftWeaponProjectile*>::const_iterator
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

		for (size_t // handle weapons and craft distance.
				i = 0;
				i < static_cast<size_t>(_craft->getRules()->getWeapons());
				++i)
		{
			const CraftWeapon* const cw = _craft->getWeapons()->at(i);
			if (cw == NULL)
				continue;

			Timer* wTimer = NULL;
			if (i == 0)
				wTimer = _w1Timer;
			else
				wTimer = _w2Timer;

			if (wTimer->isRunning() == false // handle weapon firing
				&& _currentDist <= cw->getRules()->getRange() * 8
				&& cw->getAmmo() > 0
				&& _mode != _btnStandoff
				&& _mode != _btnDisengage
				&& _ufo->isCrashed() == false
				&& _craft->isDestroyed() == false)
			{
				wTimer->start();

				if (i == 0)
					fireWeapon1();
				else
					fireWeapon2();
			}
			else if (wTimer->isRunning() == true
				&& (_currentDist > cw->getRules()->getRange() * 8
					|| (cw->getAmmo() == 0
						&& projectileInFlight == false)
					|| _mode == _btnStandoff
					|| _mode == _btnDisengage
					|| _ufo->isCrashed() == true
					|| _craft->isDestroyed() == true))
			{
				wTimer->stop();

				if (cw->getAmmo() == 0 // handle craft distance according to option set by user and available ammo.
					&& _craft->isDestroyed() == false)
				{
					if (_mode == _btnCautious)
						minimumDistance();
					else if (_mode == _btnStandard)
						maximumDistance();
				}
			}
		}

		if (_ufoWtimer->isRunning() == false // handle UFO firing.
			&& _currentDist <= _ufo->getRules()->getWeaponRange() * 8
			&& _ufo->isCrashed() == false
			&& _craft->isDestroyed() == false)
		{
			if (_ufo->getShootingAt() == 0)
			{
				_ufo->setShootingAt(_interceptionNumber);
				_ufoWtimer->start();

				ufoFireWeapon();
			}
		}
		else if (_ufoWtimer->isRunning() == true
			&& (_currentDist > _ufo->getRules()->getWeaponRange() * 8
				|| _ufo->isCrashed() == true
				|| _craft->isDestroyed() == true))
		{
			_ufo->setShootingAt(0);
			_ufoWtimer->stop();
		}
	}

	if (_end // check when battle is over.
		&& (((_currentDist > 640
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

	if (_currentDist > 640
		&& _ufoBreakingOff == true)
	{
		finalRun = true;
	}

	if (_end == false // end dogfight if craft is destroyed.
		&& _craft->isDestroyed() == true)
	{
		setStatus("STR_INTERCEPTOR_DESTROYED");

		_timeout += 30;
		playSoundFX(ResourcePack::INTERCEPTOR_EXPLODE);
/*		_game->getResourcePack()->getSound(
										"GEO.CAT",
										ResourcePack::INTERCEPTOR_EXPLODE)
									->play(); */

		finalRun = true;
		_destroyCraft = true;

		_ufo->setShootingAt(0);
		_ufoWtimer->stop();
		_w1Timer->stop();
		_w2Timer->stop();
	}

	if (_end == false // end dogfight if UFO is crashed, or destroyed.
		&& _ufo->isCrashed() == true)
	{
//		_ufoBreakingOff = false;
//		_ufo->setSpeed(0);

		AlienMission* const mission = _ufo->getMission();
		mission->ufoShotDown(
						*_ufo,
						*_game,
						*_globe);


		// check for retaliation trigger.
		if (RNG::percent(4 + (4 * static_cast<int>(_game->getSavedGame()->getDifficulty()))) == true)
		{
			std::string targetRegion; // spawn retaliation mission.
			if (RNG::percent(50 - (6 * static_cast<int>(_game->getSavedGame()->getDifficulty()))) == true)
				targetRegion = _ufo->getMission()->getRegion(); // Attack on UFO's mission region
			else // try to find and attack the originating base.
				targetRegion = _game->getSavedGame()->locateRegion(*_craft->getBase())->getRules()->getType();
				// TODO: If the base is removed, the mission is cancelled.

			// Difference from original: No retaliation until final UFO lands (Original: Is spawned).
			if (_game->getSavedGame()->getAlienMission(
													targetRegion,
													"STR_ALIEN_RETALIATION") == NULL)
			{
				const RuleAlienMission& rule = *_game->getRuleset()->getAlienMission("STR_ALIEN_RETALIATION");
				AlienMission* const mission = new AlienMission(rule);

				mission->setId(_game->getSavedGame()->getId("ALIEN_MISSIONS"));
				mission->setRegion(
								targetRegion,
								*_game->getRuleset());
				mission->setRace(_ufo->getAlienRace());
				mission->start();

				_game->getSavedGame()->getAlienMissions().push_back(mission);
			}
		}

		_ufoEscapeTimer->stop();

		const double
			ufoLon = _ufo->getLongitude(),
			ufoLat = _ufo->getLatitude();

		if (_ufo->isDestroyed() == true)
		{
			if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
			{
				setStatus("STR_UFO_DESTROYED");
				playSoundFX(ResourcePack::UFO_EXPLODE);
/*				_game->getResourcePack()->getSound(
												"GEO.CAT",
												ResourcePack::UFO_EXPLODE)
											->play(); */

				for (std::vector<Region*>::const_iterator
						region = _game->getSavedGame()->getRegions()->begin();
						region != _game->getSavedGame()->getRegions()->end();
						++region)
				{
					if ((*region)->getRules()->insideRegion(
														ufoLon,
														ufoLat))
					{
						(*region)->addActivityXcom(_ufo->getRules()->getScore() * 2);
						(*region)->recentActivityXCOM();
						break;
					}
				}

				for (std::vector<Country*>::const_iterator
						country = _game->getSavedGame()->getCountries()->begin();
						country != _game->getSavedGame()->getCountries()->end();
						++country)
				{
					if ((*country)->getRules()->insideCountry(
															ufoLon,
															ufoLat))
					{
						(*country)->addActivityXcom(_ufo->getRules()->getScore() * 2);
						(*country)->recentActivityXCOM();
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
				playSoundFX(ResourcePack::UFO_CRASH);
/*				_game->getResourcePack()->getSound(
												"GEO.CAT",
												ResourcePack::UFO_CRASH)
											->play(); */

				for (std::vector<Region*>::const_iterator
						region = _game->getSavedGame()->getRegions()->begin();
						region != _game->getSavedGame()->getRegions()->end();
						++region)
				{
					if ((*region)->getRules()->insideRegion(
														ufoLon,
														ufoLat))
					{
						(*region)->addActivityXcom(_ufo->getRules()->getScore());
						(*region)->recentActivityXCOM();
						break;
					}
				}

				for (std::vector<Country*>::const_iterator
						country = _game->getSavedGame()->getCountries()->begin();
						country != _game->getSavedGame()->getCountries()->end();
						++country)
				{
					if ((*country)->getRules()->insideCountry(
															ufoLon,
															ufoLat))
					{
						(*country)->addActivityXcom(_ufo->getRules()->getScore());
						(*country)->recentActivityXCOM();
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
			else // set up Crash site.
			{
				// kL_note: This is how long, in seconds, the crashed uFo remains....
				_ufo->setSecondsRemaining(RNG::generate(24, 96) * 3600);
				_ufo->setAltitude("STR_GROUND");

				if (_ufo->getCrashId() == 0)
					_ufo->setCrashId(_game->getSavedGame()->getId("STR_CRASH_SITE"));

				const int hull = _ufo->getDamagePercent(); // kL
				//Log(LOG_INFO) << "DogfightState::move(), crashPower = " << hull;
				_ufo->setCrashPower(hull); // kL
			}
		}

		_timeout += 30;
		if (_ufo->getShotDownByCraftId() != _craft->getUniqueId())
		{
			_timeout += 50;
			_ufo->setHitFrame(3);
		}

		finalRun = true;
//		_ufoBreakingOff = false;
//		_ufo->setSpeed(0);
	}

	if (_end == false
		&& _ufo->getStatus() == Ufo::LANDED)
	{
		_timeout += 30;
		finalRun = true;
		_ufo->setShootingAt(0);

		_ufoWtimer->stop();
		_w1Timer->stop();
		_w2Timer->stop();
	}

	if (projectileInFlight == false
		&& finalRun == true)
	{
		_end = true;
	}
}

/**
 * Fires a shot from the first weapon equipped on the craft.
 */
void DogfightState::fireWeapon1()
{
	if (_weapon1Enabled)
	{
		CraftWeapon* w1 = _craft->getWeapons()->at(0);
		if (w1->setAmmo(w1->getAmmo() - 1))
		{
			std::wostringstream ss;
			ss << w1->getAmmo();
			_txtAmmo1->setText(ss.str());

			CraftWeaponProjectile* p = w1->fire();
			p->setDirection(D_UP);
			p->setHorizontalPosition(HP_LEFT);
			_projectiles.push_back(p);

			playSoundFX(w1->getRules()->getSound(), true);
/*			_game->getResourcePack()->getSound(
											"GEO.CAT",
											w1->getRules()->getSound())
										->play(); */
		}
	}
}

/**
 * Fires a shot from the second weapon equipped on the craft.
 */
void DogfightState::fireWeapon2()
{
	if (_weapon2Enabled)
	{
		CraftWeapon* w2 = _craft->getWeapons()->at(1);
		if (w2->setAmmo(w2->getAmmo() - 1))
		{
			std::wostringstream ss;
			ss << w2->getAmmo();
			_txtAmmo2->setText(ss.str());

			CraftWeaponProjectile* p = w2->fire();
			p->setDirection(D_UP);
			p->setHorizontalPosition(HP_RIGHT);
			_projectiles.push_back(p);

			playSoundFX(w2->getRules()->getSound(), true);
/*			_game->getResourcePack()->getSound(
											"GEO.CAT",
											w2->getRules()->getSound())
										->play(); */
		}
	}
}

/**
 * Each time a UFO will try to fire its cannons a calculation
 * is made. There's only 10% chance that it will actually fire.
 */
void DogfightState::ufoFireWeapon()
{
	Uint32 reload = static_cast<Uint32>(
						static_cast<int>(_ufoFireInterval)
						+ RNG::generate(
									5,
									_ufo->getRules()->getWeaponReload())
					* _timeScale);
	if (reload < 1)
		reload = 1;
	_ufoWtimer->setInterval(reload);

	setStatus("STR_UFO_RETURN_FIRE");

	CraftWeaponProjectile* p = new CraftWeaponProjectile();
	p->setType(CWPT_PLASMA_BEAM);
	p->setAccuracy(60);
	p->setDamage(_ufo->getRules()->getWeaponPower());
	p->setDirection(D_DOWN);
	p->setHorizontalPosition(HP_CENTER);
	p->setPosition(_currentDist - (_ufo->getRules()->getRadius() / 2));
	_projectiles.push_back(p);

	playSoundFX(ResourcePack::UFO_FIRE, true);
/*	_game->getResourcePack()->getSound(
									"GEO.CAT",
									ResourcePack::UFO_FIRE)
								->play(); */
}

/**
 * Sets the craft to the minimum distance required to fire a weapon.
 */
void DogfightState::minimumDistance()
{
	int dist = 0;

	for (std::vector<CraftWeapon*>::iterator
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

	for (std::vector<CraftWeapon*>::iterator
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
 * @param status, New status text.
 */
void DogfightState::setStatus(const std::string& status)
{
	_txtStatus->setText(tr(status));
	_timeout = 50;
}

/**
 * Minimizes the dogfight window.
 * @param action, Pointer to an action.
 */
void DogfightState::btnMinimizeClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		if (_currentDist >= STANDOFF_DIST)
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
			_txtInterceptionNumber->setVisible();

			_ufoEscapeTimer->stop();
		}
		else
			setStatus("STR_MINIMISE_AT_STANDOFF_RANGE_ONLY");
	}
}

/**
 * Switches to Standoff mode (maximum range).
 * @param action, Pointer to an action.
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
 * @param action, Pointer to an action.
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
			&& _craft->getWeapons()->at(0) != 0)
		{
			_w1Timer->setInterval(_craft->getWeapons()->at(0)->getRules()->getCautiousReload() * _timeScale);
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != 0)
		{
			_w2Timer->setInterval(_craft->getWeapons()->at(1)->getRules()->getCautiousReload() * _timeScale);
		}

		minimumDistance();
		_ufoEscapeTimer->start();
	}
}

/**
 * Switches to Standard mode (minimum weapon range).
 * @param action, Pointer to an action.
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
			_w1Timer->setInterval(_craft->getWeapons()->at(0)->getRules()->getStandardReload() * _timeScale);
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2Timer->setInterval(_craft->getWeapons()->at(1)->getRules()->getStandardReload() * _timeScale);
		}

		maximumDistance();
		_ufoEscapeTimer->start();
	}
}

/**
 * Switches to Aggressive mode (minimum range).
 * @param action, Pointer to an action.
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
			_w1Timer->setInterval(_craft->getWeapons()->at(0)->getRules()->getAggressiveReload() * _timeScale);
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2Timer->setInterval(_craft->getWeapons()->at(1)->getRules()->getAggressiveReload() * _timeScale);
		}

		_targetDist = 64;
		_ufoEscapeTimer->start();
	}
}

/**
 * Disengages from the UFO.
 * @param action, Pointer to an action.
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
		_ufoEscapeTimer->stop();
	}
}

/**
 * Shows a front view of the UFO.
 * @param action, Pointer to an action.
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
 * @param action, Pointer to an action.
 */
void DogfightState::previewPress(Action*) // action)
{
//	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
//		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
//	{
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
//	}
}

/**
 * Sets UFO to break off mode. Started via timer.
 */
void DogfightState::ufoBreakOff()
{
	if (_ufo->isCrashed() == false
		&& _ufo->isDestroyed() == false
		&& _craft->isDestroyed() == false)
	{
		_ufoBreakingOff = true;
		_ufo->setSpeed(_ufo->getRules()->getMaxSpeed());
	}
}

/**
 * Draws the UFO blob on the radar screen.
 * Currently works only for original sized blobs
 * 13 x 13 pixels.
 */
void DogfightState::drawUfo()
{
	if (_ufoSize < 0
		|| _ufo->isDestroyed()
		|| _minimized)
	{
		return;
	}

	const int
		currentUfoXposition = _battle->getWidth() / 2 - 6,
		currentUfoYposition = _battle->getHeight() - (_currentDist / 8) - 6;

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
			Uint8 pixelOffset = _ufoBlobs[_ufoSize + _ufo->getHitFrame()][y][x];
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
																currentUfoXposition + x + 3,
																currentUfoYposition + y + 3); // + 3 'cause of the window frame
				Uint8 color = radarPixelColor - pixelOffset;
				if (color < 108)
					color = 108;

				_battle->setPixelColor(
							currentUfoXposition + x,
							currentUfoYposition + y,
							color);
			}
		}
	}
}

/**
 * Draws projectiles on the radar screen.
 * Depending on what type of projectile it is, its shape will be different.
 * Currently works for original sized blobs 3 x 6 pixels.
 * @param p - pointer to CraftWeaponProjectile
 */
void DogfightState::drawProjectile(const CraftWeaponProjectile* p)
{
	if (_minimized == true)
		return;

	int xPos = _battle->getWidth() / 2 + p->getHorizontalPosition();
	if (p->getGlobalType() == CWPGT_MISSILE) // Draw missiles.
	{
		xPos -= 1;
		const int yPos = _battle->getHeight() - p->getPosition() / 8;
		for (int
				x = 0;
				x < 3;
				++x)
		{
			for (int
					y = 0;
					y < 6;
					++y)
			{
				const int pixelOffset = _projectileBlobs[p->getType()][y][x];
				if (pixelOffset == 0)
					continue;
				else
				{
					const Uint8 radarPixelColor = _window->getPixelColor(
																	xPos + x + 3,
																	yPos + y + 3); // + 3 cause of the window frame
					Uint8 color = radarPixelColor - pixelOffset;
					if (color < 108)
						color = 108;

					_battle->setPixelColor(
										xPos + x,
										yPos + y,
										color);
				}
			}
		}
	}
	else if (p->getGlobalType() == CWPGT_BEAM) // Draw beams.
	{
		const int
			yStart = _battle->getHeight() - 2,
			yEnd = _battle->getHeight() - (_currentDist / 8);
		const Uint8 pixelOffset = p->getState();

		for (int
				y = yStart;
				y > yEnd;
				--y)
		{
			const Uint8 radarPixelColor = _window->getPixelColor(xPos + 3, y + 3);
			Uint8 color = radarPixelColor - pixelOffset;
			if (color < 108)
				color = 108;

			_battle->setPixelColor(xPos, y, color);
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
 * Changes the colors of craft's weapon icons, range
 * indicators, and ammo texts base on current weapon state.
 * @param weaponPod, Craft weapon for which colors must be changed
 * @param enabled, State of weapon
 */
void DogfightState::recolor(
		const int weaponPod,
		const bool enabled)
{
	const int
		offset1 = 25,	//kL 24
		offset2 = 8;	//kL 7
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
 * Returns true if state is minimized. Otherwise returns false.
 * @return Is the dogfight minimized?
 */
bool DogfightState::isMinimized() const
{
	return _minimized;
}

/**
 * Sets the state to minimized/maximized status.
 * @param minimized Is the dogfight minimized?
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

	Surface* srfTexture = _game->getResourcePack()->getSurface(getTextureIcon());
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
	_txtInterceptionNumber->setVisible(false);
	_preview->setVisible(false);
}

/**
 * Sets interception number. Used to draw proper number when window minimized.
 * @param number - ID number
 */
void DogfightState::setInterceptionNumber(const int number)
{
	_interceptionNumber = number;
}

/**
 * Sets interceptions count. Used to properly position the window.
 * @param count - amount of interception windows
 */
void DogfightState::setInterceptionsCount(const size_t count)
{
	_interceptionsCount = count;
	calculateWindowPosition();

	moveWindow();
}

/**
 * Calculates dogfight window position according to number of active interceptions.
 */
void DogfightState::calculateWindowPosition()
{
	_minimizedIconX = 5;
	_minimizedIconY = (5 * _interceptionNumber) + (16 * (_interceptionNumber - 1));

	if (_interceptionsCount == 1)
	{
		_x = 80;
		_y = 52;
	}
	else if (_interceptionsCount == 2)
	{
		if (_interceptionNumber == 1)
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
	else if (_interceptionsCount == 3)
	{
		if (_interceptionNumber == 1)
		{
			_x = 80;
			_y = 0;
		}
		else if (_interceptionNumber == 2)
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
		if (_interceptionNumber == 1)
		{
			_x = 0;
			_y = 0;
		}
		else if (_interceptionNumber == 2)
		{
			_x = 320 - _window->getWidth(); // 160;
			_y = 0;
		}
		else if (_interceptionNumber == 3)
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

/*	_btnStandoff->setX(_x + 83);
	_btnStandoff->setY(_y + 4);

	_btnCautious->setX(_x + 120);
	_btnCautious->setY(_y + 4);

	_btnStandard->setX(_x + 83);
	_btnStandard->setY(_y + 20);

	_btnAggressive->setX(_x + 120);
	_btnAggressive->setY(_y + 20);

	_btnDisengage->setX(_x + 120);
	_btnDisengage->setY(_y + 36);

	_btnUfo->setX(_x + 120);
	_btnUfo->setY(_y + 52); */

	// kL_begin:
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
	// kL_end.

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

	_txtInterceptionNumber->setX(_minimizedIconX + 18);
	_txtInterceptionNumber->setY(_minimizedIconY + 6);
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
 * Returns the UFO associated to this dogfight.
 * @return, pointer to UFO object associated with this dogfight
 */
Ufo* DogfightState::getUfo() const
{
	return _ufo;
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
 * Returns interception number.
 * @return, interception number
 */
int DogfightState::getInterceptionNumber() const
{
	return _interceptionNumber;
}

/**
 * kL. Gets the globe texture icon to display for the interception.
 * IMPORTANT: This does not return the actual battleField terrain; that is done
 * in ConfirmLandingState. This is merely an indicator .... cf. UfoDetectedState
 * IE, the first terrain found for proper Globe-texture is chosen
 * @return, string for the icon of the texture of the globe's surface under the dogfight ha!
 */
const std::string DogfightState::getTextureIcon() // kL
{
	int // look up polygon's texture
		texture,
		shade;
	_globe->getPolygonTextureAndShade(
									_ufo->getLongitude(),
									_ufo->getLatitude(),
									&texture,
									&shade);

	if (texture == 0)	return "FOREST"; // these could get expanded/redef'd. in future
	if (texture == 1)	return "CULTA";
	if (texture == 2)	return "CULTA";
	if (texture == 3)	return "FOREST";
	if (texture == 4)	return "POLAR";
	if (texture == 5)	return "MOUNT";
	if (texture == 6)	return "JUNGLE";
	if (texture == 7)	return "DESERT";
	if (texture == 8)	return "DESERT";
	if (texture == 9)	return "POLAR";
	if (texture == 10)	return "URBAN";
	if (texture == 11)	return "POLAR";
	if (texture == 12)	return "POLAR";

	return "WATER"; // tex = -1
}
/*	RuleTerrain* terrainRule = NULL;
	std::string ret;

	const std::vector<std::string>& terrains = _game->getRuleset()->getTerrainList();
	for (std::vector<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end()
				&& ret.empty() == true;
			++i)
	{
		//Log(LOG_INFO) << ". cycle terrains";
		terrainRule = _game->getRuleset()->getTerrain(*i);
		for (std::vector<int>::const_iterator
				j = terrainRule->getTextures()->begin();
				j != terrainRule->getTextures()->end()
					&& ret.empty() == true;
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
				//Log(LOG_INFO) << ". . . terrainRule-texture MATCH found: tex = " << texture;
				ret = terrainRule->getName();
			}
		}
	}
	//Log(LOG_INFO) << ". ret = " << ret;

	if (ret.empty() == true)
		ret = "WATER";
	else if (ret == "JUNGLE"
		|| ret == "FORESTMOUNT"
		|| ret == "MUJUNGLE")
	{
		ret = "FOREST";
	}
	else if (ret == "CULTAFARMA"
		|| ret == "CULTAFARMB")
	{
		ret = "CULTA";
	}
	else if (ret == "DESERTMOUNT"
		|| ret == "ATLANTDESERT")
	{
		ret = "DESERT";
	}
	else if (ret == "POLARMOUNT")
		ret = "POLAR";
//	else if (ret == "COMRCURBAN"	// these are Terror sites only:
//		|| ret == "DAWNURBANA"		// ie. not referenced by any of the Globe's polygon textures.
//		|| ret == "DAWNURBANB"
//		|| ret == "INDUSTRIALURBAN"
//		|| ret == "MADURBAN"
//		|| ret == "NATIVEURBAN"
//		|| ret == "PORTURBAN"
//		|| ret == "RAILYARDURBAN")
//	{
//		ret = "URBAN";
//	}

	// + MOUNT


	//Log(LOG_INFO) << "DogfightState::getTextureIcon() EXIT : " << ret;
	return ret; */

/**
 * kL. Plays a sound effect in stereo.
 * @param sound		- sound to play
 * @param randAngle	- true to randomize the sound angle, default false to center it
 */
void DogfightState::playSoundFX( // kL
		const int sound,
		const bool randAngle)
{
	int dir = 360; // stereo center

	if (randAngle == true)
	{
		const int var = 67; // maximum deflection left or right
		dir += (RNG::generate(-var, var)
			  + RNG::generate(-var, var))
			/ 2;
	}

	_game->getResourcePack()->getSound(
									"GEO.CAT",
									sound)
								->play(-1, dir);
}

}
