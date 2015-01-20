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

#include "Map.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <fstream>

#include "BattlescapeMessage.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "Particle.h"
#include "Pathfinding.h"
#include "Position.h"
#include "Projectile.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "UnitSprite.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
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

/*
Map origin is top corner (NW corner).
- X axis goes downright (width of the map) eastward
- Y axis goes downleft (length of the map) southward
- Z axis goes up (height of the map) upward

   0,0
    /\
y+ /  \ x+
   \  /
    \/      */

namespace OpenXcom
{

bool kL_noReveal = true;


/**
 * Sets up a map with the specified size and position.
 * @param game				- pointer to the core Game
 * @param width				- width in pixels
 * @param height			- height in pixels
 * @param x					- X position in pixels
 * @param y					- Y position in pixels
 * @param visibleMapHeight	- current visible map height
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
		_projectile(NULL),
		_projectileInFOV(false),
		_explosionInFOV(false),
		_launch(false),
		_visibleMapHeight(visibleMapHeight),
		_unitDying(false),
		_reveal(0),
		_smoothingEngaged(false),
		_noDraw(false),
		_save(game->getSavedGame()->getSavedBattle()),
		_res(game->getResourcePack())

{
	_iconWidth = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->w;
	_iconHeight = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->h;
	_messageColor = _game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color;

	_previewSetting	= Options::battleNewPreviewPath;
	if (Options::traceAI == true) // turn everything on because we want to see the markers.
		_previewSetting = PATH_FULL;

	_spriteWidth = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();

	const size_t depth = static_cast<size_t>(_save->getDepth());
	if (_res->getLUTs()->size() > depth)
		_transparencies = &_res->getLUTs()->at(depth);

	_camera = new Camera(
					_spriteWidth,
					_spriteHeight,
					_save->getMapSizeX(),
					_save->getMapSizeY(),
					_save->getMapSizeZ(),
					this,
					visibleMapHeight);

	_hidden = new BattlescapeMessage( // "Hidden Movement..." screen
								320,
								visibleMapHeight < 200? visibleMapHeight: 200,
								0,
								0);
	_hidden->setX(_game->getScreen()->getDX());
	_hidden->setY(_game->getScreen()->getDY());
//	_hidden->setY((visibleMapHeight - _hidden->getHeight()) / 2);
//	_hidden->setTextColor(_game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color);
	_hidden->setTextColor(_messageColor);

	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)& Map::scrollMouse);

	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)& Map::scrollKey);

	_camera->setScrollTimer(
						_scrollMouseTimer,
						_scrollKeyTimer);
/*kL
	_txtAccuracy = new Text(24, 9, 0, 0);
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
	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
	delete _arrow;
	delete _hidden;
	delete _camera;
	delete _txtAccuracy;
}

/**
 * Initializes the map.
 */
void Map::init()
{
	// load the unit-selected bobbing arrow into a surface
	const int
		f = Palette::blockOffset(1),	// Fill, yellow
		b = 14,							// Border, black
		pixels[81] = { 0, 0, b, b, b, b, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   b, b, b, f, f, f, b, b, b,
					   b, f, f, f, f, f, f, f, b,
					   0, b, f, f, f, f, f, b, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, 0, b, f, b, 0, 0, 0,
					   0, 0, 0, 0, b, 0, 0, 0, 0 };

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
	const int pixels_kneel[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, b, 0, 0, 0, 0,
								   0, 0, 0, b, f, b, 0, 0, 0,
								   0, 0, b, f, f, f, b, 0, 0,
								   0, b, f, f, f, f, f, b, 0,
								   b, f, f, f, f, f, f, f, b,
								   b, b, b, f, f, f, b, b, b,
								   0, 0, b, f, f, f, b, 0, 0,
								   0, 0, b, b, b, b, b, 0, 0 };

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

	_projectile = NULL;

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
					Tile* tile = _save->getTile(Position(x,y,z));
					if (tile)
						tile->setDiscovered(true, 2);
				}
			}
		}
	} // kL_end. */
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
 * Draws the whole map, bit by bit.
 */
void Map::draw()
{
	//Log(LOG_INFO) << "Map::draw()";

	// kL_note: removed setting this here and in BattlescapeGame::handleState(),
	// Camera::scrollXY(), ProjectileFlyBState::think() x2.
	// NOTE: The calls to draw() in above functions are prob. redundant,
	// since this state probably refreshes auto, via gameTicks ...
//	if (_redraw == false) return;
//	_redraw = false;

	// Normally call for a Surface::draw();
	// but you don't want to clear the background with color 0, which is transparent
	// (aka black) -- you use color 15 because that actually corresponds to the
	// colour you DO want in all variations of the xcom and tftd palettes.
//	Surface::draw();
	clear(Palette::blockOffset(0)+15);

	const Tile* tile = NULL;

	if (_projectile != NULL)
//		&& _save->getSide() == FACTION_PLAYER)
	{
		//Log(LOG_INFO) << "Map::draw() _projectile = true";
		tile = _save->getTile(Position(
									_projectile->getPosition(0).x / 16,
									_projectile->getPosition(0).y / 16,
									_projectile->getPosition(0).z / 24));
		if (tile != NULL
			&& (tile->getTileVisible() == true
				|| _save->getSide() != FACTION_PLAYER))	// shows projectile during aLien berserk
		{
			//Log(LOG_INFO) << "Map::draw() _projectileInFOV = true";
			_projectileInFOV = true;
		}
	}
	else
		_projectileInFOV = _save->getDebugMode();


	if (_explosions.empty() == false)
	{
		for (std::list<Explosion*>::const_iterator
				i = _explosions.begin();
				i != _explosions.end();
				++i)
		{
			tile = _save->getTile(Position(
										(*i)->getPosition().x / 16,
										(*i)->getPosition().y / 16,
										(*i)->getPosition().z / 24));
			if (tile != NULL
				&& (tile->getTileVisible() == true
					|| (*i)->isBig() == true
					|| _save->getSide() != FACTION_PLAYER))	// kL: shows hit-explosion during aLien berserk
			{
				_explosionInFOV = true;
				break;
			}
		}
	}
	else
		_explosionInFOV = _save->getDebugMode();


	//Log(LOG_INFO) << ". selUnit & selUnit->VISIBLE = " << (_save->getSelectedUnit() && _save->getSelectedUnit()->getVisible());
	//Log(LOG_INFO) << ". selUnit NULL = " << (_save->getSelectedUnit() == NULL);
	//Log(LOG_INFO) << ". unitDying = " << _unitDying;
	//Log(LOG_INFO) << ". deBug = " << _save->getDebugMode();
	//Log(LOG_INFO) << ". projectile = " << _projectileInFOV;
	//Log(LOG_INFO) << ". explosion = " << _explosionInFOV;
	//Log(LOG_INFO) << ". reveal & !preReveal = " << (_reveal > 0 && !kL_noReveal);
	//Log(LOG_INFO) << ". kL_noReveal = " << kL_noReveal;

	if (_noDraw == false) // don't draw if MiniMap is open.
	{
		if (_save->getSelectedUnit() == NULL
			|| (_save->getSelectedUnit() != NULL
				&& _save->getSelectedUnit()->getUnitVisible() == true)
			|| _unitDying == true
			|| _save->getDebugMode() == true
			|| _projectileInFOV == true
			|| _explosionInFOV == true)
		{
			//Log(LOG_INFO) << ". . drawTerrain()";
			kL_noReveal = false;

			drawTerrain(this);
		}
		else
		{
			//Log(LOG_INFO) << ". . blit( Hidden Movement )";
			if (kL_noReveal == false)
			{
				kL_noReveal = true;
				SDL_Delay(372);
			}

			_hidden->blit(this);
		}
	}
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
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

	_hidden->setPalette(colors, firstcolor, ncolors);
	_hidden->setBackground(_res->getSurface("TAC00.SCR"));
	_hidden->initText(
					_res->getFont("FONT_BIG"),
					_res->getFont("FONT_SMALL"),
					_game->getLanguage());
	_hidden->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"));
}

/**
 * Draw the terrain.
 * Keep this function as optimised as possible. It's big so minimise overhead of function calls.
 * @param surface - the surface to draw on
 */
void Map::drawTerrain(Surface* surface)
{
//	static const int arrowBob[8] = {0,1,2,1,0,1,2,1};
//	static const int arrowBob[10] = {0,2,3,3,2,0,-2,-3,-3,-2}; // DarkDefender

	Position bullet;
	int
		bulletLowX	= 16000,
		bulletLowY	= 16000,
		bulletLowZ	= 16000,
		bulletHighX	= 0,
		bulletHighY	= 0,
		bulletHighZ	= 0;

	if (_projectile != NULL // if there is a bullet get the highest x and y tiles to draw it on
		&& _explosions.empty() == true)
	{
		int part;
		if (_projectile->getItem() != NULL)
			part = 0;
		else
			part = BULLET_SPRITES - 1;

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
		bulletLowX  /= 16;
		bulletLowY  /= 16;
		bulletLowZ  /= 24;
		bulletHighX /= 16;
		bulletHighY /= 16;
		bulletHighZ /= 24;

		// if the projectile is outside the viewport - center it back on it
		_camera->convertVoxelToScreen(
								_projectile->getPosition(),
								&bullet);

		if (_projectileInFOV == true)
		{
			if (Options::battleSmoothCamera == true)
			{
				if (_launch == true)
				{
					_launch = false;

					if (   bullet.x < 1
						|| bullet.x > surface->getWidth() - 1
						|| bullet.y < 1
						|| bullet.y > _visibleMapHeight - 1)
					{
						_camera->centerOnPosition(
												Position(
													bulletLowX,
													bulletLowY,
													bulletHighZ),
												false);
						_camera->convertVoxelToScreen(
												_projectile->getPosition(),
												&bullet);
					}
				}

				if (_smoothingEngaged == false)
				{
					const Position target = _projectile->getFinalTarget();
					if (_camera->isOnScreen(target) == false
						|| bullet.x < 1
						|| bullet.x > surface->getWidth() - 1
						|| bullet.y < 1
						|| bullet.y > _visibleMapHeight - 1)
					{
						_smoothingEngaged = true;
					}
				}
				else // smoothing Engaged
				{
					_camera->jumpXY(
								surface->getWidth() / 2 - bullet.x,
								_visibleMapHeight / 2 - bullet.y);

					const int posBullet_z = _projectile->getPosition().z / 24;
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
					if (bullet.x < 0)
					{
						_camera->jumpXY(+surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.x > surface->getWidth())
					{
						_camera->jumpXY(-surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.y < 0)
					{
						_camera->jumpXY(0, +_visibleMapHeight);
						enough = false;
					}
					else if (bullet.y > _visibleMapHeight)
					{
						_camera->jumpXY(0, -_visibleMapHeight);
						enough = false;
					}
					_camera->convertVoxelToScreen(
											_projectile->getPosition(),
											&bullet);
				}
				while (enough == false);
			}
		}
	}
	else // if (no projectile OR explosions waiting)
		_smoothingEngaged = false;

	NumberText* wpID = NULL;
	const bool pathPreview = _save->getPathfinding()->isPathPreviewed();

	if (_waypoints.empty() == false
		|| (pathPreview == true
			&& (_previewSetting & PATH_TU_COST)))
	{
		// note: WpID is used for both pathPreview and BL waypoints.
		// kL_note: Leave them the same color.
		wpID = new NumberText(15, 15, 20, 30);
		wpID->setPalette(getPalette());

		Uint8 wpColor;
/*		if (_save->getTerrain() == "DESERT")
			|| _save->getTerrain() == "DESERTMOUNT")
		{
			wpColor = Palette::blockOffset(0)+1; // white
		else
			wpColor = Palette::blockOffset(1)+4; */
		if (_save->getTerrain() == "POLAR"
			|| _save->getTerrain() == "POLARMOUNT")
		{
			wpColor = Palette::blockOffset(1)+4; // orange
		}
		else
			wpColor = Palette::blockOffset(0)+1; // white

		wpID->setColor(wpColor);
	}


	// get Map's corner-coordinates for rough boundaries in which to draw tiles.
	int
		beginX,
		beginY,
		beginZ = 0,
		endX, //= _save->getMapSizeX() - 1,
		endY, //= _save->getMapSizeY() - 1,
		endZ, //= _camera->getViewLevel(),
		d;

	if (_camera->getShowAllLayers() == true)
		endZ = _save->getMapSizeZ() - 1;
	else
		endZ = _camera->getViewLevel();

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
	if (beginX < 0) beginX = 0;
	if (beginY < 0) beginY = 0;


	BattleUnit* unit = NULL;
	Surface* srfSprite = NULL;
	Tile* tile = NULL;

	Position
		mapPosition,
		screenPosition,
		walkOffset;

	int
		frame,
		tileShade,
		wallShade,
		shade,
		shade2,
		animOffset,
		quad;

	bool
		invalid = false,
		hasFloor, // these denote characteristics of 'tile' as in the current Tile of the loop.
		hasWestWall,
		hasObject,
		unitNorthValid,
		half;

	surface->lock();
	for (int
			itZ = beginZ; // 3. finally lap the levels bottom to top
			itZ <= endZ;
			++itZ)
	{
		for (int
				itX = beginX; // 2. next draw those columns eastward
				itX <= endX;
				++itX)
		{
			for (int
					itY = beginY; // 1. first draw terrain in columns north to south
					itY <= endY;
					++itY)
			{
				mapPosition = Position(
									itX,
									itY,
									itZ);
				_camera->convertMapToScreen(
										mapPosition,
										&screenPosition);
				screenPosition += _camera->getMapOffset();

				if (   screenPosition.x > -_spriteWidth // only render cells that are inside the surface (ie viewport ala player's monitor)
					&& screenPosition.x < surface->getWidth() + _spriteWidth
					&& screenPosition.y > -_spriteHeight
					&& screenPosition.y < surface->getHeight() + _spriteHeight)
				{
					tile = _save->getTile(mapPosition);

					if (tile == NULL)
						continue;

					if (tile->isDiscovered(2) == true)
					{
						unit = tile->getUnit();
						tileShade = tile->getShade();
					}
					else
					{
						unit = NULL;
						tileShade = 16;
					}

//					tileColor = tile->getMarkerColor();
					hasFloor = false;
					hasWestWall = false;
					hasObject = false;
					unitNorthValid = false;

					// Draw floor
					srfSprite = tile->getSprite(MapData::O_FLOOR);
					if (srfSprite)
					{
						srfSprite->blitNShade(
								surface,
								screenPosition.x,
								screenPosition.y - tile->getMapData(MapData::O_FLOOR)->getYOffset(),
								tileShade);

						hasFloor = true;

						// kL_begin #1 of 3:
						// This ensures the rankIcon isn't half-hidden by a floor above & west of soldier.
						// Also, make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						// ... should probably be a subfunction
						if (itX < endX
							&& itZ > 0)
						{
							const Tile* const tileEast = _save->getTile(mapPosition + Position(1,0,0));

							if (tileEast->getSprite(MapData::O_FLOOR) == NULL)
							{
								const Tile* const tileEastBelow = _save->getTile(mapPosition + Position(1,0,-1));

								const BattleUnit* const unitEastBelow = tileEastBelow->getUnit();

								if (unitEastBelow != NULL
									&& unitEastBelow != _save->getSelectedUnit()
									&& unitEastBelow->getGeoscapeSoldier() != NULL
									&& unitEastBelow->getFaction() == unitEastBelow->getOriginalFaction())
								{
									if (unitEastBelow->getFatalWounds() > 0)
									{
										srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													screenPosition.x + 2 + 16,
													screenPosition.y + 3 + 32 + getTerrainLevel(
																							unitEastBelow->getPosition(),
																							unitEastBelow->getArmor()->getSize()),
													0);

										srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													screenPosition.x + 4 + 16,
													screenPosition.y + 4 + 32 + getTerrainLevel(
																							unitEastBelow->getPosition(),
																							unitEastBelow->getArmor()->getSize()),
													_animFrame * 2,
													false,
													3); // 1=white, 3=red
									}
									else
									{
										std::string soldierRank = unitEastBelow->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
										soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

										srfSprite = _res->getSurface(soldierRank);
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													screenPosition.x + 2 + 16,
													screenPosition.y + 3 + 32 + getTerrainLevel(
																							unitEastBelow->getPosition(),
																							unitEastBelow->getArmor()->getSize()),
													0);

/*										const int strength = static_cast<int>(Round(
															 static_cast<double>(unitEastBelow->getBaseStats()->strength) * (unitEastBelow->getAccuracyModifier() / 2. + 0.5)));
										if (unitEastBelow->getCarriedWeight() > strength)
										{
											srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(96); // dot
											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x + 9 + 16,
													screenPosition.y + walkOffset.y + 4 + 32,
													1, // 10
													false,
													1); // 1=white, 2=yellow-red, 3=red, 6=lt.brown, 9=blue
										} */
									}
								}
							}
						} // kL_end.
					}

					// Draw cursor back
					if (_cursorType != CT_NONE
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX < itX + 1
						&& _selectorY < itY + 1
						&& _save->getBattleState()->getMouseOverIcons() == false)
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _save->getDebugMode() == true)
									&& (_cursorType != CT_PSI
										|| unit->getFaction() != _save->getSide()))
								{
									frame = (_animFrame %2); // yellow box
								}
								else
									frame = 0; // red box
							}
							else // CT_AIM ->
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _save->getDebugMode() == true))
								{
									frame = 7 + (_animFrame / 2); // yellow animated crosshairs
								}
								else
									frame = 6; // red static crosshairs
							}

							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							if (srfSprite != NULL)
								srfSprite->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y,
										0);
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frame = 2; // blue box

							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							if (srfSprite != NULL)
								srfSprite->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y,
										0);
						}
					}

//* _START_ADVANCED_DRAWING_CYCLE_ *//
					if (itY > 0) // special handling for a unit moving along the north or west side of a content-object.
					{
						const Tile* const tileNorth = _save->getTile(mapPosition + Position(0,-1,0));

						BattleUnit* const unitNorth = tileNorth->getUnit();

						if (unitNorth != NULL
							&& unitNorth->getUnitVisible() == true
							&& unitNorth->getStatus() == STATUS_WALKING
							&& tileNorth->getTerrainLevel() <= tile->getTerrainLevel())
						{
							// Phase I: re-render the unit to NORTH to make sure it
							// don't get drawn over any walls or under any tiles.
							unitNorthValid = true;

							if (tileNorth->isDiscovered(2) == true)
								shade = tileNorth->getShade();
							else
								shade = 16;

							// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
							// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
							quad = tileNorth->getPosition().x - unitNorth->getPosition().x
								+ (tileNorth->getPosition().y - unitNorth->getPosition().y) * 2;

							srfSprite = unitNorth->getCache(&invalid, quad);
							if (srfSprite)
							{
								calculateWalkingOffset(
													unitNorth,
													&walkOffset);

								srfSprite->blitNShade(
										surface,
										screenPosition.x + walkOffset.x + 16,
										screenPosition.y + walkOffset.y - 8,
										shade);

								if (unitNorth->getFire() != 0)
								{
									frame = 4 + (_animFrame / 2);
									srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									if (srfSprite)
										srfSprite->blitNShade(
												surface,
												screenPosition.x + walkOffset.x + 16,
												screenPosition.y + walkOffset.y - 8,
												0);
								}
							}

							// Phase II: re-render any east wall type objects in the tile to the NORTH of the moving unit;
							// only applies to movement in the north/south direction.
							if (itY > 1
								&& (unitNorth->getDirection() == 0
									|| unitNorth->getDirection() == 4))
							{
								const Tile* const tileNorthNorth = _save->getTile(mapPosition + Position(0,-2,0));

								srfSprite = tileNorthNorth->getSprite(MapData::O_OBJECT);
								if (srfSprite
									&& tileNorthNorth->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_EAST)
								{
									if (tileNorthNorth->isDiscovered(2) == true)
										shade2 = tileNorthNorth->getShade();
									else
										shade2 = 16;

									srfSprite->blitNShade(
											surface,
											screenPosition.x + 32,
											screenPosition.y - 16 - tileNorthNorth->getMapData(MapData::O_OBJECT)->getYOffset(),
											shade2);
								}
							}

							// Phase III: re-render any south bigWall objects in the tile to the NORTH-WEST.
							if (itX > 0)
							{
								const Tile* const tileNorthWest = _save->getTile(mapPosition + Position(-1,-1,0));

								srfSprite = tileNorthWest->getSprite(MapData::O_OBJECT);
								if (srfSprite
									&& tileNorthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_SOUTH)
								{
									if (tileNorthWest->isDiscovered(2) == true)
										shade2 = tileNorthWest->getShade();
									else
										shade2 = 16;

									srfSprite->blitNShade(
											surface,
											screenPosition.x,
											screenPosition.y - 16 - tileNorthWest->getMapData(MapData::O_OBJECT)->getYOffset(),
											shade2);
								}
							}

							// Phase IV: render any south or east bigWall objects in the tile to the NORTH.
							srfSprite = tileNorth->getSprite(MapData::O_OBJECT);
							if (srfSprite
								&& tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() > Pathfinding::BIGWALL_NORTH // do East,South,East_South
								&& tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_W_N)
							{
								srfSprite->blitNShade(
										surface,
										screenPosition.x + 16,
										screenPosition.y - 8 - tileNorth->getMapData(MapData::O_OBJECT)->getYOffset(),
										shade);
							}

							if (itX > 0)
							{
								// Phase V: re-render objects in the tile to the SOUTH-WEST (half-tile).
								if (itY < endY
									&& (unitNorth->getDirection() == 1 // only apply this to movement in a north-easterly or south-westerly direction.
										|| unitNorth->getDirection() == 5))
								{
									const Tile* const tileSouthWest = _save->getTile(mapPosition + Position(-1,1,0));

									if (tileSouthWest->getTerrainLevel() == 0) // Stop concealing lower right bit of unit to west of redrawn tile w/ raised terrain.
									{
										if (tileSouthWest->isDiscovered(2) == true)
											shade = tileSouthWest->getShade();
										else
											shade = 16;

										srfSprite = tileSouthWest->getSprite(MapData::O_OBJECT);
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													screenPosition.x - 32,
													screenPosition.y - tileSouthWest->getMapData(MapData::O_OBJECT)->getYOffset(),
													shade,
													true); // only render halfRight so it won't overlap other areas that are already drawn
									}
								}

								// Phase VI: re-render everything in the tile to the WEST (half-tile).
								const Tile* const tileWest = _save->getTile(mapPosition + Position(-1,0,0));

								BattleUnit* unitWest;

								if (tileWest->isDiscovered(2) == true)
								{
									unitWest = tileWest->getUnit();
									shade = tileWest->getShade();
								}
								else
								{
									unitWest = NULL;
									shade = 16;
								}

								if (unitNorth != unit) // NOT for large units
								{
									srfSprite = tileWest->getSprite(MapData::O_WESTWALL);
									if (srfSprite)
									{
										if (tileWest->isDiscovered(0) == true
											&& (tileWest->getMapData(MapData::O_WESTWALL)->isDoor() == true
												|| tileWest->getMapData(MapData::O_WESTWALL)->isUFODoor() == true))
										{
											wallShade = tileWest->getShade();
										}
										else
											wallShade = shade;

										srfSprite->blitNShade(
												surface,
												screenPosition.x - 16,
												screenPosition.y - 8 - tileWest->getMapData(MapData::O_WESTWALL)->getYOffset(),
												wallShade,
												true); // only render halfRight so it won't overlap other areas that are already drawn
									}
								}

								srfSprite = tileWest->getSprite(MapData::O_NORTHWALL);
								if (srfSprite)
								{
									if (tileWest->isDiscovered(1) == true
										&& (tileWest->getMapData(MapData::O_NORTHWALL)->isDoor() == true
											|| tileWest->getMapData(MapData::O_NORTHWALL)->isUFODoor() == true))
									{
										wallShade = tileWest->getShade();
									}
									else
										wallShade = shade;

									srfSprite->blitNShade(
											surface,
											screenPosition.x - 16,
											screenPosition.y - 8 - tileWest->getMapData(MapData::O_NORTHWALL)->getYOffset(),
											wallShade,
											true); // only render halfRight so it won't overlap other areas that are already drawn
								}

								srfSprite = tileWest->getSprite(MapData::O_OBJECT);
								if (srfSprite
									&& tileWest->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_NWSE // do none,Block,NESW,West,North,West&North
									&& (tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < Pathfinding::BIGWALL_EAST
										|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N))
								{
									srfSprite->blitNShade(
											surface,
											screenPosition.x - 16,
											screenPosition.y - 8 - tileWest->getMapData(MapData::O_OBJECT)->getYOffset(),
											shade,
											true); // only render halfRight so it won't overlap other areas that are already drawn

									// if the object in the tile to the west is a diagonal NESW bigWall
									// it needs to have the black triangle at the bottom covered up
									if (tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW)
									{
										srfSprite = tile->getSprite(MapData::O_FLOOR);
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													screenPosition.x,
													screenPosition.y - tile->getMapData(MapData::O_FLOOR)->getYOffset(),
													tileShade);
									}
								}

								// draw an item on top of the floor (if any)
								const int sprite = tileWest->getTopItemSprite();
								if (sprite != -1)
								{
									srfSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
									srfSprite->blitNShade(
											surface,
											screenPosition.x - 16,
											screenPosition.y - 8 + tileWest->getTerrainLevel(),
											shade,
											true); // only render halfRight so it won't overlap other areas that are already drawn
								}

								if (unitWest != NULL
									&& unitWest != unit // large units don't need to redraw their westerly parts
									&& (unitWest->getUnitVisible() == true
										|| _save->getDebugMode() == true)
									&& (tileWest->getMapData(MapData::O_OBJECT) == NULL
										|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < Pathfinding::BIGWALL_EAST // do none,[Block,diagonals],West,North,West&North
										|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N))
								{
									// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
									// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
									quad = tileWest->getPosition().x - unitWest->getPosition().x
										+ (tileWest->getPosition().y - unitWest->getPosition().y) * 2;

									srfSprite = unitWest->getCache(&invalid, quad);
									if (srfSprite)
									{
										if (unitWest->getStatus() != STATUS_WALKING)
										{
											half = true;
											walkOffset.x = 0;
											walkOffset.y = getTerrainLevel(
																		unitWest->getPosition(),
																		unitWest->getArmor()->getSize());
										}
										else // isWalking
										{
											half = false;
											calculateWalkingOffset(
																unitWest,
																&walkOffset);
										}

										srfSprite->blitNShade(
												surface,
												screenPosition.x - 16 + walkOffset.x,
												screenPosition.y - 8 + walkOffset.y,
												shade,
												half);

										if (unitWest->getFire() != 0)
										{
											frame = 4 + (_animFrame / 2);
											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														screenPosition.x - 16 + walkOffset.x,
														screenPosition.y - 8 + walkOffset.y,
														0,
														half);
										}
									}
								}
								// end unitWest TRUE w/ unitNorth valid

								// Draw SMOKE & FIRE
								if (tileWest->getSmoke() != 0
									&& tileWest->isDiscovered(2) == true)
								{
									if (tileWest->getFire() == 0)
									{
										if (_save->getDepth() > 0)
											frame = ResourcePack::UNDERWATER_SMOKE_OFFSET;
										else
											frame = ResourcePack::SMOKE_OFFSET;

										frame += (tileWest->getSmoke() + 1) / 2;
									}
									else
									{
										frame = 0;
										shade = 0;
									}

									animOffset = _animFrame / 2 + tileWest->getAnimationOffset();
									if (animOffset > 3) animOffset -= 4;
									frame += animOffset;

									srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									if (srfSprite)
										srfSprite->blitNShade(
												surface,
												screenPosition.x - 16,
												screenPosition.y - 8 + tileWest->getTerrainLevel(),
												shade,
												true); // only render halfRight so it won't overlap other areas that are already drawn
								}

								// Draw front bigWall object
								srfSprite = tileWest->getSprite(MapData::O_OBJECT);
								if (srfSprite
									&& tileWest->getMapData(MapData::O_OBJECT)->getBigWall() > Pathfinding::BIGWALL_NORTH // do East,South,East&South
									&& tileWest->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_W_N)
								{
									srfSprite->blitNShade(
											surface,
											screenPosition.x - 16,
											screenPosition.y - 8 - tileWest->getMapData(MapData::O_OBJECT)->getYOffset(),
											shade,
											true); // only render halfRight so it won't overlap other areas that are already drawn
								}
							}
							// end (mapPosition.x > 0)
						}
						// end unitNorth TRUE
					}
					// end (itY > 0)
//* _END_ADVANCED_DRAWING_CYCLE_ *//


					// Draw tile's background
					if (tile->isVoid(true) == false)
					{
						// Draw west wall
						srfSprite = tile->getSprite(MapData::O_WESTWALL);
						if (srfSprite)
						{
							hasWestWall = true;

							if (tile->isDiscovered(0) == true
								&& (tile->getMapData(MapData::O_WESTWALL)->isDoor() == true
									|| tile->getMapData(MapData::O_WESTWALL)->isUFODoor() == true))
							{
								wallShade = tile->getShade();
							}
							else
								wallShade = tileShade;

							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y - tile->getMapData(MapData::O_WESTWALL)->getYOffset(),
									wallShade);
						}

						// Draw north wall
						srfSprite = tile->getSprite(MapData::O_NORTHWALL);
						if (srfSprite)
						{
							if (tile->isDiscovered(1) == true
								&& (tile->getMapData(MapData::O_NORTHWALL)->isDoor() == true
									|| tile->getMapData(MapData::O_NORTHWALL)->isUFODoor() == true))
							{
								wallShade = tile->getShade();
							}
							else
								wallShade = tileShade;

							if (tile->getMapData(MapData::O_WESTWALL) != NULL)
								half = true; // only render halfRight so it won't overlap other areas that are already drawn
							else
								half = false;

							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y - tile->getMapData(MapData::O_NORTHWALL)->getYOffset(),
									wallShade,
									half);
						}

						// Draw back or center object
						srfSprite = tile->getSprite(MapData::O_OBJECT);
						if (srfSprite
							&& (tile->getMapData(MapData::O_OBJECT)->getBigWall() < Pathfinding::BIGWALL_EAST // do none,Block,diagonals,West,North,West&North
								|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N))
						{
							hasObject = true;

							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y - tile->getMapData(MapData::O_OBJECT)->getYOffset(),
									tileShade);
						}

						// Draw item on floor (if any)
						const int sprite = tile->getTopItemSprite();
						if (sprite != -1)
						{
							srfSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
							if (srfSprite)
								srfSprite->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y + tile->getTerrainLevel(),
										tileShade);
						}

						// kL_begin: reDraw unit to WEST. ... unit Walking and no unitNorth exists.
						// TODO: smoke/fire render from advanced cycle below this, use check to draw or not
						// cf. Phase VI in advanced cycle.
						if (itX > 0 // special handling for a moving unit.
							&& unit == NULL // don't draw over unit in front
							&& hasFloor == true
							&& hasObject == false
							&& hasWestWall == false
							&& unitNorthValid == false
							&& (tile->getMapData(MapData::O_OBJECT) == NULL
								|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NORTH))
						{
							const Tile* const tileWest = _save->getTile(mapPosition + Position(-1,0,0));

							BattleUnit* const unitWest = tileWest->getUnit();

							if (unitWest != NULL
								&& unitWest != unit // large units don't need to redraw their westerly parts
								&& unitWest->getStatus() == STATUS_WALKING
								&& (unitWest->getDirection() == 2
									|| unitWest->getDirection() == 3) // unit is walking towards current tile
								&& (unitWest->getUnitVisible() == true
									|| _save->getDebugMode() == true))
							{
								if (tileWest->getMapData(MapData::O_OBJECT) == NULL
									|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < Pathfinding::BIGWALL_EAST // do none,[Block,diagonals],West,North,West&North
									|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N)
								{
									const Tile* const tileSouth = _save->getTile(mapPosition + Position(0,1,0));

									if (tileSouth == NULL
										|| (tileSouth->getMapData(MapData::O_NORTHWALL) == NULL
											&& tileSouth->getMapData(MapData::O_OBJECT) == NULL
											&& tileSouth->getUnit() == NULL))
									{
										const Tile* const tileSouthWest = _save->getTile(mapPosition + Position(-1,1,0));

										if (tileSouthWest == NULL
											|| (tileSouthWest->getUnit() == NULL
												&& tileSouthWest->getMapData(MapData::O_NORTHWALL) == NULL
												&& (tileSouthWest->getMapData(MapData::O_OBJECT) == NULL
													|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NONE
													|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NWSE // trouble
													|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_WEST)))
//													|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_SOUTH)))
										{
											if (tileSouthWest != NULL
												&& tileSouthWest->getMapData(MapData::O_OBJECT) != NULL
												&& (tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NONE
													|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NWSE)) // trouble - should redraw this NWSE_Object ...
											{
												half = true; // only render halfRight so it won't overlap other areas that are already drawn
											}
											else
												half = false;

											// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
											// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
											quad = tileWest->getPosition().x - unitWest->getPosition().x
												+ (tileWest->getPosition().y - unitWest->getPosition().y) * 2;

											srfSprite = unitWest->getCache(&invalid, quad);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (tileWest->isDiscovered(2) == true)
													shade = tileWest->getShade();
												else
													shade = 16;

												calculateWalkingOffset(
																	unitWest,
																	&walkOffset);

												srfSprite->blitNShade(
																	surface,
																	screenPosition.x - 16 + walkOffset.x,
																	screenPosition.y - 8 + walkOffset.y,
																	shade,
																	half); // trouble // only render halfRight so it won't overlap other areas that are already drawn

												if (unitWest->getFire() != 0)
												{
													frame = 4 + (_animFrame / 2);
													srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
													if (srfSprite)
														srfSprite->blitNShade(
																			surface,
																			screenPosition.x - 16 + walkOffset.x,
																			screenPosition.y - 8 + walkOffset.y,
																			0);
												}
											}

											if (half == true) // redraw NWSE bigWall in tileSouthWest
											{
												if (tileSouthWest->isDiscovered(2) == true)
													shade = tileSouthWest->getShade();
												else
													shade = 16;

												srfSprite = tileSouthWest->getSprite(MapData::O_OBJECT);
												if (srfSprite)
													srfSprite->blitNShade(
															surface,
															screenPosition.x - 32,
															screenPosition.y - tileSouthWest->getMapData(MapData::O_OBJECT)->getYOffset(),
															shade,
															true); // only render halfRight so it won't overlap other areas that are already drawn
											}
										}
									}
								}
							}
						} // end draw walking unitWest

						// kL_begin: reDraw unit to NORTH-WEST. ... unit Walking (dir = 3,7).
						// TODO: smoke/fire render from advanced cycle below this, use check to draw or not
						// cf. Phase III in advanced cycle. // special handling for a moving unit.
						if (hasFloor == true //tile->getMapData(MapData::O_FLOOR) != NULL
							&& itX > 0 && itY > 0
							&& unitNorthValid == false
							&& tile->getMapData(MapData::O_WESTWALL) == NULL
							&& tile->getMapData(MapData::O_NORTHWALL) == NULL)
//							&& (tile->getMapData(MapData::O_OBJECT) == NULL // Assumes map-modules don't have walls or objects stuck on raised terrain
//								|| tile->getTerrainLevel() < 0))			// ... other than the pure ground, which I want to overwrite w/ Unit, here.
						{
							const Tile* const tileNorthWest = _save->getTile(mapPosition + Position(-1,-1,0));

							int levelDiff = tileNorthWest->getTerrainLevel() - tile->getTerrainLevel(); // neg.result means that tileNW is higher
							if ((levelDiff > 0 && levelDiff < 8)
								|| tile->getMapData(MapData::O_OBJECT) == NULL)
							{
								BattleUnit* unitNorthWest = tileNorthWest->getUnit();

								if (unitNorthWest != NULL
									&& unitNorthWest != unit // large units don't need to redraw their westerly parts
									&& unitNorthWest->getStatus() == STATUS_WALKING
									&& (unitNorthWest->getDirection() == 3
										|| unitNorthWest->getDirection() == 7)
									&& (unitNorthWest->getUnitVisible() == true
										|| _save->getDebugMode() == true))
								{
									if (tileNorthWest->getMapData(MapData::O_OBJECT) == NULL
										|| tileNorthWest->getMapData(MapData::O_OBJECT)->getBigWall() < Pathfinding::BIGWALL_EAST // do none,[Block, diagonals],West,North,West&North
										|| tileNorthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N)
									{
										// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
										// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
										quad = tileNorthWest->getPosition().x - unitNorthWest->getPosition().x
											+ (tileNorthWest->getPosition().y - unitNorthWest->getPosition().y) * 2;

										srfSprite = unitNorthWest->getCache(&invalid, quad);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileNorthWest->isDiscovered(2) == true)
												shade = tileNorthWest->getShade();
											else
												shade = 16;

											calculateWalkingOffset(
																unitNorthWest,
																&walkOffset);

											srfSprite->blitNShade(
																surface,
																screenPosition.x + walkOffset.x,
																screenPosition.y - 16 + walkOffset.y,
																shade,
																true);

											if (unitNorthWest->getFire() != 0)
											{
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												srfSprite->blitNShade(
																	surface,
																	screenPosition.x + walkOffset.x,
																	screenPosition.y - 16 + walkOffset.y,
																	0,
																	true);
											}
										}

										// if (tileNorth has west or southwall) redraw it.
										const Tile* const tileNorth = _save->getTile(mapPosition + Position(0,-1,0));
										if (tileNorth->getMapData(MapData::O_OBJECT) != NULL
											&& tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == 0) // draw content-object, not bigWall.
//											&& tileNorth->getMapData(MapData::O_OBJECT)->getDataset()->getName() == "LIGHTNIN"
//											&& tileNorth->getMapData(MapData::O_OBJECT)->getSprite(0) == 42) // the ramp's top, redraw it.
										{
											srfSprite = tileNorth->getSprite(MapData::O_OBJECT);
											if (srfSprite)
											{
												if (tileNorth->isDiscovered(2) == true)
													shade = tileNorth->getShade();
												else
													shade = 16;

												srfSprite->blitNShade(
														surface,
														screenPosition.x + 16,
														screenPosition.y - 8 - tileNorth->getMapData(MapData::O_OBJECT)->getYOffset(),
														shade);
											}
											//Log(LOG_INFO) << ". " << Position(mapPosition) << " dataSet = " << tileNorth->getMapData(MapData::O_OBJECT)->getDataset()->getName();
											//Log(LOG_INFO) << ". sprite = " << tileNorth->getMapData(MapData::O_OBJECT)->getSprite(0);
										}
									}
								}
							}
						} // end draw walking unitNorthWest

						// kL_begin #2 of 3: Make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						if (mapPosition.z > 0
							&& hasFloor == false
							&& hasObject == false)
						{
							const Tile* const tileBelow = _save->getTile(mapPosition + Position(0,0,-1));

							BattleUnit* const unitBelow = tileBelow->getUnit();

							if (unitBelow != NULL
								&& unitBelow != _save->getSelectedUnit()
								&& unitBelow->getGeoscapeSoldier() != NULL
								&& unitBelow->getFaction() == unitBelow->getOriginalFaction())
							{
								if (unitBelow->getFatalWounds() > 0)
								{
									srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
									if (srfSprite)
										srfSprite->blitNShade(
												surface,
												screenPosition.x + 2,
												screenPosition.y + 3 + 24 + getTerrainLevel(
																						unitBelow->getPosition(),
																						unitBelow->getArmor()->getSize()),
												0);

									srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
									if (srfSprite)
										srfSprite->blitNShade(
												surface,
												screenPosition.x + 4,
												screenPosition.y + 4 + 24 + getTerrainLevel(
																						unitBelow->getPosition(),
																						unitBelow->getArmor()->getSize()),
												_animFrame * 2,
												false,
												3); // red
								}
								else
								{
									std::string soldierRank = unitBelow->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
									soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

									srfSprite = _res->getSurface(soldierRank);
									if (srfSprite)
										srfSprite->blitNShade(
												surface,
												screenPosition.x + 2,
												screenPosition.y + 3 + 24 + getTerrainLevel(
																						unitBelow->getPosition(),
																						unitBelow->getArmor()->getSize()),
												0);

/*									const int strength = static_cast<int>(Round(
														 static_cast<double>(unitBelow->getBaseStats()->strength) * (unitBelow->getAccuracyModifier() / 2. + 0.5)));
									if (unitBelow->getCarriedWeight() > strength)
									{
										srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(96); // dot
										srfSprite->blitNShade(
												surface,
												screenPosition.x + walkOffset.x + 9,
												screenPosition.y + walkOffset.y + 4 + 24,
												1, // 10
												false,
												1); // 1=white, 2=yellow-red, 3=red, 6=lt.brown, 9=blue
									} */
								}
							}
						}
					}

					// check if we got bullet && it is in Field Of View
					if (_projectile != NULL)
//kL					&& _projectileInFOV)
					{
//						srfSprite = NULL;

						if (_projectile->getItem() != NULL) // thrown item ( grenade, etc.)
						{
							srfSprite = _projectile->getSprite();
							if (srfSprite)
							{
								Position voxelPos = _projectile->getPosition(); // draw shadow on the floor
								voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
								if (   voxelPos.x / 16 >= itX
									&& voxelPos.y / 16 >= itY
									&& voxelPos.x / 16 <= itX + 1
									&& voxelPos.y / 16 <= itY + 1
									&& voxelPos.z / 24 == itZ)
	//kL							&& _save->getTileEngine()->isVoxelVisible(voxelPos))
								{
									_camera->convertVoxelToScreen(
																voxelPos,
																&bullet);
									srfSprite->blitNShade(
											surface,
											bullet.x - 16,
											bullet.y - 26,
											16);
								}

								voxelPos = _projectile->getPosition(); // draw thrown object
								if (   voxelPos.x / 16 >= itX
									&& voxelPos.y / 16 >= itY
									&& voxelPos.x / 16 <= itX + 1
									&& voxelPos.y / 16 <= itY + 1
									&& voxelPos.z / 24 == itZ)
	//kL							&& _save->getTileEngine()->isVoxelVisible(voxelPos))
								{
									_camera->convertVoxelToScreen(
																voxelPos,
																&bullet);
									srfSprite->blitNShade(
											surface,
											bullet.x - 16,
											bullet.y - 26,
											0);
								}
							}
						}
						else // fired projectile ( a bullet-sprite, not a thrown item )
						{
							if (   itX >= bulletLowX // draw bullet on the correct tile
								&& itX <= bulletHighX
								&& itY >= bulletLowY
								&& itY <= bulletHighY)
							{
//								for (int
//										i = 0;
//										i < BULLET_SPRITES;
//										++i)
								int
									bulletSprite_start,
									bulletSprite_end,
									direction;

								if (_projectile->isReversed() == true)
								{
									bulletSprite_start = BULLET_SPRITES - 1;
									bulletSprite_end = -1;
									direction = -1;
								}
								else
								{
									bulletSprite_start = 0;
									bulletSprite_end = BULLET_SPRITES;
									direction = 1;
								}

								for (int
										i = bulletSprite_start;
										i != bulletSprite_end;
										i += direction)
								{
									srfSprite = _projectileSet->getFrame(_projectile->getParticle(i));
									if (srfSprite)
									{
										Position voxelPos = _projectile->getPosition(1 - i); // draw shadow on the floor
										voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
										if (   voxelPos.x / 16 == itX
											&& voxelPos.y / 16 == itY
											&& voxelPos.z / 24 == itZ)
//kL										&& _save->getTileEngine()->isVoxelVisible(voxelPos))
										{
											_camera->convertVoxelToScreen(
																		voxelPos,
																		&bullet);

											bullet.x -= srfSprite->getWidth() / 2;
											bullet.y -= srfSprite->getHeight() / 2;
											srfSprite->blitNShade(
													surface,
													bullet.x,
													bullet.y,
													16);
										}

										voxelPos = _projectile->getPosition(1 - i); // draw bullet itself
										if (   voxelPos.x / 16 == itX
											&& voxelPos.y / 16 == itY
											&& voxelPos.z / 24 == itZ)
//kL										&& _save->getTileEngine()->isVoxelVisible(voxelPos))
										{
											_camera->convertVoxelToScreen(
																		voxelPos,
																		&bullet);

											bullet.x -= srfSprite->getWidth() / 2;
											bullet.y -= srfSprite->getHeight() / 2;
											srfSprite->blitNShade(
													surface,
													bullet.x,
													bullet.y,
													0);
										}
									}
								}
							}
						}
					}

					// Main Draw BattleUnit ->
					if (unit != NULL
						&& (unit->getUnitVisible() == true
							|| _save->getDebugMode() == true))
					{
						half = false; // don't overwrite walls in tile SOUTH-WEST
						if (   itX > 0
							&& itY < endY
							&& unit->getStatus() == STATUS_WALKING
//							&& _save->getPathfinding()->getStartDirection() < Pathfinding::DIR_UP
							&& (unit->getDirection() == 2
								|| unit->getDirection() == 6))
						{
							const Tile* const tileSouthWest = _save->getTile(mapPosition + Position(-1,1,0));

							if (tileSouthWest->getMapData(MapData::O_NORTHWALL) != NULL
								|| (tileSouthWest->getMapData(MapData::O_OBJECT) != NULL
									&& (tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
										|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW
										|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NWSE
										|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NORTH
										|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_EAST
										|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_E_S
										|| tileSouthWest->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N)))
							{
								half = true;
							}
						}

						// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
						// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
						quad = tile->getPosition().x - unit->getPosition().x
							+ (tile->getPosition().y - unit->getPosition().y) * 2;

						srfSprite = unit->getCache(&invalid, quad);
//						srfSprite = NULL;
						if (srfSprite)
						{
							calculateWalkingOffset(
												unit,
												&walkOffset);

							srfSprite->blitNShade(
									surface,
									screenPosition.x + walkOffset.x,
									screenPosition.y + walkOffset.y,
									tileShade,
									half);

							if (unit->getFire() != 0)
							{
								frame = 4 + (_animFrame / 2);
								srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
								if (srfSprite)
									srfSprite->blitNShade(
											surface,
											screenPosition.x + walkOffset.x,
											screenPosition.y + walkOffset.y,
											0,
											half);
							}

							if (unit->getBreathFrame() > 0)
							{
								// kL_note: Don't throw my offsets off ...
								int bubbleOffset_x;
								if (unit->getStatus() == STATUS_AIMING)	// enlarge the unit sprite when aiming to accomodate the weapon
									bubbleOffset_x = 0;					// adjust as necessary
								else
									bubbleOffset_x = walkOffset.x;

								const int bubbleOffset_y = walkOffset.y + (22 - unit->getHeight()); // lower the bubbles for shorter or kneeling units

								srfSprite = _res->getSurfaceSet("BREATH-1.PCK")->getFrame(unit->getBreathFrame() - 1);
								if (srfSprite)
									srfSprite->blitNShade(
											surface,
											screenPosition.x + bubbleOffset_x,
											screenPosition.y + bubbleOffset_y - 30,
											tileShade);
							}

							// kL_begin #3 of 3:
							const Tile* const tileAbove = _save->getTile(mapPosition + Position(0,0,1));

							if (_camera->getViewLevel() == itZ
								|| (tileAbove != NULL
									&& tileAbove->getSprite(MapData::O_FLOOR) == NULL))
							{
								if (unit != _save->getSelectedUnit()
									&& unit->getGeoscapeSoldier() != NULL
									&& unit->getFaction() == unit->getOriginalFaction())
								{
									if (unit->getFatalWounds() > 0)
									{
										srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x + 2,
													screenPosition.y + walkOffset.y + 3,
													0);

										srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross;
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x + 4,
													screenPosition.y + walkOffset.y + 4,
													_animFrame * 2,
													false,
													3); // red

/*										if (unit->getFatalWounds() < 10) // stick to under 10 wounds ... else double digits.
										{
										SurfaceSet* setDigits = _res->getSurfaceSet("DIGITS");
										srfSprite = setDigits->getFrame(unit->getFatalWounds());
										if (srfSprite != NULL)
										{
											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x + 7,
													screenPosition.y + walkOffset.y + 4,
													0,
													false,
													3); // 1=white, 3=red.
										}
										} */
									}
									else
									{
										std::string soldierRank = unit->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
										soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

										srfSprite = _res->getSurface(soldierRank);
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x + 2,
													screenPosition.y + walkOffset.y + 3,
													0);

/*										const int strength = static_cast<int>(Round(
															 static_cast<double>(unit->getBaseStats()->strength) * (unit->getAccuracyModifier() / 2. + 0.5)));
										if (unit->getCarriedWeight() > strength)
										{
											srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(96); // dot
											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x + 9,
													screenPosition.y + walkOffset.y + 4,
													1, // 10
													false,
													1); // 1=white, 2=yellow-red, 3=red, 6=lt.brown, 9=blue
										} */
									}
								}
							} // kL_end.


							// Feet getting chopped by tilesBelow when moving vertically; redraw lowForeground walls.
							// note: This is a quickfix - feet can also get chopped by walls in tileBelowSouth & tileBelowEast ...
							if (unit->getVerticalDirection() != 0
								&& itX < endX && itY < endY && itZ > 0)
							{
								const Tile* const tileBelowSouthEast = _save->getTile(mapPosition + Position(1,1,-1));

								srfSprite = tileBelowSouthEast->getSprite(MapData::O_NORTHWALL);
								if (srfSprite)
								{
									if (tileBelowSouthEast->isDiscovered(2) == true)
										shade = tileBelowSouthEast->getShade();
									else
										shade = 16;

									srfSprite->blitNShade(
											surface,
											screenPosition.x,
											screenPosition.y + 40 - tileBelowSouthEast->getMapData(MapData::O_NORTHWALL)->getYOffset(),
											shade);

									if (itX < endX - 1)
									{
										const Tile* const tileBelowSouthEastEast = _save->getTile(mapPosition + Position(2,1,-1));

										srfSprite = tileBelowSouthEastEast->getSprite(MapData::O_NORTHWALL);
										if (srfSprite)
										{
											if (tileBelowSouthEastEast->isDiscovered(2) == true)
												shade = tileBelowSouthEastEast->getShade();
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													screenPosition.x + 16,
													screenPosition.y + 48 - tileBelowSouthEastEast->getMapData(MapData::O_NORTHWALL)->getYOffset(),
													shade,
													false,
													0,
													true);
										}
									}
								}
							}
						}
					}
					// end Main Draw BattleUnit.

					// kL_begin: Might well want to redundant this, like rankIcons.
					if (unit == NULL)
					{
						const int status = tile->getHasUnconsciousSoldier();
						if (status != 0)
						{
							srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
							if (srfSprite != NULL)
								srfSprite->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y,
										0);

							Uint8 color = 1;	// white, unconscious soldier here
							if (status == 2)
								color = 3;		// red, wounded unconscious soldier

							srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
							if (srfSprite)
								srfSprite->blitNShade(
										surface,
										screenPosition.x + 2,
										screenPosition.y + 1,
										_animFrame * 2,
										false,
										color);
						}
					} // kL_end.


					// if there is no floor, draw the unit below that if it is on raised ground
					const Tile* tileNorth;
					if (itY > 0)
						tileNorth = _save->getTile(mapPosition + Position(0,-1,0));
					else
						tileNorth = NULL;

					const Tile* tileNorthWest;
					if (itX > 0 && itY > 0)
						tileNorthWest = _save->getTile(mapPosition + Position(-1,-1,0));
					else
						tileNorthWest = NULL;

					const Tile* tileWest;
					if (itX > 0)
						tileWest = _save->getTile(mapPosition + Position(-1,0,0));
					else
						tileWest = NULL;

					const Tile* const tileBelow = _save->getTile(mapPosition + Position(0,0,-1));

					if (itZ > 0
						&& tile->hasNoFloor(tileBelow) == true
						&& (tile->getMapData(MapData::O_WESTWALL) != NULL
							|| tile->getMapData(MapData::O_NORTHWALL) != NULL
							|| (tileNorth != NULL
								&& (tileNorth->getMapData(MapData::O_FLOOR) != NULL
									|| (tileNorth->getMapData(MapData::O_OBJECT) != NULL
										&& tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK)))
							|| (tileNorthWest != NULL
								&& tileNorthWest->isVoid() == false) // flooring mainly, but prob. also south & east walls
							|| (tileWest != NULL
								&& tileWest->isVoid() == false))) // flooring mainly, but prob. also north & east walls
					// that also needs tileWest's northWall & bigWall east
					// and tileNorth's westWall & bigWall south
					// and tileBelow's east and south walls
					// and tileBelowEast's west & south walls ..... fuck 3d isometric.
					// Screw it; i'm changing the map resource!!! ( DESERTMOUNT08, eg. )
					// ... damn, now i'm actually implementing it anyway .....
					{
//						const Position posBelow = Position(itX, itY, itZ - 1);
//						const Tile* const tileBelow1 = _save->getTile(posBelow);
//						BattleUnit* const unitBelow1 = _save->selectUnit(posBelow);
						BattleUnit* const unitBelow = tileBelow->getUnit();

						if (unitBelow != NULL
							&& unitBelow->getUnitVisible() == true
							&& tileBelow->getTerrainLevel() < 0
							&& tileBelow->isDiscovered(2) == true)
						{
							// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
							// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
							quad = tileBelow->getPosition().x - unitBelow->getPosition().x
								+ (tileBelow->getPosition().y - unitBelow->getPosition().y) * 2;

							srfSprite = unitBelow->getCache(&invalid, quad);
//							srfSprite = NULL;
							if (srfSprite)
							{
								calculateWalkingOffset(
													unitBelow,
													&walkOffset);
								walkOffset.y += 24;

								srfSprite->blitNShade(
										surface,
										screenPosition.x + walkOffset.x,
										screenPosition.y + walkOffset.y,
										tileBelow->getShade()); // <- might want to do the undiscovered thing for shade here

								if (unitBelow->getArmor()->getSize() > 1)
									walkOffset.y += 4;

								if (unitBelow->getFire() != 0)
								{
									frame = 4 + (_animFrame / 2);
									srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									if (srfSprite)
										srfSprite->blitNShade(
												surface,
												screenPosition.x + walkOffset.x,
												screenPosition.y + walkOffset.y,
												0);
								}

								// special case if tileSouthEast don't have a floor
								// and tileBelowSouthEast does have a sprite that should conceal the redrawn unit's right leg.
								if (itX < endX
									&& itY < endY)
								{
									const Tile* const tileSouthEast = _save->getTile(mapPosition + Position(1,1,0));

									if (tileSouthEast->getMapData(MapData::O_FLOOR) == NULL)
									{
										const Tile* const tileBelowSouthEast = _save->getTile(mapPosition + Position(1,1,-1));

										// kL_note: I'm only doing content-object w/out bigWall here because that's the case I ran into.
										// This also needs to redraw northWall, as well as tileEast's west & south walls, tileBelow's east & south walls, etc.
										// ... aaaaaaand, this gives rise to infinite regression.
										// so try to head it off at the pass, above, by not redrawing unitBelow unless it *has* to be done.
										if (tileBelowSouthEast->getMapData(MapData::O_OBJECT) != NULL
											&& tileBelowSouthEast->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NONE)
										{
											srfSprite = tileBelowSouthEast->getSprite(MapData::O_OBJECT);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (tileBelowSouthEast->isDiscovered(2) == true)
													shade = tileBelowSouthEast->getShade();
												else
													shade = 16;

												srfSprite->blitNShade(
														surface,
														screenPosition.x,
														screenPosition.y + 40 - tileBelowSouthEast->getMapData(MapData::O_OBJECT)->getYOffset(),
														shade,
														true);

												if (itX < endX - 1)
												{
													const Tile* const tileBelowSouthEastEast = _save->getTile(mapPosition + Position(2,1,-1));

													srfSprite = tileBelowSouthEastEast->getSprite(MapData::O_OBJECT);
//													srfSprite = NULL;
													if (srfSprite)
													{
														if (tileBelowSouthEastEast->isDiscovered(2) == true)
															shade = tileBelowSouthEastEast->getShade();
														else
															shade = 16;

														srfSprite->blitNShade(
																surface,
																screenPosition.x + 16,
																screenPosition.y + 48 - tileBelowSouthEastEast->getMapData(MapData::O_OBJECT)->getYOffset(),
																shade,
																false,
																0,
																true);
														// note: tileBelowSouthEastEastEast is now bugging-out on DESERTMOUNT08. but i fixed it in the resource.
														// Note2: I may have fixed this by adding 'halfLeft' to blitNShade()
													}
												}
											}
										}
									}
								}
							}
						}
					}

					// Draw SMOKE & FIRE
					if (//unit == NULL || unit->getStatus() == STATUS_STANDING))
						tile->getSmoke() != 0
						&& tile->isDiscovered(2) == true)
					{
						if (tile->getFire() == 0)
						{
							if (_save->getDepth() > 0)
								frame = ResourcePack::UNDERWATER_SMOKE_OFFSET;
							else
								frame = ResourcePack::SMOKE_OFFSET; // =7

							frame += (tile->getSmoke() + 1) / 2;
							shade = tileShade;
						}
						else
						{
							frame = 0;
							shade = 0;
						}

						animOffset = _animFrame / 2 + tile->getAnimationOffset();
						if (animOffset > 3) animOffset -= 4;
						frame += animOffset;

						srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
						if (srfSprite)
							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y + tile->getTerrainLevel(),
									shade);
					}

					for (std::list<Particle*>::const_iterator // draw particle clouds
							i = tile->getParticleCloud()->begin();
							i != tile->getParticleCloud()->end();
							++i)
					{
						const int
							vaporX = static_cast<int>(static_cast<float>(screenPosition.x) + (*i)->getX()),
							vaporY = static_cast<int>(static_cast<float>(screenPosition.y) + (*i)->getY());

						if (_transparencies->size() >= (static_cast<size_t>((*i)->getColor()) + 1) * 1024)
						{
							switch ((*i)->getSize())
							{
								case 3:
									surface->setPixelColor(
												vaporX + 1,
												vaporY + 1,
												(*_transparencies)[((*i)->getColor() * 1024)
																	+ ((*i)->getOpacity() * 256) + surface->getPixelColor(
																													vaporX + 1,
																													vaporY + 1)]);
								case 2:
									surface->setPixelColor(
												vaporX + 1,
												vaporY,
												(*_transparencies)[((*i)->getColor() * 1024)
																	+ ((*i)->getOpacity() * 256) + surface->getPixelColor(
																													vaporX + 1,
																													vaporY)]);
								case 1:
									surface->setPixelColor(
												vaporX,
												vaporY + 1,
												(*_transparencies)[((*i)->getColor() * 1024)
																	+ ((*i)->getOpacity() * 256) + surface->getPixelColor(
																													vaporX,
																													vaporY + 1)]);
								default:
									surface->setPixelColor(
												vaporX,
												vaporY,
												(*_transparencies)[((*i)->getColor() * 1024)
																	+ ((*i)->getOpacity() * 256) + surface->getPixelColor(
																													vaporX,
																													vaporY)]);
							}
						}
					}

					// Draw pathPreview
					if (tile->getPreview() != -1
						&& (_previewSetting & PATH_ARROWS)
						&& tile->isDiscovered(0) == true)
					{
						if (itZ > 0
							&& tile->hasNoFloor(tileBelow) == true)
						{
							srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(11);
							if (srfSprite)
								srfSprite->blitNShade(
										surface,
										screenPosition.x,
										screenPosition.y + 2,
										0,
										false,
//										tileColor);
										tile->getMarkerColor());
						}

						srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(tile->getPreview());
						if (srfSprite)
							srfSprite->blitNShade(
									surface,
									screenPosition.x,
//									screenPosition.y + tile->getTerrainLevel(),
									screenPosition.y, // kL
									0,
									false,
//									tileColor);
									tile->getMarkerColor());
					}


//					if (tile->isVoid() == false) // THIS CAME BEFORE Draw pathPreview above in Old builds.
//					{
					// Draw front object
					srfSprite = tile->getSprite(MapData::O_OBJECT);
					if (srfSprite
						&& tile->getMapData(MapData::O_OBJECT)->getBigWall() > Pathfinding::BIGWALL_NORTH // do East,South,East&South
						&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_W_N)
					{
						srfSprite->blitNShade(
								surface,
								screenPosition.x,
								screenPosition.y - tile->getMapData(MapData::O_OBJECT)->getYOffset(),
								tileShade);
					}
//					}


					if (itZ > 0 // THIS IS LEADING TO A NEAR-INFINITE REGRESSION!!
						&& tile->isVoid(true) == false)
					{
						bool redrawLowForeground = false;

						// redraw units that are moving up from tileBelow
						// so their heads don't get cut off as they're moving up but before they enter the same Z-level.
						if (   tile->getMapData(MapData::O_WESTWALL) != NULL // TODO: draw only if there's no southern wall in tileEast that covers the unit's decapitation
							|| tile->getMapData(MapData::O_NORTHWALL) != NULL
							|| (tile->getMapData(MapData::O_OBJECT) != NULL
								&& (   tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NONE
									|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_WEST
									|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NORTH
									|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N)))
						{
							// TODO: insert checks for tileEast !south&tc bigWalls & tileSouthEast !north&tc Walls

//							const Tile* const tileBelow = _save->getTile(mapPosition + Position(0,0,-1)); // def'n above still good.
							BattleUnit* const unitBelow = tileBelow->getUnit();

							if (unitBelow != NULL
								&& unitBelow->getVerticalDirection() != 0
								&& (unitBelow->getUnitVisible() == true
									|| _save->getDebugMode() == true))
//								&& unitBelow->getStatus() == STATUS_FLYING
							{
								redrawLowForeground = true; // TODO:
								// redraw tileBelow foreground (south & east bigWalls)
								// redraw tileBelowEast foreground (south bigWall)
								// redraw tileBelowSouthEast background (north & west Walls)

								if (tileBelow->isDiscovered(2) == true)
									shade = tileBelow->getShade();
								else
									shade = 16;

								// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
								// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
								quad = tileBelow->getPosition().x - unitBelow->getPosition().x
									+ (tileBelow->getPosition().y - unitBelow->getPosition().y) * 2;

								srfSprite = unitBelow->getCache(&invalid, quad);
//								srfSprite = NULL;
								if (srfSprite)
								{
									calculateWalkingOffset(
														unitBelow,
														&walkOffset);

									srfSprite->blitNShade(
											surface,
											screenPosition.x + walkOffset.x,
											screenPosition.y + 24 + walkOffset.y,
											shade);

									if (unitBelow->getFire() != 0)
									{
										frame = 4 + (_animFrame / 2);
										srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
										srfSprite->blitNShade(
												surface,
												screenPosition.x + walkOffset.x,
												screenPosition.y + 24+ walkOffset.y,
												0);
									}
								}
							}
						}

						// redraw units that are moving up from tileBelowSouth & tileBelowSouthEast
						// so their heads don't get cut off just before they enter the same Z-level.
						// - both:
						//		- floor
						//		- bigWallBlock,bigWallSouth,bigWallSouth&East,
						// - if UnitSouth:
						//		- WestWall
						//		- bigWallWest,bigWallNorth&West (maybe content-object w/ bigWallNone)
						// - if UnitSouthEast
						//		- bigWallEast,content-object w/ bigWallNone
						if (itY < endY - 1) // why does this want (endY - 1) else CTD.
						{
							// for both:
							const bool redrawUnit = tile->getMapData(MapData::O_FLOOR) != NULL
												|| (tile->getMapData(MapData::O_OBJECT) != NULL
													&& (tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
														|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_SOUTH
														|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_E_S));
							// for tileBelow-South:
							// Note: unit will be redrawn *again* if conditions for an East-tile are true right after this;
							// so checks could/should be done for those sprites here also ... and if true postpone redrawing the unit to then.
							if (redrawUnit == true
								|| tile->getMapData(MapData::O_WESTWALL) != NULL
								|| (tile->getMapData(MapData::O_OBJECT) != NULL
									&& (tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_WEST
										|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_W_N)))
							{
								// TODO: insert checks for tileSouthEast !south&tc bigWalls & tileSouthSouthEast !north&tc Walls

								const Tile* const tileBelowSouth = _save->getTile(mapPosition + Position(0,1,-1));
								//tileBelowSouth != NULL

								// Note: redrawing unit to South will overwrite any stuff to south & southeast & east. Assume nothing will be there for now ... BZZZZZZZT!!!
								// TODO: don't draw unitSouth if there is a foreground object on this Z-level above the unit (ie. it hides his head getting chopped off anyway).
								// TODO: redraw foreground objects on lower Z-level
								BattleUnit* const unitBelowSouth = tileBelowSouth->getUnit(); // else CTD here.
/*								BattleUnit* unitBelowSouth;
								if (tileBelowSouth != NULL)
									unitBelowSouth = tileBelowSouth->getUnit();
								else
									unitBelowSouth = NULL; */

								if (unitBelowSouth != NULL
									&& unitBelowSouth->getVerticalDirection() != 0
//									&& unitBelowSouth->getStatus() == STATUS_FLYING
									&& (unitBelowSouth->getUnitVisible() == true
										|| _save->getDebugMode() == true))
								{
									redrawLowForeground = true;

									if (tileBelowSouth->isDiscovered(2) == true)
										shade = tileBelowSouth->getShade();
									else
										shade = 16;

									// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
									// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
									quad = tileBelowSouth->getPosition().x - unitBelowSouth->getPosition().x
										+ (tileBelowSouth->getPosition().y - unitBelowSouth->getPosition().y) * 2;

									srfSprite = unitBelowSouth->getCache(&invalid, quad);
//									srfSprite = NULL;
									if (srfSprite)
									{
										calculateWalkingOffset(
															unitBelowSouth,
															&walkOffset);

										srfSprite->blitNShade(
												surface,
												screenPosition.x - 16 + walkOffset.x,
												screenPosition.y + 32 + walkOffset.y,
												shade);

										if (unitBelowSouth->getFire() != 0)
										{
											frame = 4 + (_animFrame / 2);
											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											srfSprite->blitNShade(
													surface,
													screenPosition.x - 16 + walkOffset.x,
													screenPosition.y + 32 + walkOffset.y,
													0);
										}
									}
								}
							}


							// for tileBelow-SouthEast:
							if (itX < endX
								&& (redrawUnit == true
									|| (tile->getMapData(MapData::O_OBJECT) != NULL
										&& (tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NONE
											|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_EAST))))
							{
								// checks if the unit will be drawn anyway as unitBelowSouth above
								// so don't draw it here because it will be redrawn when X increments.
								const Tile* const tileBelowSouthEast = _save->getTile(mapPosition + Position(1,1,-1));

								// Note: redrawing unit to South will overwrite any stuff to south & southeast & east.
								BattleUnit* const unitBelowSouthEast = tileBelowSouthEast->getUnit();

								if (unitBelowSouthEast != NULL
									&& unitBelowSouthEast->getVerticalDirection() != 0
//										&& unitBelowSouthEast->getStatus() == STATUS_FLYING
									&& (unitBelowSouthEast->getUnitVisible() == true
										|| _save->getDebugMode() == true))
								{
									const Tile* const tileEast = _save->getTile(mapPosition + Position(1,0,0));
									//tileEast != NULL

									if (   tileEast->getMapData(MapData::O_FLOOR) == NULL // these should be exactly opposite to check for unitBelowSouth above.
										&& tileEast->getMapData(MapData::O_WESTWALL) == NULL
										&& (tileEast->getMapData(MapData::O_OBJECT) == NULL
											|| (tileEast->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
												&& tileEast->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_WEST
												&& tileEast->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_SOUTH
												&& tileEast->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_E_S
												&& tileEast->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_W_N)))
										// TODO: insert checks for tileSouthEastEast !south&tc bigWalls & tileSouthSouthEastEast !north&tc Walls
									{
										redrawLowForeground = true;

										if (tileBelowSouthEast->isDiscovered(2) == true)
											shade = tileBelowSouthEast->getShade();
										else
											shade = 16;

										// The quadrant# is 0 for small units; large units also have quadrants 1,2 & 3 -
										// the relative x/y Position of the unit's primary Position vs the drawn Tile's Position.
										quad = tileBelowSouthEast->getPosition().x - unitBelowSouthEast->getPosition().x
											+ (tileBelowSouthEast->getPosition().y - unitBelowSouthEast->getPosition().y) * 2;

										srfSprite = unitBelowSouthEast->getCache(&invalid, quad);
//										srfSprite = NULL;
										if (srfSprite)
										{
											calculateWalkingOffset(
																unitBelowSouthEast,
																&walkOffset);

											srfSprite->blitNShade(
													surface,
													screenPosition.x + walkOffset.x,
													screenPosition.y + 40 + walkOffset.y,
													shade);

											if (unitBelowSouthEast->getFire() != 0)
											{
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												srfSprite->blitNShade(
														surface,
														screenPosition.x + walkOffset.x,
														screenPosition.y + 40 + walkOffset.y,
														0);
											}
										}
									}
								}
							}

							if (redrawLowForeground == true) // The order/ structure of these following have not been optimized ....
							{
								//Log(LOG_INFO) << "redrawLowForeground TRUE";
								if (itY < endY - 1)
								{
									const Tile* const tileBelowSouthSouth = _save->getTile(mapPosition + Position(0,2,-1));

									srfSprite = tileBelowSouthSouth->getSprite(MapData::O_NORTHWALL);
//									srfSprite = NULL;
									if (srfSprite)
									{
										if (tileBelowSouthSouth->isDiscovered(2) == true)
											shade = tileBelowSouthSouth->getShade();
										else
											shade = 16;

										srfSprite->blitNShade(
												surface,
												screenPosition.x - 32,
												screenPosition.y + 40 - tileBelowSouthSouth->getMapData(MapData::O_NORTHWALL)->getYOffset(),
												shade);
									}

									if (itX < endX)
									{
										const Tile* const tileBelowSouthSouthEast = _save->getTile(mapPosition + Position(1,2,-1));

										srfSprite = tileBelowSouthSouthEast->getSprite(MapData::O_NORTHWALL);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileBelowSouthSouthEast->isDiscovered(2) == true)
												shade = tileBelowSouthSouthEast->getShade();
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													screenPosition.x - 16,
													screenPosition.y + 48 - tileBelowSouthSouthEast->getMapData(MapData::O_NORTHWALL)->getYOffset(),
													shade);
										}
									}
								}

								if (itX < endX)
								{
									// TODO: if unitBelow this needs to redraw walls to south & east also.
									const Tile* const tileBelowSouthEast = _save->getTile(mapPosition + Position(1,1,-1));

									srfSprite = tileBelowSouthEast->getSprite(MapData::O_NORTHWALL);
//									srfSprite = NULL;
									if (srfSprite)
									{
										if (tileBelowSouthEast->isDiscovered(2) == true)
											shade = tileBelowSouthEast->getShade();
										else
											shade = 16;

										srfSprite->blitNShade(
												surface,
												screenPosition.x,
												screenPosition.y + 40 - tileBelowSouthEast->getMapData(MapData::O_NORTHWALL)->getYOffset(),
												shade,
												false,
												0,
												true);
									}

									srfSprite = tileBelowSouthEast->getSprite(MapData::O_OBJECT);
//									srfSprite = NULL;
									if (srfSprite
										&& tileBelowSouthEast->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW)
									{
										if (tileBelowSouthEast->isDiscovered(2) == true)
											shade = tileBelowSouthEast->getShade();
										else
											shade = 16;

										srfSprite->blitNShade(
												surface,
												screenPosition.x,
												screenPosition.y + 40 - tileBelowSouthEast->getMapData(MapData::O_OBJECT)->getYOffset(),
												shade,
												false,
												0,
												true);
									}
								}

								if (itX < endX
									&& itY < endY -1)
								{
									const Tile* const tileBelowSouthEastEast = _save->getTile(mapPosition + Position(2,1,-1));

									srfSprite = tileBelowSouthEastEast->getSprite(MapData::O_OBJECT);
//									srfSprite = NULL;
									if (srfSprite
										&& tileBelowSouthEastEast->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW)
									{
										if (tileBelowSouthEastEast->isDiscovered(2) == true)
											shade = tileBelowSouthEastEast->getShade();
										else
											shade = 16;

										srfSprite->blitNShade(
												surface,
												screenPosition.x + 16,
												screenPosition.y + 48 - tileBelowSouthEastEast->getMapData(MapData::O_OBJECT)->getYOffset(),
												shade);
									}
								}
							}
						}
					}


					// Draw cursor front
					if (_cursorType != CT_NONE
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX < itX + 1
						&& _selectorY < itY + 1
						&& _save->getBattleState()->getMouseOverIcons() == false)
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _save->getDebugMode() == true)
									&& (_cursorType != CT_PSI
										|| unit->getFaction() != _save->getSide()))
								{
									frame = 3 + (_animFrame %2); // yellow box
								}
								else
									frame = 3; // red box
							}
							else // CT_AIM ->
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _save->getDebugMode() == true))
								{
									frame = 7 + (_animFrame / 2); // yellow animated crosshairs
								}
								else
									frame = 6; // red static crosshairs
							}

							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);

							// UFOExtender Accuracy: display adjusted accuracy value on crosshair (and more).
//							if (Options::battleUFOExtenderAccuracy == true) // kL_note: one less condition to check
//							{
							if (_cursorType == CT_AIM) // indicator for Firing.
							{
								// kL_note: Use stuff from ProjectileFlyBState::init()
								// as well as TileEngine::canTargetUnit() & TileEngine::canTargetTile()
								// to turn accuracy to 'red 0' if target is out of LoS/LoF.
								const BattleAction* const action = _save->getBattleGame()->getCurrentAction();

								int accuracy = static_cast<int>(Round(
													action->actor->getFiringAccuracy(
																				action->type,
																				action->weapon) * 100.));

								const RuleItem* const weaponRule = action->weapon->getRules();
								const int
									lowerLimit = weaponRule->getMinRange(),
									distance = _save->getTileEngine()->distance(
																			Position(
																					itX,
																					itY,
																					itZ),
																			action->actor->getPosition());
								int upperLimit = 200;
								switch (action->type)
								{
									case BA_AIMEDSHOT:	upperLimit = weaponRule->getAimRange();		break;
									case BA_SNAPSHOT:	upperLimit = weaponRule->getSnapRange();	break;
									case BA_AUTOSHOT:	upperLimit = weaponRule->getAutoRange();
								}

								Uint8 color = Palette::blockOffset(3)+3; // green

								if (distance > upperLimit)
								{
									accuracy -= (distance - upperLimit) * weaponRule->getDropoff();
									color = Palette::blockOffset(1)+2; // orange
								}
								else if (distance < lowerLimit)
								{
									accuracy -= (lowerLimit - distance) * weaponRule->getDropoff();
									color = Palette::blockOffset(1)+2; // orange
								}

								if (accuracy < 1 // zero accuracy or out of range: set it red.
									|| distance > weaponRule->getMaxRange())
								{
									accuracy = 0;
									color = Palette::blockOffset(2)+3; // red
								}

								_txtAccuracy->setValue(static_cast<unsigned int>(accuracy));
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
								BattleAction* const action = _save->getBattleGame()->getCurrentAction();
								action->target = Position(
														itX,
														itY,
														itZ);

								const Position
									originVoxel = _save->getTileEngine()->getOriginVoxel(
																					*action,
																					NULL),
									targetVoxel = Position(
														itX * 16 + 8,
														itY * 16 + 8,
														itZ * 24 + 2 - _save->getTile(action->target)->getTerrainLevel());

								unsigned int accuracy = 0;
								Uint8 color = Palette::blockOffset(2)+3; // red

								const bool canThrow = _save->getTileEngine()->validateThrow(
																						*action,
																						originVoxel,
																						targetVoxel);
								if (canThrow == true)
								{
									accuracy = static_cast<unsigned int>(Round(_save->getSelectedUnit()->getThrowingAccuracy() * 100.));
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
//							}
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frame = 5; // blue box
							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}

						if (_cursorType > 2
							&& _camera->getViewLevel() == itZ)
						{
							const int arrowFrame[6] = {0, 0, 0, 11, 13, 15};
							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(arrowFrame[_cursorType] + (_animFrame / 4));
							srfSprite->blitNShade(
									surface,
									screenPosition.x,
									screenPosition.y,
									0);
						}
					}

					int // Draw waypoints if any on this tile
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
								srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(7);
								srfSprite->blitNShade(
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

								waypXOff += (waypid > 9)? 8: 6;
								if (waypXOff > 25)
								{
									waypXOff = 2;
									waypYOff += 8;
								}
							}
						}

						++waypid;
					}


					// kL_begin:
					if (itZ == _save->getGroundLevel() // draw Map's border-sprite only on ground tiles
						|| (itZ == 0
							&& _save->getGroundLevel() == -1))
					{
						if (   itX == 0
							|| itX == _save->getMapSizeX() - 1
							|| itY == 0
							|| itY == _save->getMapSizeY() - 1)
						{
							srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(330); // gray square cross
							srfSprite->blitNShade(
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


	if (getCursorType() != CT_NONE
		&& (_save->getSide() == FACTION_PLAYER
			|| _save->getDebugMode() == true))
	{
		unit = _save->getSelectedUnit();
		if (unit != NULL
			&& unit->getStatus() != STATUS_WALKING
			&& unit->getPosition().z <= _camera->getViewLevel())
		{
			_camera->convertMapToScreen(
									unit->getPosition(),
									&screenPosition);
			screenPosition += _camera->getMapOffset();

			calculateWalkingOffset( // this calculates terrainLevel, but is otherwise bloat here.
								unit,
								&walkOffset);
			walkOffset.y += 21 - unit->getHeight();

			if (unit->getArmor()->getSize() > 1)
				walkOffset.y += 9;

			if (unit->isKneeled() == true)
			{
				walkOffset.y -= 5;
				_arrow_kneel->blitNShade(
								surface,
								screenPosition.x
									+ walkOffset.x
									+ _spriteWidth / 2
									- _arrow->getWidth() / 2,
								screenPosition.y
									+ walkOffset.y
									- _arrow->getHeight()
									+ static_cast<int>(
										-4. * std::sin(22.5 / static_cast<double>(_animFrame + 1))),
								0);
			}
			else
				_arrow->blitNShade(
								surface,
								screenPosition.x
									+ walkOffset.x
									+ _spriteWidth / 2
									- _arrow->getWidth() / 2,
								screenPosition.y
									+ walkOffset.y
									- _arrow->getHeight()
									+ static_cast<int>(
										4. * std::sin(22.5 / static_cast<double>(_animFrame + 1))),
								0);
		}
	}

	if (pathPreview == true)
	{
		if (wpID) wpID->setBordered(); // make a border for the pathfinding display

		for (int
				itZ = beginZ;
				itZ <= endZ;
				++itZ)
		{
			for (int
					itX = beginX;
					itX <= endX;
					++itX)
			{
				for (int
						itY = beginY;
						itY <= endY;
						++itY)
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
					if (   screenPosition.x > -_spriteWidth
						&& screenPosition.x < surface->getWidth() + _spriteWidth
						&& screenPosition.y > -_spriteHeight
						&& screenPosition.y < surface->getHeight() + _spriteHeight)
					{
						tile = _save->getTile(mapPosition);

						if (tile == NULL
							|| tile->isDiscovered(0) == false
							|| tile->getPreview() == -1)
						{
							continue;
						}

						int offset_y = -tile->getTerrainLevel();

						if (_previewSetting & PATH_ARROWS)
						{
							const Tile* const tileBelow = _save->getTile(mapPosition + Position(0,0,-1));

							if (itZ > 0
								&& tile->hasNoFloor(tileBelow) == true)
							{
								srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(23);
								if (srfSprite)
									srfSprite->blitNShade(
											surface,
											screenPosition.x,
											screenPosition.y + 2,
											0,
											false,
											tile->getMarkerColor());
							}

							const int overlay = tile->getPreview() + 12;
							srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(overlay);
							if (srfSprite)
								srfSprite->blitNShade(
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

							if (_save->getSelectedUnit() != NULL
								&& _save->getSelectedUnit()->getArmor()->getSize() > 1)
							{
								offset_y += 1;
								if (!(_previewSetting & PATH_ARROWS))
									offset_y += 7;
							}

							wpID->setValue(tile->getTUMarker());
							wpID->draw();

							if (!(_previewSetting & PATH_ARROWS))
								wpID->blitNShade(
											surface,
											screenPosition.x + 16 - offset_x,
//											screenPosition.y + 29 - offset_y,
											screenPosition.y + 37 - offset_y, // kL
											0,
											false,
											tile->getMarkerColor());
							else
								wpID->blitNShade(
											surface,
											screenPosition.x + 16 - offset_x,
//											screenPosition.y + 22 - offset_y,
											screenPosition.y + 30 - offset_y, // kL
											0);
						}
					}
				}
			}
		}

		if (wpID) wpID->setBordered(false); // remove the border in case it's used for missile waypoints.
	}

	delete wpID;


	if (_explosionInFOV == true) // check if we got hit or explosion animations
	{
		for (std::list<Explosion*>::const_iterator
				i = _explosions.begin();
				i != _explosions.end();
				++i)
		{
			_camera->convertVoxelToScreen(
									(*i)->getPosition(),
									&bullet);

			if ((*i)->getCurrentFrame() > -1)
			{
				if ((*i)->isBig() == true) // explosion, http://ufopaedia.org/index.php?title=X1.PCK
				{
					srfSprite = _res->getSurfaceSet("X1.PCK")->getFrame((*i)->getCurrentFrame());
					srfSprite->blitNShade(
							surface,
							bullet.x - 64,
							bullet.y - 64,
							0);
				}
				else if ((*i)->isHit() == 1) // melee or psiamp, http://ufopaedia.org/index.php?title=HIT.PCK
				{
					srfSprite = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame());
					srfSprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 25,
							0);
				}
				else if ((*i)->isHit() == 0) // bullet, http://ufopaedia.org/index.php?title=SMOKE.PCK
				{
					srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
					srfSprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 15,
							0);
				}
			}
		}
	}
	surface->unlock();
}

/**
 * Handles mouse presses on the map.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Map::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);
	_camera->mousePress(action, state);
}

/**
 * Handles mouse releases on the map.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Map::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);
	_camera->mouseRelease(action, state);
}

/**
 * Handles keyboard presses on the map.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Map::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);
	_camera->keyboardPress(action, state);
}

/**
 * Handles keyboard releases on the map.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Map::keyboardRelease(Action* action, State* state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_camera->keyboardRelease(action, state);
}

/**
 * Handles mouse over events on the map.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
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
 * Finds the current mouse position XY on this Map.
 * @param mousePos - reference the mouse position
 */
void Map::findMousePosition(Position& mousePos)
{
	mousePos.x = _mouseX;
	mousePos.y = _mouseY;
	mousePos.z = 0;
}

/**
 * Handles animating tiles - 8 Frames per animation [0..7].
 * @param redraw - true to redraw the battlescape map
 */
void Map::animate(bool redraw)
{
	++_animFrame;
 	if (_animFrame == 8)
		_animFrame = 0;

	// animate tiles
	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		_save->getTiles()[i]->animate();
	}

	// animate certain units (large flying units have a propulsion animation)
	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (_save->getDepth() > 0
			&& (*i)->getFloorAbove() == false)
		{
			(*i)->breathe();
		}

		if ((*i)->getArmor()->getConstantAnimation() == true)
		{
			(*i)->setCache(NULL);
			cacheUnit(*i);
		}
	}

	if (redraw == true)
		_redraw = true;
}

/**
 * Sets the rectangular selector to a certain tile.
 * @param mx - mouse x position
 * @param my - mouse y position
 */
void Map::setSelectorPosition(
		int mx,
		int my)
{
	const int
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
 * @param pos - pointer to a Position
 */
void Map::getSelectorPosition(Position* pos) const
{
	pos->x = _selectorX;
	pos->y = _selectorY;
	pos->z = _camera->getViewLevel();
}

/**
 * Calculates the offset of a BattleUnit when it is walking between 2 tiles.
 * @param unit		- pointer to a BattleUnit
 * @param offset	- pointer to the Position offset to return the calculation
 */
void Map::calculateWalkingOffset(
		BattleUnit* const unit,
		Position* offset)
{
	*offset = Position(0,0,0); // kL

	const int
		offsetX[8] = {1, 1, 1, 0,-1,-1,-1, 0},
		offsetY[8] = {1, 0,-1,-1,-1, 0, 1, 1},

		phase = unit->getWalkingPhase() + unit->getDiagonalWalkingPhase(),

		dir = unit->getDirection(),
		unitSize = unit->getArmor()->getSize();
	int
		midphase = 4 + 4 * (dir %2),
		endphase = 8 + 8 * (dir %2);


	if (unitSize > 1)
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
			offset->y = -phase * offsetY[dir];
		}
		else
		{
			offset->x = (phase - endphase) * 2 * offsetX[dir];
			offset->y = -(phase - endphase) * offsetY[dir];
		}
	}

	// If we are walking in between tiles, interpolate its terrain level.
	if (unit->getStatus() == STATUS_WALKING
		|| unit->getStatus() == STATUS_FLYING)
	{
		if (phase < midphase)
		{
			const int fromLevel = getTerrainLevel(
										unit->getPosition(),
										unitSize);
			int toLevel = getTerrainLevel(
										unit->getDestination(),
										unitSize);

			if (unit->getPosition().z > unit->getDestination().z)
				// going down a level, so toLevel 0 becomes +24, -8 becomes  16
				toLevel += 24 * (unit->getPosition().z - unit->getDestination().z);
			else if (unit->getPosition().z < unit->getDestination().z)
				// going up a level, so toLevel 0 becomes -24, -8 becomes -16
				toLevel = -24 * (unit->getDestination().z - unit->getPosition().z) + std::abs(toLevel);

			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * phase) / endphase);
		}
		else
		{
			// from phase 4 onwards the unit behind the scenes already is on the destination tile
			// we have to get its last position to calculate the correct offset
			int fromLevel = getTerrainLevel(
										unit->getLastPosition(),
										unitSize);
			const int toLevel = getTerrainLevel(
										unit->getDestination(),
										unitSize);

			if (unit->getLastPosition().z > unit->getDestination().z)
				// going down a level, so fromLevel 0 becomes -24, -8 becomes -32
				fromLevel -= 24 * (unit->getLastPosition().z - unit->getDestination().z);
			else if (unit->getLastPosition().z < unit->getDestination().z)
				// going up a level, so fromLevel 0 becomes +24, -8 becomes 16
				fromLevel = 24 * (unit->getDestination().z - unit->getLastPosition().z) - std::abs(fromLevel);

			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * phase) / endphase);
		}
	}
	else
	{
		offset->y += getTerrainLevel(
								unit->getPosition(),
								unitSize);

		if (unit->getStatus() == STATUS_AIMING
			&& unit->getArmor()->getCanHoldWeapon() == true)
		{
			offset->x = -16;
		}

		if (_save->getDepth() > 0)
		{
			unit->setFloorAbove(false);

			// make sure this unit isn't obscured by the floor above him, otherwise it looks weird.
			if (_camera->getViewLevel() > unit->getPosition().z)
			{
				for (int
						z = std::min(
								_camera->getViewLevel(),
								_save->getMapSizeZ() - 1);
						z != unit->getPosition().z;
						--z)
				{
					if (_save->getTile(Position(
											unit->getPosition().x,
											unit->getPosition().y,
											z))->hasNoFloor(NULL) == false)
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
  * For a large sized unit, pick the highest terrain level, which is the lowest number...
  * @param pos		- Position
  * @param unitSize	- size of the unit to get the level from
  * @return, terrain height
  */
int Map::getTerrainLevel(
		Position pos,
		int unitSize)
{
	int
		lowLevel = 0,
		lowTest = 0;

	for (int
			x = 0;
			x < unitSize;
			++x)
	{
		for (int
				y = 0;
				y < unitSize;
				++y)
		{
			lowTest = _save->getTile(pos + Position(x,y,0))->getTerrainLevel();
			if (lowTest < lowLevel)
				lowLevel = lowTest;
		}
	}

	return lowLevel;
}

/**
 * Sets the 3D cursor to selection/aim mode.
 * @param type - CursorType
 * @param cursorSize - size of cursor
 */
void Map::setCursorType(
		CursorType type,
		int cursorSize)
{
	_cursorType = type;

	if (_cursorType == CT_NORMAL)
		_cursorSize = cursorSize;
	else
		_cursorSize = 1;
}

/**
 * Gets the cursor type.
 * @return, CursorType
 */
CursorType Map::getCursorType() const
{
	return _cursorType;
}

/**
 * Checks all units for need to be redrawn.
 */
void Map::cacheUnits()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		cacheUnit(*i);
	}
}

/**
 * Check if a certain unit needs to be redrawn.
 * @param unit - pointer to a BattleUnit
 */
void Map::cacheUnit(BattleUnit* unit)
{
	//Log(LOG_INFO) << "cacheUnit() : " << unit->getId();
	UnitSprite* const unitSprite = new UnitSprite(
											unit->getStatus() == STATUS_AIMING? _spriteWidth * 2: _spriteWidth,
											_spriteHeight,
											0,
											0,
											_save->getDepth() != 0);
	unitSprite->setPalette(this->getPalette());

	const int unitSize = unit->getArmor()->getSize() * unit->getArmor()->getSize();
	bool
		d = false,
		invalid = false;

	unit->getCache(&invalid);
	if (invalid == true)
	{
		//Log(LOG_INFO) << ". (invalid)";
		for (int // 1 or 4 iterations, depending on unit size
				i = 0;
				i < unitSize;
				++i)
		{
			//Log(LOG_INFO) << ". . i = " << i;
			Surface* cache = unit->getCache(&d, i);
			if (cache == NULL) // no cache created yet
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
			BattleItem
				* const rhandItem = unit->getItem("STR_RIGHT_HAND"),
				* const lhandItem = unit->getItem("STR_LEFT_HAND");
			if ((rhandItem == NULL
					|| rhandItem->getRules()->isFixed() == true)
				&& (lhandItem == NULL
					|| lhandItem->getRules()->isFixed() == true))
			{
				unitSprite->setBattleItem(NULL);
			}
			else
			{
				if (rhandItem != NULL)
//					&& rhandItem->getRules()->isFixed() == false)
				{
					unitSprite->setBattleItem(rhandItem);
				}

				if (lhandItem != NULL)
//					&& lhandItem->getRules()->isFixed() == false)
				{
					unitSprite->setBattleItem(lhandItem);
				}
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
 * @param projectile - projectile to place
 */
void Map::setProjectile(Projectile* projectile)
{
	_projectile = projectile;

	if (projectile != NULL
		&& Options::battleSmoothCamera == true)
	{
		_launch = true;
	}
}

/**
 * Gets the current projectile sprite on the map.
 * @return, pointer to Projectile or NULL if there is no projectile sprite on the map
 */
Projectile* Map::getProjectile() const
{
	return _projectile;
}

/**
 * Gets a list of explosion sprites on the map.
 * @return, list of explosion sprites
 */
std::list<Explosion*>* Map::getExplosions()
{
	return &_explosions;
}

/**
 * Gets the pointer to the camera.
 * @return, pointer to Camera
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
 * @return, pointer to a vector of Positions
 */
std::vector<Position>* Map::getWaypoints()
{
	return &_waypoints;
}

/**
 * Sets mouse-buttons' pressed state.
 * @param button	- index of the button
 * @param pressed	- the state of the button
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
 * @param flag - true if a unit is dying
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
 * @param height - the new base screen height
 */
void Map::setHeight(int height)
{
	Surface::setHeight(height);

	_visibleMapHeight = height - _iconHeight;

	_hidden->setHeight((_visibleMapHeight < 200)? _visibleMapHeight: 200);
	_hidden->setY((_visibleMapHeight - _hidden->getHeight()) / 2);
}

/**
 * Special handling for setting the width of the map viewport.
 * @param width - the new base screen width
 */
void Map::setWidth(int width)
{
	Surface::setWidth(width);

	_hidden->setX(_hidden->getX() + (width - getWidth()) / 2);
}

/**
 * Gets the hidden movement screen's vertical position.
 * @return, the vertical position of the hidden movement window
 */
const int Map::getMessageY()
{
	return _hidden->getY();
}

/**
 * Gets the icon height.
 * @return, icon panel height
 */
const int Map::getIconHeight()
{
	return _iconHeight;
}

/**
 * Gets the icon width.
 * @return, icon panel width
 */
const int Map::getIconWidth()
{
	return _iconWidth;
}

/**
 * Returns the angle (left/right balance) of a sound effect based on a map position.
 * @param pos - the map position to calculate the sound angle from
 * @return, the angle of the sound (360 = 0 degrees center)
 */
const int Map::getSoundAngle(Position pos)
{
	int midPoint = getWidth() / 2;

	Position screenPos;
	_camera->convertMapToScreen(
							pos,
							&screenPos);

	// cap the position to the screen edges relative to the center,
	// negative values indicating a left-shift, and positive values shifting to the right.
	screenPos.x = std::max(
						-midPoint,
						std::min(
								midPoint,
								screenPos.x + _camera->getMapOffset().x - midPoint));

	// Convert the relative distance left or right to a relative angle off-center.
	// Since Mix_SetPosition() uses modulo 360, we can't feed it a negative number, so add 360.
	// The integer-factor below is the allowable maximum deflection from center
	return (screenPos.x * 35 / midPoint) + 360;
}

/**
 * Resets the camera smoothing bool.
 */
void Map::resetCameraSmoothing()
{
	_smoothingEngaged = false;
}

/**
 * Sets whether to draw or not.
 * @param noDraw - true to stop this Map from drawing
 */
void Map::setNoDraw(bool noDraw)
{
	_noDraw = noDraw;
}

/**
 * Gets the SavedBattleGame.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* Map::getSavedBattle() const
{
	return _save;
}

}
