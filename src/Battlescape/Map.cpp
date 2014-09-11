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

#define _USE_MATH_DEFINES

#include "Map.h"

#include <cmath>
#include <fstream>

#include "BattlescapeMessage.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "Pathfinding.h"
#include "Position.h"
#include "Projectile.h"
#include "ProjectileFlyBState.h" // kL
#include "TileEngine.h"
#include "UnitSprite.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h" // kL
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

//#include "../Ruleset/RuleTerrain.h" // kL


/*
  1) Map origin is top corner.
  2) X axis goes downright. (width of the map)
  3) Y axis goes downleft. (length of the map
  4) Z axis goes up (height of the map)

           0,0
			/\
	    y+ /  \ x+
		   \  /
		    \/
*/

namespace OpenXcom
{

bool kL_preReveal = true; // kL


/**
 * Sets up a map with the specified size and position.
 * @param game Pointer to the core game.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param visibleMapHeight Current visible map height.
 */
Map::Map(
		Game* game,
		int width,
		int height,
		int x,
		int y,
		int visibleMapHeight)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_game(game),
		_arrow(NULL),
		_selectorX(0),
		_selectorY(0),
		_mouseX(0),
		_mouseY(0),
		_cursorType(CT_NORMAL),
		_cursorSize(1),
		_animFrame(0),
//kL	_cursorFrame(0), // DarkDefender
		_projectile(NULL),
		_projectileInFOV(false),
		_explosionInFOV(false),
		_launch(false),
		_visibleMapHeight(visibleMapHeight),
		_unitDying(false),
		_reveal(0), // kL
		_smoothingEngaged(false)
{
	//Log(LOG_INFO) << "Create Map";
	_iconWidth = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->w;
	_iconHeight = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->h;
	_messageColor = _game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color;

//kL	_smoothCamera = Options::battleSmoothCamera;

	_previewSetting	= Options::battleNewPreviewPath;
	if (Options::traceAI) // turn everything on because we want to see the markers.
		_previewSetting = PATH_FULL;

	_res			= _game->getResourcePack();
	_spriteWidth	= _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight	= _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();

	_save = _game->getSavedGame()->getSavedBattle();

	_camera = new Camera(
					_spriteWidth,
					_spriteHeight,
					_save->getMapSizeX(),
					_save->getMapSizeY(),
					_save->getMapSizeZ(),
					this,
					visibleMapHeight);

	_message = new BattlescapeMessage( // Hidden Movement... screen
									320,
									visibleMapHeight < 200? visibleMapHeight: 200,
									0,
									0);
	_message->setX(_game->getScreen()->getDX());
	_message->setY(_game->getScreen()->getDY());
//	_message->setY((visibleMapHeight - _message->getHeight()) / 2);
//	_message->setTextColor(_game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color);
	_message->setTextColor(_messageColor);

	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)& Map::scrollMouse);

	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)& Map::scrollKey);

	_camera->setScrollTimer(
						_scrollMouseTimer,
						_scrollKeyTimer);

/*kL	_txtAccuracy = new Text(24, 9, 0, 0);
	_txtAccuracy->setSmall();
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());
	_txtAccuracy->setHighContrast();
	_txtAccuracy->initText(
						_res->getFont("FONT_BIG"),
						_res->getFont("FONT_SMALL"),
						_game->getLanguage()); */
	_txtAccuracy = new NumberText(24, 9, 0, 0);					// kL
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());	// kL
}

/**
 * Deletes the map.
 */
Map::~Map()
{
	//Log(LOG_INFO) << "Delete Map";

	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
	delete _arrow;
	delete _message;
	delete _camera;
	delete _txtAccuracy;
}

/**
 * Initializes the map.
 */
void Map::init()
{
	// load the unit-selected bobbing-arrow into a surface
	int f = Palette::blockOffset(1);	// Fill,	yellow
	int b = 14;							// Border,	black
	int pixels[81] = { 0, 0, b, b, b, b, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   b, b, b, f, f, f, b, b, b,
					   b, f, f, f, f, f, f, f, b,
					   0, b, f, f, f, f, f, b, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, 0, b, f, b, 0, 0, 0,
					   0, 0, 0, 0, b, 0, 0, 0, 0 };
/*	int pixels[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, // kL
					   0, 0, b, b, b, b, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   b, b, b, f, f, f, b, b, b,
					   b, f, f, f, f, f, f, f, b,
					   0, b, f, f, f, f, f, b, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, 0, b, f, b, 0, 0, 0,
					   0, 0, 0, 0, b, 0, 0, 0, 0 }; */

	_arrow = new Surface(9, 9);
	_arrow->setPalette(this->getPalette());

	_arrow->lock();
	for (int
			y = 0;
			y < 9;
			++y)
	{
		for (int
				x = 0;
				x < 9;
				++x)
		{
			_arrow->setPixelColor(
								x,
								y,
								pixels[x + (y * 9)]);
		}
	}
	_arrow->unlock();

	// DarkDefender_begin:
/*	int pixels_kneel[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, b, b, b, b, b, 0, 0,
							 0, 0, b, f, f, f, b, 0, 0,
							 b, b, b, f, f, f, b, b, b,
							 b, f, f, f, f, f, f, f, b,
							 0, b, f, f, f, f, f, b, 0,
							 0, 0, b, f, f, f, b, 0, 0,
							 0, 0, 0, b, f, b, 0, 0, 0,
							 0, 0, 0, 0, b, 0, 0, 0, 0 }; */
	int pixels_kneel[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, // kL
							 0, 0, 0, 0, b, 0, 0, 0, 0,
							 0, 0, 0, b, f, b, 0, 0, 0,
							 0, 0, b, f, f, f, b, 0, 0,
							 0, b, f, f, f, f, f, b, 0,
							 b, f, f, f, f, f, f, f, b,
							 b, b, b, f, f, f, b, b, b,
							 0, 0, b, f, f, f, b, 0, 0,
							 0, 0, b, b, b, b, b, 0, 0 };
/*	int pixels_kneel[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, // kL
							 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, b, 0, 0, 0, 0,
							 0, 0, 0, b, f, b, 0, 0, 0,
							 0, 0, b, f, f, f, b, 0, 0,
							 0, b, f, f, f, f, f, b, 0,
							 b, f, f, f, f, f, f, f, b,
							 b, b, b, f, f, f, b, b, b,
							 0, 0, b, b, b, b, b, 0, 0 }; */

	_arrow_kneel = new Surface(9, 9);
	_arrow_kneel->setPalette(this->getPalette());

	_arrow_kneel->lock();
	for (int
			y = 0;
			y < 9;
			++y)
	{
		for (int
				x = 0;
				x < 9;
				++x)
		{
			_arrow_kneel->setPixelColor(
									x,
									y,
									pixels_kneel[x + (y * 9)]);
		}
	}
	_arrow_kneel->unlock(); // DarkDefender_end.

	_projectile = 0;

	if (_save->getDepth() == 0)
		_projectileSet = _res->getSurfaceSet("Projectiles");
	else
		_projectileSet = _res->getSurfaceSet("UnderwaterProjectiles");

/*	int // kL_begin: reveal map's border tiles.
		size_x = _save->getMapSizeX(),
		size_y = _save->getMapSizeY(),
		size_z = _save->getMapSizeZ();

	for (int
			x = 0;
			x < size_x;
			++x)
	{
		for (int
				y = 0;
				y < size_y;
				++y)
		{
			for (int
					z = 0;
					z < size_z;
					++z)
			{
				if (x == 0
					|| y == 0
					|| x == size_x - 1
					|| y == size_y - 1)
				{
					Tile* tile = _save->getTile(Position(x, y, z));
					if (tile)
						tile->setDiscovered(true, 2);
				}
			}
		}
	} */ // kL_end.
}

/**
 * Keeps the animation timers running.
 */
void Map::think()
{
	_scrollMouseTimer->think(NULL, this);
	_scrollKeyTimer->think(NULL, this);
}

/**
 * Draws the whole map, part by part.
 */
void Map::draw()
{
	//Log(LOG_INFO) << "Map::draw()";

	// kL_note: removed setting this in BattlescapeGame::handleState().
/*kL	if (!_redraw)
		return; */

//	Surface::draw();
	// normally we'd call for a Surface::draw();
	// but we don't want to clear the background with colour 0, which is transparent (aka black)
	// we use colour 15 because that actually corresponds to the colour we DO want in all variations of the xcom and tftd palettes.
	_redraw = false;
	clear(Palette::blockOffset(0)+15);

	Tile* t = NULL;

	_projectileInFOV = _save->getDebugMode();
	if (_projectile)
//kL		&& _save->getSide() == FACTION_PLAYER)
	{
		//Log(LOG_INFO) << "Map::draw() _projectile = true";
		t = _save->getTile(Position(
								_projectile->getPosition(0).x / 16,
								_projectile->getPosition(0).y / 16,
								_projectile->getPosition(0).z / 24));
		if (t
			&& (t->getVisible()
				|| _save->getSide() != FACTION_PLAYER))	// kL: shows projectile during aLien berserk
		{
			//Log(LOG_INFO) << "Map::draw() _projectileInFOV = true";
			_projectileInFOV = true;
		}
	}

	_explosionInFOV = _save->getDebugMode();
	if (!_explosions.empty())
	{
//		for (std::set<Explosion*>::iterator // kL
		for (std::list<Explosion*>::iterator // expl CTD
				i = _explosions.begin();
				i != _explosions.end();
				++i)
		{
			t = _save->getTile(Position(
									(*i)->getPosition().x / 16,
									(*i)->getPosition().y / 16,
									(*i)->getPosition().z / 24));
			if (t
				&& ((*i)->isBig()
					|| t->getVisible()
					|| _save->getSide() != FACTION_PLAYER))	// kL: shows hit-explosion during aLien berserk
			{
				_explosionInFOV = true;

				break;
			}
		}
	}

	//Log(LOG_INFO) << ". . kL_preReveal = " << kL_preReveal;
/*kL
	if ((_save->getSelectedUnit() && _save->getSelectedUnit()->getVisible())
		|| _unitDying
		|| _save->getSelectedUnit() == 0
		|| _save->getDebugMode()
		|| _projectileInFOV
		|| _explosionInFOV)
	{
		drawTerrain(this);
	} */


	if ((_save->getSelectedUnit()
			&& _save->getSelectedUnit()->getVisible())
		|| _save->getSelectedUnit() == 0
		|| _unitDying
		|| _save->getDebugMode()
		|| _projectileInFOV
		|| _explosionInFOV
		|| (_reveal > 0
			&& !kL_preReveal))
	{
		if (_reveal > 0
			&& !kL_preReveal)
		{
			_reveal--;
			//Log(LOG_INFO) << ". . . . . . drawTerrain() _reveal = " << _reveal;
		}
		else
		{
			_reveal = 3;
			//Log(LOG_INFO) << ". . . . . . drawTerrain() Set _reveal = " << _reveal;
		}

		if (_save->getSide() == FACTION_PLAYER)
			_save->getBattleState()->toggleIcons(true);

		drawTerrain(this);
	}
	else // "hidden movement"
	{
		if (kL_preReveal)
		{
			kL_preReveal = false;
			_reveal = 0;
			//Log(LOG_INFO) << ". . . . . . kL_preReveal, set " << kL_preReveal;
			//Log(LOG_INFO) << ". . . . . . _reveal, set " << _reveal;
		}

		_save->getBattleState()->toggleIcons(false);

		//Log(LOG_INFO) << ". . . . blit( hidden movement )";
		_message->blit(this);
	}
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Map::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	for (std::vector<MapDataSet*>::const_iterator
			i = _save->getMapDataSets()->begin();
			i != _save->getMapDataSets()->end();
			++i)
	{
		(*i)->getSurfaceset()->setPalette(colors, firstcolor, ncolors);
	}

	_message->setPalette(colors, firstcolor, ncolors);
	_message->setBackground(_res->getSurface("TAC00.SCR"));
	_message->initText(
					_res->getFont("FONT_BIG"),
					_res->getFont("FONT_SMALL"),
					_game->getLanguage());
	_message->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"));
}

/**
 * Draw the terrain.
 * Keep this function as optimised as possible. It's big so minimise overhead of function calls.
 * @param surface, The surface to draw on.
 */
void Map::drawTerrain(Surface* surface)
{
	//Log(LOG_INFO) << "Map::drawTerrain()";
	BattleUnit* unit	= NULL;
	NumberText* wpID	= NULL;
	Surface* tmpSurface	= NULL;
	Tile* tile			= NULL;
	Position
		mapPosition,
		screenPosition,
		bulletScreen;

//kL	static const int arrowBob[8] = {0,1,2,1,0,1,2,1};
//kL	static const int arrowBob[10] = {0,2,3,3,2,0,-2,-3,-3,-2}; // DarkDefender

	bool invalid = false;
	int
		bulletLowX	= 16000,
		bulletLowY	= 16000,
		bulletLowZ	= 16000,
		bulletHighX	= 0,
		bulletHighY	= 0,
		bulletHighZ	= 0,

		frame	= 0,
		d		= 0,

		tileShade = 0,
		wallShade = 0,
		tileColor = 0,

		beginX	= 0,
		beginY	= 0,
		beginZ	= 0,
		endX	= _save->getMapSizeX() - 1,
		endY	= _save->getMapSizeY() - 1,
		endZ	= _camera->getViewLevel();

	if (_camera->getShowAllLayers())
		endZ = _save->getMapSizeZ() - 1;


	if (_projectile // if we got bullet, get the highest x and y tiles to draw it on
		&& _explosions.empty())
	{
		int part = BULLET_SPRITES - 1;
		if (_projectile->getItem())
			part = 0;

		for (int
				i = 0;
				i <= part;
				++i)
		{
			if (_projectile->getPosition(1 - i).x < bulletLowX)
				bulletLowX = _projectile->getPosition(1 - i).x;

			if (_projectile->getPosition(1 - i).y < bulletLowY)
				bulletLowY = _projectile->getPosition(1 - i).y;

			if (_projectile->getPosition(1 - i).z < bulletLowZ)
				bulletLowZ = _projectile->getPosition(1 - i).z;

			if (_projectile->getPosition(1 - i).x > bulletHighX)
				bulletHighX = _projectile->getPosition(1 - i).x;

			if (_projectile->getPosition(1 - i).y > bulletHighY)
				bulletHighY = _projectile->getPosition(1 - i).y;

			if (_projectile->getPosition(1 - i).z > bulletHighZ)
				bulletHighZ = _projectile->getPosition(1 - i).z;
		}

		// convert bullet position from voxelspace to tilespace
		bulletLowX	= bulletLowX  / 16;
		bulletLowY	= bulletLowY  / 16;
		bulletLowZ	= bulletLowZ  / 24;

		bulletHighX = bulletHighX / 16;
		bulletHighY = bulletHighY / 16;
		bulletHighZ = bulletHighZ / 24;

		// if the projectile is outside the viewport - center it back on it
		_camera->convertVoxelToScreen(
								_projectile->getPosition(),
								&bulletScreen);

		if (_projectileInFOV)
		{
			if (Options::battleSmoothCamera)
			{
				if (_launch)
				{
					_launch = false;

					if (bulletScreen.x < 1
						|| bulletScreen.x > surface->getWidth() - 1
						|| bulletScreen.y < 1
						|| bulletScreen.y > _visibleMapHeight - 1)
					{
						_camera->centerOnPosition(
												Position(
													bulletLowX,
													bulletLowY,
													bulletHighZ),
												false);
						_camera->convertVoxelToScreen(
												_projectile->getPosition(),
												&bulletScreen);
					}
				}

				if (!_smoothingEngaged)
				{
					Position target = _projectile->getFinalTarget();	// kL
					if (!_camera->isOnScreen(target)					// kL
						|| bulletScreen.x < 1
						|| bulletScreen.x > surface->getWidth() - 1
						|| bulletScreen.y < 1
						|| bulletScreen.y > _visibleMapHeight - 1)
					{
						_smoothingEngaged = true;
					}
				}
				else // smoothing Engaged
				{
					_camera->jumpXY(
								surface->getWidth() / 2 - bulletScreen.x,
								_visibleMapHeight / 2 - bulletScreen.y);

					int posBullet_z = _projectile->getPosition().z / 24;
					if (_camera->getViewLevel() != posBullet_z)
					{
						if (_camera->getViewLevel() > posBullet_z)
							_camera->setViewLevel(posBullet_z);
						else
							_camera->setViewLevel(posBullet_z);
					}
				}
			}
			else // NOT smoothCamera
			// kL_note: Camera remains stationary when xCom actively fires at target.
			// That is, Target is already onScreen due to targetting cursor click!
			// ( And, player should already know what unit is shooting... )
//			if (_projectile->getActor()->getFaction() != FACTION_PLAYER) // kL
			{
				bool enough;
				do
				{
					enough = true;
					if (bulletScreen.x < 0)
					{
						_camera->jumpXY(+surface->getWidth(), 0);
						enough = false;
					}
					else if (bulletScreen.x > surface->getWidth())
					{
						_camera->jumpXY(-surface->getWidth(), 0);
						enough = false;
					}
					else if (bulletScreen.y < 0)
					{
						_camera->jumpXY(0, +_visibleMapHeight);
						enough = false;
					}
					else if (bulletScreen.y > _visibleMapHeight)
					{
						_camera->jumpXY(0, -_visibleMapHeight);
						enough = false;
					}
					_camera->convertVoxelToScreen(
											_projectile->getPosition(),
											&bulletScreen);
				}
				while (!enough);
			}
		}
	}
	else // if (no projectile OR explosions waiting)
		_smoothingEngaged = false;

	// get corner map coordinates to give rough boundaries in which tiles to redraw are
	_camera->convertScreenToMap(
							0,
							0,
							&beginX,
							&d);
	_camera->convertScreenToMap(
							surface->getWidth(),
							0,
							&d,
							&beginY);
	_camera->convertScreenToMap(
							surface->getWidth() + _spriteWidth,
							surface->getHeight() + _spriteHeight,
							&endX,
							&d);
	_camera->convertScreenToMap(
							0,
							surface->getHeight() + _spriteHeight,
							&d,
							&endY);

	beginY -= (_camera->getViewLevel() * 2);
	beginX -= (_camera->getViewLevel() * 2);
	if (beginX < 0)
		beginX = 0;
	if (beginY < 0)
		beginY = 0;

	bool
		pathPreview = _save->getPathfinding()->isPathPreviewed(),
//		drawRankIcon;
		floor,
		object;

	if (!_waypoints.empty()
		|| (pathPreview
			&& (_previewSetting & PATH_TU_COST)))
	{
		// note: WpID is used for both pathPreview and BL waypoints.
		// kL_note: Leave them the same color.
		wpID = new NumberText(15, 15, 20, 30);
		wpID->setPalette(getPalette());
//		wpID->setColor(pathPreview? _messageColor + 1: Palette::blockOffset(1));

		Uint8 wpColor;
		if (_save->getTerrain() == "DESERT")
			wpColor = Palette::blockOffset(0)+2; // light gray
		else
			wpColor = Palette::blockOffset(1)+2; // yellow

		wpID->setColor(wpColor);
	}

	surface->lock();
	for (int
			itZ = beginZ;
			itZ <= endZ;
			itZ++)
	{
		for (int
				itX = beginX;
				itX <= endX;
				itX++)
		{
			for (int
					itY = beginY;
					itY <= endY;
					itY++)
			{
				mapPosition = Position(
									itX,
									itY,
									itZ);
				_camera->convertMapToScreen(
										mapPosition,
										&screenPosition);
				screenPosition += _camera->getMapOffset();

				if (screenPosition.x > -_spriteWidth // only render cells that are inside the surface
					&& screenPosition.x < surface->getWidth() + _spriteWidth
					&& screenPosition.y > -_spriteHeight
					&& screenPosition.y < surface->getHeight() + _spriteHeight)
				{
					tile = _save->getTile(mapPosition);

					if (tile == NULL)
						continue;

					if (tile->isDiscovered(2))
						tileShade = tile->getShade();
					else
					{
						tileShade = 16;
						unit = NULL;
					}

					tileColor = tile->getMarkerColor();
//					drawRankIcon = true;
					floor = false;
					object = false;

					// Draw floor
					tmpSurface = tile->getSprite(MapData::O_FLOOR);
					if (tmpSurface)
					{
						tmpSurface->blitNShade(
								surface,
								screenPosition.x,
								screenPosition.y - tile->getMapData(MapData::O_FLOOR)->getYOffset(),
								tileShade);

						// kL_begin:
						floor = true;

						Tile* tileEastDown = _save->getTile(mapPosition + Position(1, 0, -1));
						if (tileEastDown != NULL)
						{
							BattleUnit* bu = tileEastDown->getUnit();
							Tile* tileEast = _save->getTile(mapPosition + Position(1, 0, 0));
							if (bu != NULL
								&& tileEast->getSprite(MapData::O_FLOOR) == NULL)
							{
								// from below_ 'Draw soldier'. This ensures the rankIcon isn't half-hidden by a floor above & west of soldier.
								// also, make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
								// ... should probably be a subfunction
								if (bu->getFaction() == FACTION_PLAYER
									&& bu != _save->getSelectedUnit()
									&& bu->getTurretType() == -1) // no tanks, pls
								{
//									drawRankIcon = false;

									Position offset;
									calculateWalkingOffset(bu, &offset);

									if (bu->getFatalWounds() > 0)
									{
										tmpSurface = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (tmpSurface != NULL)
										{
											tmpSurface->blitNShade(
													surface,
													screenPosition.x + offset.x + 2 + 16,
													screenPosition.y + offset.y + 3 + 32,
													0);
										}

										tmpSurface = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // 11, small gray cross; 209, big red cross
										tmpSurface->blitNShade(
												surface,
												screenPosition.x + offset.x + 4 + 16,
												screenPosition.y + offset.y + 4 + 32,
												0,
												false,
												3); // 1=white, 3=red
									}
									else
									{
										std::string soldierRank = bu->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
										soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

										tmpSurface = _res->getSurface(soldierRank);
										if (tmpSurface != NULL)
										{
											tmpSurface->blitNShade(
													surface,
													screenPosition.x + offset.x + 2 + 16,
													screenPosition.y + offset.y + 3 + 32,
													0);
										}
									}
								}
							}
						} // kL_end.
					}

					unit = tile->getUnit();

					// Draw cursor back
					if (_cursorType != CT_NONE
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX < itX + 1
						&& _selectorY < itY + 1
						&& !_save->getBattleState()->getMouseOverIcons())
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit
									&& (unit->getVisible() || _save->getDebugMode()))
								{
									frame = (_animFrame %2); // yellow box
								}
								else
									frame = 0; // red box
							}
							else
							{
								if (unit
									&& (unit->getVisible() || _save->getDebugMode()))
								{
									frame = 7 + (_animFrame / 2); // yellow animated crosshairs
								}
								else
									frame = 6; // red static crosshairs
							}

							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frame = 2; // blue box

							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}
					}


// START ADVANCED DRAWING CYCLE:
					if (mapPosition.y > 0) // special handling for a moving unit.
					{
						Tile* tileNorth = _save->getTile(mapPosition - Position(0, 1, 0));
						BattleUnit* buNorth = NULL;

						int
							tileNorthShade,
							tileTwoNorthShade,
//kL						tileWestShade,
							tileNorthWestShade,
							tileSouthWestShade;

						if (tileNorth->isDiscovered(2))
						{
							buNorth = tileNorth->getUnit();
							tileNorthShade = tileNorth->getShade();
						}
						else
							tileNorthShade = 16;

						// Phase I: rerender the unit to make sure they don't get drawn over any walls or under any tiles
						if (buNorth
							&& buNorth->getVisible()
							&& buNorth->getStatus() == STATUS_WALKING
							&& tile->getTerrainLevel() >= tileNorth->getTerrainLevel())
						{
							Position tileOffset = Position(16,-8, 0);
							// the part is 0 for small units, large units have parts 1,2 & 3 depending
							// on the relative x/y position of this tile vs the actual unit position.
							int part = 0;
							part += tileNorth->getPosition().x - buNorth->getPosition().x;
							part += (tileNorth->getPosition().y - buNorth->getPosition().y) * 2;

							tmpSurface = buNorth->getCache(&invalid, part);
							if (tmpSurface)
							{
								Position offset; // draw unit
								calculateWalkingOffset(buNorth, &offset);
								tmpSurface->blitNShade(
													surface,
													screenPosition.x + offset.x + tileOffset.x,
													screenPosition.y + offset.y  + tileOffset.y,
													tileNorthShade);

								if (buNorth->getFire() > 0) // draw fire
								{
									frame = 4 + (_animFrame / 2);
									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									tmpSurface->blitNShade(
														surface,
														screenPosition.x + offset.x + tileOffset.x,
														screenPosition.y + offset.y + tileOffset.y,
														0);
								}
							}

							// Phase II: rerender any east wall type objects in the tile to the north of the unit
							// only applies to movement in the north/south direction.
							if ((buNorth->getDirection() == 0
									|| buNorth->getDirection() == 4)
								&& mapPosition.y >= 2)
							{
								Tile* tileTwoNorth = _save->getTile(mapPosition - Position(0, 2, 0));
								if (tileTwoNorth->isDiscovered(2))
									tileTwoNorthShade = tileTwoNorth->getShade();
								else
									tileTwoNorthShade = 16;

								tmpSurface = tileTwoNorth->getSprite(MapData::O_OBJECT);
								if (tmpSurface
									&& tileTwoNorth->getMapData(MapData::O_OBJECT)->getBigWall() == 6)
								{
									tmpSurface->blitNShade(
														surface,
														screenPosition.x + tileOffset.x * 2,
														screenPosition.y - tileTwoNorth->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y * 2,
														tileTwoNorthShade);
								}
							}

							// Phase III: render any south wall type objects in the tile to the northWest
							if (mapPosition.x > 0)
							{
								Tile* tileNorthWest = _save->getTile(mapPosition - Position(1, 1, 0));
								if (tileNorthWest->isDiscovered(2))
									tileNorthWestShade = tileNorthWest->getShade();
								else
									tileNorthWestShade = 16;

								tmpSurface = tileNorthWest->getSprite(MapData::O_OBJECT);
								if (tmpSurface
									&& tileNorthWest->getMapData(MapData::O_OBJECT)->getBigWall() == 7)
								{
									tmpSurface->blitNShade(
														surface,
														screenPosition.x,
														screenPosition.y - tileNorthWest->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y * 2,
														tileNorthWestShade);
								}
							}

							// Phase IV: render any south or east wall type objects in the tile to the north
							if (tileNorth->getMapData(MapData::O_OBJECT)
								&& tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() >= 6)
							{
								tmpSurface = tileNorth->getSprite(MapData::O_OBJECT);
								if (tmpSurface)
									tmpSurface->blitNShade(
														surface,
														screenPosition.x + tileOffset.x,
														screenPosition.y - tileNorth->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y,
														tileNorthShade);
							}

							if (mapPosition.x > 0)
							{
								// Phase V: re-render objects in the tile to the south west
								// only render half so it won't overlap other areas that are already drawn
								// and only apply this to movement in a north easterly or south westerly direction.
								if ((buNorth->getDirection() == 1
										|| buNorth->getDirection() == 5)
									&& mapPosition.y < endY - 1)
								{
									Tile* tileSouthWest = _save->getTile(mapPosition + Position(-1, 1, 0));
									if (tileSouthWest->isDiscovered(2))
										tileSouthWestShade = tileSouthWest->getShade();
									else
										tileSouthWestShade = 16;

									tmpSurface = tileSouthWest->getSprite(MapData::O_OBJECT);
									if (tmpSurface)
										tmpSurface->blitNShade(
															surface,
															screenPosition.x - tileOffset.x * 2,
															screenPosition.y - tileSouthWest->getMapData(MapData::O_OBJECT)->getYOffset(),
															tileSouthWestShade,
															true);
								}

								// Phase VI: we need to re-render everything in the tile to the west.
								Tile* tileWest = _save->getTile(mapPosition - Position(1, 0, 0));
								BattleUnit* buWest = NULL;

								int tileWestShade;
								if (tileWest->isDiscovered(2))
								{
									buWest = tileWest->getUnit();
									tileWestShade = tileWest->getShade();
								}
								else
									tileWestShade = 16;

								tmpSurface = tileWest->getSprite(MapData::O_WESTWALL);
								if (tmpSurface
									&& buNorth != unit)
								{
									if ((tileWest->getMapData(MapData::O_WESTWALL)->isDoor()
											|| tileWest->getMapData(MapData::O_WESTWALL)->isUFODoor())
										&& tileWest->isDiscovered(0))
									{
										wallShade = tileWest->getShade();
									}
									else
										wallShade = tileWestShade;

									tmpSurface->blitNShade(
														surface,
														screenPosition.x - tileOffset.x,
														screenPosition.y - tileWest->getMapData(MapData::O_WESTWALL)->getYOffset() + tileOffset.y,
														wallShade,
														true);
								}

								tmpSurface = tileWest->getSprite(MapData::O_NORTHWALL);
								if (tmpSurface)
								{
									if ((tileWest->getMapData(MapData::O_NORTHWALL)->isDoor()
											|| tileWest->getMapData(MapData::O_NORTHWALL)->isUFODoor())
										&& tileWest->isDiscovered(1))
									{
										wallShade = tileWest->getShade();
									}
									else
										wallShade = tileWestShade;

									tmpSurface->blitNShade(
														surface,
														screenPosition.x - tileOffset.x,
														screenPosition.y - tileWest->getMapData(MapData::O_NORTHWALL)->getYOffset() + tileOffset.y,
														wallShade,
														true);
								}

								tmpSurface = tileWest->getSprite(MapData::O_OBJECT);
								if (tmpSurface
									&& tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < 6
									&& tileWest->getMapData(MapData::O_OBJECT)->getBigWall() != 3)
								{
									tmpSurface->blitNShade(
														surface,
														screenPosition.x - tileOffset.x,
														screenPosition.y - tileWest->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y,
														tileWestShade,
														true);

									// if the object in the tile to the west is a diagonal big wall, we need to cover up the black triangle at the bottom
									if (tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == 2)
									{
										tmpSurface = tile->getSprite(MapData::O_FLOOR);
										if (tmpSurface)
											tmpSurface->blitNShade(
																surface,
																screenPosition.x,
																screenPosition.y - tile->getMapData(MapData::O_FLOOR)->getYOffset(),
																tileShade);
									}
								}

								// draw an item on top of the floor (if any)
								int sprite = tileWest->getTopItemSprite();
								if (sprite != -1)
								{
									tmpSurface = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
									tmpSurface->blitNShade(
														surface,
														screenPosition.x - tileOffset.x,
														screenPosition.y + tileWest->getTerrainLevel() + tileOffset.y,
														tileWestShade,
														true);
								}

								// Draw soldier
								if (buWest
									&& buWest->getStatus() != STATUS_WALKING
									&& (!tileWest->getMapData(MapData::O_OBJECT)
										|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < 6)
									&& (buWest->getVisible()
										|| _save->getDebugMode()))
								{
									// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
									int part = 0;
									part += tileWest->getPosition().x - buWest->getPosition().x;
									part += (tileWest->getPosition().y - buWest->getPosition().y) * 2;

									tmpSurface = buWest->getCache(&invalid, part);
									if (tmpSurface)
									{
										tmpSurface->blitNShade(
															surface,
															screenPosition.x - tileOffset.x,
															screenPosition.y + tileOffset.y + getTerrainLevel(
																										buWest->getPosition(),
																										buWest->getArmor()->getSize()),
															tileWestShade,
															true);

										if (buWest->getFire() > 0)
										{
											frame = 4 + (_animFrame / 2);
											tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											tmpSurface->blitNShade(
																surface,
																screenPosition.x - tileOffset.x,
																screenPosition.y + tileOffset.y + getTerrainLevel(
																											buWest->getPosition(),
																											buWest->getArmor()->getSize()),
																0,
																true);
										}
									}
								}

								// Draw smoke/fire
								if (tileWest->getSmoke()
									&& tileWest->isDiscovered(2))
								{
									frame = 0;
									int shade = 0;
									if (tileWest->getFire() == 0)
									{
										frame = 7 + ((tileWest->getSmoke() + 1) / 2);
										shade = tileWestShade;
									}

									int offset = _animFrame / 2 + tileWest->getAnimationOffset();
									if (offset > 3)
										offset -= 4;
									frame += offset;

									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									tmpSurface->blitNShade(
											surface,
											screenPosition.x - tileOffset.x,
											screenPosition.y + tileOffset.y,
											shade,
											true);
								}

								// Draw object
								if (tileWest->getMapData(MapData::O_OBJECT)
									&& tileWest->getMapData(MapData::O_OBJECT)->getBigWall() > 5)
								{
									tmpSurface = tileWest->getSprite(MapData::O_OBJECT);
									tmpSurface->blitNShade(
											surface,
											screenPosition.x - tileOffset.x,
											screenPosition.y - tileWest->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y,
											tileWestShade,
											true);
								}
							}
						}
					}
/*kL - their tileWest SMOKE:
								{
									frame = 0;

									if (!tileWest->getFire())
										frame = 8 + int(floor((tileWest->getSmoke() / 6.0) - 0.1)); // see http://www.ufopaedia.org/images/c/cb/Smoke.gif

									if ((_animFrame / 2) + tileWest->getAnimationOffset() > 3)
										frame += ((_animFrame / 2) + tileWest->getAnimationOffset() - 4);
									else
										frame += (_animFrame / 2) + tileWest->getAnimationOffset();

									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									tmpSurface->blitNShade(
														surface,
														screenPosition.x - tileOffset.x,
														screenPosition.y + tileOffset.y,
														0,
														true);
								} */
// END ADVANCED DRAWING CYCLE


					// Draw walls
					if (!tile->isVoid())
					{
						// Draw west wall
						tmpSurface = tile->getSprite(MapData::O_WESTWALL);
						if (tmpSurface)
						{
							if ((tile->getMapData(MapData::O_WESTWALL)->isDoor()
									|| tile->getMapData(MapData::O_WESTWALL)->isUFODoor())
								&& tile->isDiscovered(0))
							{
								wallShade = tile->getShade();
							}
							else
								wallShade = tileShade;

							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y - tile->getMapData(MapData::O_WESTWALL)->getYOffset(),
									wallShade);
						}

						// Draw north wall
						tmpSurface = tile->getSprite(MapData::O_NORTHWALL);
						if (tmpSurface)
						{
							if ((tile->getMapData(MapData::O_NORTHWALL)->isDoor()
									|| tile->getMapData(MapData::O_NORTHWALL)->isUFODoor())
								&& tile->isDiscovered(1))
							{
								wallShade = tile->getShade();
							}
							else
								wallShade = tileShade;

							if (tile->getMapData(MapData::O_WESTWALL))
								tmpSurface->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y - tile->getMapData(MapData::O_NORTHWALL)->getYOffset(),
										wallShade,
										true);
							else
								tmpSurface->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y - tile->getMapData(MapData::O_NORTHWALL)->getYOffset(),
										wallShade);
						}

						// Draw object
						if (tile->getMapData(MapData::O_OBJECT)
							&& tile->getMapData(MapData::O_OBJECT)->getBigWall() < 6)
						{
							object = true; // kL

							tmpSurface = tile->getSprite(MapData::O_OBJECT);
							if (tmpSurface)
								tmpSurface->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y - tile->getMapData(MapData::O_OBJECT)->getYOffset(),
										tileShade);
						}

						// draw an item on top of the floor (if any)
						int sprite = tile->getTopItemSprite();
						if (sprite != -1)
						{
//							object = true; // kL

							tmpSurface = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y + tile->getTerrainLevel(),
									tileShade);
						}

						// kL_begin:
						if (floor == false
							&& object == false)
						{
							Tile* tileBelow = _save->getTile(mapPosition + Position(0, 0,-1));
							if (tileBelow != NULL)
							{
								// make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
								BattleUnit* buBelow = tileBelow->getUnit();
								if (buBelow != NULL
									&& buBelow->getFaction() == FACTION_PLAYER
									&& buBelow != _save->getSelectedUnit()
									&& buBelow->getTurretType() == -1) // no tanks, pls
								{
									Position offset;
									calculateWalkingOffset(buBelow, &offset);

									if (buBelow->getFatalWounds() > 0)
									{
										tmpSurface = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (tmpSurface != NULL)
										{
											tmpSurface->blitNShade(
													surface,
													screenPosition.x + offset.x + 2,
													screenPosition.y + offset.y + 3 + 24,
													0);
										}

										tmpSurface = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // 11, small gray cross; 209, big red cross
										tmpSurface->blitNShade(
												surface,
												screenPosition.x + offset.x + 4,
												screenPosition.y + offset.y + 4 + 24,
												0,
												false,
												3); // 1=white, 3=red
									}
									else
									{
										std::string soldierRank = buBelow->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
										soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

										tmpSurface = _res->getSurface(soldierRank);
										if (tmpSurface != NULL)
										{
											tmpSurface->blitNShade(
													surface,
													screenPosition.x + offset.x + 2,
													screenPosition.y + offset.y + 3 + 24,
													0);
										}
									}
								}
							}
						}
					}

					// check if we got bullet && it is in Field Of View
					if (_projectile)
//kL						&& _projectileInFOV)
					{
						tmpSurface = 0;

						if (_projectile->getItem()) // thrown item ( grenade, etc.)
						{
							tmpSurface = _projectile->getSprite();

							Position voxelPos = _projectile->getPosition();
							// draw shadow on the floor
							voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
							if (voxelPos.x / 16 >= itX
								&& voxelPos.y / 16 >= itY
								&& voxelPos.x / 16 <= itX + 1
								&& voxelPos.y / 16 <= itY + 1
								&& voxelPos.z / 24 == itZ)
//kL							&& _save->getTileEngine()->isVoxelVisible(voxelPos))
							{
								_camera->convertVoxelToScreen(
															voxelPos,
															&bulletScreen);
								tmpSurface->blitNShade(
										surface,
										bulletScreen.x - 16,
										bulletScreen.y - 26,
										16);
							}

							voxelPos = _projectile->getPosition();
							// draw thrown object
							if (voxelPos.x / 16 >= itX
								&& voxelPos.y / 16 >= itY
								&& voxelPos.x / 16 <= itX + 1
								&& voxelPos.y / 16 <= itY + 1
								&& voxelPos.z / 24 == itZ)
//kL							&& _save->getTileEngine()->isVoxelVisible(voxelPos))
							{
								_camera->convertVoxelToScreen(
															voxelPos,
															&bulletScreen);
								tmpSurface->blitNShade(
										surface,
										bulletScreen.x - 16,
										bulletScreen.y - 26,
										0);
							}
						}
						else // fired projectile ( a bullet-sprite, not a thrown item )
						{
							// draw bullet on the correct tile
							if (itX >= bulletLowX
								&& itX <= bulletHighX
								&& itY >= bulletLowY
								&& itY <= bulletHighY)
							{
//								for (int
//										i = 0;
//										i < BULLET_SPRITES;
//										++i)
								int begin = 0;
								int end = BULLET_SPRITES;
								int direction = 1;
								if (_projectile->isReversed())
								{
									begin = BULLET_SPRITES - 1;
									end = -1;
									direction = -1;
								}

								for (int
										i = begin;
										i != end;
										i += direction)
								{
									tmpSurface = _projectileSet->getFrame(_projectile->getParticle(i));
									if (tmpSurface)
									{
										Position voxelPos = _projectile->getPosition(1 - i);
										// draw shadow on the floor
										voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
										if (voxelPos.x / 16 == itX
											&& voxelPos.y / 16 == itY
											&& voxelPos.z / 24 == itZ)
//kL										&& _save->getTileEngine()->isVoxelVisible(voxelPos))
										{
											_camera->convertVoxelToScreen(
																		voxelPos,
																		&bulletScreen);

											bulletScreen.x -= tmpSurface->getWidth() / 2;
											bulletScreen.y -= tmpSurface->getHeight() / 2;
											tmpSurface->blitNShade(
													surface,
													bulletScreen.x,
													bulletScreen.y,
													16);
										}

										// draw bullet itself
										voxelPos = _projectile->getPosition(1 - i);
										if (voxelPos.x / 16 == itX
											&& voxelPos.y / 16 == itY
											&& voxelPos.z / 24 == itZ)
//kL										&& _save->getTileEngine()->isVoxelVisible(voxelPos))
										{
											_camera->convertVoxelToScreen(
																		voxelPos,
																		&bulletScreen);

											bulletScreen.x -= tmpSurface->getWidth() / 2;
											bulletScreen.y -= tmpSurface->getHeight() / 2;
											tmpSurface->blitNShade(
													surface,
													bulletScreen.x,
													bulletScreen.y,
													0);
										}
									}
								}
							}
						}
					}

					// Draw soldier
					unit = tile->getUnit();
					if (unit
						&& (unit->getVisible()
							|| _save->getDebugMode()))
					{
						//Log(LOG_INFO) << "draw Soldier ID = " << unit->getId();

						// the part is 0 for small units, large units have parts 1,2 & 3 depending
						// on the relative x/y position of this tile vs the actual unit position.
						int part = 0;
						part += tile->getPosition().x - unit->getPosition().x;
						part += (tile->getPosition().y - unit->getPosition().y) * 2;

						tmpSurface = unit->getCache(&invalid, part);
						if (tmpSurface)
						{
							Position offset;
							calculateWalkingOffset(unit, &offset);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x + offset.x,
									screenPosition.y + offset.y,
									tileShade);

							if (unit->getFire() > 0)
							{
								frame = 4 + (_animFrame / 2);
								tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
								tmpSurface->blitNShade(
										surface,
										screenPosition.x + offset.x,
										screenPosition.y + offset.y,
										0);
							}

							if (unit->getBreathFrame() > 0)
							{
//kL							if (unit->getStatus() == STATUS_AIMING) // enlarge the unit sprite when aiming to accomodate the weapon. adjust as necessary.
//kL								offset.x = 0;

								tmpSurface = _res->getSurfaceSet("BREATH-1.PCK")->getFrame(unit->getBreathFrame() - 1);
								if (tmpSurface)
								{
									tmpSurface->blitNShade(
											surface,
											screenPosition.x + offset.x,
											screenPosition.y + offset.y - 30,
											tileShade);
								}
							}

							// kL_begin:
//							if (drawRankIcon == true)
							{
								Tile* tileUp = _save->getTile(mapPosition + Position(0, 0, 1));
								if (tileUp
									&& (tileUp->getSprite(MapData::O_FLOOR) == NULL
										|| _camera->getViewLevel() == itZ))
								{
									if (unit->getFaction() == FACTION_PLAYER
										//unit != dynamic_cast<BattleUnit*>(_save->getSelectedUnit());
										&& unit != _save->getSelectedUnit()
										&& unit->getTurretType() == -1) // no tanks, pls
									{
										if (unit->getFatalWounds() > 0)
//											&& unit->getFatalWounds() < 10)
										{
											tmpSurface = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
											if (tmpSurface != NULL)
											{
												tmpSurface->blitNShade(
														surface,
														screenPosition.x + offset.x + 2,
														screenPosition.y + offset.y + 3,
														0);
											}

//											tmpSurface->drawRect(1, 1, 7, 5, 0); // clear it.
//											tmpSurface->drawRect(1, 1, 7, 5, Palette::blockOffset(2)+2); // red block on rankIcon.
											//Log(LOG_INFO) << ". wounded Soldier ID = " << unit->getId();
											tmpSurface = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // 11, small gray cross; 209, big red cross
											tmpSurface->blitNShade(
													surface,
													screenPosition.x + offset.x + 4,
													screenPosition.y + offset.y + 4,
													0,
													false,
													3); // 1=white, 3=red.

//											if (unit->getFatalWounds() < 10) // stick to under 10 wounds ... else double digits.
//											{
/*											SurfaceSet* setDigits = _res->getSurfaceSet("DIGITS");
											tmpSurface = setDigits->getFrame(unit->getFatalWounds());
											if (tmpSurface != NULL)
											{
												tmpSurface->blitNShade(
														surface,
														screenPosition.x + offset.x + 7,
														screenPosition.y + offset.y + 4,
														0,
														false,
														3); // 1=white, 3=red.
											} */
//											}
										}
										else
										{
											std::string soldierRank = unit->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
											soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

											tmpSurface = _res->getSurface(soldierRank);
											if (tmpSurface != NULL)
											{
												tmpSurface->blitNShade(
														surface,
														screenPosition.x + offset.x + 2,
														screenPosition.y + offset.y + 3,
														0);
											}
										}
									}
								}
							} // kL_end.
						}
					}

					// kL_begin:
					if (unit == NULL
						&& tile->getHasUnconsciousSoldier())
					{
						tmpSurface = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
						if (tmpSurface != NULL)
						{
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}

						tmpSurface = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // 11, small gray cross; 209, big red cross
						tmpSurface->blitNShade(
								surface,
								screenPosition.x + 2,
								screenPosition.y + 1,
								0,
								false,
								3); // 1=white, 3=red.

					} // kL_end.

					// if we can see through the floor, draw the soldier below it if it is on stairs
					Tile* tileBelow = _save->getTile(mapPosition + Position(0, 0,-1));

					if (itZ > 0
						&& tile->hasNoFloor(tileBelow))
					{
						Tile* ttile = _save->getTile(Position(itX, itY, itZ - 1));
						BattleUnit* tunit = _save->selectUnit(Position(itX, itY, itZ - 1));
						if (tunit
							&& tunit->getVisible()
							&& ttile->getTerrainLevel() < 0
							&& ttile->isDiscovered(2))
						{
							// the part is 0 for small units, large units have parts 1,2 & 3 depending
							// on the relative x/y position of this tile vs the actual unit position.
							int part = 0;
							part += ttile->getPosition().x - tunit->getPosition().x;
							part += (ttile->getPosition().y - tunit->getPosition().y) * 2;

							tmpSurface = tunit->getCache(&invalid, part);
							if (tmpSurface)
							{
								Position offset;
								calculateWalkingOffset(tunit, &offset);
								offset.y += 24;
								tmpSurface->blitNShade(
										surface,
										screenPosition.x + offset.x,
										screenPosition.y + offset.y,
										ttile->getShade());

								if (tunit->getArmor()->getSize() > 1)
									offset.y += 4;

								if (tunit->getFire() > 0)
								{
									frame = 4 + (_animFrame / 2);
									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									tmpSurface->blitNShade(
											surface,
											screenPosition.x + offset.x,
											screenPosition.y + offset.y,
											0);
								}
							}
						}
					}

					// Draw smoke/fire
					if (tile->getSmoke() > 0
						&& tile->isDiscovered(2))
					{
						frame = 0;
						int shade = 0;
						if (tile->getFire() == 0)
						{
							frame = 7 + ((tile->getSmoke() + 1) / 2);
							shade = tileShade;
						}

						int offset = _animFrame / 2 + tile->getAnimationOffset();
						if (offset > 3)
							offset -= 4;
						frame += offset;

						tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
						tmpSurface->blitNShade(
								surface,
								screenPosition.x,
								screenPosition.y,
								shade);
					}

					// Draw Path Preview
					if (tile->getPreview() != -1
						&& tile->isDiscovered(0)
						&& (_previewSetting & PATH_ARROWS))
					{
						if (itZ > 0
							&& tile->hasNoFloor(tileBelow))
						{
							tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(11);
							if (tmpSurface)
							{
								tmpSurface->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y + 2,
										0,
										false,
										tile->getMarkerColor());
							}
						}

						tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(tile->getPreview());
						if (tmpSurface)
						{
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
//kL								screenPosition.y + tile->getTerrainLevel(),
									screenPosition.y, // kL
									0,
									false,
									tileColor);
						}
					}

					if (!tile->isVoid()) // THIS CAME BEFORE Draw Path Preview above in Old builds.
					{
						// Draw object
						if (tile->getMapData(MapData::O_OBJECT)
							&& tile->getMapData(MapData::O_OBJECT)->getBigWall() >= 6)
						{
							tmpSurface = tile->getSprite(MapData::O_OBJECT);
							if (tmpSurface)
								tmpSurface->blitNShade(
													surface,
													screenPosition.x,
													screenPosition.y - tile->getMapData(MapData::O_OBJECT)->getYOffset(),
													tileShade);
						}
					}

					// Draw cursor front
					if (_cursorType != CT_NONE
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX < itX + 1
						&& _selectorY < itY + 1
						&& !_save->getBattleState()->getMouseOverIcons())
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit
									&& (unit->getVisible() || _save->getDebugMode()))
								{
									frame = 3 + (_animFrame %2); // yellow box
								}
								else
									frame = 3; // red box
							}
							else
							{
								if (unit
									&& (unit->getVisible() || _save->getDebugMode()))
								{
									frame = 7 + (_animFrame / 2); // yellow animated crosshairs
								}
								else
									frame = 6; // red static crosshairs
							}

							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);

							// UFOExtender Accuracy: display adjusted accuracy value on crosshair (and more).
							if (Options::battleUFOExtenderAccuracy)
							{
//								BattleAction* action = _save->getBattleGame()->getCurrentAction();
/*								if (action->type == BA_SNAPSHOT
									|| action->type == BA_AUTOSHOT
									|| action->type == BA_AIMEDSHOT
									|| action->type == BA_THROW) */
//								{
								if (_cursorType == CT_AIM) // indicator for Firing.
								{
									// kL_note: Use stuff from ProjectileFlyBState::init()
									// as well as TileEngine::canTargetUnit() & TileEngine::canTargetTile()
									// to turn accuracy to 'red 0' if target is out of LoS/LoF.
									BattleAction* action = _save->getBattleGame()->getCurrentAction();

									int accuracy = static_cast<int>( // _save->getSelectedUnit()->
															action->actor->getFiringAccuracy(
																						action->type,
																						action->weapon)
																					* 100.0);

									RuleItem* weapon = action->weapon->getRules();

									int
										upperLimit = 200,
										lowerLimit = weapon->getMinRange(),
										distance = _save->getTileEngine()->distance(
																				Position(
																						itX,
																						itY,
																						itZ),
																				action->actor->getPosition());

									switch (action->type)
									{
										case BA_AIMEDSHOT:
											upperLimit = weapon->getAimRange();
										break;
										case BA_SNAPSHOT:
											upperLimit = weapon->getSnapRange();
										break;
										case BA_AUTOSHOT:
											upperLimit = weapon->getAutoRange();
										break;

										default:
										break;
									}

									Uint8 color = Palette::blockOffset(3)+3; // green

									if (distance > upperLimit)
									{
										accuracy -= (distance - upperLimit) * weapon->getDropoff();
										color = Palette::blockOffset(1)+2; // orange
									}
									else if (distance < lowerLimit)
									{
										accuracy -= (lowerLimit - distance) * weapon->getDropoff();
										color = Palette::blockOffset(1)+2; // orange
									}

									if (accuracy < 1 // zero accuracy or out of range: set it red.
										|| distance > weapon->getMaxRange())
									{
										accuracy = 0;
										color = Palette::blockOffset(2)+3; // red
									}

									//Log(LOG_INFO) << "Map::drawTerrain(), accuracy = " << accuracy;
									_txtAccuracy->setValue(static_cast<unsigned>(accuracy));
									_txtAccuracy->setColor(color);
									_txtAccuracy->draw();
									_txtAccuracy->blitNShade(
														surface,
														screenPosition.x,
														screenPosition.y,
														0);
								}
								else if (_cursorType == CT_THROW) // indicator for Throwing.
								{
									//Log(LOG_INFO) << "Map::drawTerrain()";
									BattleAction* action = _save->getBattleGame()->getCurrentAction();
									Position
										posOrigin = action->actor->getPosition(),
										posTarget = Position(
															itX,
															itY,
															itZ);
									action->target = posTarget;

									Position originVoxel = _save->getTileEngine()->getOriginVoxel(
																								*action,
																								0);
									Position targetVoxel = Position(
																itX * 16 + 8,
																itY * 16 + 8,
																itZ * 24 + 2);
									targetVoxel.z -= _save->getTile(posTarget)->getTerrainLevel();
									//Log(LOG_INFO) << ". originVoxel = " << originVoxel;
									//Log(LOG_INFO) << ". targetVoxel = " << targetVoxel;

									unsigned accuracy = 0;
									Uint8 color = Palette::blockOffset(2)+3; // red

									bool canThrow = _save->getTileEngine()->validateThrow(
																						*action,
																						originVoxel,
																						targetVoxel);
									//Log(LOG_INFO) << ". canThrow = " << canThrow;
									if (canThrow)
									{
										accuracy = static_cast<unsigned>(_save->getSelectedUnit()->getThrowingAccuracy() * 100.0);
										color = Palette::blockOffset(3)+3; // green
									}

									_txtAccuracy->setValue(accuracy);
									_txtAccuracy->setColor(color);
									_txtAccuracy->draw();
									_txtAccuracy->blitNShade(
														surface,
														screenPosition.x,
														screenPosition.y,
														0);
								}
//								}
							}
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frame = 5; // blue box

							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}

						if (_cursorType > 2
							&& _camera->getViewLevel() == itZ)
						{
							int aFrame[6] = {0, 0, 0, 11, 13, 15};

							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(aFrame[_cursorType] + (_animFrame / 4));
							tmpSurface->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}
					}

					// Draw waypoints if any on this tile
					int
						waypid = 1,
						waypXOff = 2,
						waypYOff = 2;

					for (std::vector<Position>::const_iterator
							i = _waypoints.begin();
							i != _waypoints.end();
							++i)
					{
						if (*i == mapPosition)
						{
							if (waypXOff == 2
								&& waypYOff == 2)
							{
								tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(7);
								tmpSurface->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y,
										0);
							}

							if (_save->getBattleGame()->getCurrentAction()->type == BA_LAUNCH)
							{
								wpID->setValue(waypid);
								wpID->draw();
								wpID->blitNShade(
										surface,
										screenPosition.x + waypXOff,
										screenPosition.y + waypYOff,
										0);

								waypXOff += waypid > 9? 8: 6;
								if (waypXOff >= 26)
								{
									waypXOff = 2;
									waypYOff += 8;
								}
							}
						}

						waypid++;
					}


					// kL_begin:
					if (itZ == 0) // draw border-marks only on ground tiles
					{
						if (itX == 0
							|| itX == _save->getMapSizeX() - 1
							|| itY == 0
							|| itY == _save->getMapSizeY() - 1)
						{
/*							SCANG.DAT:
							264 - gray, dark
							043 - gray, medium dark
							377 - gray, dark (bit of slate blue)
							233 - gray, medium
							376 - gray, medium light (bit of slate blue)
							392 - gray, light
							330 - gray, square cross
							331 - brown, small light square
							373 - gray, fancy cross
							409 - blue, big square */
							tmpSurface = _res->getSurfaceSet("SCANG.DAT")->getFrame(330);
							tmpSurface->blitNShade(
									surface,
									screenPosition.x + 14,
									screenPosition.y + 31,
									0);
						}
					} // kL_end.
				}
				// is inside the Surface
			}
			// end Tiles_y looping.
		}
		// end Tiles_x looping.
	}
	// end Tiles_z looping.


//kL	unit = (BattleUnit*)_save->getSelectedUnit();
	unit = dynamic_cast<BattleUnit*>(_save->getSelectedUnit()); // kL
	if (unit
		&& (_save->getSide() == FACTION_PLAYER
			|| _save->getDebugMode())
		&& unit->getPosition().z <= _camera->getViewLevel())
	{
		_camera->convertMapToScreen(
								unit->getPosition(),
								&screenPosition);
		screenPosition += _camera->getMapOffset();

		Position offset;
		calculateWalkingOffset(
							unit,
							&offset);

		offset.y += 21 - unit->getHeight();

		if (unit->getArmor()->getSize() > 1)
			offset.y += 6;

		if (unit->isKneeled()
			&& getCursorType() != CT_NONE) // DarkDefender
		{
			offset.y -= 5;
			_arrow_kneel->blitNShade( // DarkDefender
								surface,
								screenPosition.x
									+ offset.x
									+ (_spriteWidth / 2)
									- (_arrow->getWidth() / 2),
								screenPosition.y
									+ offset.y
									- _arrow->getHeight()
//kL								+ arrowBob[_cursorFrame],
									+ static_cast<int>( // kL
//										4.0 * sin((static_cast<double>(_animFrame) * 2.0 * M_PI) / 8.0)),
//										-4.0 * sin(180.0 / static_cast<double>(_animFrame + 1) / 8.0)),
										-4.0 * sin(22.5 / static_cast<double>(_animFrame + 1))),
								0);
		}
		else // DarkDefender
		if (getCursorType() != CT_NONE)
			_arrow->blitNShade(
							surface,
							screenPosition.x
								+ offset.x
								+ (_spriteWidth / 2)
								- (_arrow->getWidth() / 2),
							screenPosition.y
								+ offset.y
								- _arrow->getHeight()
//kL							+ arrowBob[_animFrame],
//kL							+ arrowBob[_cursorFrame], // DarkDefender
								+ static_cast<int>( // kL
//									4.0 * sin((static_cast<double>(_animFrame) * 2.0 * M_PI) / 8.0)),
//									4.0 * sin(static_cast<double>(4 - _animFrame) * 45.0 / 8.0)), // fast up, slow down
//									4.0 * sin(static_cast<double>(_animFrame - 4) * 22.5)),
//									4.0 * sin(static_cast<double>(_animFrame) * M_PI / 4.0)),
//									4.0 * sin(180.0 / static_cast<double>(_animFrame + 1) / 8.0)),
									4.0 * sin(22.5 / static_cast<double>(_animFrame + 1))),
							0);
	}

	if (pathPreview)
	{
		// make a border for the pathfinding display
//		if (wpID)
//			wpID->setBordered(true);

		for (int
				itZ = beginZ;
				itZ <= endZ;
				itZ++)
		{
			for (int
					itX = beginX;
					itX <= endX;
					itX++)
			{
				for (int
						itY = beginY;
						itY <= endY;
						itY++)
				{
					mapPosition = Position(
										itX,
										itY,
										itZ);
					_camera->convertMapToScreen(
											mapPosition,
											&screenPosition);
					screenPosition += _camera->getMapOffset();

					// only render cells that are inside the surface
					if (screenPosition.x > -_spriteWidth
						&& screenPosition.x < surface->getWidth() + _spriteWidth
						&& screenPosition.y > -_spriteHeight
						&& screenPosition.y < surface->getHeight() + _spriteHeight)
					{
						tile = _save->getTile(mapPosition);

						if (!tile
							|| !tile->isDiscovered(0)
							|| tile->getPreview() == -1)
						{
							continue;
						}

//kL					int offset_y = -tile->getTerrainLevel();
						int offset_y = 0; // kL
						if (_previewSetting & PATH_ARROWS)
						{
							Tile* tileBelow = _save->getTile(mapPosition - Position(0, 0, 1));

							if (itZ > 0
								&& tile->hasNoFloor(tileBelow))
							{
								tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(23);
								if (tmpSurface)
									tmpSurface->blitNShade(
											surface,
											screenPosition.x,
											screenPosition.y + 2,
											0,
											false,
											tile->getMarkerColor());
							}

							int overlay = tile->getPreview() + 12;
							tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(overlay);
							if (tmpSurface)
								tmpSurface->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y - offset_y,
										0,
										false,
										tile->getMarkerColor());
						}

						if ((_previewSetting & PATH_TU_COST)
							&& tile->getTUMarker() > -1)
						{
							int offset_x = 2;
							if (tile->getTUMarker() > 9)
								offset_x = 4;

							if (_save->getSelectedUnit()
								&& _save->getSelectedUnit()->getArmor()->getSize() > 1)
							{
								offset_y += 1;

								if (!(_previewSetting & PATH_ARROWS))
									offset_y += 7;
							}

							wpID->setValue(tile->getTUMarker());
							wpID->draw();

							if (!(_previewSetting & PATH_ARROWS))
							{
								wpID->blitNShade(
												surface,
												screenPosition.x + 16 - offset_x,
//kL											screenPosition.y + 29 - offset_y,
												screenPosition.y + 37 - offset_y, // kL
												0,
												false,
												tile->getMarkerColor());
							}
							else
							{
								wpID->blitNShade(
												surface,
												screenPosition.x + 16 - offset_x,
//kL											screenPosition.y + 22 - offset_y,
												screenPosition.y + 30 - offset_y, // kL
												0);
							}
						}
					}
				}
			}
		}

		// remove the border in case it's being used for missile waypoints.
//		if (wpID)
//			wpID->setBordered(false);
	}

	delete wpID;

	if (_explosionInFOV) // check if we got hit or explosion animations
	{
//		for (std::set<Explosion*>::const_iterator // kL
		for (std::list<Explosion*>::const_iterator // expl CTD
				i = _explosions.begin();
				i != _explosions.end();
				++i)
		{
			_camera->convertVoxelToScreen(
									(*i)->getPosition(),
									&bulletScreen);
			if ((*i)->isBig())
			{
				if ((*i)->getCurrentFrame() > -1)
				{
					tmpSurface = _res->getSurfaceSet("X1.PCK")->getFrame((*i)->getCurrentFrame());
					tmpSurface->blitNShade(
							surface,
							bulletScreen.x - 64,
							bulletScreen.y - 64,
							0);
				}
			}
			else if ((*i)->isHit())
			{
				tmpSurface = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame());
				tmpSurface->blitNShade(
						surface,
						bulletScreen.x - 15,
						bulletScreen.y - 25,
						0);
			}
			else
			{
				tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
				tmpSurface->blitNShade(
						surface,
						bulletScreen.x - 15,
						bulletScreen.y - 15,
						0);
			}
		}
	}

	surface->unlock();
	//Log(LOG_INFO) << "Map::drawTerrain() EXIT";
}

/**
 * Handles mouse presses on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);
	_camera->mousePress(action, state);
}

/**
 * Handles mouse releases on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);
	_camera->mouseRelease(action, state);
}

/**
 * Handles keyboard presses on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);
	_camera->keyboardPress(action, state);
}

/**
 * Handles keyboard releases on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::keyboardRelease(Action* action, State* state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_camera->keyboardRelease(action, state);
}

/**
 * Handles mouse over events on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::mouseOver(Action* action, State* state)
{
	InteractiveSurface::mouseOver(action, state);
	_camera->mouseOver(action, state);

	_mouseX = static_cast<int>(action->getAbsoluteXMouse());
	_mouseY = static_cast<int>(action->getAbsoluteYMouse());
	setSelectorPosition(
					_mouseX,
					_mouseY);
}

/**
 * Handles animating tiles. 8 Frames per animation.
 * @param redraw - true to redraw the battlescape map
 */
void Map::animate(bool redraw)
{
	_animFrame++;
 	if (_animFrame == 8)
		_animFrame = 0;

//kL	_cursorFrame++;			// DarkDefender
//kL	if (_cursorFrame == 10)	// DarkDefender
//kL		_cursorFrame = 0;	// DarkDefender

	// kL_begin:
/*	if (_animUp)
	{
		_animFrame++;
		if (_animFrame == 7)
			_animUp = false;
	}
	else
	{
		_animFrame--;
		if (_animFrame == 0)
			_animUp = true;
	} */ // kL_end.

	// animate tiles
	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		_save->getTiles()[i]->animate();
	}

	// animate certain units (large flying units have a propulsion animation)
	for (std::vector<BattleUnit*>::iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (_save->getDepth() > 0
			&& !(*i)->getFloorAbove())
		{
			(*i)->breathe();
		}

		if ((*i)->getArmor()->getConstantAnimation())
		{
			(*i)->setCache(NULL);
			cacheUnit(*i);
		}
	}

	if (redraw)
		_redraw = true;
}

/**
 * Sets the rectangular selector to a certain tile.
 * @param mx Mouse x position.
 * @param my Mouse y position.
 */
void Map::setSelectorPosition(
		int mx,
		int my)
{
	int
		oldX = _selectorX,
		oldY = _selectorY;

	_camera->convertScreenToMap(
								mx,
								my + _spriteHeight / 4,
								&_selectorX,
								&_selectorY);

	if (oldX != _selectorX
		|| oldY != _selectorY)
	{
		_redraw = true;
	}
}

/**
 * Gets the position of the rectangular selector.
 * @param pos Pointer to a position.
 */
void Map::getSelectorPosition(Position* pos) const
{
	pos->x = _selectorX;
	pos->y = _selectorY;
	pos->z = _camera->getViewLevel();
}

/**
 * Calculates the offset of a soldier, when it is walking between 2 tiles.
 * @param unit, Pointer to BattleUnit.
 * @param offset, Pointer to the offset to return the calculation.
 */
void Map::calculateWalkingOffset(
		BattleUnit* unit,
		Position* offset)
{
	int
		offsetX[8] = {1, 1, 1, 0,-1,-1,-1, 0},
		offsetY[8] = {1, 0,-1,-1,-1, 0, 1, 1},

		phase = unit->getWalkingPhase() + unit->getDiagonalWalkingPhase(),

		dir = unit->getDirection(),

		midphase = 4 + 4 * (dir %2),
		endphase = 8 + 8 * (dir %2),

		size = unit->getArmor()->getSize();

	if (size > 1)
	{
		if (dir < 1 || dir > 5)
			midphase = endphase;
		else if (dir == 5)
			midphase = 12;
		else if (dir == 1)
			midphase = 5;
		else
			midphase = 1;
	}

	if (unit->getVerticalDirection())
	{
		midphase = 4;
		endphase = 8;
	}
	else if (unit->getStatus() == STATUS_WALKING
		|| unit->getStatus() == STATUS_FLYING)
	{
		if (phase < midphase)
		{
			offset->x = phase * 2 * offsetX[dir];
			offset->y = - phase * offsetY[dir];
		}
		else
		{
			offset->x = (phase - endphase) * 2 * offsetX[dir];
			offset->y = - (phase - endphase) * offsetY[dir];
		}
	}

	// If we are walking in between tiles, interpolate its terrain level.
	if (unit->getStatus() == STATUS_WALKING
		|| unit->getStatus() == STATUS_FLYING)
	{
		if (phase < midphase)
		{
			int
				fromLevel = getTerrainLevel(
										unit->getPosition(),
										size),
				toLevel = getTerrainLevel(
										unit->getDestination(),
										size);

			if (unit->getPosition().z > unit->getDestination().z)
				// going down a level, so toLevel 0 becomes +24, -8 becomes  16
				toLevel += 24*(unit->getPosition().z - unit->getDestination().z);
			else if (unit->getPosition().z < unit->getDestination().z)
				// going up a level, so toLevel 0 becomes -24, -8 becomes -16
				toLevel = -24*(unit->getDestination().z - unit->getPosition().z) + abs(toLevel);

			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * phase) / endphase);
		}
		else
		{
			// from phase 4 onwards the unit behind the scenes already is on the destination tile
			// we have to get its last position to calculate the correct offset
			int
				fromLevel = getTerrainLevel(
										unit->getLastPosition(),
										size),
				toLevel = getTerrainLevel(
										unit->getDestination(),
										size);

			if (unit->getLastPosition().z > unit->getDestination().z)
				// going down a level, so fromLevel 0 becomes -24, -8 becomes -32
				fromLevel -= 24*(unit->getLastPosition().z - unit->getDestination().z);
			else if (unit->getLastPosition().z < unit->getDestination().z)
				// going up a level, so fromLevel 0 becomes +24, -8 becomes 16
				fromLevel = 24*(unit->getDestination().z - unit->getLastPosition().z) - abs(fromLevel);

			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * phase) / endphase);
		}
	}
	else
	{
		offset->y += getTerrainLevel(
								unit->getPosition(),
								size);

/*		if (unit->getArmor()->getDrawingRoutine() == 0
			|| unit->getArmor()->getDrawingRoutine() == 1
			|| unit->getArmor()->getDrawingRoutine() == 4
			|| unit->getArmor()->getDrawingRoutine() == 6
			|| unit->getArmor()->getDrawingRoutine() == 10) */
		if (unit->getArmor()->getCanHoldWeapon())
		{
			if (unit->getStatus() == STATUS_AIMING)
				offset->x = -16;
		}

		if (_save->getDepth() > 0)
		{
			unit->setFloorAbove(false);

			// make sure this unit isn't obscured by the floor above him, otherwise it looks weird.
			if (_camera->getViewLevel() > unit->getPosition().z)
			{
				for (int
						z = _camera->getViewLevel();
						z != unit->getPosition().z;
						--z)
				{
					if (!_save->getTile(Position(
											unit->getPosition().x,
											unit->getPosition().y,
											z))
										->hasNoFloor(0))
					{
						unit->setFloorAbove(true);

						break;
					}
				}
			}
		}
	}
}

/**
  * Terrainlevel goes from 0 to -24 (bottom to top).
  * For a large sized unit, we need to pick the highest
  * terrain level, which is the lowest number...
  * @param pos, Position.
  * @param size, Size of the unit we want to get the level from.
  * @return, terrainlevel.
  */
int Map::getTerrainLevel(
		Position pos,
		int size)
{
	int lowestLevel = 0;
	int lowTest = 0;

	for (int
			x = 0;
			x < size;
			x++)
	{
		for (int
				y = 0;
				y < size;
				y++)
		{
			lowTest = _save->getTile(pos + Position(x, y, 0))->getTerrainLevel();
			if (lowTest < lowestLevel)
				lowestLevel = lowTest;
		}
	}

	return lowestLevel;
}

/**
 * Sets the 3D cursor to selection/aim mode.
 * @param type Cursor type.
 * @param size Size of cursor.
 */
void Map::setCursorType(
		CursorType type,
		int size)
{
	_cursorType = type;

	if (_cursorType == CT_NORMAL)
		_cursorSize = size;
	else
		_cursorSize = 1;
}

/**
 * Gets the cursor type.
 * @return cursortype.
 */
CursorType Map::getCursorType() const
{
	return _cursorType;
}

/**
 * Checks all units for if they need to be redrawn.
 */
void Map::cacheUnits()
{
	for (std::vector<BattleUnit*>::iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		cacheUnit(*i);
	}
}

/**
 * Check if a certain unit needs to be redrawn.
 * @param unit, Pointer to battleUnit.
 */
void Map::cacheUnit(BattleUnit* unit)
{
	//Log(LOG_INFO) << "cacheUnit() : " << unit->getId();
	UnitSprite* unitSprite = new UnitSprite(
										unit->getStatus() == STATUS_AIMING? _spriteWidth * 2: _spriteWidth,
										_spriteHeight,
										0,
										0,
										_save->getDepth() != 0);
	unitSprite->setPalette(this->getPalette());

//kL	int parts = unit->getArmor()->getSize() == 1? 1: (unit->getArmor()->getSize() * 2);
	int parts = 1;									// kL
	if (unit->getArmor()->getSize() > 1)			// kL
		parts = unit->getArmor()->getSize() * 2;	// kL

	bool
		d = false,
		invalid = false;

	unit->getCache(&invalid);
	if (invalid)
	{
		//Log(LOG_INFO) << ". (invalid)";

		// 1 or 4 iterations, depending on unit size
		for (int
				i = 0;
				i < parts;
				i++)
		{
			//Log(LOG_INFO) << ". . i = " << i;

			Surface* cache = unit->getCache(&d, i);
			if (!cache) // no cache created yet
			{
				//Log(LOG_INFO) << ". . . (!cache)";
				cache = new Surface(
								_spriteWidth,
								_spriteHeight);
				cache->setPalette(this->getPalette());
				//Log(LOG_INFO) << ". . . end (!cache)";
			}

			//Log(LOG_INFO) << ". . cache Sprite & setBattleUnit()";
			cache->setWidth(unitSprite->getWidth());

			unitSprite->setBattleUnit(unit, i);

			//Log(LOG_INFO) << ". . getItem()";
			BattleItem* rhandItem = unit->getItem("STR_RIGHT_HAND");
			BattleItem* lhandItem = unit->getItem("STR_LEFT_HAND");
			if (!lhandItem
				&& !rhandItem)
			{
				unitSprite->setBattleItem(0);
			}
			else
			{
				if (rhandItem)
					unitSprite->setBattleItem(rhandItem);

				if (lhandItem)
					unitSprite->setBattleItem(lhandItem);
			}


			//Log(LOG_INFO) << ". . setSurfaces()";
			unitSprite->setSurfaces(
					_res->getSurfaceSet(unit->getArmor()->getSpriteSheet()),
					_res->getSurfaceSet("HANDOB.PCK"),
					_res->getSurfaceSet("HANDOB2.PCK"));
			//Log(LOG_INFO) << ". . setAnimationFrame() " << _animFrame;
			unitSprite->setAnimationFrame(_animFrame);
			//Log(LOG_INFO) << ". . clear()";
			cache->clear();

			//Log(LOG_INFO) << ". . blit() : cache = " << cache;
			unitSprite->blit(cache);
			//Log(LOG_INFO) << ". . blit() Ok";

			//Log(LOG_INFO) << ". . setCache()";
			unit->setCache(cache, i);
		}
		//Log(LOG_INFO) << ". end (invalid)";
	}

	delete unitSprite;
	//Log(LOG_INFO) << "exit cacheUnit() : " << unit->getId();
}

/**
 * Puts a projectile sprite on the map.
 * @param projectile, Projectile to place.
 */
void Map::setProjectile(Projectile* projectile)
{
	_projectile = projectile;

	if (projectile
		&& Options::battleSmoothCamera)
	{
		_launch = true;
	}

}

/**
 * Gets the current projectile sprite on the map.
 * @return Projectile or 0 if there is no projectile sprite on the map.
 */
Projectile* Map::getProjectile() const
{
	return _projectile;
}

/**
 * Gets a list of explosion sprites on the map.
 * @return A list of explosion sprites.
 */
//std::set<Explosion*>* Map::getExplosions() // kL
std::list<Explosion*>* Map::getExplosions() // expl CTD
{
	return &_explosions;
}

/**
 * Gets the pointer to the camera.
 * @return, Pointer to camera.
 */
Camera* Map::getCamera()
{
	return _camera;
}

/**
 * Timers only work on surfaces so we have to pass this on to the camera object.
 */
void Map::scrollMouse()
{
	_camera->scrollMouse();
}

/**
 * Timers only work on surfaces so we have to pass this on to the camera object.
 */
void Map::scrollKey()
{
	_camera->scrollKey();
}

/**
 * Gets a list of waypoints on the map.
 * @return A list of waypoints.
 */
std::vector<Position>* Map::getWaypoints()
{
	return &_waypoints;
}

/**
 * Sets mouse-buttons' pressed state.
 * @param button Index of the button.
 * @param pressed The state of the button.
 */
void Map::setButtonsPressed(
		Uint8 button,
		bool pressed)
{
	setButtonPressed(button, pressed);
}

/**
 * Sets the unitDying flag.
 * This reveals the dying unit during Hidden Movement.
 * @param flag, True if the unit is dying.
 */
void Map::setUnitDying(bool flag)
{
	_unitDying = flag;
}

/**
 * Updates the selector to the last-known mouse position.
 */
void Map::refreshSelectorPosition()
{
	setSelectorPosition(
					_mouseX,
					_mouseY);
}

/**
 * Special handling for setting the height of the map viewport.
 * @param height the new base screen height.
 */
void Map::setHeight(int height)
{
	Surface::setHeight(height);

	_visibleMapHeight = height - _iconHeight;

	_message->setHeight((_visibleMapHeight < 200)? _visibleMapHeight: 200);
	_message->setY((_visibleMapHeight - _message->getHeight()) / 2);
}

/**
 * Special handling for setting the width of the map viewport.
 * @param width the new base screen width.
 */
void Map::setWidth(int width)
{
	Surface::setWidth(width);

//	_message->setX(width / 2 - _message->getWidth() / 2);
	int dX = width - getWidth();
	_message->setX(_message->getX() + dX / 2);
}

/*
 * Gets the hidden movement screen's vertical position.
 * @return, the vertical position of the hidden movement window
 */
const int Map::getMessageY()
{
	return _message->getY();
}

/**
 * Gets the icon height.
 */
const int Map::getIconHeight()
{
	return _iconWidth;
}

/**
 * Gets the icon width.
 */
const int Map::getIconWidth()
{
	return _iconHeight;
}

}
