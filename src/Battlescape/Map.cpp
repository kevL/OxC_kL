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
//#include "Particle.h"
#include "Pathfinding.h"
#include "Position.h"
#include "Projectile.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "UnitSprite.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/NumberText.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"

#include "../Ruleset/RuleArmor.h"
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
			x,y),
		_game(game),
		_arrow(NULL),
		_arrow_kneel(NULL),
		_selectorX(0),
		_selectorY(0),
		_mouseX(0),
		_mouseY(0),
		_cursorType(CT_NORMAL),
		_cursorSize(1),
		_animFrame(0),
		_projectile(NULL),
		_projectileSet(NULL),
		_projectileInFOV(false),
		_explosionInFOV(false),
		_waypointAction(false),
		_firstBulletFrame(false),
		_visibleMapHeight(visibleMapHeight),
		_unitDying(false),
		_reveal(0),
		_smoothingEngaged(false),
//		_flashScreen(false),
		_mapIsHidden(false),
		_noDraw(false),
		_showProjectile(true),
		_battleSave(game->getSavedGame()->getBattleSave()),
		_res(game->getResourcePack()),
		_fuseColor(31),
		_tile(NULL)
{
	_iconWidth = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->w;
	_iconHeight = _game->getRuleset()->getInterface("battlescape")->getElement("icons")->h;

	_previewSetting	= Options::battleNewPreviewPath;
//	if (Options::traceAI == true) // turn everything on because we want to see the markers.
//		_previewSetting = PATH_FULL;

	_spriteWidth = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();

	_camera = new Camera(
					_spriteWidth,
					_spriteHeight,
					_battleSave->getMapSizeX(),
					_battleSave->getMapSizeY(),
					_battleSave->getMapSizeZ(),
					this,
					visibleMapHeight);

	_hiddenScreen = new BattlescapeMessage( // "Hidden Movement..." screen
										320,
										visibleMapHeight < 200 ? visibleMapHeight : 200);
	_hiddenScreen->setX(_game->getScreen()->getDX());
	_hiddenScreen->setY(_game->getScreen()->getDY());
//	_hiddenScreen->setY((visibleMapHeight - _hiddenScreen->getHeight()) / 2);
	_hiddenScreen->setTextColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color));

	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)& Map::scrollMouse);

	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)& Map::scrollKey);

	_camera->setScrollTimer(
						_scrollMouseTimer,
						_scrollKeyTimer);

/*	_txtAccuracy = new Text(24, 9, 0, 0);
	_txtAccuracy->setSmall();
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());
	_txtAccuracy->setHighContrast();
	_txtAccuracy->initText(
						_res->getFont("FONT_BIG"),
						_res->getFont("FONT_SMALL"),
						_game->getLanguage()); */
	_numAccuracy = new NumberText(24,9);
	_numAccuracy->setPalette(_game->getScreen()->getPalette());

	_numExposed = new NumberText(24,9);
	_numExposed->setPalette(_game->getScreen()->getPalette());
	_numExposed->setColor(3);
}

/**
 * Deletes the map.
 */
Map::~Map()
{
	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
	delete _arrow;
	delete _arrow_kneel;
	delete _hiddenScreen;
	delete _camera;
	delete _numAccuracy;
	delete _numExposed;
}

/**
 * Initializes the map.
 */
void Map::init()
{
	// load the unit-selected bobbing arrow into a surface
	const Uint8
		f = 16, // Fill, yellow // Palette::blockOffset(1)
		b = 14; // Border, black
	const Uint8 pixels_stand[81] = { 0, 0, b, b, b, b, b, 0, 0,
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
	for (size_t
			y = 0;
			y != 9;
			++y)
	{
		for (size_t
				x = 0;
				x != 9;
				++x)
		{
			_arrow->setPixelColor(
							static_cast<int>(x),
							static_cast<int>(y),
							pixels_stand[x + (y * 9)]);
		}
	}
	_arrow->unlock();

	// DarkDefender_begin:
	const Uint8 pixels_kneel[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
	for (size_t
			y = 0;
			y != 9;
			++y)
	{
		for (size_t
				x = 0;
				x != 9;
				++x)
		{
			_arrow_kneel->setPixelColor(
									static_cast<int>(x),
									static_cast<int>(y),
									pixels_kneel[x + (y * 9)]);
		}
	}
	_arrow_kneel->unlock(); // DarkDefender_end.

	_projectile = NULL;

//	if (_battleSave->getDepth() == 0)
	_projectileSet = _res->getSurfaceSet("Projectiles");
//	else
//		_projectileSet = _res->getSurfaceSet("UnderwaterProjectiles");

/*	int // kL_begin: reveal map's border tiles.
		size_x = _battleSave->getMapSizeX(),
		size_y = _battleSave->getMapSizeY(),
		size_z = _battleSave->getMapSizeZ();

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
					Tile* tile = _battleSave->getTile(Position(x,y,z));
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
 * Draws the whole map blit by blit.
 */
void Map::draw()
{
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
	clear(15); // black

	if (_noDraw == false) // don't draw if MiniMap is open.
	{
		const Tile* tile;

		if (_projectile != NULL)
//			&& _battleSave->getSide() == FACTION_PLAYER)
		{
			tile = _battleSave->getTile(Position(
											_projectile->getPosition(0).x / 16,
											_projectile->getPosition(0).y / 16,
											_projectile->getPosition(0).z / 24));
			if (tile != NULL
				&& (tile->getTileVisible() == true
					|| _battleSave->getSide() != FACTION_PLAYER)) // shows projectile during aLien berserk
			{
				_projectileInFOV = true;
			}
		}
		else
			_projectileInFOV = _battleSave->getDebugMode(); // reveals prj in debugmode.


		if (_explosions.empty() == false)
		{
			for (std::list<Explosion*>::const_iterator
					i = _explosions.begin();
					i != _explosions.end();
					++i)
			{
				tile = _battleSave->getTile(Position(
												(*i)->getPosition().x / 16,
												(*i)->getPosition().y / 16,
												(*i)->getPosition().z / 24));
				if (tile != NULL
					&& (tile->getTileVisible() == true
						|| (tile->getUnit() != NULL
							&& tile->getUnit()->getUnitVisible() == true)
						|| (*i)->isBig() == true
						|| _battleSave->getSide() != FACTION_PLAYER)) // shows hit-explosion during aLien berserk
				{
					_explosionInFOV = true;
					break;
				}
			}
		}
		else
			_explosionInFOV = _battleSave->getDebugMode(); // reveals expl in debugmode.


		if (_battleSave->getSelectedUnit() == NULL
			|| _battleSave->getSelectedUnit()->getUnitVisible() == true
			|| _unitDying == true
			|| _battleSave->getDebugMode() == true
			|| _projectileInFOV == true
			|| _waypointAction == true // stop flashing the Hidden Movement screen between waypoints.
			|| _explosionInFOV == true)
		{
			_mapIsHidden = false;

			kL_noReveal = false;
			drawTerrain(this);
		}
		else
		{
			if (kL_noReveal == false)
			{
				kL_noReveal = true;
				SDL_Delay(372);
			}

			_mapIsHidden = true;
			_hiddenScreen->blit(this);
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
			i = _battleSave->getMapDataSets()->begin();
			i != _battleSave->getMapDataSets()->end();
			++i)
	{
		(*i)->getSurfaceset()->setPalette(colors, firstcolor, ncolors);
	}

	_hiddenScreen->setPalette(colors, firstcolor, ncolors);
	_hiddenScreen->setBackground(_res->getSurface("TAC00.SCR"));
	_hiddenScreen->initText(
					_res->getFont("FONT_BIG"),
					_res->getFont("FONT_SMALL"),
					_game->getLanguage());
	_hiddenScreen->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"));
}

/**
 * Draw the terrain.
 * @note Keep this function as optimised as possible. It's big so minimise
 * overhead of function calls.
 * @param surface - the Surface to draw on
 */
void Map::drawTerrain(Surface* const surface) // private.
{
//	static const int arrowBob[8] = {0,1,2,1,0,1,2,1};
//	static const int arrowBob[10] = {0,2,3,3,2,0,-2,-3,-3,-2}; // DarkDefender

	Position bullet; // x-y position of bullet on screen.
	int
		bulletLowX	= 16000,
		bulletLowY	= 16000,
//		bulletLowZ	= 16000, // not used.
		bulletHighX	= 0,
		bulletHighY	= 0,
		bulletHighZ	= 0;

	if (_projectile != NULL // if there is a bullet get the highest x and y tiles to draw it on
		&& _explosions.empty() == true)
	{
		int
			part,
			trjOffset;

		if (_projectile->getItem() != NULL) // thrown item
			part = 0;
		else
			part = BULLET_SPRITES - 1;

		for (int
				i = 0;
				i <= part;
				++i)
		{
			trjOffset = 1 - i;

			if (_projectile->getPosition(trjOffset).x < bulletLowX)
				bulletLowX = _projectile->getPosition(trjOffset).x;

			if (_projectile->getPosition(trjOffset).y < bulletLowY)
				bulletLowY = _projectile->getPosition(trjOffset).y;

//			if (_projectile->getPosition(trjOffset).z < bulletLowZ)
//				bulletLowZ = _projectile->getPosition(trjOffset).z;

			if (_projectile->getPosition(trjOffset).x > bulletHighX)
				bulletHighX = _projectile->getPosition(trjOffset).x;

			if (_projectile->getPosition(trjOffset).y > bulletHighY)
				bulletHighY = _projectile->getPosition(trjOffset).y;

			if (_projectile->getPosition(trjOffset).z > bulletHighZ)
				bulletHighZ = _projectile->getPosition(trjOffset).z;
		}

		// convert bullet position from voxelspace to tilespace
		bulletLowX  /= 16;
		bulletLowY  /= 16;
//		bulletLowZ  /= 24;
		bulletHighX /= 16;
		bulletHighY /= 16;
		bulletHighZ /= 24;

		// if the projectile is outside the viewport - center back on it
		if (_projectileInFOV == true)
		{
			_camera->convertVoxelToScreen(
									_projectile->getPosition(),
									&bullet);

			if (Options::battleSmoothCamera == true)
			{
				if (_firstBulletFrame == true)
				{
					_firstBulletFrame = false;

					if (   bullet.x < 0
						|| bullet.x > surface->getWidth() - 1
						|| bullet.y < 0
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

						//Log(LOG_INFO) << "map:drawTerrain() set cameraPos";
						BattleAction* const action = _battleSave->getTileEngine()->getRfAction(); // rf ->
						if (action->actor->getFaction() != _battleSave->getSide())	// moved here from TileEngine::reactionShot()
						{															// because this is the (accurate) position of the bullet-shot-actor's Camera mapOffset.
							//Log(LOG_INFO) << ". " << action->actor->getId() << " to " << action->cameraPosition;
							std::map<int, Position>* rfShotList = _battleSave->getTileEngine()->getRfShotList();
							rfShotList->insert(std::pair<int, Position>(
																	action->actor->getId(),
																	_camera->getMapOffset()));
						}
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
						_camera->setPauseAfterShot();
						_smoothingEngaged = true;
					}
					else if (_projectile->getItem() != NULL) // thrown item
						_camera->setPauseAfterShot();
				}
				else // smoothing Engaged
					_camera->jumpXY(
								surface->getWidth() / 2 - bullet.x,
								_visibleMapHeight / 2 - bullet.y);

				const int posBullet_z = (_projectile->getPosition().z) / 24;
				if (_camera->getViewLevel() != posBullet_z)
					_camera->setViewLevel(posBullet_z);
			}
			else // NOT smoothCamera
			// kL_note: Camera remains stationary when xCom actively fires at target.
			// That is, Target is already onScreen due to targetting cursor click!
			// (And, player already knows what unit is shooting ...)
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


	const bool pathPreview = _battleSave->getPathfinding()->isPathPreviewed();
	NumberText* wpID = NULL;

	if (_waypoints.empty() == false
		|| (pathPreview == true
			&& (_previewSetting & PATH_TU_COST)))
	{
		// note: WpID is used for both pathPreview and BL waypoints.
		// kL_note: Leave them the same color.
		wpID = new NumberText(15, 15, 20, 30);
		wpID->setPalette(getPalette());
		wpID->setColor(1); // white

/*		Uint8 wpColor;
		if (_battleSave->getBattleTerrain() == "DESERT")
			|| _battleSave->getBattleTerrain() == "DESERTMOUNT")
		{
			wpColor = Palette::blockOffset(0)+1; // white
		else
			wpColor = Palette::blockOffset(1)+4; */
/*		if (_battleSave->getBattleTerrain() == "POLAR"
			|| _battleSave->getBattleTerrain() == "POLARMOUNT")
		{
			wpColor = Palette::blockOffset(1)+4; // orange
		}
		else
			wpColor = Palette::blockOffset(0)+1; // white

		wpID->setColor(wpColor); */
	}


	int // get Map's corner-coordinates for rough boundaries in which to draw tiles.
		beginX,
		beginY,
		beginZ = 0,
		endX, //= _battleSave->getMapSizeX() - 1,
		endY, //= _battleSave->getMapSizeY() - 1,
		endZ, //= _camera->getViewLevel(),
		d;

	if (_camera->getShowAllLayers() == true)
		endZ = _battleSave->getMapSizeZ() - 1;
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
	Surface* srfSprite;
//	Tile* tile;
	const Tile* tileBelow;

	Position
		posMap,
		posScreen,
		walkOffset;

	int
		frame,
		tileShade,
		wallShade,
		shade = 16, // avoid VC++ linker warning.
//		shade2,
		animOffset,
		quad;		// The quadrant is 0 for small units; large units also have quadrants 1,2 & 3; the
					// relative x/y Position of the unit's primary quadrant vs the current tile's Position.
	bool
		invalid = false,

		hasFloor, // these denote characteristics of 'tile' as in the current Tile of the loop.
//		hasWestWall,
//		hasNorthWall,
		hasObject,
//		unitNorthValid,

		halfRight = false; // avoid VC++ linker warning.
//		redraw,

//		kL_Debug_stand = false, // for debugging ->
//		kL_Debug_walk = true,
//		kL_Debug_vert = false,
//		kL_Debug_main = false;


	surface->lock();
	for (int
			itZ = beginZ; // 3. finally lap the levels bottom to top
			itZ < endZ + 1;
			++itZ)
	{
		for (int
				itX = beginX; // 2. next draw those columns eastward
				itX < endX + 1;
				++itX)
		{
			for (int
					itY = beginY; // 1. first draw terrain in columns north to south
					itY < endY + 1;
					++itY)
			{
				posMap = Position(
								itX,
								itY,
								itZ);
				_camera->convertMapToScreen(
										posMap,
										&posScreen);
				posScreen += _camera->getMapOffset();

				if (   posScreen.x > -_spriteWidth // only render cells that are inside the surface (ie viewport ala player's monitor)
					&& posScreen.x < surface->getWidth() + _spriteWidth
					&& posScreen.y > -_spriteHeight
					&& posScreen.y < surface->getHeight() + _spriteHeight)
				{
					_tile = _battleSave->getTile(posMap);
					tileBelow = _battleSave->getTile(posMap + Position(0,0,-1));

					if (_tile == NULL)
						continue;

					if (_tile->isDiscovered(2) == true)
						tileShade = _tile->getShade();
					else
						tileShade = 16;

					unit = _tile->getUnit();

//					tileColor = _tile->getPreviewColor();
					hasFloor = false;
//					hasWestWall = false;
//					hasNorthWall = false;
					hasObject = false;
//					unitNorthValid = false;

// Draw Floor
					srfSprite = _tile->getSprite(O_FLOOR);
					if (srfSprite)
					{
						hasFloor = true;

						srfSprite->blitNShade(
								surface,
								posScreen.x,
								posScreen.y - _tile->getMapData(O_FLOOR)->getYOffset(),
								tileShade);

						// kL_begin #1 of 3:
						// This ensures the rankIcon isn't half-hidden by a floor above & west of soldier.
						// ... unless there's also a floor directly above soldier
						// Special case: crazy battleship tile+half floors; so check for content object diagonal wall directly above soldier also.
						// Also, make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						// ... should probably be a subfunction
						if (itX < endX && itZ > 0)
						{
							const Tile* const tileEast = _battleSave->getTile(posMap + Position(1,0,0));

							if (tileEast != NULL
								&& tileEast->getSprite(O_FLOOR) == NULL
								&& (tileEast->getMapData(O_OBJECT) == NULL // special case ->
									|| tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NWSE))
							{
								const Tile* const tileEastBelow = _battleSave->getTile(posMap + Position(1,0,-1));

								const BattleUnit* unitEastBelow;
								if (tileEastBelow != NULL)
									unitEastBelow = tileEastBelow->getUnit();
								else
									unitEastBelow = NULL;

								if (unitEastBelow != NULL
									&& unitEastBelow != _battleSave->getSelectedUnit()
									&& unitEastBelow->getGeoscapeSoldier() != NULL
									&& unitEastBelow->getFaction() == unitEastBelow->getOriginalFaction())
								{
									if (unitEastBelow->getFatalWounds() > 0)
									{
										srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 2 + 16,
													posScreen.y + 3 + 32 + getTerrainLevel(
																						unitEastBelow->getPosition(),
																						unitEastBelow->getArmor()->getSize()),
													0);

										srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 4 + 16,
													posScreen.y + 4 + 32 + getTerrainLevel(
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
//										srfSprite = NULL;
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 2 + 16,
													posScreen.y + 3 + 32 + getTerrainLevel(
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
													posScreen.x + walkOffset.x + 9 + 16,
													posScreen.y + walkOffset.y + 4 + 32,
													1, // 10
													false,
													1); // 1=white, 2=yellow-red, 3=red, 6=lt.brown, 9=blue
										} */
									}
								}
							}
						} // kL_end.
					} // end draw floor


// Draw Cursor Background
					if (_cursorType != CT_NONE
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX < itX + 1
						&& _selectorY < itY + 1
						&& _battleSave->getBattleState()->getMouseOverIcons() == false)
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true)
									&& (_cursorType != CT_PSI
										|| ((_battleSave->getBattleGame()->getCurrentAction()->type == BA_PSICOURAGE
												&& unit->getFaction() != FACTION_HOSTILE)
											|| (_battleSave->getBattleGame()->getCurrentAction()->type != BA_PSICOURAGE
												&& unit->getFaction() != FACTION_PLAYER))))
								{
									frame = (_animFrame % 2); // yellow box
								}
								else
									frame = 0; // red box
							}
							else // CT_AIM ->
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true))
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
										posScreen.x,
										posScreen.y,
										0);
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frame = 2; // blue box

							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							if (srfSprite != NULL)
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y,
										0);
						}
					}
					// end cursor bg

/*
// Redraws Unit when it moves along the NORTH, WEST, or NORTH-WEST side of a content-object. Et&.
					if (itY > 0)
//					if (false)
					{
						const Tile* const tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));

						BattleUnit* unitNorth;
						if (tileNorth != NULL)
							unitNorth = tileNorth->getUnit();
						else
							unitNorth = NULL;

						if (unitNorth != NULL
							&& (unitNorth->getUnitVisible() == true
								|| _battleSave->getDebugMode() == true)
							&& (unitNorth->getStatus() == STATUS_WALKING
								|| unitNorth->getStatus() == STATUS_FLYING))
						{
							if (tileNorth->getTerrainLevel() - _tile->getTerrainLevel() < 1) // positive -> Tile is higher
							{
// Phase I: redraw unit NORTH to make sure it doesn't get drawn [over any walls or] under any tiles.
								if (unitNorth->getDirection() != 2
									&& unitNorth->getDirection() != 6 // perhaps !0 && !4 also; ie. do dir=1,5,3,7
									&& unitNorth->getStatus() != STATUS_FLYING)
								{
									if (itX > 0 && itY < endY)
									{
//										const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0)); // Reaper sticks its righthind leg out through south wall if dir=1

//										if (tileSouthWest == NULL // hopefully Draw Main Unit will keep things covered if this fails.
//											|| tileSouthWest->getMapData(O_NORTHWALL) == NULL) // or do this check specifically for Reapers
										{
//											unitNorthValid = true;

											if (tileNorth->isDiscovered(2) == true)
												shade = tileNorth->getShade();
											else
												shade = 16;

											quad = tileNorth->getPosition().x - unitNorth->getPosition().x
												+ (tileNorth->getPosition().y - unitNorth->getPosition().y) * 2;

											srfSprite = unitNorth->getCache(&invalid, quad);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [10]";
												calculateWalkingOffset(
																	unitNorth,
																	&walkOffset);

												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x + 16,
														posScreen.y + walkOffset.y - 8,
														shade);

												if (unitNorth->getFireOnUnit() != 0)
												{
													frame = 4 + (_animFrame / 2);
													srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
													if (srfSprite)
														srfSprite->blitNShade(
																surface,
																posScreen.x + walkOffset.x + 16,
																posScreen.y + walkOffset.y - 8,
																0);
												}
											}
										}
									}
								}

// Phase II: redraw any east wall object in the tile NORTH of the moving unit;
								// only applies to movement in the north/south direction.
								if (itY > 1)
								{
									if (unitNorth->getDirection() == 0
										|| unitNorth->getDirection() == 4)
									{
										const Tile* const tileNorthNorth = _battleSave->getTile(posMap + Position(0,-2,0));

										if (tileNorthNorth != NULL)
										{
											srfSprite = tileNorthNorth->getSprite(O_OBJECT);
											if (srfSprite
												&& tileNorthNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST)
											{
												if (tileNorthNorth->isDiscovered(2) == true)
													shade2 = tileNorthNorth->getShade();
												else
													shade2 = 16;

												srfSprite->blitNShade(
														surface,
														posScreen.x + 32,
														posScreen.y - 16 - tileNorthNorth->getMapData(O_OBJECT)->getYOffset(),
														shade2);
											}
										}
									}
								}

// Phase III: redraw any south wall object in the tile NORTH-WEST.
								if (itX > 0)
								{
									if (unitNorth->getDirection() == 2
										|| unitNorth->getDirection() == 6)
									{
										const Tile* const tileNorthWest = _battleSave->getTile(posMap + Position(-1,-1,0));

										if (tileNorthWest != NULL)
										{
											srfSprite = tileNorthWest->getSprite(O_OBJECT);
											if (srfSprite
												&& tileNorthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH)
											{
												if (tileNorthWest->isDiscovered(2) == true)
													shade2 = tileNorthWest->getShade();
												else
													shade2 = 16;

												srfSprite->blitNShade(
														surface,
														posScreen.x,
														posScreen.y - 16 - tileNorthWest->getMapData(O_OBJECT)->getYOffset(),
														shade2);
											}
										}
									}
								}

// Phase IV: redraw any south or east bigWall object in the tile NORTH.
								srfSprite = tileNorth->getSprite(O_OBJECT);
								if (srfSprite
									&& tileNorth->getMapData(O_OBJECT)->getBigWall() > BIGWALL_NORTH // do East,South,East_South
									&& tileNorth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_W_N)
								{
									srfSprite->blitNShade(
											surface,
											posScreen.x + 16,
											posScreen.y - 8 - tileNorth->getMapData(O_OBJECT)->getYOffset(),
											shade);
								}

								// Phase V: redraw object in the tile SOUTH-WEST (half-tile). kL: moved below_
// Phase VI: redraw everything in the tile WEST (half-tile).
								if (itX > 0)
								{
									Tile* const tileWest = _battleSave->getTile(posMap + Position(-1,0,0));

									if (tileWest != NULL)
									{
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

										if (unitNorth != unit // NOT for large units
											&& tileWest->getTerrainLevel() > -1) // don't draw if it overwrites foreground lower-stairs in that same tile.
										{
											srfSprite = tileWest->getSprite(O_WESTWALL);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (tileWest->isDiscovered(0) == true
													&& (tileWest->getMapData(O_WESTWALL)->isDoor() == true
														|| tileWest->getMapData(O_WESTWALL)->isUfoDoor() == true))
												{
													wallShade = tileWest->getShade();
												}
												else
													wallShade = shade;

												srfSprite->blitNShade(
														surface,
														posScreen.x - 16,
														posScreen.y - 8 - tileWest->getMapData(O_WESTWALL)->getYOffset(),
														wallShade,
														true); // halfRight
											}
										}

										srfSprite = tileWest->getSprite(O_NORTHWALL);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileWest->isDiscovered(1) == true
												&& (tileWest->getMapData(O_NORTHWALL)->isDoor() == true
													|| tileWest->getMapData(O_NORTHWALL)->isUfoDoor() == true))
											{
												wallShade = tileWest->getShade();
											}
											else
												wallShade = shade;

											srfSprite->blitNShade(
													surface,
													posScreen.x - 16,
													posScreen.y - 8 - tileWest->getMapData(O_NORTHWALL)->getYOffset(),
													wallShade,
													true); // halfRight
										}

										if (unitWest == NULL // NOT for large units
											|| unitWest != unit)
										{
											const Tile* const tileNorthWest = _battleSave->getTile(posMap + Position(-1,-1,0));

											if (tileNorthWest != NULL)
											{
												if (tileNorthWest->getTerrainLevel() == _tile->getTerrainLevel()) // this could perhaps be refined ...
												{
													srfSprite = tileWest->getSprite(O_OBJECT);	// This is what creates probls if (unitWest == unit);
//													srfSprite = NULL;							// the (large) unitsprite gets overwritten by a slope ....
													if (srfSprite
														&& tileWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NWSE // do none,Block,NESW,West,North,West&North
														&& (tileWest->getMapData(O_OBJECT)->getBigWall() < BIGWALL_EAST
															|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N))
													{
														srfSprite->blitNShade(
																surface,
																posScreen.x - 16,
																posScreen.y - 8 - tileWest->getMapData(O_OBJECT)->getYOffset(),
																shade,
																true); // halfRight

														// if the object in the tile to the west is a diagonal NESW bigWall
														// it needs to have the black triangle at the bottom covered up
														if (tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW)
														{
															srfSprite = _tile->getSprite(O_FLOOR);
															if (srfSprite)
																srfSprite->blitNShade(
																		surface,
																		posScreen.x,
																		posScreen.y - _tile->getMapData(O_FLOOR)->getYOffset(),
																		tileShade);
														}
													}
												}
											}
										}

										// Draw corpse + item if any on top of the floor
										bool val;
										int sprite = tileWest->getCorpseSprite(&val);
										if (sprite != -1)
										{
											srfSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
											if (srfSprite)
											{
												srfSprite->blitNShade(
														surface,
														posScreen.x - 16,
														posScreen.y - 8 + tileWest->getTerrainLevel(),
														shade,
														true); // halfRight
											}

											if (val == true
												&& tileWest->isDiscovered(2) == true)
											{
												// Draw SMOKE & FIRE if itemUnit is on Fire
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												if (srfSprite)
													srfSprite->blitNShade(
															surface,
															posScreen.x - 16,
															posScreen.y - 8 + tileWest->getTerrainLevel(),
															0);
												break;
											}
										}

										sprite = tileWest->getTopSprite(&val);
										if (sprite != -1)
										{
											srfSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
											if (srfSprite)
											{
												srfSprite->blitNShade(
														surface,
														posScreen.x - 16,
														posScreen.y - 8 + tileWest->getTerrainLevel(),
														shade,
														true); // halfRight

												if (val == true
													&& tileWest->isDiscovered(2) == true)	// sprite== 21, standard proxy grenade
												{
													srfSprite->setPixelColor(				// cycles from 31 down to 16 and jumps to 31 again
																		16,28,				// <- the pixel coords
																		_fuseColor);		// 17 is the proxy-pixel's spritesheet color
												}
											}
										}

										if (unitWest != NULL
											&& unitWest->getStatus() != STATUS_FLYING // <- unitNorth maybe.
											&& (unitWest->getUnitVisible() == true
												|| _battleSave->getDebugMode() == true)
											&& (unitWest != unit
												|| (unitWest->getDirection() != 2 // Should these tie into STATUS_WALKING; they do, see below_
													&& unitWest->getDirection() != 6))
											&& (tileWest->getMapData(O_OBJECT) == NULL
												|| tileWest->getMapData(O_OBJECT)->getBigWall() < BIGWALL_EAST // do none,[Block,diagonals],West,North,West&North
												|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N))
										{
											redraw = true;

											if (itY < endY)
											{
												const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

												if (tileSouthWest != NULL)
												{
													if (tileSouthWest->getMapData(O_OBJECT) != NULL
														&& tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH)
													{
														redraw = false;
													}

													if (itY < endY - 1)
													{
														if (unitWest->getArmor()->getSize() == 2
															&& (unitWest->getDirection() == 1 // Should these tie into STATUS_WALKING; they do, see below_
																|| unitWest->getDirection() == 5
																|| unitWest->getDirection() == 3
																|| unitWest->getDirection() == 7)) // ... large unitsWest are sticking their noses & butts out, through northerly walls in tileSouthWest.
														{
															const Tile* const tileSouthSouthWest = _battleSave->getTile(posMap + Position(-1,2,0));

															if (tileSouthSouthWest != NULL)
															{
																if (tileSouthSouthWest->getMapData(O_NORTHWALL) != NULL
																	|| (tileSouthSouthWest->getMapData(O_OBJECT) != NULL
																		&& (tileSouthSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
																			|| tileSouthSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NORTH
																			|| tileSouthSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N)))
																{
																	redraw = false; // ok, so this stops the nose sticking out when Reaper moves dir=5 (which is .. odd)
																}
															}
														}
													}
												}
											}

											if (redraw == true)
											{
												quad = tileWest->getPosition().x - unitWest->getPosition().x
													+ (tileWest->getPosition().y - unitWest->getPosition().y) * 2;

												srfSprite = unitWest->getCache(&invalid, quad);
//												srfSprite = NULL;
												if (srfSprite)
												{
													if (unitWest->getStatus() != STATUS_WALKING)
													{
														if (kL_Debug_stand) Log(LOG_INFO) << ". drawUnit [20]";
														halfRight = true;
														walkOffset.x = 0;
														walkOffset.y = getTerrainLevel(
																					unitWest->getPosition(),
																					unitWest->getArmor()->getSize());
													}
													else // isWalking
													{
														if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [25]";
														halfRight = false;
														calculateWalkingOffset(
																			unitWest,
																			&walkOffset);
													}

													srfSprite->blitNShade(
															surface,
															posScreen.x - 16 + walkOffset.x,
															posScreen.y - 8 + walkOffset.y,
															shade,
															halfRight);

													if (unitWest->getFireOnUnit() != 0)
													{
														frame = 4 + (_animFrame / 2);
														srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
														if (srfSprite)
															srfSprite->blitNShade(
																	surface,
																	posScreen.x - 16 + walkOffset.x,
																	posScreen.y - 8 + walkOffset.y,
																	0,
																	halfRight);
													}
												}
											}
										}
										// end Redraw unitWest w/ unitNorth walking

										// Draw SMOKE & FIRE
										if (tileWest->isDiscovered(2) == true
											&& (tileWest->getSmoke() != 0
												|| tileWest->getFire() != 0))
										{
											if (tileWest->getFire() == 0)
											{
												frame = ResourcePack::SMOKE_OFFSET;
												frame += (tileWest->getSmoke() + 1) / 2;
											}
											else
											{
												frame =
												shade = 0;
											}

											animOffset = _animFrame / 2 + tileWest->getAnimationOffset();
											if (animOffset > 3) animOffset -= 4;
											frame += animOffset;

											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
//											srfSprite = NULL;
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														posScreen.x - 16,
														posScreen.y - 8 + tileWest->getTerrainLevel(),
														shade,
														true); // halfRight
										}

										// Draw front bigWall object
										srfSprite = tileWest->getSprite(O_OBJECT);
										if (srfSprite
											&& tileWest->getMapData(O_OBJECT)->getBigWall() > BIGWALL_NORTH // do East,South,East&South
											&& tileWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_W_N)
										{
											srfSprite->blitNShade(
													surface,
													posScreen.x - 16,
													posScreen.y - 8 - tileWest->getMapData(O_OBJECT)->getYOffset(),
													shade,
													true); // halfRight
										}
									}
									// end redraw Tile West w/ unitNorth valid.

// Phase V-b: redraw object in the tile SOUTH-WEST (half-tile).
									if (itY < endY)
									{
										if (unitNorth->getDirection() == 1
											|| unitNorth->getDirection() == 5)
										{
											const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

											if (tileSouthWest != NULL
												&& tileSouthWest->getTerrainLevel() == 0) // Stop concealing lower right bit of unit to west of redrawn tile w/ raised terrain.
											{
												srfSprite = tileSouthWest->getSprite(O_OBJECT); // This might be unnecessary after tileSouthWest check was put in above^ (but i doubt it)
//												srfSprite = NULL;
												if (srfSprite)
												{
													if (tileSouthWest->isDiscovered(2) == true)
														shade = tileSouthWest->getShade();
													else
														shade = 16;

													srfSprite->blitNShade(
															surface,
															posScreen.x - 32,
															posScreen.y - tileSouthWest->getMapData(O_OBJECT)->getYOffset(),
															shade,
															true); // halfRight

													// Draw SMOKE & FIRE -> note will muck w/ foreground parts
													if (tileSouthWest->isDiscovered(2) == true
														&& (tileSouthWest->getSmoke() != 0
															|| tileSouthWest->getFire() != 0))
													{
														if (tileSouthWest->getFire() == 0)
														{
															frame = ResourcePack::SMOKE_OFFSET;
															frame += (tileSouthWest->getSmoke() + 1) / 2;
														}
														else
														{
															frame =
															shade = 0;
														}

														animOffset = _animFrame / 2 + tileSouthWest->getAnimationOffset();
														if (animOffset > 3) animOffset -= 4;
														frame += animOffset;

														srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
//														srfSprite = NULL;
														if (srfSprite)
															srfSprite->blitNShade(
																	surface,
																	posScreen.x - 32,
																	posScreen.y + tileSouthWest->getTerrainLevel(),
																	shade,
																	true); // halfRight
													}
												}
											}
										}
									}
								} // end (itX > 0)
							}
						} // end unitNorth walking TRUE
					} // end (itY > 0)
*/

// Draw Tile Background
					if (_tile->isVoid(true, false) == false) // need to take this out because, flying units moving on/off roofs
					{
						bool drawUnitNorth = false;

// Draw west wall
						srfSprite = _tile->getSprite(O_WESTWALL);
						if (srfSprite)
						{
//							hasWestWall = true;
							drawUnitNorth = true;

							if (_tile->isDiscovered(0) == true
								&& (_tile->getMapData(O_WESTWALL)->isDoor() == true
									|| _tile->getMapData(O_WESTWALL)->isUfoDoor() == true))
							{
								wallShade = _tile->getShade();
							}
							else
								wallShade = tileShade;

							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_WESTWALL)->getYOffset(),
									wallShade);
						}

// Draw North Wall
						srfSprite = _tile->getSprite(O_NORTHWALL);
						if (srfSprite)
						{
//							hasNorthWall = true;

							if (_tile->isDiscovered(1) == true
								&& (_tile->getMapData(O_NORTHWALL)->isDoor() == true
									|| _tile->getMapData(O_NORTHWALL)->isUfoDoor() == true))
							{
								wallShade = _tile->getShade();
							}
							else
								wallShade = tileShade;

							if (_tile->getMapData(O_WESTWALL) != NULL)
								halfRight = true;
							else
								halfRight = false;

							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_NORTHWALL)->getYOffset(),
									wallShade,
									halfRight);
						}

// Draw Object in Background & Center
						srfSprite = _tile->getSprite(O_OBJECT);
						if (srfSprite
							&& (_tile->getMapData(O_OBJECT)->getBigWall() < BIGWALL_EAST // do none,Block,diagonals,West,North,West&North
								|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N))
						{
							hasObject = true;
							if (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST)
								drawUnitNorth = true;

							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_OBJECT)->getYOffset(),
									tileShade);
						}

// Draw Corpse + Item on Floor if any
						bool val;
						int sprite = _tile->getCorpseSprite(&val);
						if (sprite != -1)
						{
							srfSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
							if (srfSprite)
							{
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + _tile->getTerrainLevel(),
										tileShade);
							}

							if (val == true
								&& _tile->isDiscovered(2) == true)
							{
								// Draw SMOKE & FIRE if itemUnit is on Fire
								frame = 4 + (_animFrame / 2);
								srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
								if (srfSprite)
									srfSprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + _tile->getTerrainLevel(),
											0);
								break;
							}
						}

						sprite = _tile->getTopSprite(&val);
						if (sprite != -1)
						{
							srfSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
							if (srfSprite)
							{
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + _tile->getTerrainLevel(),
										tileShade);

								if (val == true
									&& _tile->isDiscovered(2) == true)	// sprite== 21, standard proxy grenade
								{
									srfSprite->setPixelColor(			// cycles from 31 down to 16 and jumps to 31 again
														16,28,			// <- the pixel coords
														_fuseColor);	// 17 is the proxy-pixel's spritesheet color
								}
							}
						}

/*
// Redraw any unit moving onto or off of this Tile wrt a lower Z-level.
						if (itZ > 0
							&& _tile->hasNoFloor(tileBelow) == false
							&& (_tile->getMapData(O_OBJECT) == NULL
								|| _tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) < 6)) // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
						{
							for (std::vector<BattleUnit*>::const_iterator
									i = _battleSave->getUnits()->begin();
									i != _battleSave->getUnits()->end();
									++i)
							{
								if ((*i)->getStatus() == STATUS_WALKING)
								{
									int
										pixelOffset_x,
										pixelOffset_y;

									if (hilltopRedraw(
													**i,
													posMap,
													pixelOffset_x,
													pixelOffset_y) == true)
									{
										srfSprite = (*i)->getCache(&invalid, 0);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [30]";

											if (_tile->isDiscovered(2) == true)
												shade = _tile->getShade();
											else
												shade = 16;

											calculateWalkingOffset(
																*i,
																&walkOffset);

											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + pixelOffset_x,
													posScreen.y + walkOffset.y + pixelOffset_y,
													shade);

											if ((*i)->getFireOnUnit() != 0)
											{
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												if (srfSprite)
													srfSprite->blitNShade(
															surface,
															posScreen.x + walkOffset.x + pixelOffset_x,
															posScreen.y + walkOffset.y + pixelOffset_y,
															0);
											}
										}
									}

									break;
								}
							}
						}
						// end hilltop Redraw.
*/
/*
// Redraw unitNorth when it's on a reverse slope. Or level slope. Or moving NS along westerly wall. Or if it's a snake ... with a long tail.
						if (itY > 0
							&& _tile->getMapData(O_NORTHWALL) == NULL)
						{
							const Tile* const tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));

							if (tileNorth != NULL
								&& (tileNorth->getMapData(O_OBJECT) == NULL
									|| tileNorth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_SOUTH))
							{
								BattleUnit* const unitNorth = tileNorth->getUnit();

								if (unitNorth != NULL
									&& (unitNorth->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true)	// this is just for the bigfooted ScoutDrone anyway .... ht. 10
									&& ((unitNorth->getHeight(true) < 13		// the problem is this draws overtop of the Cursor's front box
											&& unitNorth->getStatus() == STATUS_STANDING)
										|| (drawUnitNorth == true
											&& unitNorth->getStatus() == STATUS_WALKING
											&& (unitNorth->getDirection() == 0
												|| unitNorth->getDirection() == 4))))
								{
									if ((tileNorth->getTerrainLevel() - _tile->getTerrainLevel() > -1 // positive -> Tile is higher
											&& _tile->getMapData(O_OBJECT) != NULL
											&& _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
											&& (tileNorth->getMapData(O_OBJECT) == NULL
												|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
												|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST
												|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NORTH
												|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N))
										|| drawUnitNorth == true) // && hasNorthWall == false, tileNorth has no foreground, etc.
									{
										quad = tileNorth->getPosition().x - unitNorth->getPosition().x
											+ (tileNorth->getPosition().y - unitNorth->getPosition().y) * 2;

										srfSprite = unitNorth->getCache(&invalid, quad);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (kL_Debug_stand && unitNorth->getStatus() == STATUS_STANDING) Log(LOG_INFO) << ". drawUnit [40]";
											if (kL_Debug_walk && unitNorth->getStatus() == STATUS_WALKING) Log(LOG_INFO) << ". drawUnit [45]";

											if (tileNorth->isDiscovered(2) == true)
												shade = tileNorth->getShade();
											else
												shade = 16;

											calculateWalkingOffset(
																unitNorth,
																&walkOffset);

											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 16,
													posScreen.y + walkOffset.y - 8,
													shade);

											if (unitNorth->getFireOnUnit() != 0)
											{
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												if (srfSprite)
													srfSprite->blitNShade(
															surface,
															posScreen.x + walkOffset.x + 16,
															posScreen.y + walkOffset.y - 8,
															0);
											}
										}
									}
								}
							}
						}
						// end Redraw unitNorth on reverse slope.
*/
/*
// Redraw westerly units that are moving easterly-westerly.
						if (itX > 0
							&& unit == NULL)
//TEST							&& (hasFloor == true						// special handling for moving units concealed by current floorTile.
//TEST								|| _tile->getMapData(O_OBJECT) != NULL))	// non-bigWall content-object might act as flooring
						{
							// If not unitSouth, unitSouthWest, (or unit).

							// TODO: Smoke & Fire render below this, use checks to draw or not.
							// TODO: reDraw unit SOUTH-WEST. ... unit Walking.
							// TODO: reDraw unit WEST. ... unit Walking and no unitNorth exists.
							const Tile* const tileWest = _battleSave->getTile(posMap + Position(-1,0,0));

							BattleUnit* unitWest = NULL;

							if (tileWest != NULL)
							{
								unitWest = tileWest->getUnit();

								if (unitWest != NULL
									&& (unitWest->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true)
									&& unitWest != unit // large units don't need to redraw their western parts
									&& (unitWest->getStatus() == STATUS_WALKING
										|| unitWest->getStatus() == STATUS_FLYING))
								{
									//Log(LOG_INFO) << "[1] unitWest is walking/flying";
									redraw = false;

									if (tileWest->getMapData(O_OBJECT) == NULL // tileWest checks were written for dir=2,6 (but scoped to affect dir=1,5 also)
										|| tileWest->getMapData(O_OBJECT)->getBigWall() == NULL
										|| (tileWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_SOUTH
											&& tileWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
											&& tileWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_E_S))
									{
										//Log(LOG_INFO) << "[1] tileWest Object NULL";
										if (unitWest->getDirection() == 2
											|| unitWest->getDirection() == 6)
										{
											//Log(LOG_INFO) << "[1] . dir 2/6";
											if ((_tile->getMapData(O_WESTWALL) == NULL			// redundant [not exactly it appears], due to
//													|| _tile->isUfoDoorOpen(O_WESTWALL) == true	// this. <- checks if westWall is valid
													|| (_tile->getMapData(O_WESTWALL)->getTuCostPart(MT_WALK) != 255 // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
														&& (_tile->getMapData(O_WESTWALL)->isUfoDoor() == false
															|| _tile->isUfoDoorOpen(O_WESTWALL) == true)))
												&& (_tile->getMapData(O_OBJECT) == NULL
													|| (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
														&& std::abs(tileWest->getTerrainLevel() - _tile->getTerrainLevel()) < 13) // positive means Tile is higher
													|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NORTH
													|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST))
											{
												//Log(LOG_INFO) << "[1] . . tile No westwall OR object";
												const Tile* const tileSouth = _battleSave->getTile(posMap + Position(0,1,0));

												if (tileSouth == NULL
													|| ((tileSouth->getMapData(O_NORTHWALL) == NULL // or tu != 255, ie. isWalkable rubble that lets sight pass over it
															|| tileSouth->isUfoDoorOpen(O_NORTHWALL) == true)
														&& (tileSouth->getMapData(O_OBJECT) == NULL
															|| (tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
																&& tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NESW
																&& tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NORTH
																&& tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_W_N))))
//														&& tileSouth->getUnit() == NULL)) // unless unit is short and lets sight pass overtop. DOES NOT BLOCK!
												{
													//Log(LOG_INFO) << ". . . tileSouth No northwall OR object";
													const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

													if (tileSouthWest == NULL
														|| (tileSouthWest->getUnit() == NULL // <- maybe.
															&& (tileSouthWest->getMapData(O_NORTHWALL) == NULL
																|| tileSouthWest->isUfoDoorOpen(O_NORTHWALL) == true)
//																|| tileSouth->getMapData(O_NORTHWALL) == NULL) // <- getting very redundant; soon needs break-out functions like checkNorthSideBlockage() & checkWestSideBlockage() etc etc etc etc, also checkObjectSightBlockageLeft/Right() etc.
															&& (tileSouthWest->getMapData(O_OBJECT) == NULL
																|| (tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
//																	&& tileSouthWest->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) != 255 // <- maybe
																	&& tileSouthWest->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) < 6 // from Dir=1/5 // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
																	// that needs to change. Use, say, a TU cost in MCDs of 6+ to denote big bushy objects that act like chairs & other non-walkable objects.
																	&& tileSouthWest->getMapData(O_OBJECT)->getDataset()->getName() != "LIGHTNIN"
																	&& tileSouthWest->getMapData(O_OBJECT)->getSprite(0) != 42)
																|| tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NWSE
																|| tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST
																|| tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH)))
													{
														//Log(LOG_INFO) << "[1] . . . . tileSouthWest No northwall OR object";
														//Log(LOG_INFO) << "[1] . . . . REDRAW halfRight";
														redraw = true;
														halfRight = true;
													}
												}
											}
										}
										else if (itY > 0
//											&& unit == NULL // unless unit is short and lets sight pass overtop.
											&& (unitWest->getDirection() == 1
												|| unitWest->getDirection() == 5))
										{
											//Log(LOG_INFO) << "[1] . dir 1/5";
											if ((_tile->getMapData(O_WESTWALL) == NULL		// or tu != 255, ie. isWalkable rubble that lets sight pass over it
													|| _tile->isUfoDoorOpen(O_WESTWALL) == true)
												&& (_tile->getMapData(O_NORTHWALL) == NULL	// or tu != 255, ie. isWalkable rubble that lets sight pass over it
													|| _tile->isUfoDoorOpen(O_NORTHWALL) == true)
												&& (_tile->getMapData(O_OBJECT) == NULL
													|| (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
														&& tileWest->getTerrainLevel() - _tile->getTerrainLevel() < 13 // positive means Tile is higher
//														&& _tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) != 255)))
														&& _tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) < 6))) // ie. not a big bushy object or chair etc. // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
											{
												//Log(LOG_INFO) << "[1] . . tile No westwall OR object";
												const Tile* const tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));

												if (tileNorth == NULL
													|| tileNorth->getMapData(O_OBJECT) == NULL
													|| (tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
														&& std::abs(tileWest->getTerrainLevel() - tileNorth->getTerrainLevel()) < 13	// positive means tileNorth is higher
														&& tileNorth->getTerrainLevel() - _tile->getTerrainLevel() < 13))				// positive means Tile is higher
												{
													//Log(LOG_INFO) << "[1] . . . tileNorth No object";
													const Tile* const tileSouth = _battleSave->getTile(posMap + Position(0,1,0));

													if (tileSouth == NULL
														|| ((tileSouth->getMapData(O_NORTHWALL) == NULL // or tu != 255, ie. isWalkable rubble that lets sight pass over it
																|| tileSouth->isUfoDoorOpen(O_NORTHWALL) == true)
															&& (tileSouth->getMapData(O_OBJECT) == NULL
																|| (tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
																	&& tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NESW
																	&& tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NORTH
																	&& tileSouth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_W_N))))
//															&& tileSouth->getUnit() == NULL)) // unless unit is short and lets sight pass overtop. DOES NOT BLOCK!
													{
														//Log(LOG_INFO) << "[1] . . . . tileSouth No northwall OR object";
														const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

														if (tileSouthWest == NULL
															|| (((unitWest->getDirection() == 1
																		&& (unitWest->getPosition() != unitWest->getDestination()
																			|| tileSouthWest->getUnit() == NULL))
																	|| (unitWest->getDirection() == 5
																		&& (unitWest->getPosition() == unitWest->getDestination()
																			|| tileSouthWest->getUnit() == NULL)))
																&& (tileSouthWest->getMapData(O_NORTHWALL) == NULL
																	|| tileSouthWest->isUfoDoorOpen(O_NORTHWALL) == true
																	|| tileSouth->getMapData(O_NORTHWALL) == NULL) // <- getting very redundant; soon needs break-out functions like checkNorthSideBlockage() & checkWestSideBlockage() etc etc etc etc, also checkObjectSightBlockageLeft/Right() etc.
																&& (tileSouthWest->getMapData(O_OBJECT) == NULL
																	|| (tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
//																		&& tileSouthWest->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) != 255
																		&& tileSouthWest->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) < 6 // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
																		// that needs to change. Use, say, a TU cost in MCDs of 6+ to denote big bushy objects that act like chairs & other non-walkable objects.
																		&& tileSouthWest->getMapData(O_OBJECT)->getDataset()->getName() != "LIGHTNIN"
																		&& tileSouthWest->getMapData(O_OBJECT)->getSprite(0) != 42)
																	|| tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NWSE
																	|| tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST)))
														{
															//Log(LOG_INFO) << "[1] . . . . . tileSouthWest No northwall OR object";
															//Log(LOG_INFO) << "[1] . . . . . REDRAW";
															redraw = true;

															const Tile* const tileSouthSouthWest = _battleSave->getTile(posMap + Position(-1,2,0));

															if ((tileSouthWest != NULL
																	&& (tileSouthWest->getMapData(O_OBJECT) != NULL
																	&& ((tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
																			&& tileSouthWest->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255) // or terrainLevel < 0 perhaps // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
																		|| tileSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NWSE)))
																|| (tileSouthSouthWest != NULL
																	&& tileSouthSouthWest->getMapData(O_NORTHWALL) != NULL)) // <- needs more ...
															{
																//Log(LOG_INFO) << "[1] . . . . . . tileSouthWest object OR tileSouthSouthWest northwall";
																if (tileSouthSouthWest != NULL
																	&& tileSouthSouthWest->getMapData(O_NORTHWALL) != NULL // confusing .... re. needs more above^ (this seems to be for large units only)
																	&& unitWest->getArmor()->getSize() == 2)
																{
																	//Log(LOG_INFO) << "[1] . . . . . . . large unit, Cancel redraw [1]";
																	redraw = false;
																}
																else
																{
																	//Log(LOG_INFO) << "[1] . . . . . . . halfRight TRUE [1]";
																	halfRight = true;
																}
															}
															else
															{
																//Log(LOG_INFO) << "[1] . . . . . . tileSouthWest No object AND tileSouthSouthWest No northwall";
																if (tileSouthSouthWest != NULL
																	&& (tileSouthSouthWest->getMapData(O_NORTHWALL) != NULL
																		|| (tileSouthSouthWest->getMapData(O_OBJECT) != NULL
																			&& tileSouthSouthWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW)))
																{
																	//Log(LOG_INFO) << "[1] . . . . . . . tileSouthSouthWest northwall OR diagBigWall, Cancel redraw [2]";
																	redraw = false;
																}
																else if (tileSouthWest->getMapData(O_NORTHWALL) == NULL)
																{
																	//Log(LOG_INFO) << "[1] . . . . . . . halfRight FALSE";
																	halfRight = false;
																}
																else
																{
																	//Log(LOG_INFO) << "[1] . . . . . . . halfRight TRUE [2]";
																	halfRight = true;
																}
															}
														}
													}
												}
											}
										}
									}

									if (redraw == true)
									{
										//Log(LOG_INFO) << "[1] Redraw unitWest";
										quad = tileWest->getPosition().x - unitWest->getPosition().x
											+ (tileWest->getPosition().y - unitWest->getPosition().y) * 2;

										srfSprite = unitWest->getCache(&invalid, quad);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [50]";
											if (tileWest->isDiscovered(2) == true)
												shade = tileWest->getShade();
											else
												shade = 16;

											calculateWalkingOffset(
																unitWest,
																&walkOffset);

											srfSprite->blitNShade(
													surface,
													posScreen.x - 16 + walkOffset.x,
													posScreen.y - 8 + walkOffset.y,
													shade,
													halfRight);

											if (unitWest->getFireOnUnit() != 0)
											{
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												if (srfSprite)
													srfSprite->blitNShade(
															surface,
															posScreen.x - 16 + walkOffset.x,
															posScreen.y - 8 + walkOffset.y,
															0);
											}
										}
									}
								}
							}
							// end draw unitWest walking


							// TODO: Smoke & Fire render below this, use checks to draw or not.

							// reDraw unit NORTH-WEST. ... unit Walking (dir = 3,7 OR 2,6).
							// note: tileWest & unitWest are in scope here; tileWest is [should be] well-defined, unitWest may be NULL ...
							if (itY > 0
								&& unitWest == NULL)
							{
								const Tile* const tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));

								if (tileNorth != NULL)
								{
//									const Tile* tileEast;
//									if (itY < endY)
//										tileEast = _battleSave->getTile(posMap + Position(1,0,0));
//									else
//										tileEast = NULL;

//									if (tileNorth->getUnit() == NULL	// TODO: looks like these 'unit' checks should be done against LoFTs!!
//										&& (tileEast == NULL			// note, started doing that below_
//											|| tileEast->getUnit() == NULL))
									{
										const Tile* const tileNorthWest = _battleSave->getTile(posMap + Position(-1,-1,0));

										if (tileNorthWest != NULL)
										{
											BattleUnit* const unitNorthWest = tileNorthWest->getUnit();

											if (unitNorthWest != NULL
												&& (unitNorthWest->getUnitVisible() == true
													|| _battleSave->getDebugMode() == true)
												&& unitNorthWest != unit // large units don't need to redraw their western parts
												&& (unitNorthWest->getStatus() == STATUS_WALKING
													|| unitNorthWest->getStatus() == STATUS_FLYING)
												&& (((unitNorthWest->getDirection() == 3
															|| unitNorthWest->getDirection() == 7)
														&& (tileNorthWest->getMapData(O_OBJECT) == NULL
															|| (tileNorthWest->getMapData(O_OBJECT) != NULL
																&& tileNorthWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_EAST
																&& tileNorthWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_SOUTH
																&& tileNorthWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_E_S)))
													|| ((unitNorthWest->getDirection() == 2			// This bit is for going down (or up) slopes and not getting unit's toes truncated.
															|| unitNorthWest->getDirection() == 6)	// EW only applicable if curTile foreground has bigWall_NONE and is higher:
														&& _tile->getMapData(O_OBJECT) != NULL
														&& _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
														&& _tile->getTerrainLevel() < tileNorthWest->getTerrainLevel())))
											{
												//Log(LOG_INFO) << "[2] unitNorthWest walking/flying dir 3/7 OR 2/6";
												if (_tile->getMapData(O_OBJECT) == NULL // exposed floor, basically.
													|| (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
														&& std::abs(tileNorthWest->getTerrainLevel() - _tile->getTerrainLevel()) < 13 // positive means Tile is higher
														&& _tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) != 255) // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
													|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST
													|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S)
												{
													//Log(LOG_INFO) << "[2] . tile No object";
													// if tileWest is blocking sight, draw unit halfRight
													// if tileNorth is blocking sight, draw unit halfLeft
													// else draw Full
													halfRight = false;
													bool halfLeft = false;

													if ((_tile->getMapData(O_WESTWALL) != NULL			// AND tu == 255, ie. isWalkable rubble that lets sight pass over it
															&& _tile->isUfoDoorOpen(O_WESTWALL) == false)
														|| (tileWest->getMapData(O_NORTHWALL) != NULL	// AND tu == 255, ie. isWalkable rubble that lets sight pass over it
															&& tileWest->isUfoDoorOpen(O_NORTHWALL) == false)
														|| (tileWest->getMapData(O_OBJECT) != NULL
															&& ((tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
																	&& tileWest->getTerrainLevel() - _tile->getTerrainLevel() > 12) // positive means Tile is higher
																|| tileWest->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255 // tentative: good for some objects, prob. not for others // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
																|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
																|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW
																|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NORTH
																|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST
																|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S
																|| tileWest->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N)
															&& tileWest->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NWSE)
														|| (tileWest->getUnit() != NULL
															&& tileWest->getUnit()->getLoft(0) == 5)) // big round thing, eg Silacoid
													{
														//Log(LOG_INFO) << "[2] . . tileWest object, halfRight TRUE";
														halfRight = true;
													}

													if ((_tile->getMapData(O_NORTHWALL) != NULL			// AND tu == 255, ie. isWalkable rubble that lets sight pass over it
															&& _tile->isUfoDoorOpen(O_NORTHWALL) == false)
														|| (tileNorth->getMapData(O_WESTWALL) != NULL	// AND tu == 255, ie. isWalkable rubble that lets sight pass over it
															&& ((tileNorth->getMapData(O_WESTWALL)->isUfoDoor() == true
																	&& tileNorth->isUfoDoorOpen(O_WESTWALL) == false)
																|| tileNorth->getMapData(O_WESTWALL)->getTuCostPart(MT_WALK) == 255)) // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
														|| (tileNorth->getMapData(O_OBJECT) != NULL
															&& ((tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
																	&& tileNorth->getTerrainLevel() - _tile->getTerrainLevel() > 12) // positive means Tile is higher
																|| tileNorth->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255 // tentative: good for some objects, prob. not for others // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
																|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
																|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW
																|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST
																|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH
																|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S
																|| tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N)
															&& tileNorth->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NWSE)
														|| (tileNorth->getUnit() != NULL
															&& tileNorth->getUnit()->getLoft(0) == 5)) // big round thing, eg Silacoid
													{
														//Log(LOG_INFO) << "[2] . . tileNorth object, halfLeft TRUE";
														halfLeft = true;
													}

													if (!
														(halfRight == true && halfLeft == true)
														&& (tileWest->getMapData(O_OBJECT) == NULL // ... (if tileWest!=NULL)
															|| (tileWest->getMapData(O_OBJECT)->getDataset()->getName() != "LIGHTNIN"
																&& tileWest->getMapData(O_OBJECT)->getSprite(0) != 42)))
													{
														//Log(LOG_INFO) << "[2] . . REDRAW";
														quad = tileNorthWest->getPosition().x - unitNorthWest->getPosition().x
															+ (tileNorthWest->getPosition().y - unitNorthWest->getPosition().y) * 2;

														srfSprite = unitNorthWest->getCache(&invalid, quad);
//														srfSprite = NULL;
														if (srfSprite)
														{
															if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [60]";
															if (tileNorthWest->isDiscovered(2) == true)
																shade = tileNorthWest->getShade();
															else
																shade = 16;

															calculateWalkingOffset(
																				unitNorthWest,
																				&walkOffset);

															srfSprite->blitNShade(
																				surface,
																				posScreen.x + walkOffset.x,
																				posScreen.y - 16 + walkOffset.y,
																				shade,
																				halfRight,
																				0,
																				halfLeft);

															if (unitNorthWest->getFireOnUnit() != 0)
															{
																frame = 4 + (_animFrame / 2);
																srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
																srfSprite->blitNShade(
																					surface,
																					posScreen.x + walkOffset.x,
																					posScreen.y - 16 + walkOffset.y,
																					0,
																					halfRight,
																					0,
																					halfLeft);
															}

															// Redraw top of Craft's ramp if walking NWSE beneath it.
															// ( if (tileNorth has west or southwall) redraw it, perhaps )
															if (tileNorth->getMapData(O_OBJECT) != NULL
																&& tileNorth->getMapData(O_OBJECT)->getBigWall() == 0 // draw content-object but not bigWall
																&& ((tileNorth->getMapData(O_OBJECT)->getDataset()->getName() == "LIGHTNIN"
																		&& tileNorth->getMapData(O_OBJECT)->getSprite(0) == 42)
																	|| (tileNorth->getMapData(O_OBJECT)->getDataset()->getName() == "PLANE"
																		&& tileNorth->getMapData(O_OBJECT)->getSprite(0) == 62)
																	|| (tileNorth->getMapData(O_OBJECT)->getDataset()->getName() == "AVENGER"
																		&& tileNorth->getMapData(O_OBJECT)->getSprite(0) == 61)))
															{
																srfSprite = tileNorth->getSprite(O_OBJECT);
//																srfSprite = NULL;
																if (srfSprite)
																{
																	if (tileNorth->isDiscovered(2) == true)
																		shade = tileNorth->getShade();
																	else
																		shade = 16;

																	srfSprite->blitNShade(
																			surface,
																			posScreen.x + 16,
																			posScreen.y - 8 - tileNorth->getMapData(O_OBJECT)->getYOffset(),
																			shade);
																}
															}

															// Redraw BigWallNONE content object w/ TU=255 in tiles South & East when dir=3,7
															// ( as long as they aren't butted against a further southern or eastern wall/object/unit ......
															// .. so forget it. )
														}
													}
												}
											}
										}
									}
								}
							}
							// end draw unitNorthWest walking.
						}
						// end draw westerly units moving east-west.
*/

						// kL_begin #2 of 3: Make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						// note: Or a westwall (ie, bulging UFO hull) in tile above & south of the soldier <- not done
						if (itZ > 0
							&& hasFloor == false
							&& hasObject == false)
						{
//							const Tile* const tileBelow = _battleSave->getTile(posMap + Position(0,0,-1));

							if (tileBelow != NULL)
							{
								BattleUnit* const unitBelow = tileBelow->getUnit();

								if (unitBelow != NULL
									&& unitBelow != _battleSave->getSelectedUnit()
									&& unitBelow->getGeoscapeSoldier() != NULL
									&& unitBelow->getFaction() == unitBelow->getOriginalFaction())
								{
									if (unitBelow->getFatalWounds() > 0)
									{
										srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 2,
													posScreen.y + 3 + 24 + getTerrainLevel(
																							unitBelow->getPosition(),
																							unitBelow->getArmor()->getSize()),
													0);

										srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 4,
													posScreen.y + 4 + 24 + getTerrainLevel(
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
//										srfSprite = NULL;
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 2,
													posScreen.y + 3 + 24 + getTerrainLevel(
																							unitBelow->getPosition(),
																							unitBelow->getArmor()->getSize()),
													0);

/*										const int strength = static_cast<int>(Round(
															 static_cast<double>(unitBelow->getBaseStats()->strength) * (unitBelow->getAccuracyModifier() / 2. + 0.5)));
										if (unitBelow->getCarriedWeight() > strength)
										{
											srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(96); // dot
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 9,
													posScreen.y + walkOffset.y + 4 + 24,
													1, // 10
													false,
													1); // 1=white, 2=yellow-red, 3=red, 6=lt.brown, 9=blue
										} */
									}
								}
							}
						}
					} // Void <- end.


// Draw Bullet if in Field Of View
					if (_showProjectile == true // <- used to hide Celatid glob while its spitting animation plays.
						&& _projectile != NULL)
//						&& _projectileInFOV)
					{
						if (_projectile->getItem() != NULL) // thrown item ( grenade, etc.)
						{
							srfSprite = _projectile->getSprite();
							if (srfSprite)
							{
								Position voxelPos = _projectile->getPosition(); // draw shadow on the floor
								voxelPos.z = _battleSave->getTileEngine()->castedShade(voxelPos);
								if (   voxelPos.x / 16 >= itX
									&& voxelPos.y / 16 >= itY
									&& voxelPos.x / 16 <= itX + 1
									&& voxelPos.y / 16 <= itY + 1
									&& voxelPos.z / 24 == itZ)
//									&& _battleSave->getTileEngine()->isVoxelVisible(voxelPos))
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
//									&& _battleSave->getTileEngine()->isVoxelVisible(voxelPos))
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
						else // fired projectile (a bullet-sprite, not a thrown item)
						{
							if (   itX >= bulletLowX // draw bullet on the correct tile
								&& itX <= bulletHighX
								&& itY >= bulletLowY
								&& itY <= bulletHighY)
							{
/*								int
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

								Position posVoxel;
								for (int
										i = bulletSprite_start;
										i != bulletSprite_end;
										i += direction) */
								Position posVoxel;
								for (int
										i = 0;
										i != BULLET_SPRITES;
										++i)
								{
									srfSprite = _projectileSet->getFrame(_projectile->getParticle(i));
									if (srfSprite)
									{
										posVoxel = _projectile->getPosition(1 - i); // draw shadow on the floor
										posVoxel.z = _battleSave->getTileEngine()->castedShade(posVoxel);
										if (   posVoxel.x / 16 == itX
											&& posVoxel.y / 16 == itY
											&& posVoxel.z / 24 == itZ)
//											&& _battleSave->getTileEngine()->isVoxelVisible(posVoxel))
										{
											_camera->convertVoxelToScreen(
																		posVoxel,
																		&bullet);

											bullet.x -= srfSprite->getWidth() / 2;
											bullet.y -= srfSprite->getHeight() / 2;
											srfSprite->blitNShade(
													surface,
													bullet.x,
													bullet.y,
													16);
										}

										posVoxel = _projectile->getPosition(1 - i); // draw bullet itself
										if (   posVoxel.x / 16 == itX
											&& posVoxel.y / 16 == itY
											&& posVoxel.z / 24 == itZ)
//											&& _battleSave->getTileEngine()->isVoxelVisible(posVoxel))
										{
											_camera->convertVoxelToScreen(
																		posVoxel,
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
					// end draw bullet.


// Main Draw BattleUnit ->
					if (unit != NULL
						&& (unit->getUnitVisible() == true
							|| _battleSave->getDebugMode() == true))
					{
						quad = _tile->getPosition().x - unit->getPosition().x
							+ (_tile->getPosition().y - unit->getPosition().y) * 2;

						srfSprite = unit->getCache(&invalid, quad);
//						srfSprite = NULL;
						if (srfSprite)
						{
							//if (kL_Debug_main) Log(LOG_INFO) << ". drawUnit [70]";
							//if (unit->getId() == 1000007) Log(LOG_INFO) << "MAP DRAW";
							if (unit->getHealth() == 0
								|| unit->getHealth() <= unit->getStun()) // -> && unit is Player
							{
								shade = std::min(
												5,
												tileShade);
							}
							else
								shade = tileShade;

							if (shade != 0 // try to even out lighting of all four quadrants of large units.
								&& quad != 0)
							{
								shade -= 1; // TODO: trickle this throughout this function!
							}

							calculateWalkingOffset(
												unit,
												&walkOffset);

							srfSprite->blitNShade(
									surface,
									posScreen.x + walkOffset.x,
									posScreen.y + walkOffset.y,
									shade);

							if (unit->getFireOnUnit() != 0)
							{
								frame = 4 + (_animFrame / 2);
								srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
								if (srfSprite)
									srfSprite->blitNShade(
											surface,
											posScreen.x + walkOffset.x,
											posScreen.y + walkOffset.y,
											0);
							}
/*
							// redraw all quadrants of large units moving up/down on gravLift
							// Should this be STATUS_WALKING (up/down gravLift); uses vertDir instead
							if (itZ > 0
								&& quad == 3
								&& unit->getVerticalDirection() != 0
								&& _tile->getMapData(O_FLOOR) != NULL
								&& _tile->getMapData(O_FLOOR)->isGravLift() == true)
							{
								const Tile* const tileNorthWest = _battleSave->getTile(posMap + Position(-1,-1,0));

								BattleUnit* const unitNorthWest = tileNorthWest->getUnit();

								if (unitNorthWest == unit) // large unit going up/down gravLift
								{
									quad = tileNorthWest->getPosition().x - unitNorthWest->getPosition().x
										+ (tileNorthWest->getPosition().y - unitNorthWest->getPosition().y) * 2;

									srfSprite = unitNorthWest->getCache(&invalid, quad);
//									srfSprite = NULL;
									if (srfSprite)
									{
										if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [80]";
										calculateWalkingOffset(
															unitNorthWest,
															&walkOffset);
										walkOffset.y -= 16;

										srfSprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												tileNorthWest->getShade());

										if (unitNorthWest->getFireOnUnit() != 0)
										{
											frame = 4 + (_animFrame / 2);
											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														0);
										}
									}
								}


								const Tile* const tileWest = _battleSave->getTile(posMap + Position(-1,0,0));

								BattleUnit* const unitWest = tileWest->getUnit();

								if (unitWest == unit) // large unit going up/down gravLift
								{
									quad = tileWest->getPosition().x - unitWest->getPosition().x
										+ (tileWest->getPosition().y - unitWest->getPosition().y) * 2;

									srfSprite = unitWest->getCache(&invalid, quad);
//									srfSprite = NULL;
									if (srfSprite)
									{
										if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [90]";
										calculateWalkingOffset(
															unitWest,
															&walkOffset);
										walkOffset.x -= 16;
										walkOffset.y -= 8;

										srfSprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												tileWest->getShade());

										if (unitWest->getFireOnUnit() != 0)
										{
											frame = 4 + (_animFrame / 2);
											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														0);
										}
									}
								}


								const Tile* const tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));

								BattleUnit* const unitNorth = tileNorth->getUnit();

								if (unitNorth == unit) // large unit going up/down gravLift
								{
									quad = tileNorth->getPosition().x - unitNorth->getPosition().x
										+ (tileNorth->getPosition().y - unitNorth->getPosition().y) * 2;

									srfSprite = unitNorth->getCache(&invalid, quad);
//									srfSprite = NULL;
									if (srfSprite)
									{
										if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [100]";
										calculateWalkingOffset(
															unitNorth,
															&walkOffset);
										walkOffset.x += 16;
										walkOffset.y -= 8;

										srfSprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												tileNorth->getShade());

										if (unitNorth->getFireOnUnit() != 0)
										{
											frame = 4 + (_animFrame / 2);
											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														0);
										}
									}
								}


								if (itY < endY) // redraw floor, was overwritten by SW quadrant of large unit moving up/down gravLift
								{
									const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

									if (tileSouthWest != NULL)
									{
										srfSprite = tileSouthWest->getSprite(O_FLOOR);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileSouthWest->isDiscovered(2) == true)
												shade = tileSouthWest->getShade(); // Or use tileShade
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													posScreen.x - 32,
													posScreen.y - tileSouthWest->getMapData(O_FLOOR)->getYOffset(),
													shade);
										}
									}
								}
							}
*/

/*							// TEST. reDraw tileSouthWest bigWall NESW
							if (itX > 0 && itY < endY)
							{
								const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

								srfSprite = tileSouthWest->getSprite(O_OBJECT);
//								srfSprite = NULL;
								if (srfSprite)
								{
									if (tileSouthWest->isDiscovered(2) == true)
										shade = tileSouthWest->getShade(); // Or use tileShade
									else
										shade = 16;

									srfSprite->blitNShade(
											surface,
											posScreen.x - 32,
											posScreen.y - tileSouthWest->getMapData(O_OBJECT)->getYOffset(),
											shade);
								}
							} */

/*
							// Redraw northWall in tileSouthSouthWest
							if (itX > 0 && itY < endY - 1				// Reaper sticks its righthind leg out through southerly wall
								&& unit->getStatus() == STATUS_WALKING	// Sectopod also, likely.
								&& unit->getDirection() == 1
								&& unit->getArmor()->getSize() == 2
								&& unit->getTurretType() == -1)
							{
								if (unit == _battleSave->getTile(posMap + Position(-1,0,0))->getUnit())
								{
									const Tile* const tileSouthSouthWest = _battleSave->getTile(posMap + Position(-1,2,0));

									if (tileSouthSouthWest != NULL)
									{
										srfSprite = tileSouthSouthWest->getSprite(O_NORTHWALL);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileSouthSouthWest->isDiscovered(1) == true)
												shade = tileSouthSouthWest->getShade(); // Or use tileShade
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													posScreen.x - 48,
													posScreen.y + 8 - tileSouthSouthWest->getMapData(O_NORTHWALL)->getYOffset(),
													shade,
													true);
										}
									}
								}
							}
*/
							// kL_begin #3 of 3:
							const Tile* const tileAbove = _battleSave->getTile(posMap + Position(0,0,1));

							if ((_camera->getViewLevel() == itZ
									&& (_camera->getShowAllLayers() == false
										|| itZ == endZ))
								|| (tileAbove != NULL
									&& tileAbove->getSprite(O_FLOOR) == NULL))
							{
								if (unit != _battleSave->getSelectedUnit()
									&& unit->getGeoscapeSoldier() != NULL
									&& unit->getFaction() == unit->getOriginalFaction())
								{
									if (unit->getFatalWounds() > 0)
									{
										srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 2,
													posScreen.y + walkOffset.y + 3,
													0);

										srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross;
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 4,
													posScreen.y + walkOffset.y + 4,
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
													posScreen.x + walkOffset.x + 7,
													posScreen.y + walkOffset.y + 4,
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
//										srfSprite = NULL;
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 2,
													posScreen.y + walkOffset.y + 3,
													0);

/*										const int strength = static_cast<int>(Round(
															 static_cast<double>(unit->getBaseStats()->strength) * (unit->getAccuracyModifier() / 2. + 0.5)));
										if (unit->getCarriedWeight() > strength)
										{
											srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(96); // dot
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 9,
													posScreen.y + walkOffset.y + 4,
													1, // 10
													false,
													1); // 1=white, 2=yellow-red, 3=red, 6=lt.brown, 9=blue
										} */
									}
								}

								// Draw Exposed mark
								if (unit->getFaction() == FACTION_PLAYER
									&& unit->getFaction() == unit->getOriginalFaction()
									&& (unit->getArmor()->getSize() == 1
										|| quad == 1))
								{
									const int exposure = unit->getExposed();
									if (exposure != -1)
									{
										_numExposed->setValue(static_cast<unsigned int>(exposure));
										_numExposed->draw();
										_numExposed->blitNShade(
															surface,
															posScreen.x + walkOffset.x + 21,
															posScreen.y + walkOffset.y + 5,
															0,
															false,
															12); // pinkish
									}
								}
							} // kL_end.

/*
							// Feet getting chopped by tilesBelow when moving vertically; redraw lowerForeground walls.
							// note: This is a quickfix - feet can also get chopped by walls in tileBelowSouth & tileBelowEast ...
//							if (false)
							if (unit->getVerticalDirection() != 0
								&& itX < endX && itY < endY && itZ > 0)
							{
								const Tile* const tileBelowSouthEast = _battleSave->getTile(posMap + Position(1,1,-1));

								if (tileBelowSouthEast != NULL)
								{
									srfSprite = tileBelowSouthEast->getSprite(O_NORTHWALL);
//									srfSprite = NULL; // test
									if (srfSprite)
									{
										if (tileBelowSouthEast->isDiscovered(2) == true)
											shade = tileBelowSouthEast->getShade();
										else
											shade = 16;

										srfSprite->blitNShade(
												surface,
												posScreen.x,
												posScreen.y + 40 - tileBelowSouthEast->getMapData(O_NORTHWALL)->getYOffset(),
												shade);

										if (itX < endX - 1)
										{
											const Tile* const tileBelowSouthEastEast = _battleSave->getTile(posMap + Position(2,1,-1));

											if (tileBelowSouthEastEast != NULL)
											{
												srfSprite = tileBelowSouthEastEast->getSprite(O_NORTHWALL);
												if (srfSprite)
												{
													if (tileBelowSouthEastEast->isDiscovered(2) == true)
														shade = tileBelowSouthEastEast->getShade();
													else
														shade = 16;

													srfSprite->blitNShade(
															surface,
															posScreen.x + 16,
															posScreen.y + 48 - tileBelowSouthEastEast->getMapData(O_NORTHWALL)->getYOffset(),
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
*/
						}
					}
					// end Main Draw BattleUnit.

/*
// reDraw unitBelow moving up/down on a gravLift.
					// Should this be STATUS_WALKING (up/down gravLift #2); uses verDir instead
					if (itZ > 0
						&& _tile->getMapData(O_FLOOR) != NULL
						&& _tile->getMapData(O_FLOOR)->isGravLift() == true
						&& tileBelow != NULL)
					{
						BattleUnit* const unitBelow = tileBelow->getUnit();

						if (unitBelow != NULL
							&& unitBelow->getVerticalDirection() != 0
							&& (unitBelow->getUnitVisible() == true
								|| _battleSave->getDebugMode() == true))
						{
							quad = tileBelow->getPosition().x - unitBelow->getPosition().x
								+ (tileBelow->getPosition().y - unitBelow->getPosition().y) * 2;

							if (quad == 3
								|| unitBelow->getArmor()->getSize() == 1)
							{
								srfSprite = unitBelow->getCache(&invalid, quad);
//								srfSprite = NULL;
								if (srfSprite)
								{
									if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [110]";
									calculateWalkingOffset(
														unitBelow,
														&walkOffset);
									walkOffset.y += 24;

									srfSprite->blitNShade(
											surface,
											posScreen.x + walkOffset.x,
											posScreen.y + walkOffset.y,
											tileBelow->getShade());

									if (unitBelow->getFireOnUnit() != 0)
									{
										frame = 4 + (_animFrame / 2);
										srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x,
													posScreen.y + walkOffset.y,
													0);
									}

									if (quad == 3) //itY > 0
									{
										const Tile* const tileBelowNorthWest = _battleSave->getTile(posMap + Position(-1,-1,-1));

										BattleUnit* const unitBelowNorthWest = tileBelowNorthWest->getUnit();

										if (unitBelowNorthWest == unitBelow) // large unit going up/down gravLift
										{
											quad = tileBelowNorthWest->getPosition().x - unitBelowNorthWest->getPosition().x
												+ (tileBelowNorthWest->getPosition().y - unitBelowNorthWest->getPosition().y) * 2;

											srfSprite = unitBelowNorthWest->getCache(&invalid, quad);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [120]";
												calculateWalkingOffset(
																	unitBelowNorthWest,
																	&walkOffset);
												walkOffset.y += 8;

												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														tileBelowNorthWest->getShade());

												if (unitBelowNorthWest->getFireOnUnit() != 0)
												{
													frame = 4 + (_animFrame / 2);
													srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
													if (srfSprite)
														srfSprite->blitNShade(
																surface,
																posScreen.x + walkOffset.x,
																posScreen.y + walkOffset.y,
																0);
												}
											}
										}


										const Tile* const tileBelowWest = _battleSave->getTile(posMap + Position(-1,0,-1));

										BattleUnit* const unitBelowWest = tileBelowWest->getUnit();

										if (unitBelowWest == unitBelow) // large unit going up/down gravLift
										{
											quad = tileBelowWest->getPosition().x - unitBelowWest->getPosition().x
												+ (tileBelowWest->getPosition().y - unitBelowWest->getPosition().y) * 2;

											srfSprite = unitBelowWest->getCache(&invalid, quad);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [130]";
												calculateWalkingOffset(
																	unitBelowWest,
																	&walkOffset);
												walkOffset.x -= 16;
												walkOffset.y += 16;

												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														tileBelowWest->getShade());

												if (unitBelowWest->getFireOnUnit() != 0)
												{
													frame = 4 + (_animFrame / 2);
													srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
													if (srfSprite)
														srfSprite->blitNShade(
																surface,
																posScreen.x + walkOffset.x,
																posScreen.y + walkOffset.y,
																0);
												}
											}
										}


										const Tile* const tileBelowNorth = _battleSave->getTile(posMap + Position(0,-1,-1));

										BattleUnit* const unitBelowNorth = tileBelowNorth->getUnit();

										if (unitBelowNorth == unitBelow) // large unit going up/down gravLift
										{
											quad = tileBelowNorth->getPosition().x - unitBelowNorth->getPosition().x
												+ (tileBelowNorth->getPosition().y - unitBelowNorth->getPosition().y) * 2;

											srfSprite = unitBelowNorth->getCache(&invalid, quad);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [140]";
												calculateWalkingOffset(
																	unitBelowNorth,
																	&walkOffset);
												walkOffset.x += 16;
												walkOffset.y += 16;

												srfSprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x,
														posScreen.y + walkOffset.y,
														tileBelowNorth->getShade());

												if (unitBelowNorth->getFireOnUnit() != 0)
												{
													frame = 4 + (_animFrame / 2);
													srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
													if (srfSprite)
														srfSprite->blitNShade(
																surface,
																posScreen.x + walkOffset.x,
																posScreen.y + walkOffset.y,
																0);
												}
											}
										}


										if (itY < endY) // redraw floor, was overwritten by SW quadrant of large unit moving up/down gravLift
										{
											const Tile* const tileSouthWest = _battleSave->getTile(posMap + Position(-1,1,0));

											if (tileSouthWest != NULL)
											{
												srfSprite = tileSouthWest->getSprite(O_FLOOR);
//												srfSprite = NULL;
												if (srfSprite)
												{
													if (tileSouthWest->isDiscovered(2) == true)
														shade = tileSouthWest->getShade(); // Or use tileShade
													else
														shade = 16;

													srfSprite->blitNShade(
															surface,
															posScreen.x - 32,
															posScreen.y - tileSouthWest->getMapData(O_FLOOR)->getYOffset(),
															shade);
												}
											}
										}
									}
								}
							}
						}
					}
					// end gravLift.
*/

// Draw Unconscious Soldier icon
					// - might want to redundant this, like rankIcons.
					if (unit == NULL)
					{
						const int status = _tile->hasUnconsciousUnit();
						if (status != 0)
						{
							srfSprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
							if (srfSprite != NULL)
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y,
										0);

							Uint8 color;
							if (status == 2)
								color = 3; // red, wounded unconscious soldier
							else
								color = 1; // white, unconscious soldier here

							srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
							if (srfSprite)
								srfSprite->blitNShade(
										surface,
										posScreen.x + 2,
										posScreen.y + 1,
										_animFrame * 2,
										false,
										color);
						}
					}
					// end unconscious soldier icon.

/*
// Draw unitBelow if it is on raised ground & there is no Floor.
					if (itZ > 0
						&& tileBelow->getTerrainLevel() < -11
						&& _tile->hasNoFloor(tileBelow) == true)
					{
						BattleUnit* const unitBelow = tileBelow->getUnit();

						if (unitBelow != NULL
							&& (unitBelow->getUnitVisible() == true
								|| _battleSave->getDebugMode() == true))
						{
							const Tile
								* tileNorth = NULL,
								* tileNorthWest = NULL,
								* tileWest = NULL,
								* tileNorthNorth = NULL,
								* tileNorthNorthWest = NULL,
								* tileNorthNorthWestWest = NULL,
								* tileWestWest = NULL;

							const int unitSize = unitBelow->getArmor()->getSize();

							if (itY > 0)
							{
								tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));
								if (itY > 1
									&& (unitSize > 1
										|| (unitBelow->getStatus() == STATUS_WALKING
											&& unitBelow->getDirection() != 2
											&& unitBelow->getDirection() != 6)))
								{
									tileNorthNorth = _battleSave->getTile(posMap + Position(0,-2,0));
								}
							}

							if (itX > 0 && itY > 0)
							{
								tileNorthWest = _battleSave->getTile(posMap + Position(-1,-1,0));
								if (itY > 1
									&& (unitSize > 1
										|| (unitBelow->getStatus() == STATUS_WALKING
											&& unitBelow->getDirection() != 1
											&& unitBelow->getDirection() != 5)))
								{
									tileNorthNorthWest = _battleSave->getTile(posMap + Position(-1,-2,0));
									if (itX > 1)
										tileNorthNorthWestWest = _battleSave->getTile(posMap + Position(-2,-2,0));
								}
							}

							if (itX > 0)
							{
								tileWest = _battleSave->getTile(posMap + Position(-1,0,0));
								if (itX > 1
									&& (unitSize > 1
										|| (unitBelow->getStatus() == STATUS_WALKING
											&& unitBelow->getDirection() != 0
											&& unitBelow->getDirection() != 4)))
								{
									tileWestWest = _battleSave->getTile(posMap + Position(-2,0,0));
								}
							}

							if (_tile->getMapData(O_WESTWALL) != NULL
								|| _tile->getMapData(O_NORTHWALL) != NULL
								|| (tileNorth != NULL
									&& (tileNorth->getMapData(O_FLOOR) != NULL
										|| (tileNorth->getMapData(O_OBJECT) != NULL
											&& tileNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK)))
								|| (tileNorthNorth != NULL
									&& (tileNorthNorth->getMapData(O_FLOOR) != NULL
										|| (tileNorthNorth->getMapData(O_OBJECT) != NULL
											&& tileNorthNorth->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK)))
								|| (tileNorthWest != NULL
									&& tileNorthWest->isVoid(true, false) == false)				// flooring mainly, but prob. also south & east walls + content-object
								|| (tileNorthNorthWest != NULL
									&& tileNorthNorthWest->isVoid(true, false) == false)		// these can be limited to Floor + foreground & content Objects.
								|| (tileNorthNorthWestWest != NULL
									&& tileNorthNorthWestWest->isVoid(true, false) == false)	// these can be limited to Floor + foreground & content Objects.
								|| (tileWest != NULL
									&& tileWest->isVoid(true, false) == false)					// flooring mainly, but prob. also north & east walls + content-object
								|| (tileWestWest != NULL
									&& tileWestWest->isVoid(true, false) == false))				// flooring mainly, but prob. also north & east walls + content-object
							// that also needs tileWest's northWall & bigWall east
							// and tileNorth's westWall & bigWall south
							// and tileBelow's east and south walls
							// and tileBelowEast's west & south walls ..... fuck 3d isometric. &Tc.
							// Screw it; i'm changing the map resource!!! ( DESERTMOUNT08, eg. )
							// ... damn, now i'm actually implementing it anyway .....
							{
								quad = tileBelow->getPosition().x - unitBelow->getPosition().x
									+ (tileBelow->getPosition().y - unitBelow->getPosition().y) * 2;

								srfSprite = unitBelow->getCache(&invalid, quad);
//								srfSprite = NULL;
								if (srfSprite)
								{
									if (kL_Debug_stand) Log(LOG_INFO) << ". drawUnit [150]";
									if (kL_Debug_walk) Log(LOG_INFO) << ". drawUnit [155]";
									if (tileBelow->isDiscovered(2) == true)
										shade = tileBelow->getShade();
									else
										shade = 16;

									calculateWalkingOffset(
														unitBelow,
														&walkOffset);
									walkOffset.y += 24;

									srfSprite->blitNShade(
											surface,
											posScreen.x + walkOffset.x,
											posScreen.y + walkOffset.y,
											shade);

									if (unitSize > 1)
										walkOffset.y += 4;

									if (unitBelow->getFireOnUnit() != 0)
									{
										frame = 4 + (_animFrame / 2);
										srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x,
													posScreen.y + walkOffset.y,
													0);
									}

									// special case if tileSouthEast doesn't have a floor
									// and tileBelowSouthEast does have a sprite that should conceal the redrawn unit's right leg.
									if (itX < endX && itY < endY)
//									if (false)
									{
										const Tile* const tileSouthEast = _battleSave->getTile(posMap + Position(1,1,0));

										if (tileSouthEast != NULL
											&& tileSouthEast->getMapData(O_FLOOR) == NULL)
										{
											const Tile* const tileBelowSouthEast = _battleSave->getTile(posMap + Position(1,1,-1));

											// kL_note: I'm only doing content-object w/out bigWall here because that's the case I ran into.
											// This also needs to redraw northWall, as well as tileEast's west & south walls, tileBelow's east & south walls, etc.
											// ... aaaaaaand, this gives rise to infinite regression.
											// so try to head it off at the pass, above, by not redrawing unitBelow unless it *has* to be done.
											if (tileBelowSouthEast != NULL
												&& tileBelowSouthEast->getTerrainLevel() - tileBelow->getTerrainLevel() < 1 // positive -> Tile is higher
												&& tileBelowSouthEast->getMapData(O_OBJECT) != NULL
												&& tileBelowSouthEast->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
												&& tileBelowSouthEast->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255)	// generally only nonwalkable content-objects // -> _tile->getTuCostTile(O_OBJECT,MT_WALK)
											{																				// rise high enough to cause an overblit.
												srfSprite = tileBelowSouthEast->getSprite(O_OBJECT);
//												srfSprite = NULL;
												if (srfSprite)
												{
													if (tileBelowSouthEast->isDiscovered(2) == true)
														shade = tileBelowSouthEast->getShade();
													else
														shade = 16;

													srfSprite->blitNShade(
															surface,
															posScreen.x,
															posScreen.y + 40 - tileBelowSouthEast->getMapData(O_OBJECT)->getYOffset(),
															shade,
															true);

													if (itX < endX - 1)
													{
														const Tile* const tileBelowSouthEastEast = _battleSave->getTile(posMap + Position(2,1,-1));

														if (tileBelowSouthEastEast != NULL)
														{
															srfSprite = tileBelowSouthEastEast->getSprite(O_OBJECT);
//															srfSprite = NULL;
															if (srfSprite)
															{
																if (tileBelowSouthEastEast->isDiscovered(2) == true)
																	shade = tileBelowSouthEastEast->getShade();
																else
																	shade = 16;

																srfSprite->blitNShade(
																		surface,
																		posScreen.x + 16,
																		posScreen.y + 48 - tileBelowSouthEastEast->getMapData(O_OBJECT)->getYOffset(),
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
						}
					}
					// end draw unitBelow.
*/

// Draw SMOKE & FIRE
					if (_tile->isDiscovered(2) == true
						&& (_tile->getSmoke() != 0
							|| _tile->getFire() != 0))
					{
						if (_tile->getFire() == 0)
						{
							frame = ResourcePack::SMOKE_OFFSET;
							frame += (_tile->getSmoke() + 1) / 2;

							shade = tileShade;
						}
						else
						{
							frame =
							shade = 0;
						}

						animOffset = _animFrame / 2 + _tile->getAnimationOffset();
						if (animOffset > 3) animOffset -= 4;
						frame += animOffset;

						srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
						if (srfSprite)
							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y + _tile->getTerrainLevel(),
									shade);
					} // end Smoke & Fire

/*
// Redraw fire on south-west & north-east Tiles of big units
					if (itX > 0 && itY > 0
						&& unit != NULL)
					{
						const Tile* const tileWest = _battleSave->getTile(posMap + Position(-1,0,0));

						if (tileWest != NULL)
						{
							if (tileWest->getUnit() == unit)
							{
								const Tile* const tileNorth = _battleSave->getTile(posMap + Position(0,-1,0));

								if (tileNorth != NULL)
								{
									if (tileNorth->getUnit() == unit)
									{
										if (tileWest->isDiscovered(2) == true
											&& (tileWest->getSmoke() != 0
												|| tileWest->getFire() != 0))
										{
											if (tileWest->getFire() == 0)
											{
												frame = ResourcePack::SMOKE_OFFSET;

												frame += (tileWest->getSmoke() + 1) / 2;
												shade = tileWest->getShade();
											}
											else
											{
												frame =
												shade = 0;
											}

											animOffset = _animFrame / 2 + tileWest->getAnimationOffset();
											if (animOffset > 3) animOffset -= 4;
											frame += animOffset;

											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														posScreen.x - 16,
														posScreen.y - 8 + tileWest->getTerrainLevel(),
														shade);
										}

										if (tileNorth->isDiscovered(2) == true
											&& (tileNorth->getSmoke() != 0
												|| tileNorth->getFire() != 0))
										{
											if (tileNorth->getFire() == 0)
											{
												frame = ResourcePack::SMOKE_OFFSET;

												frame += (tileNorth->getSmoke() + 1) / 2;
												shade = tileNorth->getShade();
											}
											else
											{
												frame =
												shade = 0;
											}

											animOffset = _animFrame / 2 + tileNorth->getAnimationOffset();
											if (animOffset > 3) animOffset -= 4;
											frame += animOffset;

											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
											if (srfSprite)
												srfSprite->blitNShade(
														surface,
														posScreen.x + 16,
														posScreen.y - 8 + tileNorth->getTerrainLevel(),
														shade);
										}
									}
								}
							}
						}
					}
					// end Smoke & Fire
*/
// Draw pathPreview
					if (_tile->getPreviewDir() != -1
						&& (_previewSetting & PATH_ARROWS)
						&& _tile->isDiscovered(0) == true)
					{
						if (itZ > 0
							&& _tile->hasNoFloor(tileBelow) == true)
						{
							srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(11);
							if (srfSprite)
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + 2,
										0,
										false,
										_tile->getPreviewColor());
						}

						srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(_tile->getPreviewDir());
						if (srfSprite)
							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y,
									0,
									false,
									_tile->getPreviewColor());
					}


// Draw Front Object
					srfSprite = _tile->getSprite(O_OBJECT);
					if (srfSprite
						&& _tile->getMapData(O_OBJECT)->getBigWall() > BIGWALL_NORTH // do East,South,East&South
						&& _tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_W_N)
					{
						srfSprite->blitNShade(
								surface,
								posScreen.x,
								posScreen.y - _tile->getMapData(O_OBJECT)->getYOffset(),
								tileShade);
					}

/*
// For VERTICAL DIRECTION:
					if (itZ > 0 // THIS IS LEADING TO A NEAR-INFINITE REGRESSION!!
						&& _tile->isVoid(false, false) == false)
					{
						redraw = false;

						// redraw units that are moving up from tileBelow
						// so their heads don't get cut off as they're moving up but before they enter the same Z-level.
						if (_tile->getMapData(O_WESTWALL) != NULL // TODO: draw only if there's no southern wall in tileEast that covers the unit's decapitation
							|| _tile->getMapData(O_NORTHWALL) != NULL
							|| (_tile->getMapData(O_OBJECT) != NULL
								&& (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
									|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST
									|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NORTH
									|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N)))
						{
							// TODO: insert checks for tileEast !south&tc bigWalls & tileSouthEast !north&tc Walls

//							const Tile* const tileBelow = _battleSave->getTile(posMap + Position(0,0,-1)); // def'n above still good.
							BattleUnit* const unitBelow = tileBelow->getUnit();

							if (unitBelow != NULL
								&& unitBelow->getVerticalDirection() != 0
								&& (unitBelow->getUnitVisible() == true
									|| _battleSave->getDebugMode() == true))
//								&& unitBelow->getStatus() == STATUS_FLYING
							{
								quad = tileBelow->getPosition().x - unitBelow->getPosition().x
									+ (tileBelow->getPosition().y - unitBelow->getPosition().y) * 2;

								srfSprite = unitBelow->getCache(&invalid, quad);
//								srfSprite = NULL;
								if (srfSprite)
								{
									if (kL_Debug_vert) Log(LOG_INFO) << ". drawUnit [160]";
									redraw = true; // TODO:
									// redraw tileBelow foreground (south & east bigWalls)
									// redraw tileBelowEast foreground (south bigWall)
									// redraw tileBelowSouthEast background (north & west Walls)

									if (tileBelow->isDiscovered(2) == true)
										shade = tileBelow->getShade();
									else
										shade = 16;

									calculateWalkingOffset(
														unitBelow,
														&walkOffset);

									srfSprite->blitNShade(
											surface,
											posScreen.x + walkOffset.x,
											posScreen.y + 24 + walkOffset.y,
											shade);

									if (unitBelow->getFireOnUnit() != 0)
									{
										frame = 4 + (_animFrame / 2);
										srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
										srfSprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + 24+ walkOffset.y,
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
							const bool redrawUnit = _tile->getMapData(O_FLOOR) != NULL
												|| (_tile->getMapData(O_OBJECT) != NULL
													&& (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
														|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH
														|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S));
							// for tileBelow-South:
							// Note: unit will be redrawn *again* if conditions for an East-tile are true right after this;
							// so checks could/should be done for those sprites here also ... and if true postpone redrawing the unit to then.
							if (redrawUnit == true
								|| _tile->getMapData(O_WESTWALL) != NULL
								|| (_tile->getMapData(O_OBJECT) != NULL
									&& (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST
										|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_W_N)))
							{
								// TODO: insert checks for tileSouthEast !south&tc bigWalls & tileSouthSouthEast !north&tc Walls
								const Tile* const tileBelowSouth = _battleSave->getTile(posMap + Position(0,1,-1));

								if (tileBelowSouth != NULL)
								{
									// Note: redrawing unit to South will overwrite any stuff to south & southeast & east. Assume nothing will be there for now ... BZZZZZZZT!!!
									// TODO: don't draw unitSouth if there is a foreground object on this Z-level above the unit (ie. it hides his head getting chopped off anyway).
									// TODO: redraw foreground objects on lower Z-level
									BattleUnit* const unitBelowSouth = tileBelowSouth->getUnit(); // else CTD here.

									if (unitBelowSouth != NULL
										&& unitBelowSouth->getVerticalDirection() != 0
										&& (unitBelowSouth->getUnitVisible() == true
											|| _battleSave->getDebugMode() == true))
									{
										quad = tileBelowSouth->getPosition().x - unitBelowSouth->getPosition().x
											+ (tileBelowSouth->getPosition().y - unitBelowSouth->getPosition().y) * 2;

										srfSprite = unitBelowSouth->getCache(&invalid, quad);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (kL_Debug_vert) Log(LOG_INFO) << ". drawUnit [170]";
											redraw = true;

											if (tileBelowSouth->isDiscovered(2) == true)
												shade = tileBelowSouth->getShade();
											else
												shade = 16;

											calculateWalkingOffset(
																unitBelowSouth,
																&walkOffset);

											srfSprite->blitNShade(
													surface,
													posScreen.x - 16 + walkOffset.x,
													posScreen.y + 32 + walkOffset.y,
													shade);

											if (unitBelowSouth->getFireOnUnit() != 0)
											{
												frame = 4 + (_animFrame / 2);
												srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												srfSprite->blitNShade(
														surface,
														posScreen.x - 16 + walkOffset.x,
														posScreen.y + 32 + walkOffset.y,
														0);
											}
										}
									}
								}
							}


							// for tileBelow-SouthEast:
							if (itX < endX - 1 // why does this want (endX - 1) else CTD.
								&& (redrawUnit == true
									|| (_tile->getMapData(O_OBJECT) != NULL
										&& (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NONE
											|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST))))
							{
								// checks if the unit will be drawn anyway as unitBelowSouth above
								// so don't draw it here because it will be redrawn when X increments.
								const Tile* const tileBelowSouthEast = _battleSave->getTile(posMap + Position(1,1,-1));

								if (tileBelowSouthEast != NULL)
								{
									// Note: redrawing unit to South will overwrite any stuff to south & southeast & east.
									BattleUnit* const unitBelowSouthEast = tileBelowSouthEast->getUnit();

									if (unitBelowSouthEast != NULL
										&& unitBelowSouthEast->getVerticalDirection() != 0
										&& (unitBelowSouthEast->getUnitVisible() == true
											|| _battleSave->getDebugMode() == true))
									{
										const Tile* const tileEast = _battleSave->getTile(posMap + Position(1,0,0));

										if (tileEast != NULL)
										{
											if (tileEast->getMapData(O_FLOOR) == NULL // these should be exactly opposite to check for unitBelowSouth above.
												&& tileEast->getMapData(O_WESTWALL) == NULL
												&& (tileEast->getMapData(O_OBJECT) == NULL
													|| (tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
														&& tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_WEST
														&& tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_SOUTH
														&& tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_E_S
														&& tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_W_N)))
												// TODO: insert checks for tileSouthEastEast !south&tc bigWalls & tileSouthSouthEastEast !north&tc Walls
											{
												quad = tileBelowSouthEast->getPosition().x - unitBelowSouthEast->getPosition().x
													+ (tileBelowSouthEast->getPosition().y - unitBelowSouthEast->getPosition().y) * 2;

												srfSprite = unitBelowSouthEast->getCache(&invalid, quad);
//												srfSprite = NULL;
												if (srfSprite)
												{
													if (kL_Debug_vert) Log(LOG_INFO) << ". drawUnit [180]";
													redraw = true;

													if (tileBelowSouthEast->isDiscovered(2) == true)
														shade = tileBelowSouthEast->getShade();
													else
														shade = 16;

													calculateWalkingOffset(
																		unitBelowSouthEast,
																		&walkOffset);

													srfSprite->blitNShade(
															surface,
															posScreen.x + walkOffset.x,
															posScreen.y + 40 + walkOffset.y,
															shade);

													if (unitBelowSouthEast->getFireOnUnit() != 0)
													{
														frame = 4 + (_animFrame / 2);
														srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
														srfSprite->blitNShade(
																surface,
																posScreen.x + walkOffset.x,
																posScreen.y + 40 + walkOffset.y,
																0);
													}
												}
											}
										}
									}
								}
							}


							if (redraw == true) // The order/ structure of these following have not been optimized ....
							{
								//Log(LOG_INFO) << "redraw TRUE";
								if (itY < endY - 1)
								{
									const Tile* const tileBelowSouthSouth = _battleSave->getTile(posMap + Position(0,2,-1));

									if (tileBelowSouthSouth != NULL)
									{
										srfSprite = tileBelowSouthSouth->getSprite(O_NORTHWALL);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileBelowSouthSouth->isDiscovered(2) == true)
												shade = tileBelowSouthSouth->getShade();
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													posScreen.x - 32,
													posScreen.y + 40 - tileBelowSouthSouth->getMapData(O_NORTHWALL)->getYOffset(),
													shade);
										}
									}

									if (itX < endX)
									{
										const Tile* const tileBelowSouthSouthEast = _battleSave->getTile(posMap + Position(1,2,-1));

										if (tileBelowSouthSouthEast != NULL)
										{
											srfSprite = tileBelowSouthSouthEast->getSprite(O_NORTHWALL);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (tileBelowSouthSouthEast->isDiscovered(2) == true)
													shade = tileBelowSouthSouthEast->getShade();
												else
													shade = 16;

												srfSprite->blitNShade(
														surface,
														posScreen.x - 16,
														posScreen.y + 48 - tileBelowSouthSouthEast->getMapData(O_NORTHWALL)->getYOffset(),
														shade);
											}
										}
									}
								}

								if (itX < endX)
								{
									// TODO: if unitBelow this needs to redraw walls to south & east also.
									const Tile* const tileBelowSouthEast = _battleSave->getTile(posMap + Position(1,1,-1));

									if (tileBelowSouthEast != NULL)
									{
										srfSprite = tileBelowSouthEast->getSprite(O_NORTHWALL);
//										srfSprite = NULL;
										if (srfSprite)
										{
											if (tileBelowSouthEast->isDiscovered(2) == true)
												shade = tileBelowSouthEast->getShade();
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													posScreen.x,
													posScreen.y + 40 - tileBelowSouthEast->getMapData(O_NORTHWALL)->getYOffset(),
													shade,
													false,
													0,
													true);
										}

										srfSprite = tileBelowSouthEast->getSprite(O_OBJECT);
//										srfSprite = NULL;
										if (srfSprite
											&& tileBelowSouthEast->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW)
										{
											if (tileBelowSouthEast->isDiscovered(2) == true)
												shade = tileBelowSouthEast->getShade();
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													posScreen.x,
													posScreen.y + 40 - tileBelowSouthEast->getMapData(O_OBJECT)->getYOffset(),
													shade,
													false,
													0,
													true);
										}
									}
								}

								if (itX < endX && itY < endY -1)
								{
									const Tile* const tileBelowSouthEastEast = _battleSave->getTile(posMap + Position(2,1,-1));

									if (tileBelowSouthEastEast != NULL)
									{
										srfSprite = tileBelowSouthEastEast->getSprite(O_OBJECT);
//										srfSprite = NULL;
										if (srfSprite
											&& tileBelowSouthEastEast->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW)
										{
											if (tileBelowSouthEastEast->isDiscovered(2) == true)
												shade = tileBelowSouthEastEast->getShade();
											else
												shade = 16;

											srfSprite->blitNShade(
													surface,
													posScreen.x + 16,
													posScreen.y + 48 - tileBelowSouthEastEast->getMapData(O_OBJECT)->getYOffset(),
													shade);
										}
									}
								}
							}
						}

						if (itX < endX)
						{
							if (_tile->getMapData(O_FLOOR) != NULL
								|| (_tile->getMapData(O_OBJECT) != NULL
									&& (_tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST
										|| _tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S)))
							{
								const Tile* const tileBelowEast = _battleSave->getTile(posMap + Position(1,0,-1));

								if (tileBelowEast != NULL)
								{
									BattleUnit* const unitBelowEast = tileBelowEast->getUnit();

									if (unitBelowEast != NULL
										&& unitBelowEast->getVerticalDirection() != 0
										&& (unitBelowEast->getUnitVisible() == true
											|| _battleSave->getDebugMode() == true))
									{
										const Tile* const tileEast = _battleSave->getTile(posMap + Position(1,0,0));

										if (tileEast->getMapData(O_OBJECT) == NULL
											|| (tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_EAST
												&& tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_SOUTH
												&& tileEast->getMapData(O_OBJECT)->getBigWall() != BIGWALL_E_S))
										{
											quad = tileBelowEast->getPosition().x - unitBelowEast->getPosition().x
												+ (tileBelowEast->getPosition().y - unitBelowEast->getPosition().y) * 2;

											srfSprite = unitBelowEast->getCache(&invalid, quad);
//											srfSprite = NULL;
											if (srfSprite)
											{
												if (kL_Debug_vert) Log(LOG_INFO) << ". drawUnit [190]";
												if (tileBelowEast->isDiscovered(2) == true)
													shade = tileBelowEast->getShade();
												else
													shade = 16;

												calculateWalkingOffset(
																	unitBelowEast,
																	&walkOffset);

												srfSprite->blitNShade(
														surface,
														posScreen.x + 16 + walkOffset.x,
														posScreen.y + 32 + walkOffset.y,
														shade);

												if (unitBelowEast->getFireOnUnit() != 0)
												{
													frame = 4 + (_animFrame / 2);
													srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
													srfSprite->blitNShade(
															surface,
															posScreen.x + 16 + walkOffset.x,
															posScreen.y + 32 + walkOffset.y,
															0);
												}
											}
										}
									}
								}
							}
						}
					}
*/

// Draw Cursor Front
					if (_cursorType != CT_NONE
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX < itX + 1
						&& _selectorY < itY + 1
						&& _battleSave->getBattleState()->getMouseOverIcons() == false)
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true)
									&& (_cursorType != CT_PSI
										|| ((_battleSave->getBattleGame()->getCurrentAction()->type == BA_PSICOURAGE
												&& unit->getFaction() != FACTION_HOSTILE)
											|| (_battleSave->getBattleGame()->getCurrentAction()->type != BA_PSICOURAGE
												&& unit->getFaction() != FACTION_PLAYER))))
								{
									frame = 3 + (_animFrame % 2);	// yellow flashing box
								}
								else
									frame = 3;						// red standard box
							}
							else // CT_AIM ->
							{
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true))
								{
									frame = 7 + (_animFrame / 2);	// yellow animated crosshairs
								}
								else
									frame = 6;						// red static crosshairs
							}

							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y,
									0);

							// UFOExtender Accuracy: display adjusted accuracy value on crosshair (and more).
//							if (Options::battleUFOExtenderAccuracy == true) // kL_note: one less condition to check
							if (_cursorType == CT_AIM) // indicator for Firing.
							{
								// begin_TEST: superimpose targetUnit *over* cursor's front
								if (unit != NULL
									&& (unit->getUnitVisible() == true
										|| _battleSave->getDebugMode() == true)
									&& _tile->isDiscovered(2) == false)
								{
									quad = _tile->getPosition().x - unit->getPosition().x
										+ (_tile->getPosition().y - unit->getPosition().y) * 2;

									srfSprite = unit->getCache(&invalid, quad);
									if (srfSprite)
									{
										shade = tileShade;

										calculateWalkingOffset(
															unit,
															&walkOffset);

										srfSprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												shade);

//										if (unit->getFireOnUnit() != 0)
//										{
//											frame = 4 + (_animFrame / 2);
//											srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
//											if (srfSprite)
//												srfSprite->blitNShade(
//														surface,
//														posScreen.x + walkOffset.x,
//														posScreen.y + walkOffset.y,
//														0);
//										}
									}
								} // end_TEST.

								// kL_note: Use stuff from ProjectileFlyBState::init()
								// as well as TileEngine::canTargetUnit() & TileEngine::canTargetTile()
								// to turn accuracy to 'red 0' if target is out of LoS/LoF.
								const BattleAction* const action = _battleSave->getBattleGame()->getCurrentAction();

								int accuracy = static_cast<int>(Round(action->actor->getAccuracy(*action) * 100.));

								const RuleItem* const weaponRule = action->weapon->getRules();
								const int
									lowerLimit = weaponRule->getMinRange(),
									distance = _battleSave->getTileEngine()->distance(
																					Position(
																							itX,
																							itY,
																							itZ),
																					action->actor->getPosition());
								int upperLimit;
								switch (action->type)
								{
									case BA_AIMEDSHOT:
										upperLimit = weaponRule->getAimRange();
									break;
									case BA_SNAPSHOT:
										upperLimit = weaponRule->getSnapRange();
									break;
									case BA_AUTOSHOT:
										upperLimit = weaponRule->getAutoRange();
									break;

									default:
										upperLimit = 200;
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

								_numAccuracy->setValue(static_cast<unsigned int>(accuracy));
								_numAccuracy->setColor(color);
								_numAccuracy->draw();
								_numAccuracy->blitNShade(
													surface,
													posScreen.x,
													posScreen.y,
													0);
							}
							else if (_cursorType == CT_THROW) // indicator for Throwing.
							{
								BattleAction* const action = _battleSave->getBattleGame()->getCurrentAction();
								action->target = Position(
														itX,
														itY,
														itZ);

								const Position
									originVoxel = _battleSave->getTileEngine()->getOriginVoxel(*action),
									targetVoxel = Position(
														itX * 16 + 8,
														itY * 16 + 8,
														itZ * 24 + 2 - _battleSave->getTile(action->target)->getTerrainLevel());

								unsigned int accuracy = 0;
								Uint8 color = Palette::blockOffset(2)+3; // red

								const bool canThrow = _battleSave->getTileEngine()->validateThrow(
																							*action,
																							originVoxel,
																							targetVoxel);
								if (canThrow == true)
								{
									accuracy = static_cast<unsigned int>(Round(_battleSave->getSelectedUnit()->getAccuracy(*action) * 100.));
									color = Palette::blockOffset(3)+3; // green
								}

								_numAccuracy->setValue(accuracy);
								_numAccuracy->setColor(color);
								_numAccuracy->draw();
								_numAccuracy->blitNShade(
													surface,
													posScreen.x,
													posScreen.y,
													0);
							}
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frame = 5; // blue box
							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y,
									0);
						}

						if (_cursorType > CT_AIM // Psi, Waypoint, Throw
							&& _camera->getViewLevel() == itZ)
						{
							const int arrowFrame[6] = {0,0,0,11,13,15};
							srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(arrowFrame[_cursorType] + (_animFrame / 4));
							srfSprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y,
									0);
						}
					}
					// end cursor front.


// Draw WayPoints if any on current Tile
					int
						waypid = 1,
						waypXOff = 2,
						waypYOff = 2;

					for (std::vector<Position>::const_iterator
							i = _waypoints.begin();
							i != _waypoints.end();
							++i)
					{
						if (*i == posMap)
						{
							if (waypXOff == 2
								&& waypYOff == 2)
							{
								srfSprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(7);
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y,
										0);
							}

							if (_battleSave->getBattleGame()->getCurrentAction()->type == BA_LAUNCH)
							{
								wpID->setValue(waypid);
								wpID->draw();
								wpID->blitNShade(
										surface,
										posScreen.x + waypXOff,
										posScreen.y + waypYOff,
										0);

								waypXOff += (waypid > 9) ? 8 : 6;
								if (waypXOff > 25)
								{
									waypXOff = 2;
									waypYOff += 8;
								}
							}
						}

						++waypid;
					}
					// end waypoints.


// Draw Map's border-sprite only on ground tiles
					if (itZ == _battleSave->getGroundLevel()
						|| (itZ == 0
							&& _battleSave->getGroundLevel() == -1))
					{
						if (   itX == 0
							|| itX == _battleSave->getMapSizeX() - 1
							|| itY == 0
							|| itY == _battleSave->getMapSizeY() - 1)
						{
							srfSprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(330); // gray square cross
							srfSprite->blitNShade(
									surface,
									posScreen.x + 14,
									posScreen.y + 31,
									0);
						}
					}
					// end border icon.
				}
				// is inside the Surface
			}
			// end Tiles_y looping.
		}
		// end Tiles_x looping.
	}
	// end Tiles_z looping.


	// Draw Bouncing Arrow over selected unit.
	if (_cursorType != CT_NONE
		&& (_battleSave->getSide() == FACTION_PLAYER
			|| _battleSave->getDebugMode() == true))
	{
		unit = _battleSave->getSelectedUnit();
		if (unit != NULL
			&& unit->getStatus() != STATUS_WALKING
			&& unit->getPosition().z <= _camera->getViewLevel())
		{
			_camera->convertMapToScreen(
									unit->getPosition(),
									&posScreen);
			posScreen += _camera->getMapOffset();

			calculateWalkingOffset( // this calculates terrainLevel, but is otherwise bloat here.
								unit,
								&walkOffset);
			walkOffset.y += 21 - unit->getHeight();

			if (unit->getArmor()->getSize() > 1)
			{
				walkOffset.y += 10;
				if (unit->getFloatHeight() != 0)
					walkOffset.y -= unit->getFloatHeight() + 1;
			}

			if (unit->isKneeled() == true)
			{
				walkOffset.y -= 5;
				_arrow_kneel->blitNShade(
								surface,
								posScreen.x
									+ walkOffset.x
									+ _spriteWidth / 2
									- _arrow->getWidth() / 2,
								posScreen.y
									+ walkOffset.y
									- _arrow->getHeight()
									+ static_cast<int>(
										-4. * std::sin(22.5 / static_cast<double>(_animFrame + 1))),
								0);
			}
			else
				_arrow->blitNShade(
								surface,
								posScreen.x
									+ walkOffset.x
									+ _spriteWidth / 2
									- _arrow->getWidth() / 2,
								posScreen.y
									+ walkOffset.y
									- _arrow->getHeight()
									+ static_cast<int>(
										4. * std::sin(22.5 / static_cast<double>(_animFrame + 1))),
								0);
		}
	}
	// end arrow.

	if (pathPreview == true)
	{
		if (wpID != NULL)
			wpID->setBordered(); // make a border for the pathfinding display

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
					posMap = Position(
									itX,
									itY,
									itZ);
					_camera->convertMapToScreen(
											posMap,
											&posScreen);
					posScreen += _camera->getMapOffset();

					// only render cells that are inside the surface
					if (   posScreen.x > -_spriteWidth
						&& posScreen.x < surface->getWidth() + _spriteWidth
						&& posScreen.y > -_spriteHeight
						&& posScreen.y < surface->getHeight() + _spriteHeight)
					{
						_tile = _battleSave->getTile(posMap);

						if (_tile == NULL
							|| _tile->isDiscovered(0) == false
							|| _tile->getPreviewDir() == -1)
						{
							continue;
						}

						int offset_y = -_tile->getTerrainLevel();

						if (_previewSetting & PATH_ARROWS)
						{
							const Tile* const tileBelow = _battleSave->getTile(posMap + Position(0,0,-1));

							if (itZ > 0
								&& _tile->hasNoFloor(tileBelow) == true)
							{
								srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(23);
								if (srfSprite)
									srfSprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + 2,
											0,
											false,
											_tile->getPreviewColor());
							}

							const int overlay = _tile->getPreviewDir() + 12;
							srfSprite = _res->getSurfaceSet("Pathfinding")->getFrame(overlay);
							if (srfSprite)
								srfSprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y - offset_y,
										0,
										false,
										_tile->getPreviewColor());
						}

						if ((_previewSetting & PATH_TU_COST)
							&& _tile->getPreviewTU() > -1)
						{
							int offset_x;
							if (_tile->getPreviewTU() > 9)
								offset_x = 4;
							else
								offset_x = 2;

							if (_battleSave->getSelectedUnit() != NULL
								&& _battleSave->getSelectedUnit()->getArmor()->getSize() > 1)
							{
								offset_y += 1;
								if (!(_previewSetting & PATH_ARROWS))
									offset_y += 7;
							}

							wpID->setValue(_tile->getPreviewTU());
							wpID->draw();

							if (!(_previewSetting & PATH_ARROWS))
								wpID->blitNShade(
											surface,
											posScreen.x + 16 - offset_x,
//											posScreen.y + 29 - offset_y,
											posScreen.y + 37 - offset_y, // kL
											0,
											false,
											_tile->getPreviewColor());
							else
								wpID->blitNShade(
											surface,
											posScreen.x + 16 - offset_x,
//											posScreen.y + 22 - offset_y,
											posScreen.y + 30 - offset_y, // kL
											0);
						}
					}
				}
			}
		}

		if (wpID != NULL)
			wpID->setBordered(false); // remove the border in case it's used for missile waypoints.
	}

	delete wpID;
	// end Path Preview.


	if (_explosionInFOV == true) // check if they got hit or explosion animations
	{
		// big explosions cause the screen to flash as bright as possible before any explosions are actually drawn.
		// this causes everything to look like EGA for a single frame.
/*		if (_flashScreen == true)
		{
			Uint8 color;
			for (int
					x = 0,
						y = 0;
					x < surface->getWidth()
						&& y < surface->getHeight();
					)
			{
//				surface->setPixelIterative(&x,&y, surface->getPixel(x,y) & 0xF0); // <- Volutar's
				color = (surface->getPixelColor(x,y) / 16) * 16; // get the brightest color in each colorgroup.
				surface->setPixelIterative(
										&x,&y,
										color);
			}
			_flashScreen = false;
		}
		else
		{ */
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
					//Log(LOG_INFO) << "Map:hitFrame = " << (*i)->getCurrentFrame();
					srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
					srfSprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 15,
							0);
				}
			}
		}
//		}
	}
//	surface->drawRect( // TEST
//				8,-16,
//				static_cast<Sint16>(getWidth()) - 8 - 8, static_cast<Sint16>(getHeight()) - 80 + 16,
//				3);
	surface->unlock();
}

/**
 * Sets and returns variables about a unit moving on or off of a hilltop.
 * @param unit			- reference a BattleUnit
 * @param posMap		- reference the current tile position being drawn
 * @param pixelOffset_x	- reference to set for sprite offset on x-axis
 * @param pixelOffset_y	- reference to set for sprite offset on y-axis
 * @return, true if @a unit should be redrawn
 */
/* bool Map::hilltopRedraw( // private
		const BattleUnit& unit,
		const Position& posMap,
		int& pixelOffset_x,
		int& pixelOffset_y) const
{
	if (unit.getArmor()->getSize() == 1
		&& unit.getVerticalDirection() == 0) // not sure if this is needed ...
	{
		const Position pos = unit.getPosition();

		if (pos.z + 1 == posMap.z
			&& (pos.x < posMap.x
				|| pos.y < posMap.y)
			&& _battleSave->getTileEngine()->distance(
													pos,
													posMap,
													false) == 1)
		{
			const Position
				prePos = unit.getLastPosition(),
				endPos = unit.getDestination();

			const int dir = unit.getDirection();

			if (pos == endPos) // moving down off the hilltop ->
			{
				switch (dir)
				{
					case 1:
						//Log(LOG_INFO) << "dir 1";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == prePos)
						{
							pixelOffset_x = 32;
							pixelOffset_y = 24;
							return true;
						}
						else if (posMap.x == prePos.x + 1)
						{
							pixelOffset_x = 16;
							pixelOffset_y = 16;
							return true;
						}
					return false;

					case 0:
						//Log(LOG_INFO) << "dir 0";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == prePos)
						{
							pixelOffset_x = 16;
							pixelOffset_y = 16;
							return true;
						}
						else if (posMap.x == prePos.x + 1)
						{
							pixelOffset_x = 0;
							pixelOffset_y = 8;
							return true;
						}
					return false;

					case 7:
						//Log(LOG_INFO) << "dir 7";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == prePos)
						{
							pixelOffset_x = 0;
							pixelOffset_y = 8;
							return true;
						}
						else if (posMap.x == prePos.x - 1)
						{
							pixelOffset_x = 16;
							pixelOffset_y = 16;
							return true;
						}
						else if (posMap.y == prePos.y - 1)
						{
							pixelOffset_x = -16;
							pixelOffset_y = 16;
							return true;
						}
					return false;

					case 6:
						//Log(LOG_INFO) << "dir 6";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == prePos)
						{
							pixelOffset_x = -16;
							pixelOffset_y = 16;
							return true;
						}
						else if (posMap.y == prePos.y + 1)
						{
							pixelOffset_x = 0;
							pixelOffset_y = 8;
							return true;
						}
					return false;

					case 5:
						//Log(LOG_INFO) << "dir 5";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == prePos)
						{
							pixelOffset_x = -32;
							pixelOffset_y = 24;
							return true;
						}
						else if (posMap.y == prePos.y + 1)
						{
							pixelOffset_x = -16;
							pixelOffset_y = 16;
							return true;
						}
				}
			}
			else if (pos == prePos) // moving up onto the hilltop ->
			{
				switch (dir)
				{
					case 1:
						//Log(LOG_INFO) << "dir 1";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == endPos)
						{
							pixelOffset_x = -32;
							pixelOffset_y = 24;
							return true;
						}
						else if (posMap.y == endPos.y + 1)
						{
							pixelOffset_x = -16;
							pixelOffset_y = 16;
							return true;
						}
					return false;

					case 2:
						//Log(LOG_INFO) << "dir 2";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == endPos)
						{
							pixelOffset_x = -16;
							pixelOffset_y = 16;
							return true;
						}
						else if (posMap.y == endPos.y + 1)
						{
							pixelOffset_x = 0;
							pixelOffset_y = 8;
							return true;
						}
					return false;

					case 3:
						//Log(LOG_INFO) << "dir 3";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == endPos)
						{
							pixelOffset_x = 0;
							pixelOffset_y = 8;
							return true;
						}
						else if (posMap.x == endPos.x - 1)
						{
							pixelOffset_x = 16;
							pixelOffset_y = 16;
							return true;
						}
						else if (posMap.y == endPos.y - 1)
						{
							pixelOffset_x = -16;
							pixelOffset_y = 16;
							return true;
						}
					return false;

					case 4:
						//Log(LOG_INFO) << "dir 4";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == endPos)
						{
							pixelOffset_x = 16;
							pixelOffset_y = 16;
							return true;
						}
						else if (posMap.x == endPos.x + 1)
						{
							pixelOffset_x = 0;
							pixelOffset_y = 8;
							return true;
						}
					return false;

					case 5:
						//Log(LOG_INFO) << "dir 5";
						//Log(LOG_INFO) << "mapPos = " << posMap;
						if (posMap == endPos)
						{
							pixelOffset_x = 32;
							pixelOffset_y = 24;
							return true;
						}
						else if (posMap.x == endPos.x + 1)
						{
							pixelOffset_x = 16;
							pixelOffset_y = 16;
							return true;
						}
				}
			}
		}
	}

	return false;
} */

/**
 * Handles mouse presses on the map.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Map::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);
	_camera->mousePress(action, state);
}

/**
 * Handles mouse releases on the map.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Map::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);
	_camera->mouseRelease(action, state);
}

/**
 * Handles keyboard presses on the map.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Map::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);
	_camera->keyboardPress(action, state);
}

/**
 * Handles keyboard releases on the map.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Map::keyboardRelease(Action* action, State* state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_camera->keyboardRelease(action, state);
}

/**
 * Handles mouse over events on the map.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
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
// * @param redraw - true to redraw the battlescape map (default true)
 */
void Map::animateMap(bool redraw)
{
	++_animFrame;
 	if (_animFrame == 8)
		_animFrame = 0;

	for (size_t // animate tiles
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->animateTile();
	}

	for (std::vector<BattleUnit*>::const_iterator	// animate certain units
			i = _battleSave->getUnits()->begin();	// (large flying units have a propulsion animation)
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false
			&& (*i)->getArmor()->getConstantAnimation() == true)
		{
			(*i)->setCache(NULL);
			cacheUnit(*i);
		}
	}


	if (--_fuseColor == 15)
		_fuseColor = 31;

	if (redraw == true)
		_redraw = true;
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
 * Sets the rectangular selector to a certain tile.
 * @param x - mouse x position
 * @param y - mouse y position
 */
void Map::setSelectorPosition(
		int x,
		int y)
{
	const int
		pre_X = _selectorX,
		pre_Y = _selectorY;

	_camera->convertScreenToMap(
							x,
							y + _spriteHeight / 4,
							&_selectorX,
							&_selectorY);

	if (pre_X != _selectorX
		|| pre_Y != _selectorY)
	{
		_redraw = true;
	}
}

/**
 * Gets the position of the rectangular selector.
 * @param pos - pointer to a Position
 */
void Map::getSelectorPosition(Position* const pos) const
{
	pos->x = _selectorX;
	pos->y = _selectorY;
	pos->z = _camera->getViewLevel();
}

/**
 * Calculates the offset of a BattleUnit when it is walking between 2 tiles.
 * @param unit		- pointer to a BattleUnit
 * @param offset	- pointer to the Position offset for returning the calculation
 */
void Map::calculateWalkingOffset(
		const BattleUnit* const unit,
		Position* const offset) const
{
	*offset = Position(0,0,0);

	const int	   // 0  1  2  3  4  5  6  7
		offsetX[8] = {1, 1, 1, 0,-1,-1,-1, 0},
		offsetY[8] = {1, 0,-1,-1,-1, 0, 1, 1},
						 // 0   1   2   3   4   5   6   7
		offsetFalseX[8] = {16, 32, 16,  0,-16,-32,-16,  0}, // for duplicate drawing units from their transient
		offsetFalseY[9] = {-8,  0,  8, 16,  8,  0, -8,-16}, // destination & last positions. See UnitWalkBState.
		offsetFalseVert = 24,

		dir = unit->getDirection(),				// 0..7
		dirVert = unit->getVerticalDirection(),	// 0= none, 8= up, 9= down
		walkPhase = unit->getWalkPhase() + unit->getDiagonalWalkPhase(),
		armorSize = unit->getArmor()->getSize();
	int
		midPhase,
		endPhase;
	unit->walkPhaseCutoffs(
						midPhase,
						endPhase);

	const bool
		stepOne = (walkPhase < midPhase),
		truePosi = (unit->getTile() == _tile);

	if (dirVert == 0)
	{
		if (unit->getStatus() == STATUS_WALKING
			|| unit->getStatus() == STATUS_FLYING)
		{
			if (stepOne == true)
			{
				offset->x =  walkPhase * offsetX[dir] * 2 + ((truePosi == true) ? 0 : -offsetFalseX[dir]);
				offset->y = -walkPhase * offsetY[dir]     + ((truePosi == true) ? 0 : -offsetFalseY[dir]);
			}
			else
			{
				offset->x =  (walkPhase - endPhase) * offsetX[dir] * 2 + ((truePosi == true) ? 0 : offsetFalseX[dir]);
				offset->y = -(walkPhase - endPhase) * offsetY[dir]     + ((truePosi == true) ? 0 : offsetFalseY[dir]);
			}
		}
	}

	if (unit->getStatus() == STATUS_WALKING // if unit is between tiles interpolate its terrain level (y-adjustment).
		|| unit->getStatus() == STATUS_FLYING)
	{
		//Log(LOG_INFO) << unit->getId() << " " << unit->getPosition() << " truePosi = " << truePosi;
		//Log(LOG_INFO) << ". dir = " << dir << " dirVert = " << dirVert;
		const int posDestZ = unit->getDestination().z;
		int
			levelStart,
			levelEnd;

		if (stepOne == true)
		{
			if (dirVert == 8)
				offset->y += (truePosi == true) ? 0 : offsetFalseVert;
			else if (dirVert == 9)
				offset->y += (truePosi == true) ? 0 : -offsetFalseVert;

			levelEnd = getTerrainLevel(
									unit->getDestination(),
									armorSize);

			const int posZ = unit->getPosition().z;
			if (posZ > posDestZ)		// going down a level so 'levelEnd' 0 becomes +24 (-8 becomes 16)
				levelEnd += 24 * (posZ - posDestZ);
			else if (posZ < posDestZ)	// going up a level so 'levelEnd' 0 becomes -24 (-8 becomes -16)
				levelEnd = -24 * (posDestZ - posZ) + std::abs(levelEnd);

			levelStart = getTerrainLevel(
									unit->getPosition(),
									armorSize);
			offset->y += ((levelStart * (endPhase - walkPhase)) / endPhase) + ((levelEnd * walkPhase) / endPhase);
		}
		else	// from midPhase onwards the unit is behind the scenes and is already on the destination tile
		{		// so get its last position to calculate the correct offset.
			if (dirVert == 8)
				offset->y += (truePosi == true) ? 0 : -offsetFalseVert;
			else if (dirVert == 9)
				offset->y += (truePosi == true) ? 0 : offsetFalseVert;

			levelStart = getTerrainLevel(
									unit->getLastPosition(),
									armorSize);

			const int posLastZ = unit->getLastPosition().z;
			if (posLastZ > posDestZ)		// going down a level so 'levelStart' 0 becomes -24 (-8 becomes -32)
				levelStart -= 24 * (posLastZ - posDestZ);
			else if (posLastZ < posDestZ)	// going up a level so 'levelStart' 0 becomes +24 (-8 becomes 16)
				levelStart =  24 * (posDestZ - posLastZ) - std::abs(levelStart);

			levelEnd = getTerrainLevel(
									unit->getDestination(),
									armorSize);
			offset->y += ((levelStart * (endPhase - walkPhase)) / endPhase) + ((levelEnd * walkPhase) / endPhase);
		}
	}
	else // standing.
	{
		offset->y += getTerrainLevel(
								unit->getPosition(),
								armorSize);

		if (unit->getStatus() == STATUS_AIMING
			&& unit->getArmor()->getCanHoldWeapon() == true)
		{
			offset->x = -16;
		}
	}
}

/**
  * Terrainlevel goes from 0 to -24 (bottom to top).
  * @note For a large sized unit pick the highest terrain level which is the
  * lowest number.
  * @param pos			- reference the Position
  * @param armorSize	- size of the unit at @a pos
  * @return, terrain height
  */
int Map::getTerrainLevel( // private.
		const Position& pos,
		int armorSize) const
{
	int
		lowLevel = 0,
		lowTest;

	for (int
			x = 0;
			x != armorSize;
			++x)
	{
		for (int
				y = 0;
				y != armorSize;
				++y)
		{
			lowTest = _battleSave->getTile(pos + Position(x,y,0))->getTerrainLevel();
			if (lowTest < lowLevel)
				lowLevel = lowTest;
		}
	}

	return lowLevel;
}

/**
 * Sets the 3D cursor to selection/aim mode.
 * @param type	- CursorType (Map.h)
 * @param quads	- size of the cursor (default 1)
 */
void Map::setCursorType(
		CursorType type,
		int quads)
{
	if ((_cursorType = type) == CT_NORMAL)
		_cursorSize = quads;
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
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		cacheUnit(*i);
	}
}

/**
 * Check if a certain unit needs to be redrawn.
 * @param unit - pointer to a BattleUnit
 */
void Map::cacheUnit(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "cacheUnit() : " << unit->getId();
	int width;
	if (unit->getStatus() == STATUS_AIMING)
		width = _spriteWidth * 2;
	else
		width = _spriteWidth;

//	Pathfinding* const pf = _battleSave->getPathfinding();
//	pf->setPathingUnit(unit);
	UnitSprite* const unitSprite = new UnitSprite(
												width,
												_spriteHeight,
												0,0);
//												pf->getMoveTypePf());
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
				unitPart = 0;
				unitPart != unitSize;
				++unitPart)
		{
			//Log(LOG_INFO) << ". . unitPart = " << unitPart;
			Surface* cache = unit->getCache(
										&d,
										unitPart);
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
			unitSprite->setBattleUnit(
									unit,
									unitPart);

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
			unit->setCache(
						cache,
						unitPart);
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
		_firstBulletFrame = true;
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
		Uint8 btn,
		bool pressed)
{
	setButtonPressed(
				btn,
				pressed);
}

/**
 * Sets the unitDying flag.
 * @note This reveals the dying unit during Hidden Movement.
 * @param flag - true if a unit is dying (default true)
 */
void Map::setUnitDying(bool flag)
{
	_unitDying = flag;
}

/**
 * Special handling for setting the height of the map viewport.
 * @param height - the new base screen height
 */
void Map::setHeight(int height)
{
	Surface::setHeight(height);

	_visibleMapHeight = height - _iconHeight;

	if (_visibleMapHeight < 200)
		height = _visibleMapHeight;
	else
		height = 200;

	_hiddenScreen->setHeight(height);
	_hiddenScreen->setY((_visibleMapHeight - _hiddenScreen->getHeight()) / 2);
}

/**
 * Special handling for setting the width of the map viewport.
 * @param width - the new base screen width
 */
void Map::setWidth(int width)
{
	Surface::setWidth(width);

	_hiddenScreen->setX(_hiddenScreen->getX() + (width - getWidth()) / 2);
}

/**
 * Gets the hidden movement screen's vertical position.
 * @return, the vertical position of the hidden movement window
 */
int Map::getMessageY() const
{
	return _hiddenScreen->getY();
}

/**
 * Gets the icon height.
 * @return, icon panel height
 */
int Map::getIconHeight() const
{
	return _iconHeight;
}

/**
 * Gets the icon width.
 * @return, icon panel width
 */
int Map::getIconWidth() const
{
	return _iconWidth;
}

/**
 * Returns the angle (left/right balance) of a sound effect based on a map position.
 * @param pos - reference the map position to calculate the sound angle from
 * @return, the angle of the sound (360 = 0 degrees center)
 */
int Map::getSoundAngle(const Position& pos) const
{
	const int midPoint = getWidth() / 2;

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
	// Since Mix_SetPosition() uses modulo 360 can't feed it a negative number so add 360.
	// The integer-factor below is the allowable maximum deflection from center
	return (screenPos.x * 35 / midPoint) + 360;
}

/**
 * Resets the camera smoothing bool.
 */
/*void Map::resetCameraSmoothing()
{
	_smoothingEngaged = false;
} */

/**
 * Sets the "explosion flash" bool.
 * @param flash - true to render the screen in EGA this frame
 */
/*void Map::setBlastFlash(bool flash)
{
	_flashScreen = flash;
} */

/**
 * Checks if the screen is still being rendered in EGA.
 * @return, true if still in EGA mode
 */
/*bool Map::getBlastFlash() const
{
	return _flashScreen;
} */

/**
 * Sets whether to draw or not.
 * @param noDraw - true to stop this Map from drawing
 */
void Map::setNoDraw(bool noDraw)
{
	_noDraw = noDraw;
}

/**
 * Gets if the Hidden Movement screen is displayed.
 * @return, true if hidden
 */
bool Map::getMapHidden() const
{
	return _mapIsHidden;
}

/**
 * Gets the SavedBattleGame.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* Map::getBattleSave() const
{
	return _battleSave;
}

/**
 * Tells the map to reveal because there's a waypoint action going down.
 * @param wp - true if there is waypoint/missile action in progress (default true)
 */
void Map::setWaypointAction(bool wp)
{
	_waypointAction = wp;
}

/**
 * Sets whether to draw the projectile on the Map.
 * @param show - true to show the projectile (default true)
 */
void Map::setShowProjectile(bool show)
{
	_showProjectile = show;
}

}
