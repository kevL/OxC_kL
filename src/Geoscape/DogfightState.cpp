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

#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/Language.h" // for logging CraftId tuple
//#include "../Engine/LocalizedText.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/ImageButton.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/CraftWeaponProjectile.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

// UFO blobs graphics ... MOVED TO GEOSCAPESTATE.

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
		Globe* const globe,
		Craft* const craft,
		Ufo* const ufo,
		GeoscapeState* const geo)
	:
		_globe(globe),
		_craft(craft),
		_ufo(ufo),
		_geo(geo),
		_gameSave(_game->getSavedGame()),
		_diff(static_cast<int>(_game->getSavedGame()->getDifficulty())),
		_timeout(MSG_TIMEOUT),
		_dist(DST_ENGAGE),
		_desired(DST_STANDOFF),
		_end(false),
		_destroyUfo(false),
		_destroyCraft(false),
		_ufoBreakingOff(false),
		_w1Enabled(true),
		_w2Enabled(true),
		_minimized(false),
		_endDogfight(false),
		_animatingHit(false),
		_cautionLevel(2),
		_ufoSize(ufo->getRadius()),
		_craftHeight(0),
		_craftHeight_pre(0),
//		_currentCraftDamageColor(14),
		_slot(0),
		_totalIntercepts(0),
		_x(0),
		_y(0),
		_minimizedIconX(0),
		_minimizedIconY(0),
		_w1FireCountdown(0),
		_w2FireCountdown(0),
		_w1FireInterval(0),
		_w2FireInterval(0)
{
	_screen = false;

	_craft->setInDogfight(true);

	_window					= new Surface(160, 96, _x, _y);

	_battleScope			= new Surface(77, 74, _x +  3, _y +  3);
	_damage					= new Surface(22, 25, _x + 93, _y + 40);
	_range1					= new Surface(21, 74, _x + 19, _y +  3);
	_range2					= new Surface(21, 74, _x + 43, _y +  3);
	_weapon1				= new InteractiveSurface(15, 17, _x +  4, _y + 52);
	_weapon2				= new InteractiveSurface(15, 17, _x + 64, _y + 52);

	_btnMinimize			= new InteractiveSurface( 12, 12, _x, _y);
	_previewUfo				= new InteractiveSurface(160, 96, _x, _y);

	_btnDisengage			= new ImageButton(36, 15, _x + 83, _y +  4);
	_btnUfo					= new ImageButton(36, 15, _x + 83, _y + 20);

	_btnAggressive			= new ImageButton(36, 15, _x + 120, _y +  4);
	_btnStandard			= new ImageButton(36, 15, _x + 120, _y + 20);
	_btnCautious			= new ImageButton(36, 15, _x + 120, _y + 36);
	_btnStandoff			= new ImageButton(36, 17, _x + 120, _y + 52);
	_craftStance = _btnStandoff;

	_texture				= new Surface(9, 9, _x + 147, _y + 72);

	_txtAmmo1				= new Text( 16, 9, _x +   4, _y + 70);
	_txtAmmo2				= new Text( 16, 9, _x +  64, _y + 70);
	_txtDistance			= new Text( 40, 9, _x + 116, _y + 72);
	_txtStatus				= new Text(150, 9, _x +   4, _y + 85);
	_txtTitle				= new Text(160, 9, _x,       _y -  9);

	_btnMinimizedIcon		= new InteractiveSurface(32, 20, _minimizedIconX, _minimizedIconY);
	_txtInterception		= new Text(150, 9, _minimizedIconX + 15, _minimizedIconY + 6);

	_craftDamageAnimTimer	= new Timer(500);

	setInterface("dogfight");

	add(_window);
	add(_battleScope);
	add(_weapon1);
	add(_range1);
	add(_weapon2);
	add(_range2);
	add(_damage);
	add(_btnMinimize);
	add(_btnDisengage,		"disengageButton",	"dogfight", _window);
	add(_btnUfo,			"ufoButton",		"dogfight", _window);
	add(_btnAggressive,		"aggressiveButton",	"dogfight", _window);
	add(_btnStandard,		"standardButton",	"dogfight", _window);
	add(_btnCautious,		"cautiousButton",	"dogfight", _window);
	add(_btnStandoff,		"standoffButton",	"dogfight", _window);
	add(_texture);
	add(_txtAmmo1,			"numbers",			"dogfight", _window);
	add(_txtAmmo2,			"numbers",			"dogfight", _window);
	add(_txtDistance,		"distance",			"dogfight", _window);
	add(_previewUfo);
	add(_txtStatus,			"text",				"dogfight", _window);
	add(_txtTitle,			"ufoButton",		"dogfight", _window);

	add(_btnMinimizedIcon);
	add(_txtInterception,	"minimizedNumber",	"dogfight");

	_btnStandoff->invalidate(false);
	_btnCautious->invalidate(false);
	_btnStandard->invalidate(false);
	_btnAggressive->invalidate(false);
	_btnDisengage->invalidate(false);
	_btnUfo->invalidate(false);

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

	_previewUfo->drawRect(graphic->getCrop(), 15);
	graphic->getCrop()->y = 96;
	graphic->getCrop()->h = 15;
	graphic->blit(_previewUfo);

	graphic->setY(67);
	graphic->getCrop()->y = 111;
	graphic->getCrop()->h = 29;
	graphic->blit(_previewUfo);

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
	graphic->blit(_previewUfo); */

	// kL_begin:
	Surface* srf = _game->getResourcePack()->getSurface("INTERWIN");
	if (srf != NULL)
		srf->blit(_window);

	srf = _game->getResourcePack()->getSurface("INTERWIN_");
	if (srf != NULL)
		srf->blit(_previewUfo);

	std::ostringstream sprite;
	sprite << "INTERWIN_" << _ufo->getRules()->getSprite();
	srf = _game->getResourcePack()->getSurface(sprite.str());
	if (srf != NULL)
	{
		srf->setY(15);
		srf->blit(_previewUfo);
	} // kL_end.
	_previewUfo->setVisible(false);
	_previewUfo->onMouseClick(
					(ActionHandler)& DogfightState::previewClick,
					SDL_BUTTON_LEFT);
	_previewUfo->onMouseClick(
					(ActionHandler)& DogfightState::previewClick,
					SDL_BUTTON_RIGHT);

	_btnMinimize->onMouseClick((ActionHandler)& DogfightState::btnMinimizeDfClick);

	_btnUfo->copy(_window);
	_btnUfo->onMouseClick((ActionHandler)& DogfightState::btnUfoClick);

	_btnDisengage->copy(_window);
	_btnDisengage->setGroup(&_craftStance);
	_btnDisengage->onMouseClick((ActionHandler)& DogfightState::btnDisengageClick);

	_btnCautious->copy(_window);
	_btnCautious->setGroup(&_craftStance);
	_btnCautious->onMouseClick((ActionHandler)& DogfightState::btnCautiousClick);

	_btnStandard->copy(_window);
	_btnStandard->setGroup(&_craftStance);
	_btnStandard->onMouseClick((ActionHandler)& DogfightState::btnStandardClick);

	_btnAggressive->copy(_window);
	_btnAggressive->setGroup(&_craftStance);
	_btnAggressive->onMouseClick((ActionHandler)& DogfightState::btnAggressiveClick);

	_btnStandoff->copy(_window);
	_btnStandoff->setGroup(&_craftStance);
	_btnStandoff->onMouseClick((ActionHandler)& DogfightState::btnStandoffClick);
	_btnStandoff->onKeyboardPress(
					(ActionHandler)& DogfightState::keyEscape,
					Options::keyCancel);

	srf = _game->getResourcePack()->getSurface(getTextureIcon());
	if (srf != NULL)
		srf->blit(_texture);
	else Log(LOG_INFO) << "ERROR: no texture icon for dogfight";

	_txtDistance->setText(Text::formatNumber(DST_ENGAGE));

	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_STANDOFF"));

	SurfaceSet* const sstInticon = _game->getResourcePack()->getSurfaceSet("INTICON.PCK");

	// Create the minimized dogfight icon.
	Surface* srfFrame = sstInticon->getFrame(_craft->getRules()->getSprite());
//	srfFrame->setX(0);
//	srfFrame->setY(0);
	srfFrame->blit(_btnMinimizedIcon);
	_btnMinimizedIcon->onMousePress((ActionHandler)& DogfightState::btnMaximizeDfPress);
	_btnMinimizedIcon->onKeyboardPress(
					(ActionHandler)& DogfightState::btnMaximizeDfPress,
					Options::keyOk); // used to Maximize all minimized interceptor icons.
	_btnMinimizedIcon->onKeyboardPress(
					(ActionHandler)& DogfightState::btnMaximizeDfPress,
					Options::keyOkKeypad); // used to Maximize all minimized interceptor icons.
	_btnMinimizedIcon->setVisible(false);

	std::wostringstream woststr;
	woststr << _craft->getName(_game->getLanguage()) << L" >" << _craft->getBase()->getName(_game->getLanguage());
	_txtTitle->setText(woststr.str());

	_txtInterception->setText(woststr.str());
	_txtInterception->setVisible(false);

	// Define the colors to be used. Note these have been further tweaked in Interfaces.rul
	const RuleInterface* const dfInterface = _game->getRuleset()->getInterface("dogfight");

	_colors[CRAFT_MIN]			= static_cast<Uint8>(dfInterface->getElement("craftRange")->color);			// 160 (10)slate gray
	_colors[CRAFT_MAX]			= static_cast<Uint8>(dfInterface->getElement("craftRange")->color2);		// 176 (11)purple
	_colors[RADAR_MIN]			= static_cast<Uint8>(dfInterface->getElement("radarRange")->color);			// 112 (7)green
	_colors[RADAR_MAX]			= static_cast<Uint8>(dfInterface->getElement("radarRange")->color2);		// 128 (8)red
	_colors[DAMAGE_MIN]			= static_cast<Uint8>(dfInterface->getElement("damageRange")->color);		//  12 (0+)yellow
	_colors[DAMAGE_MAX]			= static_cast<Uint8>(dfInterface->getElement("damageRange")->color2);		//  14 (0+)red
	_colors[BLOB_MIN]			= static_cast<Uint8>(dfInterface->getElement("radarDetail")->color);		// 108 (6)+12 green
	_colors[RANGE_METER]		= static_cast<Uint8>(dfInterface->getElement("radarDetail")->color2);		// 111 (6)+15 green
	_colors[DISABLED_WEAPON]	= static_cast<Uint8>(dfInterface->getElement("disabledWeapon")->color);		//  24
	_colors[DISABLED_RANGE]		= static_cast<Uint8>(dfInterface->getElement("disabledWeapon")->color2);	//   7
	_colors[DISABLED_AMMO]		= static_cast<Uint8>(dfInterface->getElement("disabledAmmo")->color);		//  24


	const CraftWeapon* cw;
	Surface
		* weapon,
		* range;
	Text* ammo;
	int
		x1,x2,
		rangeY, // 1 km = 1 pixel
		connectY = 57,
		minY,
		maxY;

	for (size_t
			i = 0;
			i != static_cast<size_t>(_craft->getRules()->getWeapons());
			++i)
	{
		cw = _craft->getWeapons()->at(i);
		if (cw != NULL)
		{
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

			srfFrame = sstInticon->getFrame(cw->getRules()->getSprite() + 5);
//			srfFrame->setX(0);
//			srfFrame->setY(0);
			srfFrame->blit(weapon);

			woststr.str(L"");
			woststr << cw->getAmmo();
			ammo->setText(woststr.str());

			range->lock();
			rangeY = range->getHeight() - cw->getRules()->getRange();

			for (int
					x = x1;
					x < x1 + 19;
					x += 2)
			{
				range->setPixelColor(
								x,
								rangeY,
								_colors[RANGE_METER]);
			}

			minY =
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
					y != maxY + 1;
					++y)
			{
				range->setPixelColor(
								x1 + x2,
								y,
								_colors[RANGE_METER]);
			}

			for (int
					x = x2;
					x != x2 + 3;
					++x)
			{
				range->setPixelColor(
								x,
								connectY,
								_colors[RANGE_METER]);
			}
			range->unlock();
		}
	}

	// Draw damage indicator.
	srfFrame = sstInticon->getFrame(_craft->getRules()->getSprite() + 11);
	srfFrame->blit(_damage);

	_craftDamageAnimTimer->onTimer((StateHandler)& DogfightState::animateCraftDamage);

	if (_ufo->getEscapeCountdown() == 0) // UFO is *not* engaged already in a different dogfight/Intercept slot.
	{
		_ufo->setFireCountdown(0); // UFO is ready to Fire pronto.

		int escape = _ufo->getRules()->getEscapeTime();
		escape += RNG::generate(
							0,
							escape);
//		escape -= _diff * 30;
		escape /= _diff + 1;

		if (escape < 1) escape = 1;

		_ufo->setEscapeCountdown(escape);
	}

	if (_craft->getRules()->getWeapons() > 0
		&& _craft->getWeapons()->at(0) != NULL)
	{
		_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getStandardReload();
		_weapon1->onMouseClick((ActionHandler)& DogfightState::weapon1Click);
	}
	else
	{
		_weapon1->setVisible(false);
		_range1->setVisible(false);
		_txtAmmo1->setVisible(false);
	}

	if (_craft->getRules()->getWeapons() > 1
		&& _craft->getWeapons()->at(1) != NULL)
	{
		_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getStandardReload();
		_weapon2->onMouseClick((ActionHandler)& DogfightState::weapon2Click);
	}
	else
	{
		_weapon2->setVisible(false);
		_range2->setVisible(false);
		_txtAmmo2->setVisible(false);
	}


	// Get the craft-graphic's height. Used for damage indication.
	Uint8 testColor;
	bool isCraftColor;

	for (int
			y = 0;
			y != _damage->getHeight();
			++y)
	{
		testColor = _damage->getPixelColor(11, y);
		isCraftColor = testColor > _colors[CRAFT_MIN] - 1
					&& testColor < _colors[CRAFT_MAX];

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

	drawCraftDamage(true);
}

/**
 * Cleans up this DogfightState.
 */
DogfightState::~DogfightState()
{
	_geo->resetTimer();

	delete _craftDamageAnimTimer;

	while (_projectiles.empty() == false)
	{
		delete _projectiles.back();
		_projectiles.pop_back();
	}

	if (_craft != NULL)
		_craft->setInDogfight(false);

	if (_ufo != NULL) // frees the ufo for the next engagement
		_ufo->setTimerTicked(false);
}

/**
 * Runs the higher level dogfight functionality.
 */
void DogfightState::think()
{
	if (_endDogfight == false)
	{
		updateDogfight();
		_craftDamageAnimTimer->think(this, NULL);

		if (_endDogfight == false //_timeout == 0 && // appears to be a safety.
			&& (_craft->getDestination() != _ufo
				|| _ufo->getUfoStatus() == Ufo::LANDED
				|| _craft->isInDogfight() == false))
		{
			//std::string st1 = _craft->getRules()->getType();
			//std::ostringstream oststr;
			//oststr << st1 << "-" << (_craft->getId()) << " to= " << _timeout;
			//Log(LOG_INFO) << "df Think END " << oststr.str().c_str();

			endDogfight();
		}
	}
}

/**
 * Animates interceptor damage by changing the color and redrawing the image.
 */
void DogfightState::animateCraftDamage()
{
	if (_minimized == false)
		drawCraftDamage();
//	{
//	--_currentCraftDamageColor;
//	if (_currentCraftDamageColor < _colors[DAMAGE_MIN]) // yellow
//		_currentCraftDamageColor = _colors[DAMAGE_MAX]; // black
//	}
}

/**
 * Draws interceptor damage according to percentage of HPs left.
 * @param init - true if called from cTor (default false)
 */
void DogfightState::drawCraftDamage(bool init)
{
	const int pct = _craft->getCraftDamagePct();
	if (pct > 0)
	{
		if (_craftDamageAnimTimer->isRunning() == false)
		{
			_craftDamageAnimTimer->start();
//			_currentCraftDamageColor = _colors[DAMAGE_MIN];
		}

		int rowsToColor = static_cast<int>(std::floor(
						  static_cast<double>(_craftHeight * pct) / 100.));

		if (rowsToColor == 0)
			return;
		else if (pct > 99)
			++rowsToColor;

		Uint8
			color,
			testColor;

		if (init == true)
			color = _colors[DAMAGE_MAX];
		else
			color = _colors[DAMAGE_MIN];

		for (int
				y = _craftHeight_pre;
				y != _craftHeight_pre + rowsToColor;
				++y)
		{
			for (int
					x = 1;
					x != 23;
					++x)
			{
				testColor = _damage->getPixelColor(x,y);

				if (testColor > _colors[CRAFT_MIN] - 1
					&& testColor < _colors[CRAFT_MAX])
				{
					_damage->setPixelColor(
										x,y,
										color);
				}
				else if (testColor == _colors[DAMAGE_MIN])
					_damage->setPixelColor(
										x,y,
										_colors[DAMAGE_MAX]);

/*				if ((testColor > _colors[DAMAGE_MIN] - 1
						&& testColor < _colors[DAMAGE_MAX] + 1)
					|| (testColor > _colors[CRAFT_MIN] - 1
						&& testColor < _colors[CRAFT_MAX]))
				{
					_damage->setPixelColor(
										x,y,
										_currentCraftDamageColor);
				} */
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
			x != _window->getWidth();
			++x)
	{
		for (int
				y = 0;
				y != _window->getHeight();
				++y)
		{
			Uint8 color = _window->getPixelColor(x,y);
			if (color > _colors[RADAR_MIN] - 1
				&& color < _colors[RADAR_MAX])
			{
				++color;
				if (color > _colors[RADAR_MAX] - 1)
					color = _colors[RADAR_MIN];

				_window->setPixelColor(
									x,y,
									color);
			}
		}
	}

	_battleScope->clear();

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


	bool finalHitFrame = false;
	if (_animatingHit == true) // animate UFO hit
	{
		int hitFrame = _ufo->getHitFrame();
		if (hitFrame != 0)
		{
			_ufo->setHitFrame(--hitFrame);
			if (hitFrame == 0)
			{
				_animatingHit = false;
				finalHitFrame = true;
			}
		}
	}

	if (_ufo->isCrashed() == true // animate UFO crash landing
		&& _ufo->getHitFrame() == 0
		&& finalHitFrame == false)
	{
		--_ufoSize;
	}
}

/**
 * Updates all the elements in the dogfight.
 * @note Includes ufo movement, weapons fire, projectile movement, ufo escape
 * conditions, craft and ufo destruction conditions, and retaliation mission
 * generation as applicable.
 */
void DogfightState::updateDogfight()
{
	bool finalRun = false;

	if (_endDogfight == false										// This runs for Craft that *do not* get the KILL. uhhh
		&& (_ufo != dynamic_cast<Ufo*>(_craft->getDestination())	// check if Craft's destination has changed
			|| _craft->getLowFuel() == true							// check if Craft is low on fuel
			|| (_timeout == 0 && _ufo->isCrashed() == true)			// check if UFO has been shot down
			|| _craft->isInDogfight() == false))
	{
		//std::string st1 = _craft->getRules()->getType();
		//std::ostringstream oststr;
		//oststr << st1 << "-" << (_craft->getId()) << " to= " << _timeout;
		//Log(LOG_INFO) << "df Update END [1] " << oststr.str().c_str();

		endDogfight();
		return;
	}

	if (_minimized == false)
	{
		animate();

		if (_ufo->isCrashed() == false
			&& _craft->isDestroyed() == false
			&& _ufo->getTimerTicked() == false)
		{
			_ufo->setTimerTicked();

			int escapeTicks = _ufo->getEscapeCountdown();
			if (escapeTicks > 0)
			{
				_geo->drawUfoBlobs();

				if (_dist < DST_STANDOFF)
					_ufo->setEscapeCountdown(--escapeTicks);

				if (escapeTicks == 0) // UFO is breaking off.
					_ufo->setSpeed(_ufo->getRules()->getMaxSpeed());
			}

			const int fireTicks = _ufo->getFireCountdown();
			if (fireTicks > 0)
				_ufo->setFireCountdown(fireTicks - 1);
		}
	}

	if (_ufo->getSpeed() > _craft->getRules()->getMaxSpeed()) // Crappy craft is chasing UFO.
	{
		_ufoBreakingOff = true;
		finalRun = true;
		resetStatus("STR_UFO_OUTRUNNING_INTERCEPTOR");

		if (_geo->getDfCCC() == false) // should need to run this only once per.
		{
			int qtyCraftVsUfo = 0;
			for (std::list<DogfightState*>::const_iterator
					i = _geo->getDogfights().begin();
					i != _geo->getDogfights().end();
					++i)
			{
				if ((*i)->getUfo() == _ufo
					&& (*i)->getCraft()->isDestroyed() == false)
				{
					++qtyCraftVsUfo;
				}
			}

			if (qtyCraftVsUfo == 1)
				_geo->setDfCCC(
							_craft->getLongitude(),
							_craft->getLatitude());
		}
	}
	else // UFO cannot break off because it's crappier than the crappy craft.
		_ufoBreakingOff = false;


	bool prjInFlight = false;

	if (_minimized == false)
	{
		int delta; // Update distance.
		const int accel = (_craft->getRules()->getAcceleration()
						- _ufo->getRules()->getAcceleration()) / 2;

		if (_ufoBreakingOff == false)
		{
			if (_dist != _desired)
			{
				if (_ufo->isCrashed() == false
					&& _craft->isDestroyed() == false)
				{
					delta = std::max(
								2,
								8 + accel);

					if (_dist < _desired)		// Craft vs UFO receding
					{
						if (_dist + delta > _desired)
							delta = _desired - _dist;
					}
					else if (_dist > _desired)	// Craft vs UFO closing
						delta = -delta;
				}
				else
					delta = 0;

				for (std::vector<CraftWeaponProjectile*>::const_iterator
						i = _projectiles.begin();
						i != _projectiles.end();
						++i)
				{
					if ((*i)->getGlobalType() != PGT_BEAM
						&& (*i)->getDirection() == PD_UP)
					{
//						(*i)->setPosition((*i)->getPosition() + delta);	// Don't let the interceptor mystically push or pull its fired projectiles. So to speak ....
						if (delta > 0)
							(*i)->setRange((*i)->getRange() + delta);	// Also don't let interceptor mystically push or pull its fired projectiles.
					}
				}
			}
			else
				delta = 0;
		}
		else // _ufoBreakingOff== true
		{
			delta = std::max(6, 12 + accel);
			// UFOs can try to outrun the missiles; don't adjust projectile positions here.
			// If UFOs ever fire anything but beams those positions need to be adjusted here though.
		}

		_dist += delta;

		std::wostringstream woststr;
		woststr << _dist;
		_txtDistance->setText(woststr.str());

		for (std::vector<CraftWeaponProjectile*>::const_iterator // Move projectiles and check for hits.
				i = _projectiles.begin();
				i != _projectiles.end();
				++i)
		{
			(*i)->moveProjectile();
			int
				hitprob,
				power;

			if ((*i)->getDirection() == PD_UP) // Projectiles fired by interceptor.
			{
				if (((*i)->getPosition() >= _dist // Projectile reached the UFO - determine if it's been hit.
						|| ((*i)->getGlobalType() == PGT_BEAM
							&& (*i)->toBeRemoved() == true))
					&& _ufo->isCrashed() == false
					&& (*i)->getMissed() == false)
				{
					hitprob = (*i)->getAccuracy(); // Could include UFO speed here ...
					hitprob += _ufoSize * 3;
					hitprob -= _diff * 5;
					hitprob += _craft->getKills() * 2;

					//Log(LOG_INFO) << "df: Craft pType = " << (*i)->getType() << " hp = " << hitprob;
					if (RNG::percent(hitprob) == true)
					{
						power = RNG::generate(
										((*i)->getPower() + 1) / 2, // Round up.
										(*i)->getPower());
						if (power != 0)
						{
							_ufo->setUfoDamage(_ufo->getUfoDamage() + power);

							if (_ufo->isCrashed() == true)
							{
								_ufo->setShotDownByCraftId(_craft->getUniqueId());
								_ufo->setSpeed(0);
								_craft->addKill();

								_ufoBreakingOff = // if the ufo got shotdown here these no longer apply ->
								_end =
								finalRun = false;
							}

							if (_ufo->getHitFrame() == 0)
							{
								_animatingHit = true;
								_ufo->setHitFrame(3);
							}

							resetStatus("STR_UFO_HIT");
							(*i)->removeProjectile();

							_game->getResourcePack()->playSoundFX(
															ResourcePack::UFO_HIT,
															true);
						}
					}
					else // Missed.
					{
						if ((*i)->getGlobalType() == PGT_BEAM)
							(*i)->removeProjectile();
						else
							(*i)->setMissed(true);
					}
				}

				if ((*i)->getGlobalType() == PGT_MISSILE) // Check if projectile passed its maximum range.
				{
					if (((*i)->getPosition() / 8) >= (*i)->getRange())
						(*i)->removeProjectile();
					else if (_ufo->isCrashed() == false)
						prjInFlight = true;
				}
			}
			else if ((*i)->getDirection() == PD_DOWN) // Projectiles fired by UFO.
			{
				if ((*i)->getGlobalType() == PGT_MISSILE
					|| ((*i)->getGlobalType() == PGT_BEAM
						&& (*i)->toBeRemoved() == true))
				{
					hitprob = (*i)->getAccuracy();

					if (_craftStance == _btnCautious
						|| _craftStance == _btnStandoff
						|| _craftStance == _btnDisengage)
					{
						hitprob -= 20;
					}
					else if (_craftStance == _btnAggressive)
						hitprob += 15;

					hitprob -= _craft->getKills();

					//Log(LOG_INFO) << "df: UFO pType = " << (*i)->getType() << " hp = " << hitprob;
					if (RNG::percent(hitprob) == true)
					{
						power = RNG::generate(
										(_ufo->getRules()->getWeaponPower() + 9) / 10, // Round up.
										_ufo->getRules()->getWeaponPower());
						if (power != 0)
						{
							_craft->setCraftDamage(_craft->getCraftDamage() + power);

							drawCraftDamage();
							resetStatus("STR_INTERCEPTOR_DAMAGED");
							_game->getResourcePack()->playSoundFX(
															ResourcePack::INTERCEPTOR_HIT,
															true);

							if (_ufo->isCrashed() == false
								&& _craft->isDestroyed() == false
								&& _ufoBreakingOff == false
								&& ((_cautionLevel > 1
										&& _craftStance == _btnCautious
										&& _craft->getCraftDamagePct() > 35)
									|| (_cautionLevel > 0
										&& _craftStance == _btnStandard
										&& _craft->getCraftDamagePct() > 60)))
							{
								if (_craftStance == _btnCautious)
									_cautionLevel = 1;
								else
									_cautionLevel = 0;

								_btnStandoff->releaseButtonGroup();

								_end = false;
								resetStatus("STR_STANDOFF");
								_desired = DST_STANDOFF;
							}
						}
					}

					(*i)->removeProjectile();
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

		const CraftWeapon* cw;
		for (size_t // Handle weapons and craft distance.
				i = 0;
				i != static_cast<size_t>(_craft->getRules()->getWeapons());
				++i)
		{
			if ((cw = _craft->getWeapons()->at(i)) != NULL)
			{
				int fireCountdown;
				if (i == 0)
					fireCountdown = _w1FireCountdown;
				else
					fireCountdown = _w2FireCountdown;

				if (fireCountdown == 0 // Handle weapon firing.
					&& _dist <= cw->getRules()->getRange() * 8
					&& cw->getAmmo() > 0
					&& _craftStance != _btnStandoff
					&& _craftStance != _btnDisengage
					&& _ufo->isCrashed() == false
					&& _craft->isDestroyed() == false)
				{
					if (i == 0)
					{
						if (_w1Enabled == true)
							fireWeapon1();
					}
					else
					{
						if (_w2Enabled == true)
							fireWeapon2();
					}
				}
				else if (fireCountdown > 0)
				{
					if (i == 0)
						--_w1FireCountdown;
					else
						--_w2FireCountdown;
				}

				if (cw->getAmmo() == 0 // Handle craft distance according to option set by user and available ammo.
					&& prjInFlight == false
					&& _craft->isDestroyed() == false)
				{
					if (_craftStance == _btnCautious)
						maximumDistance();
					else if (_craftStance == _btnStandard)
						minimumDistance();
				}
			}
		}

		const int ufoWRange = _ufo->getRules()->getWeaponRange() * 8; // Handle UFO firing.

		if (_ufo->isCrashed() == true
			|| (_ufo->getShootingAt() == _slot // this Craft out of range and/or destroyed.
				&& (_dist > ufoWRange
					|| _craft->isDestroyed() == true)))
		{
			_ufo->setShootingAt(0);
		}
		else if (_ufo->isCrashed() == false // UFO is gtg.
			&& _ufo->getFireCountdown() == 0
			&& (_ufo->getShootingAt() == 0
				|| _ufo->getShootingAt() == _slot))
		{
			if (_dist <= ufoWRange
				 && _craft->isDestroyed() == false)
			{
				std::vector<size_t> altIntercepts; // Randomize UFO's target.

				for (std::list<DogfightState*>::const_iterator
						i = _geo->getDogfights().begin();
						i != _geo->getDogfights().end();
						++i)
				{
					if (*i != this // target can be either '_slot' OR in 'altIntercept' but not both.
						&& (*i)->isMinimized() == false
						&& (*i)->getDistance() <= ufoWRange
						&& (*i)->getCraft()->isDestroyed() == false
						&& (*i)->getUfo() == _ufo)
					{
						altIntercepts.push_back((*i)->getInterceptSlot());
					}
				}

				if (altIntercepts.size() == 0) // this->craft.
				{
					_ufo->setShootingAt(_slot);
					ufoFireWeapon();
				}
				else //if (altIntercepts.size() > 0)
				{
					int noSwitch = static_cast<int>(Round(100. / static_cast<double>(altIntercepts.size() + 1))); // +1 for this->craft.

					if (_ufo->getShootingAt() == _slot)
						noSwitch += 18; // arbitrary increase for UFO to continue shooting at this->craft.

					if (RNG::percent(noSwitch) == true)
					{
						_ufo->setShootingAt(_slot);
						ufoFireWeapon();
					}
					else // This is where the magic happens, Lulzor!!
					{
						const size_t pick = static_cast<size_t>(RNG::generate(0,
											static_cast<int>(altIntercepts.size()) - 1));
						_ufo->setShootingAt(altIntercepts.at(pick));
					}
				}
			}
			else
				_ufo->setShootingAt(0);
		}
	}


	if (_end == true // dogfight is over. This runs for Craft that gets the KILL. uhhh
		&& ((_timeout == 0 && _ufo->isCrashed() == true)
			|| ((_minimized == true || _dist > DST_ENGAGE)
				&& (_ufoBreakingOff == true || _craftStance == _btnDisengage))))
	{
		if (_ufoBreakingOff == true)
		{
			_ufo->moveTarget();
			_craft->setDestination(_ufo);
		}

		if (_destroyCraft == false
			&& (_destroyUfo == true || _craftStance == _btnDisengage))
		{
			_craft->returnToBase();
		}

		if (_endDogfight == false)
		{
			//std::string st1 = _craft->getRules()->getType();
			//std::ostringstream oststr;
			//oststr << st1 << "-" << (_craft->getId()) << " to= " << _timeout;
			//Log(LOG_INFO) << "df Update END [2] " << oststr.str().c_str();

			endDogfight();
		}
	}

	if (_ufoBreakingOff == true && _dist > DST_ENGAGE)
		finalRun = true;


	if (_end == false)
	{
		if (_craft->isDestroyed() == true) // End dogfight if craft is destroyed.
		{
			resetStatus("STR_INTERCEPTOR_DESTROYED");
			_timeout *= 2;
			_game->getResourcePack()->playSoundFX(ResourcePack::INTERCEPTOR_EXPLODE);

			finalRun =
			_destroyCraft = true;
			_ufo->setShootingAt(0);
		}
		else if (_ufo->isCrashed() == true) // End dogfight if UFO is crashed or destroyed.
		{
			_ufo->getAlienMission()->ufoShotDown(*_ufo);

			const double
				lon = _ufo->getLongitude(),
				lat = _ufo->getLatitude();

			if (_ufo->getTrajectory().getId() != UfoTrajectory::RETALIATION_ASSAULT_RUN) // shooting down an assault-battleship does *not* generate a Retal-Mission.
			{
				int retalCoef;
				if (_ufo->getAlienMission()->getRetaliation() != -1)
					retalCoef = _ufo->getAlienMission()->getRules().getRetaliation();
				else
					retalCoef = _game->getRuleset()->getRetaliation();

				if (RNG::percent((_diff + 1) * retalCoef) == true) // Check retaliation trigger. -> Spawn retaliation mission.
				{
					std::string targetRegion;
					if (RNG::percent(_diff * 10 + 10) == true)
						targetRegion = _gameSave->locateRegion(*_craft->getBase())->getRules()->getType();	// Try to find the originating base.
						// TODO: If the base is removed, the mission is cancelled. nah.
					else if (RNG::generate(0,1) == 0)
						targetRegion = _ufo->getAlienMission()->getRegion();								// Retaliation vs UFO's mission region.
					else
						targetRegion = _gameSave->locateRegion(lon,lat)->getRules()->getType();				// Retaliation vs UFO's shootdown region.

					// Difference from original: No retaliation until final UFO lands (Original: Is spawned).
					if (_game->getSavedGame()->findAlienMission(targetRegion, alm_RETAL) == false)
					{
						const RuleAlienMission& missionRule = *_game->getRuleset()->getRandomMission(
																								alm_RETAL,
																								_game->getSavedGame()->getMonthsPassed());
						AlienMission* const mission = new AlienMission(missionRule, *_gameSave);
						mission->setId(_gameSave->getCanonicalId("ALIEN_MISSIONS"));
						mission->setRegion(
										targetRegion,
										*_game->getRuleset());
						mission->setRace(_ufo->getAlienRace());
						mission->start(mission->getRules().getWave(0).spawnTimer); // delay for first wave/scout

						_gameSave->getAlienMissions().push_back(mission);
					}
				}
			}

			int pts = 0;

			if (_ufo->isDestroyed() == true)
			{
				_destroyUfo = true;

				if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
				{
					resetStatus("STR_UFO_DESTROYED");
					_game->getResourcePack()->playSoundFX(ResourcePack::UFO_EXPLODE);

					pts = _ufo->getRules()->getScore() * 2;
				}
			}
			else if (_ufo->getShotDownByCraftId() == _craft->getUniqueId()) // crashed.
			{
				resetStatus("STR_UFO_CRASH_LANDS");
				_game->getResourcePack()->playSoundFX(ResourcePack::UFO_CRASH);

				pts = _ufo->getRules()->getScore();

				if (_globe->insideLand(lon,lat) == false)
				{
					_destroyUfo = true;
					_ufo->setUfoStatus(Ufo::DESTROYED);

					pts *= 2;
				}
				else if (_ufo->getCrashId() == 0) // Set up Crash site.
				{
					_ufo->setCrashId(_gameSave->getCanonicalId("STR_CRASH_SITE"));

					_ufo->setSecondsLeft(RNG::generate(24,96) * 3600);
					_ufo->setAltitude("STR_GROUND");
				}
			}

			if (pts != 0)
				_gameSave->scorePoints(lon, lat, pts, false);

			if (_ufo->getShotDownByCraftId() != _craft->getUniqueId())
				_ufo->setHitFrame(3);
			else
			{
				for (std::list<DogfightState*>::const_iterator
					i = _geo->getDogfights().begin();
					i != _geo->getDogfights().end();
					++i)
				{
					if (*i != this
						&& (*i)->getUfo() == _ufo
						&& (*i)->isMinimized() == false)
					{
						(*i)->setTimeout(MSG_TIMEOUT); // persist other port(s).
					}
				}

				_timeout *= 2; // persist this port twice normal duration.
			}

			finalRun = true;
		}
		else if (_ufo->getUfoStatus() == Ufo::LANDED)
		{
			_timeout *= 2;
			finalRun = true;
			_ufo->setShootingAt(0);
		}
	}


	if (prjInFlight == false && finalRun == true)
		_end = true; // prevent further Craft/UFO destruction; send existing Craft/UFO on their way and flag dogfight for deletion.
}

/**
 * Fires a shot from the first weapon equipped on the Craft.
 */
void DogfightState::fireWeapon1()
{
	CraftWeapon* const cw = _craft->getWeapons()->at(0);
	if (cw->setAmmo(cw->getAmmo() - 1))
	{
		_w1FireCountdown = _w1FireInterval;
		if (_w1FireCountdown < 1) _w1FireCountdown = 1;

		std::wostringstream woststr;
		woststr << cw->getAmmo();
		_txtAmmo1->setText(woststr.str());

		CraftWeaponProjectile* const prj = cw->fire();
		prj->setDirection(PD_UP);
		prj->setHorizontalPosition(PH_LEFT);
		_projectiles.push_back(prj);

		_game->getResourcePack()->playSoundFX(
										cw->getRules()->getSound(),
										true);
	}
}

/**
 * Fires a shot from the second weapon equipped on the Craft.
 */
void DogfightState::fireWeapon2()
{
	CraftWeapon* const cw = _craft->getWeapons()->at(1);
	if (cw->setAmmo(cw->getAmmo() - 1))
	{
		_w2FireCountdown = _w2FireInterval;
		if (_w2FireCountdown < 1) _w2FireCountdown = 1;

		std::wostringstream woststr;
		woststr << cw->getAmmo();
		_txtAmmo2->setText(woststr.str());

		CraftWeaponProjectile* const prj = cw->fire();
		prj->setDirection(PD_UP);
		prj->setHorizontalPosition(PH_RIGHT);
		_projectiles.push_back(prj);

		_game->getResourcePack()->playSoundFX(
										cw->getRules()->getSound(),
										true);
	}
}

/**
 * Each time a UFO fires its weapon a new reload interval is calculated.
 */
void DogfightState::ufoFireWeapon()
{
	resetStatus("STR_UFO_RETURN_FIRE");

	CraftWeaponProjectile* const prj = new CraftWeaponProjectile();
	prj->setType(PT_PLASMA_BEAM);
	prj->setAccuracy(60);
	prj->setPower(_ufo->getRules()->getWeaponPower());
	prj->setDirection(PD_DOWN);
	prj->setHorizontalPosition(PH_CENTER);
	prj->setPosition(_dist - (_ufo->getRules()->getRadius() / 2));
	_projectiles.push_back(prj);

	_game->getResourcePack()->playSoundFX(
									ResourcePack::UFO_FIRE,
									true);


	int reload = _ufo->getRules()->getWeaponReload();
	reload += RNG::generate(
						0,
						reload / 2);
	reload -= _diff * 2;

	if (reload < 1) reload = 1;

	_ufo->setFireCountdown(reload);
}

/**
 * Sets the craft to the maximum distance required to fire a weapon.
 */
void DogfightState::maximumDistance()
{
	int dist = 0;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _craft->getWeapons()->begin();
			i != _craft->getWeapons()->end();
			++i)
	{
		if (*i != NULL
			&& (*i)->getAmmo() != 0
			&& (*i)->getRules()->getRange() > dist)
		{
			dist = (*i)->getRules()->getRange();
		}
	}

	if (dist == 0)
		_desired = DST_STANDOFF;
	else
		_desired = dist * 8;
}

/**
 * Sets the craft to the minimum distance required to fire a weapon.
 */
void DogfightState::minimumDistance()
{
	int dist = 1000;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _craft->getWeapons()->begin();
			i != _craft->getWeapons()->end();
			++i)
	{
		if (*i != NULL
			&& (*i)->getAmmo() != 0
			&& (*i)->getRules()->getRange() < dist)
		{
			dist = (*i)->getRules()->getRange();
		}
	}

	if (dist == 1000)
		_desired = DST_STANDOFF;
	else
		_desired = dist * 8;
}

/**
 * Updates the status text and restarts the message-timeout counter.
 * @param status - reference to status
 */
void DogfightState::resetStatus(const std::string& status) // private.
{
	_txtStatus->setText(tr(status));
	_timeout = MSG_TIMEOUT;
}
// statii ->
// STR_INTERCEPTOR_DAMAGED
// STR_INTERCEPTOR_DESTROYED
// STR_UFO_OUTRUNNING_INTERCEPTOR
// STR_UFO_HIT
// STR_UFO_DESTROYED
// STR_UFO_CRASH_LANDS
// STR_UFO_RETURN_FIRE
// STR_STANDOFF
// STR_CAUTIOUS_ATTACK
// STR_STANDARD_ATTACK
// STR_AGGRESSIVE_ATTACK
// STR_DISENGAGING
// STR_STANDOFF_RANGE_ONLY

/**
 * Puts craft in standoff-stance if engaged.
 * @note If already in standoff it tries to minimize all dogfights.
 */
void DogfightState::keyEscape(Action*)
{
	if (_craftStance != _btnStandoff)
	{
		_btnStandoff->releaseButtonGroup();
		btnStandoffClick(NULL);
	}
	else if (_minimized == false)
	{
		bool miniAll = true;
		for (std::list<DogfightState*>::const_iterator
				i = _geo->getDogfights().begin();
				i != _geo->getDogfights().end();
				++i)
		{
			if ((*i)->isStandingOff() == false) //_dist >= DST_STANDOFF
			{
				miniAll = false;
				break;
			}
		}

		if (miniAll == true)
			btnMinimizeDfClick(NULL);
	}
}

/**
 * Switches to Standoff mode - maximum range.
 * @param action - pointer to an Action
 */
void DogfightState::btnStandoffClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		resetStatus("STR_STANDOFF");
		_desired = DST_STANDOFF;
	}
}

/**
 * Switches to Cautious mode - maximum weapon range.
 * @param action - pointer to an Action
 */
void DogfightState::btnCautiousClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		resetStatus("STR_CAUTIOUS_ATTACK");

		if (_craft->getRules()->getWeapons() > 0
			&& _craft->getWeapons()->at(0) != NULL)
		{
			_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getCautiousReload();
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getCautiousReload();
		}

		maximumDistance();
	}
}

/**
 * Switches to Standard mode - minimum weapon range.
 * @param action - pointer to an Action
 */
void DogfightState::btnStandardClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		resetStatus("STR_STANDARD_ATTACK");

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

		minimumDistance();
	}
}

/**
 * Switches to Aggressive mode - minimum range.
 * @param action - pointer to an Action
 */
void DogfightState::btnAggressiveClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = false;
		resetStatus("STR_AGGRESSIVE_ATTACK");

		if (_craft->getRules()->getWeapons() > 0
			&& _craft->getWeapons()->at(0) != NULL)
		{
			_w1FireInterval = _craft->getWeapons()->at(0)->getRules()->getAggressiveReload();
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getAggressiveReload();
		}

		_desired = DST_CLOSE;
	}
}

/**
 * Disengages from the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::btnDisengageClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		_end = true;
		resetStatus("STR_DISENGAGING");
		_desired = DST_ENGAGE + 10;
	}

	if (_geo->getMinimizedDfCount() == _totalIntercepts - 1)
	{
		if (_geo->getDfZoomInTimer()->isRunning() == true)
			_geo->getDfZoomInTimer()->stop();

		if (_geo->getDfZoomOutTimer()->isRunning() == false)
			_geo->getDfZoomOutTimer()->start();
	}
}

/**
 * Shows a front view of the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::btnUfoClick(Action*)
{
	_previewUfo->setVisible();

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
void DogfightState::previewClick(Action*)
{
	_previewUfo->setVisible(false);

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
 * Minimizes the dogfight window.
 * @param action - pointer to an Action
 */
void DogfightState::btnMinimizeDfClick(Action*)
{
	_geo->resetTimer();

	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		if (_dist >= DST_STANDOFF)
		{
			if (_projectiles.empty() == true)
			{
				_minimized = true;

				_window->setVisible(false);
				_previewUfo->setVisible(false);
				_btnStandoff->setVisible(false);
				_btnCautious->setVisible(false);
				_btnStandard->setVisible(false);
				_btnAggressive->setVisible(false);
				_btnDisengage->setVisible(false);
				_btnUfo->setVisible(false);
				_texture->setVisible(false);
				_btnMinimize->setVisible(false);
				_battleScope->setVisible(false);
				_weapon1->setVisible(false);
				_range1->setVisible(false);
				_weapon2->setVisible(false);
				_range2->setVisible(false);
				_damage->setVisible(false);
				_txtAmmo1->setVisible(false);
				_txtAmmo2->setVisible(false);
				_txtDistance->setVisible(false);
				_previewUfo->setVisible(false);
				_txtStatus->setVisible(false);
				_txtTitle->setVisible(false);

				_btnMinimizedIcon->setVisible();
				_txtInterception->setVisible();

				if (_geo->getMinimizedDfCount() == _totalIntercepts)
				{
					if (_geo->getDfZoomInTimer()->isRunning() == true)
						_geo->getDfZoomInTimer()->stop();

					if (_geo->getDfZoomOutTimer()->isRunning() == false)
						_geo->getDfZoomOutTimer()->start();
				}
				else
					_geo->resetInterceptPorts();
			}
			else
				resetStatus("STR_PROJECTILE_IN_FLIGHT");
		}
		else
			resetStatus("STR_STANDOFF_RANGE_ONLY");
	}
}

/**
 * Maximizes the dogfight window.
 * @param action - pointer to an Action
 */
void DogfightState::btnMaximizeDfPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) // note that this includes keyboard press for whatever reason.
	{
		_texture->clear();

		Surface* const srfTexture = _game->getResourcePack()->getSurface(getTextureIcon());
		if (srfTexture != NULL)
			srfTexture->blit(_texture);
		else Log(LOG_WARNING) << "Texture icon for dogfight not available.";

		_minimized = false;

		_window->setVisible();
		_btnStandoff->setVisible();
		_btnCautious->setVisible();
		_btnStandard->setVisible();
		_btnAggressive->setVisible();
		_btnDisengage->setVisible();
		_btnUfo->setVisible();
		_texture->setVisible();
		_btnMinimize->setVisible();
		_battleScope->setVisible();
		_weapon1->setVisible();
		_range1->setVisible();
		_weapon2->setVisible();
		_range2->setVisible();
		_damage->setVisible();
		_txtAmmo1->setVisible();
		_txtAmmo2->setVisible();
		_txtDistance->setVisible();
		_txtStatus->setVisible();
		_txtTitle->setVisible();

		_btnMinimizedIcon->setVisible(false);
		_txtInterception->setVisible(false);
		_previewUfo->setVisible(false);

		_geo->resetInterceptPorts();

		if (_geo->getDfZoomOutTimer()->isRunning() == true)
			_geo->getDfZoomOutTimer()->stop();

		if (_geo->getMinimizedDfCount() == _totalIntercepts - 1)
			_geo->storePreDfCoords();

		_globe->center(
					_craft->getLongitude(),
					_craft->getLatitude());

		if (_geo->getDfZoomInTimer()->isRunning() == false)
			_geo->getDfZoomInTimer()->start();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		_game->pushState(new GeoscapeCraftState(
											_craft,
											_geo));
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
 * Returns true if Craft stance is in stand-off.
 * @return, true if standing off
 */
bool DogfightState::isStandingOff() const
{
	return (_craftStance == _btnStandoff);
}

/**
 * Draws the UFO blob on the radar screen.
 * @note Currently works only for original sized blobs 13 x 13 pixels.
 */
void DogfightState::drawUfo()
{
	if (_ufoSize > -1) //&& _ufo->isDestroyed() == false
	{
		const int
			ufo_x = _battleScope->getWidth() / 2 - 6,
			ufo_y = _battleScope->getHeight() - (_dist / 8) - 6;
		Uint8 color;

		for (int
				y = 0;
				y != 13;
				++y)
		{
			for (int
					x = 0;
					x != 13;
					++x)
			{
				color = static_cast<Uint8>(GeoscapeState::_ufoBlobs[static_cast<size_t>(_ufoSize + _ufo->getHitFrame())]
																   [static_cast<size_t>(y)]
																   [static_cast<size_t>(x)]);
				if (color != 0)
				{
					if (_ufo->isCrashed() == true || _ufo->getHitFrame() != 0)
						color *= 2;

					color = _window->getPixelColor(
												ufo_x + x + 3,
												ufo_y + y + 3) - color;
					if (color < _colors[BLOB_MIN])
						color = _colors[BLOB_MIN];

					_battleScope->setPixelColor(
											ufo_x + x,
											ufo_y + y,
											color);
				}
			}
		}
	}
}

/**
 * Draws projectiles on the radar screen.
 * @note Its shape will be different depending on what type of projectile it is.
 * @note Currently works for original sized blobs 3 x 6 pixels.
 * @param prj - pointer to CraftWeaponProjectile
 */
void DogfightState::drawProjectile(const CraftWeaponProjectile* const prj)
{
	int posX = (_battleScope->getWidth() / 2) + (prj->getHorizontalPosition() * 3);
	Uint8
		color,
		colorOffset;

	if (prj->getGlobalType() == PGT_MISSILE) // Draw missiles.
	{
		--posX;
		const int posY = _battleScope->getHeight() - prj->getPosition() / 8;
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
				colorOffset = static_cast<Uint8>(_projectileBlobs[static_cast<size_t>(prj->getType())]
																 [static_cast<size_t>(y)]
																 [static_cast<size_t>(x)]);
				if (colorOffset != 0)
				{
					color = _window->getPixelColor(
												posX + x + 3,
												posY + y + 3); // +3 'cause of the window frame
					color -= colorOffset;
					if (color < _colors[BLOB_MIN])
						color = _colors[BLOB_MIN];

					_battleScope->setPixelColor(
											posX + x,
											posY + y,
											color);
				}
			}
		}
	}
	else if (prj->getGlobalType() == PGT_BEAM) // Draw beams.
	{
		colorOffset = static_cast<Uint8>(prj->getBeamPhase());
		const int
			stop = _battleScope->getHeight() - 2,
			start = _battleScope->getHeight() - (_dist / 8);

		for (int
				y = stop;
				y != start;
				--y)
		{
			if (prj->getDirection() == PD_UP)
			{
				color = _window->getPixelColor(
											posX + 3,
											y + 3);
				color -= colorOffset;
				if (color < _colors[BLOB_MIN])
					color = _colors[BLOB_MIN];
			}
			else
				color = 128; // red

			_battleScope->setPixelColor(
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
	_w1Enabled = !_w1Enabled;
	recolor(0, _w1Enabled);
}

/**
 * Toggles usage of weapon number 2.
 * @param action - pointer to an Action
 */
void DogfightState::weapon2Click(Action*)
{
	_w2Enabled = !_w2Enabled;
	recolor(1, _w2Enabled);
}

/**
 * Changes the colors of craft's weapon icons, range indicators, and ammo texts
 * based on current weapon state.
 * @param hardPt - craft weapon to change colors of
 * @param enable - true if weapon is enabled
 */
void DogfightState::recolor(
		const int hardPt,
		const bool enable)
{
	InteractiveSurface* weapon;
	Text* ammo;
	Surface* range;

	if (hardPt == 0)
	{
		weapon = _weapon1;
		ammo = _txtAmmo1;
		range = _range1;
	}
	else if (hardPt == 1)
	{
		weapon = _weapon2;
		ammo = _txtAmmo2;
		range = _range2;
	}
	else
		return;

	if (enable == true)
	{
		weapon->offset(-_colors[DISABLED_WEAPON]);
		ammo->offset(-_colors[DISABLED_AMMO]);
		range->offset(-_colors[DISABLED_RANGE]);
	}
	else
	{
		weapon->offset(_colors[DISABLED_WEAPON]);
		ammo->offset(_colors[DISABLED_AMMO]);
		range->offset(_colors[DISABLED_RANGE]);
	}
}

/**
 * Gets this Dogfight's interception slot.
 * @return, interception number
 */
size_t DogfightState::getInterceptSlot() const
{
	return _slot;
}

/**
 * Sets this Dogfight's interception slot.
 * @param intercept - #ID
 */
void DogfightState::setInterceptSlot(const size_t intercept)
{
	_slot = intercept;
}

/**
 * Sets total interceptions count.
 * @note Used to properly position the window.
 * @param intercepts - amount of interception windows
 */
void DogfightState::setTotalIntercepts(const size_t intercepts)
{
	_totalIntercepts = intercepts;
}

/**
 * Calculates dogfight window position according to number of active interceptions.
 * @param dfOpen		- current iteration
 * @param dfOpenTotal	- total quantity of open dogfight windows
 */
void DogfightState::resetInterceptPort(
		size_t dfOpen,
		size_t dfOpenTotal)
{
	if (_slot > _totalIntercepts)
		_slot = _geo->getOpenDfSlot(); // not sure what this is doing anymore ...

	_minimizedIconX = 5;
	_minimizedIconY = (5 * _slot) + (16 * (_slot - 1));

	if (_minimized == false)
	{
		if (dfOpenTotal == 1)
		{
			_x = 80;
			_y = 52;
		}
		else if (dfOpenTotal == 2)
		{
			if (dfOpen == 1)
			{
				_x = 80;
				_y = 0;
			}
			else // 2
			{
				_x = 80;
				_y = 201 - _window->getHeight(); // 96;
			}
		}
		else if (dfOpenTotal == 3)
		{
			if (dfOpen == 1)
			{
				_x = 80;
				_y = 0;
			}
			else if (dfOpen == 2)
			{
				_x = 0;
				_y = 201 - _window->getHeight(); // 96;
			}
			else // 3
			{
				_x = 320 - _window->getWidth(); // 160;
				_y = 201 - _window->getHeight(); // 96;
			}
		}
		else // dfOpenTotal== 4
		{
			if (dfOpen == 1)
			{
				_x =
				_y = 0;
			}
			else if (dfOpen == 2)
			{
				_x = 320 - _window->getWidth(); // 160;
				_y = 0;
			}
			else if (dfOpen == 3)
			{
				_x = 0;
				_y = 201 - _window->getHeight(); // 96;
			}
			else // 4
			{
				_x = 320 - _window->getWidth(); // 160;
				_y = 201 - _window->getHeight(); // 96;
			}
		}

		_x += _game->getScreen()->getDX();
		_y += _game->getScreen()->getDY();

		placePort();
	}
}

/**
 * Relocates all dogfight window elements to calculated position.
 * @note This is used when multiple interceptions are running.
 */
void DogfightState::placePort() // private.
{
	const int
		x = _window->getX() - _x,
		y = _window->getY() - _y;

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() - x);
		(*i)->setY((*i)->getY() - y);
	}

	_btnMinimizedIcon->setX(_minimizedIconX);
	_btnMinimizedIcon->setY(_minimizedIconY);

	_txtInterception->setX(_minimizedIconX + 18);
	_txtInterception->setY(_minimizedIconY + 6);
}

/**
 * Ends the dogfight.
 */
void DogfightState::endDogfight() // private.
{
	_endDogfight = true;

	if (_craft != NULL)
		_craft->setInDogfight(false);

	// note: Might want to reset UFO's timer-ticker here also. See ~dTor.
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
 * Gets the UFO associated with this dogfight.
 * @return, pointer to UFO object associated with this dogfight
 */
Ufo* DogfightState::getUfo() const
{
	return _ufo;
}

/**
 * Sets the UFO associated with this dogfight to NULL.
 * @note Used when destructing GeoscapeState.
 */
void DogfightState::clearUfo()
{
	_ufo = NULL;
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
 * Sets pointer to the xCom Craft in this dogfight to NULL.
 * @note Used when destructing GeoscapeState.
 */
void DogfightState::clearCraft()
{
	_craft = NULL;
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
 * Gets the globe texture icon to display for the interception.
 * @note This does not return the actual battleField terrain; that is done in
 * ConfirmLandingState. This is merely an indicator .... cf. UfoDetectedState.
 * @return, string for the icon of the texture of the globe's surface under the dogfight ha!
 */
const std::string DogfightState::getTextureIcon() // private.
{
	int texture;
	_globe->getPolygonTexture(
						_ufo->getLongitude(),
						_ufo->getLatitude(),
						&texture);
	switch (texture)
	{
		case  0:
		case  3:
		case  6: return "FOREST"; // 6= JUNGLE

		case  1:
		case  2: return "CULTA";

		case  5: return "MOUNT";

		case  7:
		case  8: return "DESERT";

		case 10: return "URBAN";

		case  4:
		case  9:
		case 11:
		case 12: return "POLAR";
	}

	return "WATER"; // tex = -1
}

}
