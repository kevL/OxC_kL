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
		Globe* globe,
		Craft* craft,
		Ufo* ufo,
		GeoscapeState* geo)
	:
		_globe(globe),
		_craft(craft),
		_ufo(ufo),
		_geo(geo),
		_gameSave(_game->getSavedGame()),
		_diff(static_cast<int>(_game->getSavedGame()->getDifficulty())),
		_timeout(MSG_TIMEOUT),
		_dist(DST_ENGAGE),
		_targetDist(DST_STANDOFF),
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

	_battle					= new Surface(77, 74, _x +  3, _y +  3);
	_damage					= new Surface(22, 25, _x + 93, _y + 40);
	_range1					= new Surface(21, 74, _x + 19, _y +  3);
	_range2					= new Surface(21, 74, _x + 43, _y +  3);
	_weapon1				= new InteractiveSurface(15, 17, _x +  4, _y + 52);
	_weapon2				= new InteractiveSurface(15, 17, _x + 64, _y + 52);

	_btnMinimize			= new InteractiveSurface( 12, 12, _x, _y);
	_preview				= new InteractiveSurface(160, 96, _x, _y);

	_btnDisengage			= new ImageButton(36, 15, _x + 83, _y +  4);
	_btnUfo					= new ImageButton(36, 15, _x + 83, _y + 20);

	_btnAggressive			= new ImageButton(36, 15, _x + 120, _y +  4);
	_btnStandard			= new ImageButton(36, 15, _x + 120, _y + 20);
	_btnCautious			= new ImageButton(36, 15, _x + 120, _y + 36);
	_btnStandoff			= new ImageButton(36, 17, _x + 120, _y + 52);
	_mode = _btnStandoff;

	_texture				= new Surface(9, 9, _x + 147, _y + 72);

	_txtAmmo1				= new Text( 16, 9, _x +   4, _y + 70);
	_txtAmmo2				= new Text( 16, 9, _x +  64, _y + 70);
	_txtDistance			= new Text( 40, 9, _x + 116, _y + 72);
	_txtStatus				= new Text(150, 9, _x +   4, _y + 85);
	_txtTitle				= new Text(160, 9, _x,       _y - 9);

	_btnMinimizedIcon		= new InteractiveSurface(32, 20, _minimizedIconX, _minimizedIconY);
	_txtInterception		= new Text(150, 9, _minimizedIconX + 15, _minimizedIconY + 6);
//	_txtInterception		= new Text(16, 9, _minimizedIconX + 18, _minimizedIconY + 6);

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
	add(_btnDisengage,		"button",			"dogfight");
	add(_btnUfo,			"button",			"dogfight");
	add(_btnAggressive,		"button",			"dogfight");
	add(_btnStandard,		"button",			"dogfight");
	add(_btnCautious,		"button",			"dogfight");
	add(_btnStandoff,		"button",			"dogfight");
	add(_texture);
	add(_txtAmmo1,			"numbers",			"dogfight");
	add(_txtAmmo2,			"numbers",			"dogfight");
	add(_txtDistance,		"numbers",			"dogfight");
	add(_preview);
	add(_txtStatus,			"text",				"dogfight");
	add(_txtTitle,			"button",			"dogfight");
	add(_btnMinimizedIcon);
	add(_txtInterception,	"minimizedNumber",	"dogfight");

	if (_txtDistance->isTFTDMode() == true)
	{
		_txtDistance->setY(_txtDistance->getY() + 1);
		_txtDistance->setX(_txtDistance->getX() + 7);
	}

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
	_btnStandoff->onKeyboardPress(
					(ActionHandler)& DogfightState::btnStandoffPress,
					Options::keyOk);

	srf = _game->getResourcePack()->getSurface(getTextureIcon());
	if (srf != NULL)
		srf->blit(_texture);

	_txtDistance->setText(Text::formatNumber(DST_ENGAGE));

	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_STANDOFF"));

	SurfaceSet* const sstInticon = _game->getResourcePack()->getSurfaceSet("INTICON.PCK");

	// Create the minimized dogfight icon.
	Surface* srfFrame = sstInticon->getFrame(_craft->getRules()->getSprite());
//	srfFrame->setX(0);
//	srfFrame->setY(0);
	srfFrame->blit(_btnMinimizedIcon);
	_btnMinimizedIcon->onMouseClick((ActionHandler)& DogfightState::btnMinimizedIconClick);
	_btnMinimizedIcon->onKeyboardPress(
					(ActionHandler)& DogfightState::btnMinimizedIconClick,
					Options::keyCancel);
	_btnMinimizedIcon->setVisible(false);

	std::wostringstream woststr;
	woststr << _craft->getName(_game->getLanguage()) << L" >" << _craft->getBase()->getName(_game->getLanguage());
	_txtTitle->setText(woststr.str());

//	woststr.str(L"");
/*	std::wstring
		wst = _craft->getName(_game->getLanguage()),
		pre = wst.substr(0,1);
	woststr << pre;
	size_t pos = wst.find_last_of('-');
	if (pos != std::string::npos)
		woststr << wst.substr(pos);
	else
		woststr << L"-" << _craft->getFlightOrder(); */
//	woststr << _craft->getFlightOrder();
//	woststr << _craft->getName(_game->getLanguage()) << L" >" << _craft->getBase()->getName(_game->getLanguage());
	_txtInterception->setText(woststr.str());
	_txtInterception->setVisible(false);
//	_txtInterception->setAlign(ALIGN_CENTER);

	// Define the colors to be used. Note these have been further tweaked in Interfaces.rul
	_colors[CRAFT_MIN]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("craftRange")->color);	// 160, (10)slate gray
	_colors[CRAFT_MAX]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("craftRange")->color2);	// 176, (11)purple
	_colors[RADAR_MIN]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("radarRange")->color);	// 112, (7)green
	_colors[RADAR_MAX]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("radarRange")->color2);	// 128, (8)red
	_colors[DAMAGE_MIN]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("damageRange")->color);	//  12, (0+)yellow
	_colors[DAMAGE_MAX]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("damageRange")->color2);	//  14, (0+)red
	_colors[BLOB_MIN]		= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("radarDetail")->color);	// 108, (6)+12 green
	_colors[RANGE_METER]	= static_cast<Uint8>(_game->getRuleset()->getInterface("dogfight")->getElement("radarDetail")->color2);	// 111, (6)+15 green


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
	delete _craftDamageAnimTimer;

	while (_projectiles.empty() == false)
	{
		delete _projectiles.back();
		_projectiles.pop_back();
	}

	if (_craft != NULL)
		_craft->setInDogfight(false);

	if (_ufo != NULL) // frees the ufo for the next engagement
		_ufo->setEngaged(false);
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

		if (_endDogfight == false // value could change in updateDogfight()^
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
	const int damagePCT = _craft->getDamagePercent();
	if (damagePCT > 0)
	{
		if (_craftDamageAnimTimer->isRunning() == false)
		{
			_craftDamageAnimTimer->start();
//			_currentCraftDamageColor = _colors[DAMAGE_MIN];
		}

		int rowsToColor = static_cast<int>(std::floor(
						  static_cast<double>(_craftHeight * damagePCT) / 100.));

		if (rowsToColor == 0)
			return;
		else if (damagePCT > 99)
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
			Uint8 radarPixelColor = _window->getPixelColor(x,y);
			if (radarPixelColor > _colors[RADAR_MIN] - 1
				&& radarPixelColor < _colors[RADAR_MAX])
			{
				++radarPixelColor;

				if (radarPixelColor > _colors[RADAR_MAX] - 1)
					radarPixelColor = _colors[RADAR_MIN];

				_window->setPixelColor(
									x,y,
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


	bool lastHitFrame = false;

	if (_animatingHit == true // animate UFO hit
		&& _ufo->getHitFrame() > 0)
	{
		_ufo->setHitFrame(_ufo->getHitFrame() - 1);

		if (_ufo->getHitFrame() == 0)
		{
			_animatingHit = false;
			lastHitFrame = true;
		}
	}

	if (_ufo->isCrashed() == true // animate UFO crash landing
		&& _ufo->getHitFrame() == 0
		&& lastHitFrame == false)
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
	bool outRun = false;

	const Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	if (ufo != _ufo							// check that Craft's destination hasn't been changed when window minimized
		|| _craft->getLowFuel() == true		// check if Craft is not low on fuel when window minimized
		|| (_minimized == true
			&& _ufo->isCrashed() == true))	// chat if UFO hasn't been shot down when window minimized
	{
		if (_endDogfight == false)
			endDogfight();

		return;
	}

	if (_minimized == false)
	{
		animate();

		int escapeTicks = _ufo->getEscapeCountdown();
		if (_ufo->isCrashed() == false
			&& _craft->isDestroyed() == false)
		{
			if (_ufo->getEngaged() == false
				&& escapeTicks > 0)
			{
				_ufo->setEngaged();
				_geo->drawUfoIndicators(); // kL

				if (_dist < DST_STANDOFF) // kL
					_ufo->setEscapeCountdown(--escapeTicks);

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
		outRun = true;
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
									_targetDist = DST_STANDOFF;
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
					maximumDistance();
				else if (_mode == _btnStandard)
					minimumDistance();
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
		else if (_ufo->getFireCountdown() == 0 // UFO is gtg.
			&& (_ufo->getShootingAt() == 0
				|| _ufo->getShootingAt() == _slot)
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
					_ufo->setShootingAt(_slot);
					ufoFireWeapon();
				}
				else if (altIntercepts.size() > 1) // [ ==0 should NEVER happen.]
				{
					int shotCraft = static_cast<int>(Round(100. / static_cast<double>(altIntercepts.size())));

					if (_ufo->getShootingAt() == _slot)
						shotCraft += 18;

					if (RNG::percent(shotCraft) == true)
					{
						_ufo->setShootingAt(_slot);
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
		&& (((_dist > DST_ENGAGE
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

		if (_endDogfight == false)
			endDogfight();
	}

	if (_dist > DST_ENGAGE
		&& _ufoBreakingOff == true)
	{
		outRun = true;
	}

	if (_end == false)
	{
		if (_craft->isDestroyed() == true) // End dogfight if craft is destroyed.
		{
			setStatus("STR_INTERCEPTOR_DESTROYED");

			_timeout += 30;
			_game->getResourcePack()->playSoundFX(ResourcePack::INTERCEPTOR_EXPLODE);

			outRun = true;
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
					targetRegion = _gameSave->locateRegion(*_craft->getBase())->getRules()->getType();
																	// TODO: If the base is removed, the mission is cancelled.

				// Difference from original: No retaliation until final UFO lands (Original: Is spawned).
				if (_game->getSavedGame()->findAlienMission(
														targetRegion,
														OBJECTIVE_RETALIATION) == false)
				{
					const RuleAlienMission& rule = *_game->getRuleset()->getAlienMission("STR_ALIEN_RETALIATION");
					AlienMission* const mission = new AlienMission(
																rule,
																*_gameSave);

					mission->setId(_gameSave->getId("ALIEN_MISSIONS"));
					mission->setRegion(
									targetRegion,
									*_game->getRuleset());
					mission->setRace(_ufo->getAlienRace());
					mission->start();

					_gameSave->getAlienMissions().push_back(mission);
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
							i = _gameSave->getRegions()->begin();
							i != _gameSave->getRegions()->end();
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
							i = _gameSave->getCountries()->begin();
							i != _gameSave->getCountries()->end();
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
							i = _gameSave->getRegions()->begin();
							i != _gameSave->getRegions()->end();
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
							i = _gameSave->getCountries()->begin();
							i != _gameSave->getCountries()->end();
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
					_ufo->setSecondsLeft(RNG::generate(24,96) * 3600);
					_ufo->setAltitude("STR_GROUND");

					if (_ufo->getCrashId() == 0)
						_ufo->setCrashId(_gameSave->getId("STR_CRASH_SITE"));
				}
			}

			_timeout += 30;
			if (_ufo->getShotDownByCraftId() != _craft->getUniqueId())
			{
				_timeout += 50;
				_ufo->setHitFrame(3);
			}

			outRun = true;
//			_ufoBreakingOff = false;
//			_ufo->setSpeed(0);
		}

		if (_ufo->getStatus() == Ufo::LANDED)
		{
			_timeout += 30;
			outRun = true;
			_ufo->setShootingAt(0);
		}
	}

	if (projectileInFlight == false
		&& outRun == true)
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
			&& (*i)->getAmmo() > 0
			&& (*i)->getRules()->getRange() > dist)
		{
			dist = (*i)->getRules()->getRange();
		}
	}

	if (dist == 0)
		_targetDist = DST_STANDOFF;
	else
		_targetDist = dist * 8;
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
			&& (*i)->getAmmo() > 0
			&& (*i)->getRules()->getRange() < dist)
		{
			dist = (*i)->getRules()->getRange();
		}
	}

	if (dist == 1000)
		_targetDist = DST_STANDOFF;
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
	_timeout = MSG_TIMEOUT;
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
		_targetDist = DST_STANDOFF;
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
 * Switches to Aggressive mode.
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
		}

		if (_craft->getRules()->getWeapons() > 1
			&& _craft->getWeapons()->at(1) != NULL)
		{
			_w2FireInterval = _craft->getWeapons()->at(1)->getRules()->getAggressiveReload();
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

		_targetDist = DST_ENGAGE + 1;
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
 * Minimizes the dogfight window.
 * @param action - pointer to an Action
 */
void DogfightState::btnMinimizeClick(Action*)
{
	if (_ufo->isCrashed() == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff == false)
	{
		if (_dist > DST_STANDOFF - 1)
		{
			_minimized = true;

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
		}
		else
			setStatus("STR_MINIMISE_AT_STANDOFF_RANGE_ONLY");
	}
}

/**
 * Maximizes the dogfight window.
 * @param action - pointer to an Action
 */
void DogfightState::btnMinimizedIconClick(Action*)
{
	_texture->clear();

	Surface* const srfTexture = _game->getResourcePack()->getSurface(getTextureIcon());
	if (srfTexture != NULL)
		srfTexture->blit(_texture);

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
	_txtTitle->setVisible();

	_btnMinimizedIcon->setVisible(false);
	_txtInterception->setVisible(false);
	_preview->setVisible(false);

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

/**
 * Returns true if state is minimized.
 * @return, true if minimized
 */
bool DogfightState::isMinimized() const
{
	return _minimized;
}

/**
 * Draws the UFO blob on the radar screen.
 * Currently works only for original sized blobs 13 x 13 pixels.
 */
void DogfightState::drawUfo()
{
	if (_ufoSize > -1
		&& _ufo->isDestroyed() == false)
	{
		const int
			ufo_x = _battle->getWidth() / 2 - 6,
			ufo_y = _battle->getHeight() - (_dist / 8) - 6;
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
				color = static_cast<Uint8>(GeoscapeState::_ufoBlobs[_ufoSize + _ufo->getHitFrame()]
																   [static_cast<size_t>(y)]
																   [static_cast<size_t>(x)]);
				if (color != 0)
				{
					if (_ufo->isCrashed() == true
						|| _ufo->getHitFrame() > 0)
					{
						color *= 2;
					}

					color = _window->getPixelColor(
												ufo_x + x + 3,
												ufo_y + y + 3) - color;
					if (color < _colors[BLOB_MIN])
						color = _colors[BLOB_MIN];

					_battle->setPixelColor(
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
					if (color < _colors[BLOB_MIN]) color = _colors[BLOB_MIN];

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
			if (color < _colors[BLOB_MIN]) color = _colors[BLOB_MIN];

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
 * Gets interception slot.
 * @return, interception number
 */
size_t DogfightState::getInterceptSlot() const
{
	return _slot;
}

/**
 * Sets interception slot.
 * @param intercept - #ID
 */
void DogfightState::setInterceptSlot(const size_t intercept)
{
	_slot = intercept;
}

/**
 * Sets total interceptions count. Used to properly position the window.
 * @param intercepts - amount of interception windows
 */
void DogfightState::setTotalIntercepts(const size_t intercepts)
{
	_totalIntercepts = intercepts;
}

/**
 * Calculates and positions this interception's view window.
 */
void DogfightState::resetInterceptPort()
{
	calcPortPosition();
	placePort();
}

/**
 * Calculates dogfight window position according to number of active interceptions.
 */
void DogfightState::calcPortPosition()
{
	if (_slot > _totalIntercepts)
		_slot = _geo->getOpenDfSlot();

	_minimizedIconX = 5;
	_minimizedIconY = (5 * _slot) + (16 * (_slot - 1));

	if (_totalIntercepts == 1)
	{
		_x = 80;
		_y = 52;
	}
	else if (_totalIntercepts == 2)
	{
		if (_slot == 1)
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
	else if (_totalIntercepts == 3)
	{
		if (_slot == 1)
		{
			_x = 80;
			_y = 0;
		}
		else if (_slot == 2)
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
		if (_slot == 1)
		{
			_x =
			_y = 0;
		}
		else if (_slot == 2)
		{
			_x = 320 - _window->getWidth(); // 160;
			_y = 0;
		}
		else if (_slot == 3)
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
void DogfightState::placePort()
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

	if (_txtDistance->isTFTDMode() == true)
	{
		_txtDistance->setX(_x + 123);
		_txtDistance->setY(_y + 73);
	}
	else
	{
		_txtDistance->setX(_x + 116);
		_txtDistance->setY(_y + 72);
	}

	_txtStatus->setX(_x + 4);
	_txtStatus->setY(_y + 85);

	_txtTitle->setX(_x);
	_txtTitle->setY(_y - 9);

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
void DogfightState::endDogfight() // private.
{
	_endDogfight = true;

	if (_craft != NULL)
		_craft->setInDogfight(false);
}

/**
 * Gets the globe texture icon to display for the interception.
 * IMPORTANT: This does not return the actual battleField terrain; that is done
 * in ConfirmLandingState. This is merely an indicator .... cf. UfoDetectedState.
 * @return, string for the icon of the texture of the globe's surface under the dogfight ha!
 */
const std::string DogfightState::getTextureIcon()
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
		case  6: return "FOREST"; // 6=JUNGLE

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
