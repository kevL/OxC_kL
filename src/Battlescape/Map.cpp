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

/**
 * Sets up a map with the specified size and position.
 * @param game				- pointer to the core Game
 * @param width				- width in pixels
 * @param height			- height in pixels
 * @param x					- X position in pixels
 * @param y					- Y position in pixels
 * @param playableHeight	- current visible map height
 */
Map::Map(
		Game* game,
		int width,
		int height,
		int x,
		int y,
		int playableHeight)
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
		_mX(0),
		_mY(0),
		_cursorType(CT_NORMAL),
		_cursorSize(1),
		_animFrame(0),
		_projectile(NULL),
		_projectileSet(NULL),
		_projectileInFOV(false),
		_explosionInFOV(false),
		_waypointAction(false),
		_bulletStart(false),
		_playableHeight(playableHeight),
		_unitDying(false),
		_reveal(0),
		_smoothingEngaged(false),
		_flashScreen(false),
		_mapIsHidden(false),
		_noDraw(false),
		_showProjectile(true),
		_battleSave(game->getSavedGame()->getBattleSave()),
		_res(game->getResourcePack()),
		_fuseColor(31),
		_tile(NULL),
		_unit(NULL)
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
					playableHeight);

	int hiddenHeight;
	if (playableHeight < 200)
		hiddenHeight = playableHeight;
	else
		hiddenHeight = 200;
	_hiddenScreen = new BattlescapeMessage(320, hiddenHeight); // "Hidden Movement..." screen
	_hiddenScreen->setX(_game->getScreen()->getDX());
	_hiddenScreen->setY(_game->getScreen()->getDY());
//	_hiddenScreen->setY((playableHeight - _hiddenScreen->getHeight()) / 2);
	_hiddenScreen->setTextColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("messageWindows")->color));

	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)& Map::scrollMouse);

	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)& Map::scrollKey);

	_camera->setScrollTimers(
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
	if (_noDraw == false) // don't draw if MiniMap is open. Or if Inventory is open.
	{
	// removed setting this here and in BattlescapeGame::handleState(),
	// Camera::scrollXY(), ProjectileFlyBState::think() x2.
//	if (_redraw == false) return;
//	_redraw = false;

	// Normally call for a Surface::draw();
	// but don't clear the background with color 0, which is transparent
	// (aka black) -- use color 15 because that actually corresponds to the
	// color you DO want in all variations of the xcom palettes.
//	Surface::draw();

		if (_flashScreen == true)
		{
			static int stickyTicks;	// TODO: This should really be some factor of the rate at which calls to
									// ExplosionBState are made against the rate at which calls to Map here are made.
			if (++stickyTicks == 5)
			{
				stickyTicks = 0;
				_flashScreen = false;
			}

			clear(SCREEN_WHITE);
			return;
		}

		clear(SCREEN_BLACK);

		const Tile* tile;
		if (_projectile != NULL) //&& _battleSave->getSide() == FACTION_PLAYER)
		{
			tile = _battleSave->getTile(Position::toTileSpace(_projectile->getPosition()));
			if (tile != NULL
				&& (tile->getTileVisible() == true
					|| _battleSave->getSide() != FACTION_PLAYER)) // shows projectile during aLien berserk
			{
				_projectileInFOV = true;
			}
		}
		else
			_projectileInFOV = _battleSave->getDebugMode(); // reveals map in debugmode; hides battlefield if no projectiles in flight.

		if (_explosions.empty() == false)
		{
			for (std::list<Explosion*>::const_iterator
					i = _explosions.begin();
					i != _explosions.end();
					++i)
			{
				tile = _battleSave->getTile(Position::toTileSpace((*i)->getPosition()));
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
			_explosionInFOV = _battleSave->getDebugMode(); // reveals map in debugmode; hides battlefield if no explosions waiting.


		static bool delayHiddenScreen;

		if (_battleSave->getSelectedUnit() == NULL
			|| _battleSave->getSelectedUnit()->getUnitVisible() == true
			|| _unitDying == true
			|| _explosionInFOV == true
			|| _projectileInFOV == true
			|| _waypointAction == true // stop flashing the Hidden Movement screen between waypoints.
			|| _battleSave->getDebugMode() == true)
		{
			// REVEAL //
			delayHiddenScreen = true;
			_mapIsHidden = false;
			drawTerrain(this);
		}
		else
		{
			// HIDE //
			if (delayHiddenScreen == true)
			{
				delayHiddenScreen = false;
				SDL_Delay(369);
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
 * Draw the battlefield.
 * @note Keep this function as optimised as possible - it's big and needs to be
 * fast so minimize overhead of function calls. Etc.
 * @param surface - the surface on which to draw the entire battlefield
 */
void Map::drawTerrain(Surface* const surface) // private.
{
	Position bullet; // x-y position of bullet on screen.
	int
		bulletLowX	= 16000,
		bulletLowY	= 16000,
		bulletHighX	= 0,
		bulletHighY	= 0,
		bulletHighZ	= 0,
		viewLevel = _camera->getViewLevel();

	if (_projectile != NULL // if there is a bullet get the highest x and y tiles to draw it on
		&& _explosions.empty() == true)
	{
		int
			qtyProjectileSpriteFrames,
			trjOffset;

		if (_projectile->getThrowItem() != NULL)
			qtyProjectileSpriteFrames = 0;
		else
			qtyProjectileSpriteFrames = BULLET_SPRITES - 1;

		for (int
				i = 0;
				i <= qtyProjectileSpriteFrames;
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

		// convert bullet position from voxel-space to tile-space
		bulletLowX  >>= 4;
		bulletLowY  >>= 4;
		bulletHighX >>= 4;
		bulletHighY >>= 4;
		bulletHighZ /= 24;

		// if the projectile is outside the viewport - center back on it
		if (_projectileInFOV == true)
		{
			_camera->convertVoxelToScreen(
									_projectile->getPosition(),
									&bullet);

/*			if (Options::battleSmoothCamera == true)
			{ */
			const Position posFinal = _projectile->getFinalPosition();
			BattleAction* const action = _projectile->getBattleAction();

			if (_bulletStart == true)
			{
				_bulletStart = false;
				if (   bullet.x < 0 // if bullet starts offScreen
					|| bullet.x >= surface->getWidth()
					|| bullet.y < 0
					|| bullet.y >= _playableHeight)
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

				const bool offScreen_final (_camera->isOnScreen(posFinal) == false);
				if (action->actor->getFaction() != _battleSave->getSide()	// moved here from TileEngine::reactionShot()
					&& offScreen_final == true)								// because this is the (accurate) position of the bullet-shot-actor's Camera mapOffset.
				{
					std::map<int, Position>* const rfShotPos (_battleSave->getTileEngine()->getReactionPositions());
					rfShotPos->insert(std::pair<int, Position>(
															action->actor->getId(),
															_camera->getMapOffset()));
				}

				if (offScreen_final == true
					|| ((_projectile->getThrowItem() != NULL
							|| action->weapon->getRules()->getArcingShot() == true)
						&& TileEngine::distance(
											action->actor->getPosition(),
											posFinal) > DIST_ARC_SMOOTH)) // no smoothing unless throw > 8 tiles
				{
					_smoothingEngaged = true;
					_camera->setPauseAfterShot();
				}
			}
			else if (_smoothingEngaged == true)
				_camera->jumpXY(
							surface->getWidth() / 2 - bullet.x,
							_playableHeight / 2 - bullet.y);

			if (_smoothingEngaged == true
				|| posFinal.z != action->actor->getPosition().z)
			{
				const int posBullet_z ((_projectile->getPosition().z) / 24);
				if (posBullet_z != viewLevel)
					_camera->setViewLevel(viewLevel = posBullet_z);
			}
/*			}
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
						_camera->jumpXY(surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.x > surface->getWidth())
					{
						_camera->jumpXY(-surface->getWidth(), 0);
						enough = false;
					}
					else if (bullet.y < 0)
					{
						_camera->jumpXY(0, _playableHeight);
						enough = false;
					}
					else if (bullet.y > _playableHeight)
					{
						_camera->jumpXY(0, -_playableHeight);
						enough = false;
					}
					_camera->convertVoxelToScreen(
											_projectile->getPosition(),
											&bullet);
				}
				while (enough == false);
			} */
		}
	}
	else _smoothingEngaged = false; // no projectile OR explosions-waiting

	const bool pathPreviewed = _battleSave->getPathfinding()->isPathPreviewed();
	NumberText* numWp = NULL;

	if (_waypoints.empty() == false
		|| (pathPreviewed == true && (_previewSetting & PATH_TU_COST)))
	{
		// note: numWp is used for both pathPreview and BL-waypoints.
		// Leave them the same color.
		numWp = new NumberText(15, 15, 20, 30);
		numWp->setPalette(getPalette());
		numWp->setColor(1); // white
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
		endZ = viewLevel;

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

	beginY -= (viewLevel * 2);
	beginX -= (viewLevel * 2);
	if (beginX < 0) beginX = 0;
	if (beginY < 0) beginY = 0;


	Surface* sprite;
	const Tile* tileBelow;

	Position
		posField,
		posScreen,
		walkOffset;

	int
		frame,
		tileShade,
		shade,
		animOffset,
		quadrant;	// The quadrant is 0 for small units; large units have quadrants 1,2 & 3 also; describes		0|1
					// the relative x/y Position of the unit's primary quadrant vs. the current tile's Position.	2|3
	bool
		hasUnit, // these denote characteristics of 'tile' as in the current Tile of the loop.
		hasFloor,
		hasObject,
		trueLoc;

//	const BattleUnit* unitBelow;


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
				posField = Position(itX,itY,itZ);
				_camera->convertMapToScreen(posField, &posScreen);
				posScreen += _camera->getMapOffset();

				if (posScreen.x > -_spriteWidth // only render cells that are inside the surface (ie viewport ala player's monitor)
					&& posScreen.x < surface->getWidth() + _spriteWidth
					&& posScreen.y > -_spriteHeight
					&& posScreen.y < surface->getHeight() + _spriteHeight)
				{
					_tile = _battleSave->getTile(posField);

					if (_tile == NULL)
						continue;

					if (itZ != 0)
						tileBelow = _battleSave->getTile(posField + Position(0,0,-1));
					else
						tileBelow = NULL;

					if (_tile->isDiscovered(2) == true)
						tileShade = _tile->getShade();
					else
						tileShade = SHADE_BLACK;

					_unit = _tile->getUnit();

					hasUnit = _unit != NULL
						  && (_unit->getUnitVisible() == true || _battleSave->getDebugMode() == true);
					hasFloor =
					hasObject = false;

// Draw Floor
					sprite = _tile->getSprite(O_FLOOR);
					if (sprite)
					{
						hasFloor = true;

						sprite->blitNShade(
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
						if (itZ > 0 && itX < endX)
						{
							const Tile* const tileEast = _battleSave->getTile(posField + Position(1,0,0));
							if (tileEast != NULL // why.
								&& tileEast->getSprite(O_FLOOR) == NULL
								&& (tileEast->getMapData(O_OBJECT) == NULL // special case ->
									|| tileEast->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NWSE))
							{
								const Tile* const tileEastBelow = _battleSave->getTile(posField + Position(1,0,-1));
								const BattleUnit* const unitEastBelow = tileEastBelow->getUnit();

								if (unitEastBelow != NULL
									&& unitEastBelow != _battleSave->getSelectedUnit()
									&& unitEastBelow->getGeoscapeSoldier() != NULL
									&& unitEastBelow->getFaction() == FACTION_PLAYER)
								{
									const int tLevel = getTerrainLevel(
																	unitEastBelow->getPosition(),
																	unitEastBelow->getArmor()->getSize());

									if (unitEastBelow->getFatalWounds() != 0)
									{
										sprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
										if (sprite != NULL)
											sprite->blitNShade(
													surface,
													posScreen.x + 2 + 16,
													posScreen.y + 3 + 32 + tLevel);

										sprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
										if (sprite != NULL)
											sprite->blitNShade(
													surface,
													posScreen.x + 4 + 16,
													posScreen.y + 4 + 32 + tLevel,
													_animFrame * 2,
													false, 3); // red
									}
									else
									{
										std::string soldierRank = unitEastBelow->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
										soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

										sprite = _res->getSurface(soldierRank);
										if (sprite != NULL)
											sprite->blitNShade(
													surface,
													posScreen.x + 2 + 16,
													posScreen.y + 3 + 32 + tLevel);
									}
								}
							}
						} // kL_end.
					} // end draw floor

// Redraw unitNorth moving NE/SW to stop current-Floor from clipping feet.
					if (itX != 0 && itY != 0
						&& hasFloor == true)
					{
						const Tile* const tileWest = _battleSave->getTile(posField + Position(-1,0,0));
						const BattleUnit* const unitWest = tileWest->getUnit();
						if (unitWest != NULL
							&& unitWest->getUnitVisible() == true // don't bother checking DebugMode.
							&& (unitWest->getUnitStatus() == STATUS_WALKING
								|| unitWest->getUnitStatus() == STATUS_FLYING)
							&& (unitWest->getUnitDirection() == 1
								|| unitWest->getUnitDirection() == 5)) // && vertical dir == 0
						{
							const Tile* tileNorth = _battleSave->getTile(posField + Position(0,-1,0));
							const BattleUnit* unitNorth = tileNorth->getUnit();
							int offsetZ_y;
							if (unitNorth == NULL && itZ != 0)
							{
								tileNorth = _battleSave->getTile(posField + Position(0,-1,-1));
								unitNorth = tileNorth->getUnit();
								offsetZ_y = 24;
							}
							else
								offsetZ_y = 0;

							if (unitNorth == unitWest)
							{
								const Tile* const tileSouthWest = _battleSave->getTile(posField + Position(-1,1,0));
								if (checkWest(tileWest, tileSouthWest, unitNorth) == true)
								{
									const Tile* const tileNorthEast = _battleSave->getTile(posField + Position(1,-1,0));
									if (checkNorth(tileNorth, tileNorthEast, unitNorth) == true)
									{
										trueLoc = isTrueLoc(unitNorth, tileNorth);
										quadrant = getQuadrant(unitNorth, tileNorth, trueLoc);
										sprite = unitNorth->getCache(quadrant);
										if (sprite)
										{
											if (unitNorth->isOut_t(OUT_HLTH_STUN) == true)
												shade = std::min(5, tileShade);
											else
												shade = tileShade;

											calculateWalkingOffset(unitNorth, &walkOffset, trueLoc);
											sprite->blitNShade(
													surface,
													posScreen.x + walkOffset.x + 16,
													posScreen.y + walkOffset.y - 8 + offsetZ_y,
													shade);

											if (unitNorth->getFireUnit() != 0)
											{
												frame = 4 + (_animFrame / 2);
												sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
												if (sprite)
													sprite->blitNShade(
															surface,
															posScreen.x + walkOffset.x + 16,
															posScreen.y + walkOffset.y - 8);
											}
										}
									}
								}
							}
						}
					}

// Draw Cursor Background
					if (_cursorType != CT_NONE
						&& _battleSave->getBattleState()->getMouseOverIcons() == false
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX <= itX
						&& _selectorY <= itY)
					{
						if (viewLevel == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (hasUnit == true
									&& (_cursorType != CT_PSI
										|| ((_battleSave->getBattleGame()->getCurrentAction()->type == BA_PSICOURAGE
												&& _unit->getFaction() != FACTION_HOSTILE)
											|| (_battleSave->getBattleGame()->getCurrentAction()->type != BA_PSICOURAGE
												&& _unit->getFaction() != FACTION_PLAYER))))
								{
									frame = (_animFrame % 2);		// yellow flashing box
								}
								else frame = 0;						// red static box
							}
							else // CT_AIM ->
							{
								if (hasUnit == true)
									frame = 7 + (_animFrame / 2);	// yellow animated crosshairs
								else
									frame = 6;						// red static crosshairs
							}

							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							if (sprite != NULL)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y);
						}
						else if (viewLevel > itZ)
						{
							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(2); // blue static box
							if (sprite != NULL)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y);
						}
					}
					// end cursor bg

// Draw Tile Background
// Draw west wall
					if (_tile->isVoid(true, false) == false)
					{
						sprite = _tile->getSprite(O_WESTWALL);
						if (sprite)
						{
							if (_tile->isDiscovered(0) == true
								&& (_tile->getMapData(O_WESTWALL)->isDoor() == true
									|| _tile->getMapData(O_WESTWALL)->isUfoDoor() == true))
							{
								shade = std::min(9, _tile->getShade());
							}
							else shade = tileShade;

							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_WESTWALL)->getYOffset(),
									shade);
						}

// Draw North Wall
						sprite = _tile->getSprite(O_NORTHWALL);
						if (sprite)
						{
							if (_tile->isDiscovered(1) == true
								&& (_tile->getMapData(O_NORTHWALL)->isDoor() == true
									|| _tile->getMapData(O_NORTHWALL)->isUfoDoor() == true))
							{
								shade = std::min(9, _tile->getShade());
							}
							else shade = tileShade;

							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_NORTHWALL)->getYOffset(),
									shade,
									(_tile->getMapData(O_WESTWALL) != NULL));
						}

// Draw Object in Background & Center
						sprite = _tile->getSprite(O_OBJECT);
						if (sprite
							&& (_tile->getMapData(O_OBJECT)->getBigwall() < BIGWALL_EAST // do none,Block,diagonals,West,North,West&North
								|| _tile->getMapData(O_OBJECT)->getBigwall() == BIGWALL_W_N))
						{
							hasObject = true;
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y - _tile->getMapData(O_OBJECT)->getYOffset(),
									tileShade);
						}

// Draw Corpse + Item on Floor if any
						bool var;
						int spriteId = _tile->getCorpseSprite(&var);
						if (spriteId != -1)
						{
							sprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(spriteId);
							if (sprite)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + _tile->getTerrainLevel(),
										tileShade);

							if (var == true && _tile->isDiscovered(2) == true)
							{
								frame = 4 + (_animFrame / 2);
								sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
								if (sprite)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y + _tile->getTerrainLevel());
							}
						}

						spriteId = _tile->getTopSprite(&var);
						if (spriteId != -1)
						{
							sprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(spriteId);
							if (sprite)
							{
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + _tile->getTerrainLevel(),
										tileShade);

								if (var == true && _tile->isDiscovered(2) == true)
								{
									for (int
											x = 0;
											x != 3;
											++x)
									{
										sprite->setPixelColor(
															15 + x, 28,
															_fuseColor);
									}
								}
							}
						}


						// kL_begin #2 of 3: Make sure the rankIcon isn't half-hidden by a westwall directly above the soldier.
						// note: Or a westwall (ie, bulging UFO hull) in tile above & south of the soldier <- not done
						if (itZ > 0
							&& hasFloor == false
							&& hasObject == false)
						{
							const BattleUnit* const unitBelow = tileBelow->getUnit();

							if (unitBelow != NULL
								&& unitBelow != _battleSave->getSelectedUnit()
								&& unitBelow->getGeoscapeSoldier() != NULL
								&& unitBelow->getFaction() == unitBelow->getOriginalFaction())
							{
								const int tLevel = getTerrainLevel(
																unitBelow->getPosition(),
																unitBelow->getArmor()->getSize());

								if (unitBelow->getFatalWounds() != 0)
								{
									sprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
									if (sprite)
										sprite->blitNShade(
												surface,
												posScreen.x + 2,
												posScreen.y + 3 + 24 + tLevel);

									sprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
									if (sprite)
										sprite->blitNShade(
												surface,
												posScreen.x + 4,
												posScreen.y + 4 + 24 + tLevel,
												_animFrame * 2,
												false, 3); // red
								}
								else
								{
									std::string soldierRank = unitBelow->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
									soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

									sprite = _res->getSurface(soldierRank);
									if (sprite)
										sprite->blitNShade(
												surface,
												posScreen.x + 2,
												posScreen.y + 3 + 24 + tLevel);
								}
							}
						}
					} // Void <- end.

// Draw Bullet if in Field Of View
					if (_showProjectile == true // <- used to hide Celatid glob while its spitting animation plays.
						&& _projectile != NULL)
					{
						Position voxel;
						if (_projectile->getThrowItem() != NULL)
						{
							sprite = _projectile->getThrowSprite();
							if (sprite)
							{
								voxel = _projectile->getPosition(); // draw shadow on the floor
								voxel.z = _battleSave->getTileEngine()->castShadow(voxel);
								if (   (voxel.x >> 4) >= itX
									&& (voxel.y >> 4) >= itY
									&& (voxel.x >> 4) <  itX + 2
									&& (voxel.y >> 4) <  itY + 2
									&&  voxel.z / 24 ==  itZ)
								{
									_camera->convertVoxelToScreen(voxel, &bullet);
									sprite->blitNShade(
											surface,
											bullet.x - 16,
											bullet.y - 26,
											SHADE_BLACK);
								}

								voxel = _projectile->getPosition(); // draw thrown object
								if (   (voxel.x >> 4) >= itX
									&& (voxel.y >> 4) >= itY
									&& (voxel.x >> 4) <  itX + 2
									&& (voxel.y >> 4) <  itY + 2
									&&  voxel.z / 24 ==  itZ)
								{
									_camera->convertVoxelToScreen(voxel, &bullet);
									sprite->blitNShade(
											surface,
											bullet.x - 16,
											bullet.y - 26);
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
								for (int
										id = 0;
										id != BULLET_SPRITES;
										++id)
								{
									sprite = _projectileSet->getFrame(_projectile->getBulletSprite(id));
									if (sprite)
									{
										voxel = _projectile->getPosition(1 - id); // draw shadow on the floor
										voxel.z = _battleSave->getTileEngine()->castShadow(voxel);
										if (   (voxel.x >> 4) == itX
											&& (voxel.y >> 4) == itY
											&&  voxel.z / 24  == itZ)
										{
											_camera->convertVoxelToScreen(voxel, &bullet);

											bullet.x -= sprite->getWidth() / 2;
											bullet.y -= sprite->getHeight() / 2;
											sprite->blitNShade(
													surface,
													bullet.x,
													bullet.y,
													SHADE_BLACK);
										}

										voxel = _projectile->getPosition(1 - id); // draw bullet itself
										if (   (voxel.x >> 4) == itX
											&& (voxel.y >> 4) == itY
											&&  voxel.z / 24  == itZ)
										{
											_camera->convertVoxelToScreen(voxel, &bullet);

											bullet.x -= sprite->getWidth() / 2;
											bullet.y -= sprite->getHeight() / 2;
											sprite->blitNShade(
													surface,
													bullet.x,
													bullet.y);
										}
									}
								}
							}
						}
					}
					// end draw bullet.

// Main Draw BattleUnit ->
					if (hasUnit == true)
/*						&& (_unit->getVerticalDirection() == 0	// -> problems with foreground-below & background-above during vertical motion:
							|| hasFloor == true					// if draw-on-tileBelow then unit-sprite's upper body gets overdrawn by background floors & walls on higher level;
																// if draw-on-tileAbove then unit-sprite's lower body gets drawn overtop of foreground walls on lower level.
//							|| tileBelow == NULL // if (hasFloor=false) then tile-below *should be* valid.
							|| _unit != tileBelow->getUnit())) // draw unit on belowTile only.
//							|| tileBelow->getMapData(O_FLOOR)->isGravLift() == true */
					{
						bool
							halfRight = false,
							draw = true;

						if ((_unit->getWalkPhase() != 0
								|| _unit->getUnitDirection() == 4
								|| _unit->getUnitDirection() == 1
								|| _unit->getUnitDirection() == 2) // weird.
							&& (_unit->getUnitStatus() == STATUS_WALKING
								|| _unit->getUnitStatus() == STATUS_FLYING))
						{
							switch (_unit->getUnitDirection())
							{
								case 0:
								case 4:
								{
									const Tile
										* const tileNorth = _battleSave->getTile(posField + Position(0,-1,0)),
										* const tileNorthEast = _battleSave->getTile(posField + Position(1,-1,0));
									draw = checkNorth(tileNorth, tileNorthEast);
								}
								break;

								case 2:
								case 6:
								{

									const Tile
										* const tileWest = _battleSave->getTile(posField + Position(-1,0,0)),
										* const tileSouthWest = _battleSave->getTile(posField + Position(-1,1,0));
									draw = checkWest(tileWest, tileSouthWest, NULL, &halfRight);
								}
								break;

								case 1:
								case 5:
								{
									const Tile
										* const tileSouthWest = _battleSave->getTile(posField + Position(-1,1,0)),
										* const tileSouthSouthWest = _battleSave->getTile(posField + Position(-1,2,0));
									draw = checkWest(tileSouthWest, tileSouthSouthWest);

									const Tile
										* const tileNorthEast = _battleSave->getTile(posField + Position(1,-1,0)),
										* const tileNorthNorthEast = _battleSave->getTile(posField + Position(1,-2,0));
									draw = draw
										&& checkNorth(tileNorthEast, tileNorthNorthEast);
								}
							}
						}

						if (draw == true)
						{
							trueLoc = isTrueLoc(_unit, _tile);
							quadrant = getQuadrant(_unit, _tile, trueLoc);
							sprite = _unit->getCache(quadrant);
							if (sprite)
							{
								if (_unit->isOut_t(OUT_HLTH_STUN) == true)
									shade = std::min(5, tileShade);
								else
									shade = tileShade;

//								if (shade != 0 && quadrant != 0)	// try to even out lighting of all four quadrants of large units.
//									shade -= 1;						// TODO: trickle this throughout this function!

								calculateWalkingOffset(_unit, &walkOffset, trueLoc);
								sprite->blitNShade(
										surface,
										posScreen.x + walkOffset.x,
										posScreen.y + walkOffset.y,
										shade, halfRight);

								if (_unit->getFireUnit() != 0)
								{
									sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(4 + (_animFrame / 2));
									if (sprite)
										sprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												0, halfRight);
								}

								// kL_begin #3 of 3:
								const Tile* const tileAbove = _battleSave->getTile(posField + Position(0,0,1));

								if ((viewLevel == itZ
										&& (_camera->getShowLayers() == false || itZ == endZ))
									|| (tileAbove != NULL && tileAbove->getSprite(O_FLOOR) == NULL))
								{
									if (_unit != _battleSave->getSelectedUnit()
										&& _unit->getGeoscapeSoldier() != NULL
										&& _unit->getFaction() == _unit->getOriginalFaction())
									{
										if (_unit->getFatalWounds() != 0)
										{
											sprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
											if (sprite)
												sprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x + 2,
														posScreen.y + walkOffset.y + 3);

											sprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
											if (sprite)
												sprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x + 4,
														posScreen.y + walkOffset.y + 4,
														_animFrame * 2,
														false, 3); // red
										}
										else
										{
											std::string soldierRank = _unit->getRankString(); // eg. STR_COMMANDER -> RANK_COMMANDER
											soldierRank = "RANK" + soldierRank.substr(3, soldierRank.length() - 3);

											sprite = _res->getSurface(soldierRank);
											if (sprite)
												sprite->blitNShade(
														surface,
														posScreen.x + walkOffset.x + 2,
														posScreen.y + walkOffset.y + 3);
										}
									}

									// Draw Exposed mark
									if (_unit->getFaction() == FACTION_PLAYER
										&& _unit->getFaction() == _unit->getOriginalFaction()
										&& (_unit->getArmor()->getSize() == 1 || quadrant == 1))
									{
										const int exposure = _unit->getExposed();
										if (exposure != -1)
										{
											int
												colorGroup,
												colorOffset;
											if (_animFrame < 4)
											{
												colorGroup = 0; // white
												colorOffset = 2;
											}
											else
											{
												colorGroup = 10; // yellow
												colorOffset = 3;
											}

											_numExposed->setValue(static_cast<unsigned>(exposure));
											_numExposed->draw();
											_numExposed->blitNShade(
																surface,
																posScreen.x + walkOffset.x + 21,
																posScreen.y + walkOffset.y + 6,
																colorOffset,
																false,
																colorGroup);
										}
									}
								} // kL_end.
							}
						}
					}
					// end Main Draw BattleUnit.

// Draw Unconscious Soldier icon - might want to redundant this, like rankIcons.
					if (_unit == NULL)
					{
						const int hurt = _tile->hasUnconsciousUnit();
						if (hurt != 0)
						{
							sprite = _res->getSurface("RANK_ROOKIE"); // background panel for red cross icon.
							if (sprite != NULL)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y);

							Uint8 color;
							if (hurt == 2)
								color = 3; // red, wounded unconscious soldier
							else
								color = 1; // white, unconscious soldier here

							sprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(11); // small gray cross
							if (sprite)
								sprite->blitNShade(
										surface,
										posScreen.x + 2,
										posScreen.y + 1,
										_animFrame * 2,
										false, color);
						}
					}
					// end unconscious soldier icon.

// Draw unitBelow if it is on raised ground & there is no Floor.
					if (itZ > 0 && _tile->hasNoFloor(tileBelow) == true)
					{
						const BattleUnit* const unitBelow = tileBelow->getUnit();
						if (unitBelow != NULL
							&& unitBelow->getUnitVisible() == true // don't bother checking DebugMode
							&& unitBelow->getHeight(true) - tileBelow->getTerrainLevel() > 25) // head sticks up (should be able to set "25" higher than 25)
						{
							trueLoc = isTrueLoc(unitBelow, tileBelow);
							quadrant = getQuadrant(unitBelow, tileBelow, trueLoc);
							sprite = unitBelow->getCache(quadrant);
							if (sprite)
							{
								if (tileBelow->isDiscovered(2) == true)
									shade = tileBelow->getShade();
								else
									shade = SHADE_BLACK;

								calculateWalkingOffset(unitBelow, &walkOffset, trueLoc);
								sprite->blitNShade(
										surface,
										posScreen.x + walkOffset.x,
										posScreen.y + walkOffset.y + 24,
										shade);

								if (unitBelow->getFireUnit() != 0)
								{
									frame = 4 + (_animFrame / 2);
									sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
									if (sprite)
										sprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y + 24);
								}
							}
						}
//						else unitBelow = NULL; // for use below when deciding whether to redraw cursorFront.
					}

// Draw SMOKE & FIRE
					if (_tile->isDiscovered(2) == true
						&& (_tile->getSmoke() != 0 || _tile->getFire() != 0))
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

						sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frame);
						if (sprite)
							sprite->blitNShade(
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
						if (itZ > 0 && _tile->hasNoFloor(tileBelow) == true)
						{
							sprite = _res->getSurfaceSet("Pathfinding")->getFrame(11);
							if (sprite)
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y + 2,
										0, false,
										_tile->getPreviewColor());
						}

						sprite = _res->getSurfaceSet("Pathfinding")->getFrame(_tile->getPreviewDir());
						if (sprite)
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y,
									0, false,
									_tile->getPreviewColor());
					}

// Draw Front Object
					sprite = _tile->getSprite(O_OBJECT);
					if (sprite
						&& _tile->getMapData(O_OBJECT)->getBigwall() > BIGWALL_NORTH // do East,South,East&South
						&& _tile->getMapData(O_OBJECT)->getBigwall() != BIGWALL_W_N)
					{
						sprite->blitNShade(
								surface,
								posScreen.x,
								posScreen.y - _tile->getMapData(O_OBJECT)->getYOffset(),
								tileShade);
					}

// Draw Cursor Front
					if (_cursorType != CT_NONE
						&& _battleSave->getBattleState()->getMouseOverIcons() == false
						&& _selectorX > itX - _cursorSize
						&& _selectorY > itY - _cursorSize
						&& _selectorX <= itX
						&& _selectorY <= itY)
					{
						if (viewLevel > itZ)
						{
							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(5); // blue static box
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y);
						}
						else if (viewLevel == itZ)
//							|| (unitBelow != NULL && unitBelow->getPosition().z == viewLevel)) // BattleUnit was redrawn below curTile.
						{
/*							int vertOffset;
							if (unitBelow != NULL)
								vertOffset = 24;
							else
								vertOffset = 0; */

							if (_cursorType != CT_AIM)
							{
								if (hasUnit == true //|| unitBelow != NULL)
									&& (_cursorType != CT_PSI
										|| ((_battleSave->getBattleGame()->getCurrentAction()->type == BA_PSICOURAGE
												&& _unit->getFaction() != FACTION_HOSTILE)
											|| (_battleSave->getBattleGame()->getCurrentAction()->type != BA_PSICOURAGE
												&& _unit->getFaction() != FACTION_PLAYER))))
								{
									frame = 3 + (_animFrame % 2);	// yellow flashing box
								}
								else
									frame = 3;						// red static box
							}
							else // CT_AIM ->
							{
								if (hasUnit == true) //|| unitBelow != NULL)
									frame = 7 + (_animFrame / 2);	// yellow animated crosshairs
								else
									frame = 6;						// red static crosshairs
							}

							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame);
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y);// + vertOffset);

// UFOExtender Accuracy
							// display adjusted accuracy value on crosshair (and more).
//							if (Options::battleUFOExtenderAccuracy == true) // note: one less condition to check
							if (_cursorType == CT_AIM) // indicator for Firing.
							{
								// draw targetUnit over cursor's front if tile is blacked-out.
								if (hasUnit == true && _tile->isDiscovered(2) == false)
								{
									trueLoc = isTrueLoc(_unit, _tile);
									quadrant = getQuadrant(_unit, _tile, trueLoc);
									sprite = _unit->getCache(quadrant);
									if (sprite)
									{
										calculateWalkingOffset(_unit, &walkOffset, trueLoc);
										sprite->blitNShade(
												surface,
												posScreen.x + walkOffset.x,
												posScreen.y + walkOffset.y,
												tileShade);
									}
								}

								// TODO: Use stuff from ProjectileFlyBState::init()
								// as well as TileEngine::canTargetUnit() & TileEngine::canTargetTile()
								// to turn accuracy to 'red 0' if target is out of LoS/LoF.
								//
								// TODO: use Projectile::rangeAccuracy() as a static function.
								const BattleAction* const action = _battleSave->getBattleGame()->getCurrentAction();

								int accuracy = static_cast<int>(Round(action->actor->getAccuracy(*action) * 100.));

								const RuleItem* const weaponRule = action->weapon->getRules();
								const int
									lowerLimit = weaponRule->getMinRange(),
									distance = TileEngine::distance(
																Position(itX,itY,itZ),
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
										upperLimit = 200; // arbitrary.
								}

								Uint8 color;
								if (distance > upperLimit)
								{
									accuracy -= (distance - upperLimit) * weaponRule->getDropoff();
									color = ACU_ORANGE;
								}
								else if (distance < lowerLimit)
								{
									accuracy -= (lowerLimit - distance) * weaponRule->getDropoff();
									color = ACU_ORANGE;
								}
								else
									color = ACU_GREEN;

								if (accuracy < 1 // zero accuracy or out of range: set it red.
									|| distance > weaponRule->getMaxRange())
								{
									accuracy = 0;
									color = ACU_RED;
								}

								_numAccuracy->setValue(static_cast<unsigned>(accuracy));
								_numAccuracy->setColor(color);
								_numAccuracy->draw();
								_numAccuracy->blitNShade(
													surface,
													posScreen.x,
													posScreen.y);// + vertOffset);
							}
							else if (_cursorType == CT_THROW) // indicator for Throwing.
							{
								BattleAction* const action = _battleSave->getBattleGame()->getCurrentAction();
								action->target = Position(itX,itY,itZ);

								unsigned accuracy;
								Uint8 color;
								const Position
									originVoxel = _battleSave->getTileEngine()->getOriginVoxel(*action),
									targetVoxel = Position::toVoxelSpaceCentered( // TODO: conform this to ProjectileFlyBState (modifier keys) & Projectile::_targetVoxel
																			Position(itX,itY,itZ),
																			2 - _battleSave->getTile(action->target)->getTerrainLevel()); // LoFT of floor is typically 2 voxels thick.
								if (hasFloor == true && _battleSave->getTileEngine()->validateThrow(*action, originVoxel, targetVoxel) == true)
								{
									accuracy = static_cast<unsigned int>(Round(_battleSave->getSelectedUnit()->getAccuracy(*action) * 100.));
									color = ACU_GREEN;
								}
								else
								{
									accuracy = 0;
									color = ACU_RED;
								}

								_numAccuracy->setValue(accuracy);
								_numAccuracy->setColor(color);
								_numAccuracy->draw();
								_numAccuracy->blitNShade(
													surface,
													posScreen.x,
													posScreen.y);// + vertOffset);
							}
						}

						if (_cursorType > CT_AIM // Psi, Waypoint, Throw
							&& viewLevel == itZ)// || unitBelow != NULL)) // BattleUnit was redrawn below curTile.
						{
							static const int cursorSprites[6] = {0,0,0,11,13,15};
							sprite = _res->getSurfaceSet("CURSOR.PCK")->getFrame(cursorSprites[_cursorType] + (_animFrame / 4));
							sprite->blitNShade(
									surface,
									posScreen.x,
									posScreen.y);
						}
					}
					// end cursor front.

// Draw WayPoints if any on current Tile
					int
						wpVal = 1,
						offset_x = 2,
						offset_y = 2;

					for (std::vector<Position>::const_iterator
							i = _waypoints.begin();
							i != _waypoints.end();
							++i, ++wpVal)
					{
						if (*i == posField) // note that 2 BL waypoints can be at the same Position.
						{
							if (offset_x == 2 && offset_y == 2)
							{
								sprite = _res->getSurfaceSet("TARGET.PCK")->getFrame(0); // was "CURSOR.PCK" frame= 7
								sprite->blitNShade(
										surface,
										posScreen.x,
										posScreen.y);
							}

							numWp->setValue(static_cast<unsigned>(wpVal));
							numWp->draw();
							numWp->blitNShade(
									surface,
									posScreen.x + offset_x,
									posScreen.y + offset_y);

							if (wpVal > 9)
								offset_x += 8;
							else
								offset_x += 6;

							if (offset_x > 25)
							{
								offset_x = 2;
								offset_y += 6;
							}
						}
					}
					// end waypoints.

// Draw Map's border-sprite only on ground tiles
					if (itZ == _battleSave->getGroundLevel()
						|| (itZ == 0 && _battleSave->getGroundLevel() == -1))
					{
						if (itX == 0
							|| itX == _battleSave->getMapSizeX() - 1
							|| itY == 0
							|| itY == _battleSave->getMapSizeY() - 1)
						{
							sprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(330); // gray square cross
							sprite->blitNShade(
									surface,
									posScreen.x + 14,
									posScreen.y + 31);
						}
					}
					// end border icon.

// Draw scanner dots
					if (itZ == viewLevel
						&& _battleSave->scannerDots().empty() == false)
					{
						const std::pair<int,int> dotTest = std::make_pair(
																		posField.x,
																		posField.y);
						if (std::find(
								_battleSave->scannerDots().begin(),
								_battleSave->scannerDots().end(),
								dotTest) != _battleSave->scannerDots().end())
						{
							sprite = _res->getSurfaceSet("SCANG.DAT")->getFrame(330); // gray square cross
							sprite->blitNShade(
									surface,
									posScreen.x + 14,
									posScreen.y + 30,
									0, false, 3); // red
						}
					}
					// end scanner dots.
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
		_unit = _battleSave->getSelectedUnit();
		if (_unit != NULL
			&& (_unit->getUnitStatus() == STATUS_STANDING
				|| _unit->getUnitStatus() == STATUS_TURNING)
			&& _unit->getPosition().z <= viewLevel)
		{
			_camera->convertMapToScreen(
									_unit->getPosition(),
									&posScreen);
			posScreen += _camera->getMapOffset();

			posScreen.y += getTerrainLevel(
										_unit->getPosition(),
										_unit->getArmor()->getSize());
			posScreen.y += 21 - _unit->getHeight();

			if (_unit->getArmor()->getSize() > 1)
			{
				posScreen.y += 10;
				if (_unit->getFloatHeight() != 0)
					posScreen.y -= _unit->getFloatHeight() + 1;
			}

			posScreen.x += _spriteWidth / 2;

			const int phaseCycle = static_cast<int>(4. * std::sin(22.5 / static_cast<double>(_animFrame + 1)));

			if (_unit->isKneeled() == true)
				_arrow_kneel->blitNShade(
								surface,
								posScreen.x - (_arrow_kneel->getWidth() / 2),
								posScreen.y - _arrow_kneel->getHeight() - 4 - phaseCycle);
			else
				_arrow->blitNShade(
								surface,
								posScreen.x - _arrow->getWidth() / 2,
								posScreen.y - _arrow->getHeight() + phaseCycle);
		}
	}
	// end arrow.

	if (pathPreviewed == true)
	{
		if (numWp != NULL) numWp->setBordered(); // make a border for the pathPreview display

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
					posField = Position(itX,itY,itZ);
					_camera->convertMapToScreen(posField, &posScreen);
					posScreen += _camera->getMapOffset();

					if (posScreen.x > -_spriteWidth // only render pathPreview inside the surface
						&& posScreen.x < surface->getWidth() + _spriteWidth
						&& posScreen.y > -_spriteHeight
						&& posScreen.y < surface->getHeight() + _spriteHeight)
					{
						_tile = _battleSave->getTile(posField);

						if (_tile != NULL
							&& _tile->isDiscovered(2) == true
							&& _tile->getPreviewDir() != -1)
						{
							int offset_y = -_tile->getTerrainLevel();

							if (_previewSetting & PATH_ARROWS)
							{
								const Tile* const tileBelow = _battleSave->getTile(posField + Position(0,0,-1));

								if (itZ > 0 && _tile->hasNoFloor(tileBelow) == true)
								{
									sprite = _res->getSurfaceSet("Pathfinding")->getFrame(23);
									if (sprite)
										sprite->blitNShade(
												surface,
												posScreen.x,
												posScreen.y + 2,
												0, false,
												_tile->getPreviewColor());
								}

								sprite = _res->getSurfaceSet("Pathfinding")->getFrame(_tile->getPreviewDir() + 12);
								if (sprite)
									sprite->blitNShade(
											surface,
											posScreen.x,
											posScreen.y - offset_y,
											0, false,
											_tile->getPreviewColor());
							}

							if ((_previewSetting & PATH_TU_COST) && _tile->getPreviewTu() > -1)
							{
								int offset_x = 2;
								if (_tile->getPreviewTu() > 9)
									offset_x += 2;

								if (_battleSave->getSelectedUnit() != NULL
									&& _battleSave->getSelectedUnit()->getArmor()->getSize() > 1)
								{
									offset_y += 1;
									if (!(_previewSetting & PATH_ARROWS))
										offset_y += 7;
								}

								numWp->setValue(static_cast<unsigned>(_tile->getPreviewTu()));
								numWp->draw();

								if (!(_previewSetting & PATH_ARROWS))
									numWp->blitNShade(
												surface,
												posScreen.x + 16 - offset_x,
												posScreen.y + 37 - offset_y,
												0, false,
												_tile->getPreviewColor());
								else
									numWp->blitNShade(
												surface,
												posScreen.x + 16 - offset_x,
												posScreen.y + 30 - offset_y);
							}
						}
					}
				}
			}
		}
	}

	delete numWp;
	// end Path Preview.

	if (_explosionInFOV == true) // check if hit or explosion animations
	{
/*		// big explosions cause the screen to flash as bright as possible before any explosions
		// are actually drawn. This causes everything to look like EGA for a single frame.
		if (_flashScreen == true)
		{
			Uint8 color;
			for (int x = 0, y = 0; x < surface->getWidth() && y < surface->getHeight();)
			{
//				surface->setPixelIterative(&x,&y, surface->getPixelColor(x,y) & 0xF0); // <- Volutar's, Lol good stuf.
				color = (surface->getPixelColor(x,y) / 16) * 16; // get the brightest color in each colorgroup.
				surface->setPixelIterative(&x,&y, color);
			}
		}
		else { */
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
					sprite = _res->getSurfaceSet("X1.PCK")->getFrame((*i)->getCurrentFrame());
					sprite->blitNShade(
							surface,
							bullet.x - 64,
							bullet.y - 64);
				}
				else if ((*i)->isHit() == 0) // bullet, http://ufopaedia.org/index.php?title=SMOKE.PCK
				{
					sprite = _res->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
					sprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 15);
				}
				else //if ((*i)->isHit() == 1) // melee or psiamp, http://ufopaedia.org/index.php?title=HIT.PCK
				{	 // put that back in to acknowledge -1 as a no-animation melee miss.
					sprite = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame());
					sprite->blitNShade(
							surface,
							bullet.x - 15,
							bullet.y - 25);

					if ((*i)->isHit() == 1) // temp kludge to show batman-type hit if melee is successful
					{
						sprite = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame() - 4);
						if (sprite)
							sprite->blitNShade(
									surface,
									bullet.x - 15,
									bullet.y - 25);
					}
				}
			}
		}
/*		} */
	}
	surface->unlock();
}

/**
 * Checks if a southwesterly wall should suppress unit-sprite drawing.
 * @note When a unit moves west it can clip through a previously drawn wall.
 * @param tile6		- pointer to the tile west of current
 * @param tile5		- pointer to the tile southwest of current
 * @param unit		- pointer to BattleUnit not '_unit' (default NULL)
 * @param halfRight	- pointer to store whether to draw halfRight only (used for large units) (default NULL)
 * @return, true to allow drawing the unit's sprite
 */
bool Map::checkWest( // private.
		const Tile* const tile6,
		const Tile* const tile5,
		const BattleUnit* unit,
		bool* halfRight) const
{
	bool ret;

	if (unit == NULL) // '_unit' is Valid.
	{
		ret = tile6 == NULL
		   || tile6->getUnit() != _unit;
	}
	else ret = false; // '_unit' is NOT Valid.

	ret = ret
		|| ((tile6->getMapData(O_OBJECT) == NULL
				|| tile6->getMapData(O_OBJECT)->getBigwall() != BIGWALL_SOUTH)
			&& (tile5 == NULL
				|| ((tile5->getMapData(O_NORTHWALL) == NULL
						|| tile5->isUfoDoorOpen(O_NORTHWALL) == true)
					&& (tile5->getMapData(O_OBJECT) == NULL
						|| (tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_BLOCK
							&& tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NESW
							&& tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_EAST)))));
//							&& tile5->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NORTH // not in UFO.

	if (ret == false) // unit might actually be too far away from wall to clip despite above conditions - now don't let the floor clip it
	{
		if (unit == NULL) unit = _unit;
		switch (unit->getUnitDirection())
		{
			case 1:
			case 2: ret = unit->getPosition() == unit->getStopPosition(); break;
			case 5:
			case 6: ret = unit->getPosition() == unit->getStartPosition();
		}

		if (halfRight != NULL && unit->getArmor()->getSize() == 2)
		{
			if (ret == true)
			{
				//if (dir != 2) *halfRight = true; // could allow this. Maybe !=1 also ...

				const Position pos (tile6->getPosition() + Position(1,0,0));
				const Tile
					* const tile (_battleSave->getTile(pos)),
					* const tileSouth (_battleSave->getTile(pos + Position(0,1,0)));
				if (!
					((tile->getMapData(O_OBJECT) == NULL
						|| tile->getMapData(O_OBJECT)->getBigwall() != BIGWALL_SOUTH)
					&& (tileSouth == NULL
						|| ((tileSouth->getMapData(O_NORTHWALL) == NULL
								|| tileSouth->isUfoDoorOpen(O_NORTHWALL) == true)
							&& (tileSouth->getMapData(O_OBJECT) == NULL
								|| (tileSouth->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NESW			// all that causes clipping when the large unit moves out eastward from along the northern side
									&& tileSouth->getMapData(O_OBJECT)->getBigwall() != BIGWALL_BLOCK))))))	// of an EW barrier but it's better than leaving a big hole in the 3rd quadrant as it moves out
				{
					*halfRight = true; // but only if a wall is directly south
				}
			}
			else if (unit->getUnitDirection() == 1 || unit->getUnitDirection() == 2)
			{
				*halfRight = true;
				ret = true;
			}
		}
	}

	return ret;
}

/**
 * Checks if a northeasterly wall should suppress unit-sprite drawing.
 * @note When a unit moves north it can clip through a previously drawn wall.
 * @param tile0	- pointer to the tile north of current
 * @param tile1	- pointer to the tile northeast of current
 * @param unit	- pointer to BattleUnit not '_unit' (default NULL)
 * @return, true to allow drawing the unit's sprite
 */
bool Map::checkNorth( // private.
		const Tile* const tile0,
		const Tile* const tile1,
		const BattleUnit* unit) const
{
	bool ret;

	if (unit == NULL) // '_unit' is Valid.
	{
		ret = tile0 == NULL
		   || tile0->getUnit() != _unit;
	}
	else ret = false; // '_unit' is NOT Valid.

	ret = ret
		|| ((tile0->getMapData(O_OBJECT) == NULL
				|| tile0->getMapData(O_OBJECT)->getBigwall() != BIGWALL_EAST)
			&& (tile1 == NULL
				|| ((tile1->getMapData(O_WESTWALL) == NULL
						|| tile1->isUfoDoorOpen(O_WESTWALL) == true)
					&& (tile1->getMapData(O_OBJECT) == NULL
						|| (tile1->getMapData(O_OBJECT)->getBigwall() != BIGWALL_NESW
							&& tile1->getMapData(O_OBJECT)->getBigwall() != BIGWALL_BLOCK)))));
//							&& tile1->getMapData(O_OBJECT)->getBigwall() != BIGWALL_WEST // not in UFO.

	if (ret == false) // unit might actually be too far away from wall to clip despite above conditions - now don't let the floor clip it
	{
		if (unit == NULL) unit = _unit;
		switch (unit->getUnitDirection())
		{
			case 0:
			case 1: ret = unit->getPosition() == unit->getStartPosition(); break;
			case 4:
			case 5: ret = unit->getPosition() == unit->getStopPosition();
		}
	}

	return ret;
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

	_mX = static_cast<int>(action->getAbsoluteXMouse());
	_mY = static_cast<int>(action->getAbsoluteYMouse());
	refreshSelectorPosition();
}

/**
 * Finds the current mouse position XY on this Map.
 * @param mousePos - reference the mouse position
 */
void Map::findMousePosition(Position& mousePos)
{
	mousePos.x = _mX;
	mousePos.y = _mY;
	mousePos.z = 0;
}

/**
 * Handles animating tiles - 8 Frames per animation [0..7].
// * @param redraw - true to redraw the battlescape map (default true)
 */
void Map::animateMap(bool redraw)
{
 	if (++_animFrame == 8) _animFrame = 0;

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
			(*i)->clearCache();
			cacheUnit(*i);
		}
	}

	if (--_fuseColor == 15) _fuseColor = 31;
	if (redraw == true) _redraw = true;
}

/*
 * Updates the selector to the last-known mouse position.
 *
void Map::refreshSelectorPosition()
{
	setSelectorPosition(_mX,_mY);
} */
/*
 * Sets the rectangular selector to a certain tile.
 * @param x - mouse x position
 * @param y - mouse y position
 *
void Map::setSelectorPosition(int x, int y)
{
	const int
		pre_X = _selectorX,
		pre_Y = _selectorY;
	_camera->convertScreenToMap(
							x,
							y + _spriteHeight / 4,
							&_selectorX,
							&_selectorY);
	if (pre_X != _selectorX || pre_Y != _selectorY)
		_redraw = true;
} */
/**
 * Sets the rectangular selector to the current mouse-position.
 */
void Map::refreshSelectorPosition()
{
	const int
		pre_X = _selectorX,
		pre_Y = _selectorY;

	_camera->convertScreenToMap(
							_mX,
							_mY + _spriteHeight / 4,
							&_selectorX,
							&_selectorY);

	if (pre_X != _selectorX || pre_Y != _selectorY)
		_redraw = true;
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
 * Gets if a Tile is a/the true location of current unit.
 * @param unit - pointer to a unit
 * @param tile - pointer to a tile
 * @return, true if true location
 */
bool Map::isTrueLoc(
		const BattleUnit* const unit,
		const Tile* const tile) const // private.
{
	if (unit->getTile() == tile
		|| (unit->getArmor()->getSize() == 2
			&& (tile->getPosition() + Position(-1,0,0) == unit->getPosition()
				|| tile->getPosition() + Position(0,-1,0) == unit->getPosition()
				|| tile->getPosition() + Position(-1,-1,0) == unit->getPosition())))
	{
		return true;
	}

	return false;
}

/**
 * Gets the unit's quadrant for drawing.
 * @param unit		- pointer to a unit
 * @param tile		- pointer to a tile
 * @param trueLoc	- true if real location; false if transient
 * @return, quadrant
 */
int Map::getQuadrant(
		const BattleUnit* const unit,
		const Tile* const tile,
		bool trueLoc) const // private.
{
/*	STATUS_STANDING,	//  0
	STATUS_WALKING,		//  1
	STATUS_FLYING,		//  2
	STATUS_TURNING,		//  3
	STATUS_AIMING,		//  4
	STATUS_COLLAPSING,	//  5
	STATUS_DEAD,		//  6
	STATUS_UNCONSCIOUS,	//  7
	STATUS_PANICKING,	//  8
	STATUS_BERSERK,		//  9
	STATUS_LIMBO,		// 10
	STATUS_DISABLED		// 11 */

	if (trueLoc == true //unit->getUnitStatus() == STATUS_STANDING ||
		|| unit->getVerticalDirection() != 0)
	{
		return tile->getPosition().x - unit->getPosition().x
			+ (tile->getPosition().y - unit->getPosition().y) * 2;
	}

	int dir = unit->getUnitDirection();
	if (unit->getPosition() == unit->getStopPosition())
		dir = (dir + 4) % 8;

	Position
		posUnit,
		posVect;
	Pathfinding::directionToVector(dir, &posVect);
	posUnit = unit->getPosition() + posVect;

	return tile->getPosition().x - posUnit.x
		+ (tile->getPosition().y - posUnit.y) * 2;
}

/**
 * Calculates the offset of a unit-sprite when it is moving between 2 tiles.
 * @param unit		- pointer to a BattleUnit
 * @param offset	- pointer to the Position offset that returns calculation
 * @param trueLoc	- true if real location; false if transient
 */
void Map::calculateWalkingOffset(
		const BattleUnit* const unit,
		Position* const offset,
		bool trueLoc) const // private.
{
	*offset = Position(0,0,0);

	//Log(LOG_INFO) << ". . Status = " << (int)unit->getUnitStatus();
	if (unit->getUnitStatus() == STATUS_WALKING
		|| unit->getUnitStatus() == STATUS_FLYING) // or STATUS_PANICKING
	{
		static const int
			offsetX[8] = {1, 1, 1, 0,-1,-1,-1, 0},
			offsetY[8] = {1, 0,-1,-1,-1, 0, 1, 1},

			offsetFalseX[8] = {16, 32, 16,  0,-16,-32,-16,  0}, // for duplicate drawing units from their transient
			offsetFalseY[8] = {-8,  0,  8, 16,  8,  0, -8,-16}, // destination & last positions. See UnitWalkBState.
			offsetFalseVert = 24;
		const int
			dir = unit->getUnitDirection(),			// 0..7
			dirVert = unit->getVerticalDirection(),	// 0= none, 8= up, 9= down
			walkPhase = unit->getTrueWalkPhase(),
			armorSize = unit->getArmor()->getSize();
		int
			halfPhase,
			fullPhase;
		unit->walkPhaseCutoffs(
							halfPhase,
							fullPhase);

		//Log(LOG_INFO) << unit->getId() << " " << unit->getPosition();
		//if (unit->getId() == 477)
		//{
		//	Log(LOG_INFO) << ". dir = " << dir << " dirVert = " << dirVert;
		//	Log(LOG_INFO) << ". trueLoc = " << (int)trueLoc;
		//	Log(LOG_INFO) << ". walkPhase = " << walkPhase;
			//Log(LOG_INFO) << ". halfPhase = " << halfPhase << " fullPhase = " << fullPhase;
		//}

		const bool start = (walkPhase < halfPhase);
		if (dirVert == 0)
		{
			if (start == true)
			{
				offset->x =  walkPhase * offsetX[dir] * 2 + ((trueLoc == true) ? 0 : -offsetFalseX[dir]);
				offset->y = -walkPhase * offsetY[dir]     + ((trueLoc == true) ? 0 : -offsetFalseY[dir]);
			}
			else
			{
				offset->x =  (walkPhase - fullPhase) * offsetX[dir] * 2 + ((trueLoc == true) ? 0 : offsetFalseX[dir]);
				offset->y = -(walkPhase - fullPhase) * offsetY[dir]     + ((trueLoc == true) ? 0 : offsetFalseY[dir]);
			}
		}

		// if unit is between tiles interpolate its terrain level (y-adjustment).
		const int
			posLastZ = unit->getStartPosition().z,
			posDestZ = unit->getStopPosition().z;
		int
			levelStart,
			levelEnd;

		if (start == true)
		{
			if (dirVert == Pathfinding::DIR_UP)
				offset->y += (trueLoc == true) ? 0 : offsetFalseVert;
			else if (dirVert == Pathfinding::DIR_DOWN)
				offset->y += (trueLoc == true) ? 0 : -offsetFalseVert;

			levelEnd = getTerrainLevel(
									unit->getStopPosition(),
									armorSize);

			if (posLastZ > posDestZ)		// going down a level so 'levelEnd' 0 becomes +24 (-8 becomes 16)
				levelEnd += 24 * (posLastZ - posDestZ);
			else if (posLastZ < posDestZ)	// going up a level so 'levelEnd' 0 becomes -24 (-8 becomes -16)
				levelEnd = -24 * (posDestZ - posLastZ) + std::abs(levelEnd);

			levelStart = getTerrainLevel(
									unit->getPosition(),
									armorSize);
			offset->y += ((levelStart * (fullPhase - walkPhase)) / fullPhase) + ((levelEnd * walkPhase) / fullPhase);
			if (trueLoc == false && dirVert == 0)
			{
				if (posLastZ > posDestZ)
					offset->y -= offsetFalseVert;
				else if (posLastZ < posDestZ)
					offset->y += offsetFalseVert;
			}
		}
		else
		{
			if (dirVert == Pathfinding::DIR_UP)
				offset->y += (trueLoc == true) ? 0 : -offsetFalseVert;
			else if (dirVert == Pathfinding::DIR_DOWN)
				offset->y += (trueLoc == true) ? 0 : offsetFalseVert;

			levelStart = getTerrainLevel(
									unit->getStartPosition(),
									armorSize);

			if (posLastZ > posDestZ)		// going down a level so 'levelStart' 0 becomes -24 (-8 becomes -32)
				levelStart -= 24 * (posLastZ - posDestZ);
			else if (posLastZ < posDestZ)	// going up a level so 'levelStart' 0 becomes +24 (-8 becomes 16)
				levelStart =  24 * (posDestZ - posLastZ) - std::abs(levelStart);

			levelEnd = getTerrainLevel(
									unit->getStopPosition(),
									armorSize);
			offset->y += ((levelStart * (fullPhase - walkPhase)) / fullPhase) + ((levelEnd * walkPhase) / fullPhase);

			if (trueLoc == false && dirVert == 0)
			{
				if (posLastZ > posDestZ)
					offset->y += offsetFalseVert;
				else if (posLastZ < posDestZ)
					offset->y -= offsetFalseVert;
			}
		}
	}
	else // standing.
	{
		offset->y += getTerrainLevel(
								unit->getPosition(),
								unit->getArmor()->getSize());

		if (unit->getUnitStatus() == STATUS_AIMING
			&& unit->getArmor()->getCanHoldWeapon() == true)
		{
			offset->x = -16; // it's maaaaaagic.
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
	int width;
	if (unit->getUnitStatus() == STATUS_AIMING)
		width = _spriteWidth * 2;
	else
		width = _spriteWidth;

	UnitSprite* const sprite = new UnitSprite(width, _spriteHeight);
	sprite->setPalette(this->getPalette());

	if (unit->getCacheInvalid() == true)
	{
		const int armorSize = unit->getArmor()->getSize() * unit->getArmor()->getSize();
		for (int
				quadrant = 0;
				quadrant != armorSize;
				++quadrant)
		{
			Surface* cache = unit->getCache(quadrant);
			if (cache == NULL)
			{
				cache = new Surface(_spriteWidth, _spriteHeight);
				cache->setPalette(this->getPalette());
			}

			cache->setWidth(sprite->getWidth());
			sprite->setBattleUnit(unit, quadrant);

			BattleItem
				* const rtItem = unit->getItem("STR_RIGHT_HAND"),
				* const ltItem = unit->getItem("STR_LEFT_HAND");
			if ((rtItem == NULL || rtItem->getRules()->isFixed() == true)
				&& (ltItem == NULL || ltItem->getRules()->isFixed() == true))
			{
				sprite->setBattleItem(NULL);
			}
			else
			{
				if (rtItem != NULL) sprite->setBattleItem(rtItem);
				if (ltItem != NULL) sprite->setBattleItem(ltItem);
			}

			sprite->setSurfaces(
							_res->getSurfaceSet(unit->getArmor()->getSpriteSheet()),
							_res->getSurfaceSet("HANDOB.PCK"),
							_res->getSurfaceSet("HANDOB2.PCK"));
			sprite->setAnimationFrame(_animFrame);
			cache->clear();
			sprite->blit(cache);

			unit->setCache(cache, quadrant);
		}
	}

	delete sprite;
}

/**
 * Puts a Projectile on this Map.
 * @param projectile - projectile to place (default NULL)
 */
void Map::setProjectile(Projectile* const projectile)
{
	_projectile = projectile;

	if (projectile != NULL
		&& Options::battleSmoothCamera == true)
	{
		_bulletStart = true;
	}
}

/**
 * Gets the current Projectile.
 * @return, pointer to Projectile or NULL
 */
Projectile* Map::getProjectile() const
{
	return _projectile;
}

/**
 * Gets a list of Explosions.
 * @return, list of explosions to display
 */
std::list<Explosion*>* Map::getExplosions()
{
	return &_explosions;
}

/**
 * Gets the Camera.
 * @return, pointer to Camera
 */
Camera* Map::getCamera()
{
	return _camera;
}

/**
 * Timers only work on surfaces so you have to pass this to the Camera.
 */
void Map::scrollMouse()
{
	_camera->scrollMouse();
}

/**
 * Timers only work on surfaces so you have to pass this to the Camera.
 */
void Map::scrollKey()
{
	_camera->scrollKey();
}

/**
 * Gets a list of waypoint-positions on this Map.
 * @return, pointer to a vector of positions
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
	setButtonPressed(btn, pressed);
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

	_playableHeight = height - _iconHeight;
	if (_playableHeight < 200)
		height = _playableHeight;
	else
		height = 200;

	_hiddenScreen->setHeight(height);
	_hiddenScreen->setY((_playableHeight - _hiddenScreen->getHeight()) / 2);
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
	_camera->convertMapToScreen(pos, &screenPos);

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

/**
 * Sets the "explosion flash" bool.
 * @param flash - true to render the screen in EGA this frame
 */
void Map::setBlastFlash(bool flash)
{
	_flashScreen = flash;
}

/**
 * Checks if the screen is still being rendered in EGA.
 * @return, true if still in EGA mode
 */
bool Map::getBlastFlash() const
{
	return _flashScreen;
}

/**
 * Sets whether to draw or not.
 * @param noDraw - true to stop this Map from drawing (default true)
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
void Map::showProjectile(bool show)
{
	_showProjectile = show;
}

}
