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
		_bulletStart(false),
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

	_previewSetting	= Options::battlePreviewPath;
//	if (Options::traceAI == true) _previewSetting = PATH_FULL; // turn everything on to see the markers.

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

/*	_txtAccuracy = new Text(24,9);
	_txtAccuracy->setSmall();
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());
	_txtAccuracy->setHighContrast();
	_txtAccuracy->initText(_res->getFont("FONT_BIG"), _res->getFont("FONT_SMALL"), _game->getLanguage()); */
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
	const Uint8
		f = 16, // yellow Fill
		b = 14; // black Border
	const Uint8 pixels_stand[81] = { 0, 0, b, b, b, b, b, 0, 0,
									 0, 0, b, f, f, f, b, 0, 0,
									 0, 0, b, f, f, f, b, 0, 0,
									 b, b, b, f, f, f, b, b, b,
									 b, f, f, f, f, f, f, f, b,
									 0, b, f, f, f, f, f, b, 0,
									 0, 0, b, f, f, f, b, 0, 0,
									 0, 0, 0, b, f, b, 0, 0, 0,
									 0, 0, 0, 0, b, 0, 0, 0, 0 };

	_arrow = new Surface(9,9);
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

	const Uint8 pixels_kneel[81] = { 0, 0, 0, 0, 0, 0, 0, 0, 0,
									 0, 0, 0, 0, b, 0, 0, 0, 0,
									 0, 0, 0, b, f, b, 0, 0, 0,
									 0, 0, b, f, f, f, b, 0, 0,
									 0, b, f, f, f, f, f, b, 0,
									 b, f, f, f, f, f, f, f, b,
									 b, b, b, f, f, f, b, b, b,
									 0, 0, b, f, f, f, b, 0, 0,
									 0, 0, b, b, b, b, b, 0, 0 };

	_arrow_kneel = new Surface(9,9);
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
	_arrow_kneel->unlock();

	_projectile = NULL;
	_projectileSet = _res->getSurfaceSet("Projectiles");

/*	int // reveal map's border tiles.
		size_x = _battleSave->getMapSizeX(),
		size_y = _battleSave->getMapSizeY(),
		size_z = _battleSave->getMapSizeZ();

	for (int x = 0; x < size_x; ++x)
		for (int y = 0; y < size_y; ++y)
			for (int z = 0; z < size_z; ++z)
				if (x == 0 || y == 0 || x == size_x - 1 || y == size_y - 1)
				{
					Tile* tile = _battleSave->getTile(Position(x,y,z));
					if (tile) tile->setDiscovered(true,2);
				}
			}
		}
	} */
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
	// removed setting this here and in BattlescapeGame::handleState(),
	// Camera::scrollXY(), ProjectileFlyBState::think() x2.
//	if (_redraw == false) return;
//	_redraw = false;

	// Normally call for a Surface::draw();
	// but don't clear the background with color 0, which is transparent
	// (aka black) -- you use color 15 because that actually corresponds to the
	// color you DO want in all variations of the xcom palettes.
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
	Position bullet; // x-y position of bullet on screen.
	int
		bulletLowX	= 16000,
		bulletLowY	= 16000,
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
				if (_bulletStart == true)
				{
					_bulletStart = false;

					const Position posFinal = _projectile->getFinalPosition();

					BattleAction* const prjAction = _projectile->getActionPrj();
					if (prjAction->actor->getFaction() != _battleSave->getSide()
						|| bullet.x < 0
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

						if (prjAction->actor->getFaction() != _battleSave->getSide()	// moved here from TileEngine::reactionShot()
							&& _camera->isOnScreen(posFinal) == false)					// because this is the (accurate) position of the bullet-shot-actor's Camera mapOffset.
						{
							Log(LOG_INFO) << "Map add rfActor " << prjAction->actor->getId() << " " << _camera->getMapOffset() << " final Pos offScreen";
							std::map<int, Position>* const rfShotList (_battleSave->getTileEngine()->getRfShotList());
							rfShotList->insert(std::pair<int, Position>(
																	prjAction->actor->getId(),
																	_camera->getMapOffset()));
						}
					}

					if (_projectile->getItem() != NULL
						|| _camera->isOnScreen(posFinal) == false)
					{
						_smoothingEngaged = true;
						_camera->setPauseAfterShot();
						Log(LOG_INFO) << ". shot going offScreen OR throw - setPauseAfterShot";
					}
				}

				if (_smoothingEngaged == true)
					_camera->jumpXY(
								surface->getWidth() / 2 - bullet.x,
								_visibleMapHeight / 2 - bullet.y);

				const int posBullet_z = (_projectile->getPosition().z) / 24;
				if (_camera->getViewLevel() != posBullet_z)
					_camera->setViewLevel(posBullet_z);
			}
			else // NOT smoothCamera: I don't use this.
			// Camera remains stationary when xCom actively fires at target;
			// that is, target is already onScreen due to targeting cursor click.
//			if (_projectile->getActor()->getFaction() != FACTION_PLAYER)
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
	NumberText* wpId = NULL;

	if (_waypoints.empty() == false
		|| (pathPreview == true
			&& (_previewSetting & PATH_TU_COST)))
	{
		// note: WpID is used for both pathPreview and BL waypoints.
		// Leave them the same color.
		wpId = new NumberText(15, 15, 20, 30);
		wpId->setPalette(getPalette());
		wpId->setColor(1); // white
	}


	int // get Map's corner-coordinates for rough boundaries in which to draw tiles.
		beginX,
		beginY,
		beginZ = 0,
		endX,
		endY,
		endZ,
		d;

	if (_camera->getShowLayers() == true)
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
		animOffset,
		quad;		// The quadrant is 0 for small units; large units have quadrants 1,2 & 3 also; the
					// relative x/y Position of the unit's primary quadrant vs the current tile's Position.
	bool
		invalid = false,

		hasFloor, // these denote characteristics of 'tile' as in the current Tile of the loop.
		hasObject,

		halfRight = false; // avoid VC++ linker warning.


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

					hasFloor = false;
					hasObject = false;

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
										if (srfSprite != NULL)
											srfSprite->blitNShade(
													surface,
													posScreen.x + 2 + 16,
													posScreen.y + 3 + 32 + getTerrainLevel(
																							unitEastBelow->getPosition(),
																							unitEastBelow->getArmor()->getSize()),
													0);
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

// Draw Tile Background
// Draw west wall
					if (_tile->isVoid(true, false) == false)
					{
						srfSprite = _tile->getSprite(O_WESTWALL);
						if (srfSprite)
						{
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

							if (val == true // Draw SMOKE & FIRE if itemUnit is on Fire
								&& _tile->isDiscovered(2) == true)
							{
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


						// kL_begin #2 of 3: Make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						// note: Or a westwall (ie, bulging UFO hull) in tile above & south of the soldier <- not done
						if (itZ > 0
							&& hasFloor == false
							&& hasObject == false)
						{
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
									}
								}
							}
						}
					} // Void <- end.

// Draw Bullet if in Field Of View
					if (_showProjectile == true // <- used to hide Celatid glob while its spitting animation plays.
						&& _projectile != NULL)
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
						if (srfSprite)
						{
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

							// kL_begin #3 of 3:
							const Tile* const tileAbove = _battleSave->getTile(posMap + Position(0,0,1));

							if ((_camera->getViewLevel() == itZ
									&& (_camera->getShowLayers() == false
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

									}
									else
									{
										std::string soldierRank = unit->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
										soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

										srfSprite = _res->getSurface(soldierRank);
										if (srfSprite)
											srfSprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 2,
													posScreen.y + walkOffset.y + 3,
													0);
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
						}
					}
					// end Main Draw BattleUnit.

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

// UFOExtender Accuracy
							// display adjusted accuracy value on crosshair (and more).
//							if (Options::battleUFOExtenderAccuracy == true) // note: one less condition to check
							if (_cursorType == CT_AIM) // indicator for Firing.
							{
								// superimpose targetUnit *over* cursor's front
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
									}
								}

								// TODO: Use stuff from ProjectileFlyBState::init()
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
								wpId->setValue(static_cast<unsigned>(waypid));
								wpId->draw();
								wpId->blitNShade(
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
			&& (unit->getStatus() == STATUS_STANDING
				|| unit->getStatus() == STATUS_TURNING)
			&& unit->getPosition().z <= _camera->getViewLevel())
		{
			_camera->convertMapToScreen(
									unit->getPosition(),
									&posScreen);
			posScreen += _camera->getMapOffset();

			walkOffset.y += getTerrainLevel(
										unit->getPosition(),
										unit->getArmor()->getSize());
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
		if (wpId != NULL)
			wpId->setBordered(); // make a border for the pathfinding display

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

							wpId->setValue(static_cast<unsigned>(_tile->getPreviewTU()));
							wpId->draw();

							if (!(_previewSetting & PATH_ARROWS))
								wpId->blitNShade(
											surface,
											posScreen.x + 16 - offset_x,
											posScreen.y + 37 - offset_y,
											0,
											false,
											_tile->getPreviewColor());
							else
								wpId->blitNShade(
											surface,
											posScreen.x + 16 - offset_x,
											posScreen.y + 30 - offset_y,
											0);
						}
					}
				}
			}
		}

		if (wpId != NULL)
			wpId->setBordered(false); // remove the border in case it's used for missile waypoints.
	}

	delete wpId;
	// end Path Preview.

	if (_explosionInFOV == true) // check if they got hit or explosion animations
	{
		// big explosions cause the screen to flash as bright as possible before
		// any explosions are actually drawn. Tthis causes everything to look
		// like EGA for a single frame.
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
				else if ((*i)->isHit() == 0) // bullet, http://ufopaedia.org/index.php?title=SMOKE.PCK
				{
					srfSprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
					srfSprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 15,
							0);
				}
				else //if ((*i)->isHit() == 1) // melee or psiamp, http://ufopaedia.org/index.php?title=HIT.PCK
				{	 // put that back in to acknowledge -1 as a no-animation melee miss.
					srfSprite = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame());
					srfSprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 25,
							0);

					if ((*i)->isHit() == 1) // temp kludge to show batman-type hit if melee is successful
					{
						srfSprite = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame() - 4);
						if (srfSprite)
							srfSprite->blitNShade(
									surface,
									bullet.x - 15,
									bullet.y - 25,
									0);
					}
				}
			}
		}
//		}
	}
	surface->unlock();
}

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
		_bulletStart = true;
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

/*
 * Resets the camera smoothing bool.
 *
void Map::resetCameraSmoothing()
{
	_smoothingEngaged = false;
} */

/*
 * Sets the "explosion flash" bool.
 * @param flash - true to render the screen in EGA this frame
 *
void Map::setBlastFlash(bool flash)
{
	_flashScreen = flash;
} */

/*
 * Checks if the screen is still being rendered in EGA.
 * @return, true if still in EGA mode
 *
bool Map::getBlastFlash() const
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
