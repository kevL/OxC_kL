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

#include "Globe.h"

//#include <algorithm>
//#define _USE_MATH_DEFINES
//#include <cmath>
//#include "../fmath.h"

#include "GeoscapeState.h"

#include "../Engine/Action.h"
//#include "../Engine/FastLineClip.h"
//#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/LocalizedText.h" huh
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
//#include "../Engine/ShaderMove.h"
//#include "../Engine/ShaderRepeat.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Polygon.h"
#include "../Ruleset/Polyline.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCity.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

bool kL_reCenter = false;

const double
	Globe::ROTATE_LONGITUDE	= 0.176,
	Globe::ROTATE_LATITUDE	= 0.176;

Uint8 // these are only fallbacks for Geography.rul->globe
	Globe::CLO_LABELBASE	= 100,	// Palette::blockOffset(6)+4;	// stock 133;
	Globe::CLO_LABELCITY	= 167,	// Palette::blockOffset(10)+7;	// stock 138
	Globe::CLO_LABELCOUNTRY	= 227,	// Palette::blockOffset(14)+3;	// stock 239
	Globe::CLO_LINE			= 162,	// Palette::blockOffset(10)+2;	// light gray
//	Globe::CLO_RADAR1		=		// let base radars do its own thing in XuLine()
	Globe::CLO_RADAR2		= 150,	// Palette::blockOffset(9)+6;	// brown
	Globe::CLO_FLIGHT		= 166,	// Palette::blockOffset(10)+6;	// steel gray
	Globe::CLO_OCEAN		= 192;	// Palette::blockOffset(12),	// blue ofc.


namespace
{

/// A helper class/struct for drawing shadows & noise on the Globe.
struct GlobeStaticData
{

	/// array of shading gradient
	Sint16 shade_gradient[240];
	/// size of x & y of noise surface
	const int random_surf_size;

	/**
	 * Function returning normal vector of sphere surface.
	 * @param ox	- x cord of sphere center
	 * @param oy	- y cord of sphere center
	 * @param r		- radius of sphere
	 * @param x		- cord of point where we getting this vector
	 * @param y		- cord of point where we getting this vector
	 * @return, normal vector of sphere surface
	 */
	inline Cord circle_norm(
			double ox,
			double oy,
			double r,
			double x,
			double y)
	{
		const double
			limit = r * r,
			norm = 1. / r;

		Cord ret;
		ret.x = (x - ox);
		ret.y = (y - oy);

		const double temp = (ret.x) * (ret.x) + (ret.y) * (ret.y);
		if (limit > temp)
		{
			ret.x *= norm;
			ret.y *= norm;
			ret.z = std::sqrt(limit - temp) * norm;

			return ret;
		}
		else
		{
			ret.x =
			ret.y =
			ret.z = 0.;

			return ret;
		}
	}

	/// initialization
	GlobeStaticData()
		:
			random_surf_size(60)
	{
		// filling terminator gradient LUT
		for (int
				i = 0;
				i != 240;
				++i)
		{
			int j = i - 120;

			if		(j < -66) j = -16;
			else if (j < -48) j = -15;
			else if (j < -33) j = -14;
			else if (j < -22) j = -13;
			else if (j < -15) j = -12;
			else if (j < -11) j = -11;
			else if (j <  -9) j = -10;

			if		(j > 120) j =  19;
			else if (j >  98) j =  18;
			else if (j >  86) j =  17;
			else if (j >  74) j =  16;
			else if (j >  54) j =  15;
			else if (j >  38) j =  14;
			else if (j >  26) j =  13;
			else if (j >  18) j =  12;
			else if (j >  13) j =  11;
			else if (j >  10) j =  10;
			else if (j >   8) j =   9;

			shade_gradient[static_cast<size_t>(i)] = static_cast<Sint16>(j) + 16;
		}
	}
};

GlobeStaticData static_data;


struct Ocean
{
	///
	static inline void func(
			Uint8& dest,
			const int&, // whots this
			const int&, // whots this
			const int&, // whots this
			const int&) // whots this
	{
		dest = Globe::CLO_OCEAN;
	}
};


struct CreateShadow
{
	///
	static inline Uint8 getShadowValue(
			const Uint8& dest,
			const Cord& earth,
			const Cord& sun,
			const Sint16& noise)
	{
		Cord temp = earth;
		temp -= sun; // diff
		temp.x *= temp.x; // norm
		temp.y *= temp.y;
		temp.z *= temp.z;
		temp.x += temp.z + temp.y;
		// we have norm of distance between 2 vectors, now stored in 'x'

		temp.x -= 2.;
		temp.x *= 125.;

		if (temp.x < -110.)
			temp.x = -31.;
		else if (temp.x > 120.)
			temp.x = 50.;
		else
			temp.x = static_cast<double>(static_data.shade_gradient[static_cast<Sint16>(temp.x) + 120]);

		temp.x -= static_cast<double>(noise);

		if (temp.x > 0.)
		{
			const Uint8 d = (dest & helper::ColorGroup);
			Uint8 val;

			if (temp.x > 31.)
				val = 31;
			else
				val = static_cast<Uint8>(temp.x);

			if (   d == Globe::CLO_OCEAN
				|| d == Globe::CLO_OCEAN + 16)
			{
				return Globe::CLO_OCEAN + val; // this pixel is ocean
			}
			else
			{
				if (dest == 0)
					return val; // this pixel is land

				const Uint8 e = dest + (val / 3);
				if (e > (d + helper::ColorShade))
					return d + helper::ColorShade;

				return static_cast<Uint8>(e);
			}
		}
		else
		{
			const Uint8 d = (dest & helper::ColorGroup);
			if (d == Globe::CLO_OCEAN
				|| d == Globe::CLO_OCEAN + 16)
			{
				return Globe::CLO_OCEAN; // this pixel is ocean
			}

			return dest; // this pixel is land
		}
	}

	///
	static inline void func(
			Uint8& dest,
			const Cord& earth,
			const Cord& sun,
			const Sint16& noise,
			const int&) // whots this
	{
		if (dest != 0
			&& AreSame(earth.z, 0.) == false)
		{
			dest = getShadowValue(
								dest,
								earth,
								sun,
								noise);
		}
		else
			dest = 0;
	}
};

}


/**
 * Sets up a globe with the specified size and position.
 * @param game		- pointer to the core Game
 * @param cenX		- X position of the center of the globe
 * @param cenY		- Y position of the center of the globe
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
Globe::Globe(
		Game* game,
		int cenX,
		int cenY,
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_rotLon(0.),
		_rotLat(0.),
		_hoverLon(0.),
		_hoverLat(0.),
		_cenX(static_cast<Sint16>(cenX)),
		_cenY(static_cast<Sint16>(cenY)),
		_game(game),
		_rules(game->getRuleset()->getGlobe()),
		_hover(false),
		_blink(-1),
//		_blinkVal(0),
		_isMouseScrolled(false),
		_isMouseScrolling(false),
		_mouseOverThreshold(false),
		_mouseScrollingStartTime(0),
		_xBeforeMouseScrolling(0),
		_yBeforeMouseScrolling(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_lonBeforeMouseScrolling(0.),
		_latBeforeMouseScrolling(0.),
		_radius(0.),
		_radiusStep(0.),
		_debugType(0)
{
	_texture	= new SurfaceSet(*_game->getResourcePack()->getSurfaceSet("TEXTURE.DAT"));
	_markerSet	= new SurfaceSet(*_game->getResourcePack()->getSurfaceSet("GlobeMarkers"));

	_countries	= new Surface(width, height, x, y);
	_markers	= new Surface(width, height, x, y);
	_radars		= new Surface(width, height, x, y);
	_clipper	= new FastLineClip(x, x + width, y, y + height);

	// Animation timers
	_blinkTimer = new Timer(200);
	_blinkTimer->onTimer((SurfaceHandler)& Globe::blink);
	_blinkTimer->start();

//	_rotTimer = new Timer(10);
	_rotTimer = new Timer(Options::geoScrollSpeed); // old.
	_rotTimer->onTimer((SurfaceHandler)& Globe::rotate);

// kL_begin:
	// Globe markers
/*	_mkXcomBase = new Surface(3, 3);
	_mkXcomBase->lock();
	_mkXcomBase->setPixelColor(0, 0, 9); // cyan border
	_mkXcomBase->setPixelColor(1, 0, 9);
	_mkXcomBase->setPixelColor(2, 0, 9);
	_mkXcomBase->setPixelColor(0, 1, 9);
	_mkXcomBase->setPixelColor(2, 1, 9);
	_mkXcomBase->setPixelColor(0, 2, 9);
	_mkXcomBase->setPixelColor(1, 2, 9);
	_mkXcomBase->setPixelColor(2, 2, 9);
	_mkXcomBase->unlock();

	_mkAlienBase = new Surface(3, 3);
	_mkAlienBase->lock();
	_mkAlienBase->setPixelColor(0, 0, 1); // pink border
	_mkAlienBase->setPixelColor(1, 0, 1);
	_mkAlienBase->setPixelColor(2, 0, 1);
	_mkAlienBase->setPixelColor(0, 1, 1);
	_mkAlienBase->setPixelColor(2, 1, 1);
	_mkAlienBase->setPixelColor(0, 2, 1);
	_mkAlienBase->setPixelColor(1, 2, 1);
	_mkAlienBase->setPixelColor(2, 2, 1);
	_mkAlienBase->unlock();

	_mkCraft = new Surface(3, 3);
	_mkCraft->lock();
	_mkCraft->setPixelColor(1, 0, 11); // yellow +
	_mkCraft->setPixelColor(0, 1, 11);
	_mkCraft->setPixelColor(2, 1, 11);
	_mkCraft->setPixelColor(1, 2, 11);
	_mkCraft->unlock();

	_mkWaypoint = new Surface(3, 3);
	_mkWaypoint->lock();
	_mkWaypoint->setPixelColor(0, 0, 3); // orange x
	_mkWaypoint->setPixelColor(0, 2, 3);
	_mkWaypoint->setPixelColor(1, 1, 3);
	_mkWaypoint->setPixelColor(2, 0, 3);
	_mkWaypoint->setPixelColor(2, 2, 3);
	_mkWaypoint->unlock();

	_mkCity = new Surface(3, 3);
	_mkCity->lock();
	_mkCity->setPixelColor(0, 0, 172); // (10)+12 dark slate gray (as per city labels)
	_mkCity->setPixelColor(1, 0, 172);
	_mkCity->setPixelColor(2, 0, 172);
	_mkCity->setPixelColor(0, 1, 172);
	_mkCity->setPixelColor(1, 1, 14); // red center
	_mkCity->setPixelColor(2, 1, 172);
	_mkCity->setPixelColor(0, 2, 172);
	_mkCity->setPixelColor(1, 2, 172);
	_mkCity->setPixelColor(2, 2, 172);
	_mkCity->unlock();

	_mkFlyingUfo = new Surface(3, 3);
	_mkFlyingUfo->lock();
	_mkFlyingUfo->setPixelColor(1, 0, 13); // red +
	_mkFlyingUfo->setPixelColor(0, 1, 13);
	_mkFlyingUfo->setPixelColor(1, 1, 13);
	_mkFlyingUfo->setPixelColor(2, 1, 13);
	_mkFlyingUfo->setPixelColor(1, 2, 13);
	_mkFlyingUfo->unlock();

	_mkLandedUfo = new Surface(3, 3);
	_mkLandedUfo->lock();
	_mkLandedUfo->setPixelColor(0, 0, 7); // green x
	_mkLandedUfo->setPixelColor(0, 2, 7);
	_mkLandedUfo->setPixelColor(1, 1, 7);
	_mkLandedUfo->setPixelColor(2, 0, 7);
	_mkLandedUfo->setPixelColor(2, 2, 7);
	_mkLandedUfo->unlock();

	_mkCrashedUfo = new Surface(3, 3);
	_mkCrashedUfo->lock();
	_mkCrashedUfo->setPixelColor(0, 0, 5); // white x
	_mkCrashedUfo->setPixelColor(0, 2, 5);
	_mkCrashedUfo->setPixelColor(1, 1, 5);
	_mkCrashedUfo->setPixelColor(2, 0, 5);
	_mkCrashedUfo->setPixelColor(2, 2, 5);
	_mkCrashedUfo->unlock();

	_mkAlienSite = new Surface(3, 3);
	_mkAlienSite->lock();
	_mkAlienSite->setPixelColor(1, 0, 1); // pink +
	_mkAlienSite->setPixelColor(0, 1, 1);
	_mkAlienSite->setPixelColor(1, 1, 1);
	_mkAlienSite->setPixelColor(2, 1, 1);
	_mkAlienSite->setPixelColor(1, 2, 1);
	_mkAlienSite->unlock(); */
//kL_end.

	_cenLon = _game->getSavedGame()->getGlobeLongitude();
	_cenLat = _game->getSavedGame()->getGlobeLatitude();

	_zoom = _game->getSavedGame()->getGlobeZoom();
	setupRadii(
			width,
			height);
	setZoom(_zoom);

	// fill random noise "texture"
	_randomNoiseData.resize(static_data.random_surf_size * static_data.random_surf_size);
	for (size_t
			i = 0;
			i != _randomNoiseData.size();
			++i)
	{
		_randomNoiseData[i] = rand() %4;
	}

	cachePolygons();
}

/**
 * Deletes the contained surfaces.
 */
Globe::~Globe()
{
	delete _texture;
//	delete _markerSet; // (they don't actually delete this.. it's been commented in stock code too.)

	delete _blinkTimer;
	delete _rotTimer;
	delete _countries;
	delete _markers;
	delete _radars;
	delete _clipper;

	for (std::list<Polygon*>::const_iterator
			i = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Converts a polar point into a cartesian point for
 * mapping a polygon onto the 3D-looking globe.
 * @param lon	- longitude of the polar point
 * @param lat	- latitude of the polar point
 * @param x		- pointer to the output X position
 * @param y		- pointer to the output Y position
 */
void Globe::polarToCart( // Orthographic projection
		double lon,
		double lat,
		Sint16* x,
		Sint16* y) const
{
	*x = _cenX + static_cast<Sint16>(std::floor(_radius * std::cos(lat) * std::sin(lon - _cenLon)));
	*y = _cenY + static_cast<Sint16>(std::floor(_radius * (std::cos(_cenLat) * std::sin(lat)
												- std::sin(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon))));
}

/**
 *
 */
void Globe::polarToCart( // Orthographic projection
		double lon,
		double lat,
		double* x,
		double* y) const
{
	*x = _cenX + static_cast<Sint16>(_radius * std::cos(lat) * std::sin(lon - _cenLon));
	*y = _cenY + static_cast<Sint16>(_radius * (std::cos(_cenLat) * std::sin(lat)
									- std::sin(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon)));
}

/**
 * Converts a cartesian point into a polar point for
 * mapping a globe click onto the flat world map.
 * @param x		- X position of the cartesian point
 * @param y		- Y position of the cartesian point
 * @param lon	- pointer to the output longitude
 * @param lat	- pointer to the output latitude
 */
void Globe::cartToPolar( // Orthographic Projection
		Sint16 x,
		Sint16 y,
		double* lon,
		double* lat) const
{
	x -= _cenX;
	y -= _cenY;

	const double
		rho = std::sqrt(static_cast<double>(x * x + y * y)),
		c = std::asin(rho / static_cast<double>(_radius));

	if (AreSame(rho, 0.))
	{
		*lat = _cenLat;
		*lon = _cenLon;
	}
	else
	{
		*lat = std::asin((y * std::sin(c) * std::cos(_cenLat)) / rho + std::cos(c) * std::sin(_cenLat));
		*lon = std::atan2(
						x * std::sin(c),
						(rho * std::cos(_cenLat) * std::cos(c) - y * std::sin(_cenLat) * std::sin(c)))
					+ _cenLon;
	}

	while (*lon < 0.) // keep between 0 and 2xPI
		*lon += 2. * M_PI;
	while (*lon > 2. * M_PI)
		*lon -= 2. * M_PI;
}

/**
 * Checks if a polar point is on the back-half of the globe, invisible to the player.
 * @param lon - longitude of the point
 * @param lat - latitude of the point
 * @return, true if it's on the back, false if it's on the front
 */
bool Globe::pointBack(
		double lon,
		double lat) const
{
	const double c = std::cos(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon)
				   + std::sin(_cenLat) * std::sin(lat);

	return (c < 0.);
}


/**
 * Returns latitude of last visible to player point on given longitude.
 * @param lon - longitude of the point
 * @return, longitude of last visible point
 */
double Globe::lastVisibleLat(double lon) const
{
//	double c = cos(_cenLat) * cos(lat) * cos(lon - _cenLon) + sin(_cenLat) * sin(lat);
//	tan(lat) = -cos(_cenLat) * cos(lon - _cenLon)/sin(_cenLat);

	return std::atan(-std::cos(_cenLat) * std::cos(lon - _cenLon) / std::sin(_cenLat));
}

/**
 * Checks if a polar point is inside a certain polygon.
 * @param lon	- longitude of the point
 * @param lat	- latitude of the point
 * @param poly	- pointer to the Polygon
 * @return, true if it's inside
 */
bool Globe::insidePolygon(
		double lon,
		double lat,
		Polygon* poly) const
{
	bool backFace = true;

	for (size_t
			i = 0;
			i != poly->getPoints();
			++i)
	{
		backFace = backFace == true
				&& pointBack(
							poly->getLongitude(i),
							poly->getLatitude(i)) == true;
	}

	if (backFace != pointBack(lon, lat))
		return false;


	bool odd = false;

	for (size_t
			i = 0;
			i != poly->getPoints();
			++i)
	{
		const size_t j = (i + 1) %poly->getPoints();

//		double x = lon, y = lat, x_i = poly->getLongitude(i), y_i = poly->getLatitude(i), x_j = poly->getLongitude(j), y_j = poly->getLatitude(j);
		double
			x,y,
			x_i,x_j,
			y_i,y_j;

		polarToCart(
				poly->getLongitude(i),
				poly->getLatitude(i),
				&x_i,
				&y_i);
		polarToCart(
				poly->getLongitude(j),
				poly->getLatitude(j),
				&x_j,
				&y_j);
		polarToCart(
				lon,
				lat,
				&x,
				&y);

		if (((y_i < y
					&& y_j >= y)
				|| (y_j < y
					&& y_i >= y))
			&& (x_i <= x
				|| x_j <= x))
		{
			odd ^= (x_i + (y - y_i) / (y_j - y_i) * (x_j - x_i) < x);
		}
	}

	return odd;
}

/**
 * Sets a leftwards rotation speed and starts the timer.
 */
void Globe::rotateLeft()
{
	_rotLon = -ROTATE_LONGITUDE;

	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Sets a rightwards rotation speed and starts the timer.
 */
void Globe::rotateRight()
{
	_rotLon = ROTATE_LONGITUDE;

	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Sets a upwards rotation speed and starts the timer.
 */
void Globe::rotateUp()
{
	_rotLat = -ROTATE_LATITUDE;

	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Sets a downwards rotation speed and starts the timer.
 */
void Globe::rotateDown()
{
	_rotLat = ROTATE_LATITUDE;

	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Resets the rotation speed and timer.
 */
void Globe::rotateStop()
{
	_rotLon =
	_rotLat = 0.;

	_rotTimer->stop();
}

/**
 * Resets longitude rotation speed and timer.
 */
void Globe::rotateStopLon()
{
	_rotLon = 0.;

	if (AreSame(_rotLat, 0.))
		_rotTimer->stop();
}

/**
 * Resets latitude rotation speed and timer.
 */
void Globe::rotateStopLat()
{
	_rotLat = 0.;

	if (AreSame(_rotLon, 0.))
		_rotTimer->stop();
}

/**
 * Sets up the radius of earth at various zoom levels.
 * @param width		- the new width of the globe
 * @param height	- the new height of the globe
 */
void Globe::setupRadii( // private.
		int width,
		int height)
{
	_zoomRadii.clear();

	// These are the globe-zoom magnifications stored as a <vector> of 6 (doubles).
	_zoomRadii.push_back(0.47 * static_cast<double>(height)); // 0 - Zoomed all out	// no detail
	_zoomRadii.push_back(0.60 * static_cast<double>(height)); // 1					// country borders
	_zoomRadii.push_back(0.85 * static_cast<double>(height)); // 2					// country labels
	_zoomRadii.push_back(1.39 * static_cast<double>(height)); // 3					// city markers
	_zoomRadii.push_back(2.13 * static_cast<double>(height)); // 4					// city labels & all detail
	_zoomRadii.push_back(3.42 * static_cast<double>(height)); // 5 - Zoomed all in

//	_zoomRadii.push_back(0.45 * height);
//	_zoomRadii.push_back(0.60 * height);
//	_zoomRadii.push_back(0.90 * height);
//	_zoomRadii.push_back(1.40 * height);
//	_zoomRadii.push_back(2.25 * height);
//	_zoomRadii.push_back(3.60 * height);

	_radius = _zoomRadii[_zoom];
//	_radiusStep = (_zoomRadii[DOGFIGHT_ZOOM] - _zoomRadii[0]) / 10.0;
	_radiusStep = (_zoomRadii[_zoomRadii.size() - 1] - _zoomRadii[0]) / 10.; // kL

	_earthData.resize(_zoomRadii.size());

	for (size_t // filling normal field for each radius
			rad = 0;
			rad != _zoomRadii.size();
			++rad)
	{
		_earthData[rad].resize(width * height);

		for (size_t
				j = 0;
				j != static_cast<size_t>(height);
				++j)
		{
			for (size_t
					i = 0;
					i != static_cast<size_t>(width);
					++i)
			{
				_earthData[rad]
						  [static_cast<size_t>(width) * j + i] = static_data.circle_norm(
																					static_cast<double>(width) / 2.,
																					static_cast<double>(height) / 2.,
																					_zoomRadii[rad],
																					static_cast<double>(i) + 0.5,
																					static_cast<double>(j) + 0.5);
			}
		}
	}
}

/**
 * Changes the current globe zoom factor.
 * @param zoom - the new zoom level
 */
void Globe::setZoom(size_t zoom) // private.
{
	_zoom = std::min(
				_zoomRadii.size() - 1,
				std::max(
						static_cast<size_t>(0), // go f*cking figure.
						zoom));

	_zoomTexture = (2 - static_cast<size_t>(std::floor(static_cast<double>(_zoom) / 2.)))
				 * (_texture->getTotalFrames() / 3);

	_radius = _zoomRadii[_zoom];
	_game->getSavedGame()->setGlobeZoom(_zoom);

	if (_isMouseScrolling == true)
	{
		_lonBeforeMouseScrolling = _cenLon;
		_latBeforeMouseScrolling = _cenLat;
		_totalMouseMoveX =
		_totalMouseMoveY = 0;
	}

	invalidate();
}

/**
 * Gets the Globe's current zoom level.
 * @return, zoom level
 */
size_t Globe::getZoom() const
{
	return _zoom;
}

/**
 * Gets the number of zoom levels available.
 * @return, number of zoom levels
 */
size_t Globe::getZoomLevels() const
{
	return _zoomRadii.size();
}

/**
 * Increases the zoom level on the globe.
 */
void Globe::zoomIn()
{
	if (_zoom < _zoomRadii.size() - 1)
		setZoom(_zoom + 1);
}

/**
 * Decreases the zoom level on the globe.
 */
void Globe::zoomOut()
{
	if (_zoom > 0)
		setZoom(_zoom - 1);
}

/**
 * Zooms the globe out as far as possible.
 */
/* void Globe::zoomMin()
{
	if (_zoom > 0)
		setZoom(0);
} */

/**
 * Zooms the globe in as close as possible.
 */
/* void Globe::zoomMax()
{
	if (_zoom < _zoomRadii.size() - 1)
		setZoom(_zoomRadii.size() - 1);
} */

/**
 * Zooms the globe smoothly into dogfight level.
 * @return, true if the globe has finished zooming in
 */
bool Globe::zoomDogfightIn()
{
	const size_t dfZoom = _zoomRadii.size() - 1;

	if (_zoom < dfZoom)
	{
		const double radius = _radius;

		if (radius + _radiusStep >= _zoomRadii[dfZoom])
			setZoom(dfZoom);
		else
		{
			if (radius + _radiusStep >= _zoomRadii[_zoom + 1])
				++_zoom;

			setZoom(_zoom);
			_radius = radius + _radiusStep;
		}

		return false;
	}

	return true;
}

/**
 * Zooms the globe smoothly out of dogfight level.
 * @return, true if the globe has finished zooming out
 */
bool Globe::zoomDogfightOut()
{
	const size_t preDfZoom = _game->getSavedGame()->getDfZoom();

	if (_zoom > preDfZoom)
	{
		const double radius = _radius;

		if (radius - _radiusStep <= _zoomRadii[preDfZoom])
			setZoom(preDfZoom);
		else
		{
			if (radius - _radiusStep <= _zoomRadii[_zoom - 1])
				--_zoom;

			setZoom(_zoom);
			_radius = radius - _radiusStep;
		}

		return false;
	}

	return true;
}

/**
 * Rotates the globe to center on a certain polar point on the world map.
 * @param lon - longitude of the point
 * @param lat - latitude of the point
 */
void Globe::center(
		double lon,
		double lat)
{
	_game->getSavedGame()->setGlobeLongitude(_cenLon = lon);
	_game->getSavedGame()->setGlobeLatitude(_cenLat = lat);

	invalidate();
}

/**
 * Checks if a polar point is inside the globe's landmass.
 * @param lon - longitude of the point
 * @param lat - latitude of the point
 * @return, true if point is over land
 */
bool Globe::insideLand(
		double lon,
		double lat) const
{
	// This only temporarily changes cenLon/cenLat so the "const" is actually preserved.
/*	Globe* const globe = const_cast<Globe* const>(this); // WARNING: BAD CODING PRACTICE < read: normal c++ practice!

	double
		oldLon = _cenLon,
		oldLat = _cenLat;
	globe->_cenLon = lon;
	globe->_cenLat = lat; */

//	bool ret = false;
	for (std::list<Polygon*>::const_iterator
			i = _rules->getPolygons()->begin();
			i != _rules->getPolygons()->end();
//				&& ret == false;
			++i)
	{
		if (insidePolygon(
						lon,
						lat,
						*i) == true)
		{
			return true;
		}
//		ret = insidePolygon(lon, lat, *i);
	}

/*	globe->_cenLon = oldLon;
	globe->_cenLat = oldLat; */

//	return ret;
	return false;
}

/**
 * Switches the amount of detail shown on the globe.
 * With detail on, country and city details are shown when zoomed in.
 */
void Globe::toggleDetail()
{
	Options::globeDetail = !Options::globeDetail;
	drawDetail();
}

/**
 * Checks if a certain target is near a certain cartesian point
 * (within a circled area around it) over the globe.
 * @param target	- pointer to Target
 * @param x			- X coordinate of point
 * @param y			- Y coordinate of point
 * @return, true if it's near
 */
bool Globe::targetNear(
		Target* target,
		int x,
		int y) const
{
	if (pointBack(
				target->getLongitude(),
				target->getLatitude()))
	{
		return false;
	}

	Sint16
		tx,
		ty;

	polarToCart(
			target->getLongitude(),
			target->getLatitude(),
			&tx,
			&ty);

	int
		dx = x - tx,
		dy = y - ty;

	return (dx * dx + dy * dy <= NEAR_RADIUS);
}

/**
 * Returns a list of all the targets currently near a certain cartesian point over the globe.
 * @param x		- X coordinate of point
 * @param y		- Y coordinate of point
 * @param craft	- true to get targets for Craft only
 * @return, vector of pointers to Targets
 */
std::vector<Target*> Globe::getTargets(
		int x,
		int y,
		bool craftOnly) const
{
	std::vector<Target*> targets;

	if (craftOnly == false)
	{
		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			if (   AreSame((*i)->getLongitude(), 0.)
				&& AreSame((*i)->getLatitude(), 0.))
			{
				continue;
			}

			if (targetNear(*i, x,y))
				targets.push_back(*i);

			for (std::vector<Craft*>::const_iterator
					j = (*i)->getCrafts()->begin();
					j != (*i)->getCrafts()->end();
					++j)
			{
				if ((*j)->getDestination() == 0
					&& AreSame((*j)->getLongitude(), (*i)->getLongitude())
					&& AreSame((*j)->getLatitude(), (*i)->getLatitude()))
				{
					continue;
				}

				if (targetNear(*j, x,y))
					targets.push_back(*j);
			}
		}
	}

	for (std::vector<Ufo*>::const_iterator // get UFOs
			i = _game->getSavedGame()->getUfos()->begin();
			i != _game->getSavedGame()->getUfos()->end();
			++i)
	{
		if ((*i)->getDetected() == true)
		{
			if ((*i)->reachedDestination() == true // kL->
				&& (*i)->getAlienMission()->getRules().getObjective() == OBJECTIVE_SITE)
//				&& (*i)->getMissionType() == "STR_ALIEN_TERROR")
			{
				// kL_note: this is a kludge; the UFO should be / have been deleted before
				// invoking SelectDestinationState or MultipleTargetsState.
				// ** see: GeoscapeState::time5Seconds(), case Ufo::FLYING **
				// Under certain circumstances (i forget) player can target, or be
				// offered to target, a UFO that is effectively already a MissionSite,
				// which then immediately causes the Craft to go back to base or bleh.
				continue;
			}

			if (targetNear(*i, x,y) == true)
				targets.push_back(*i);
		}
	}

	for (std::vector<Waypoint*>::const_iterator // get Waypoints
			i = _game->getSavedGame()->getWaypoints()->begin();
			i != _game->getSavedGame()->getWaypoints()->end();
			++i)
	{
		if (targetNear(*i, x,y) == true)
			targets.push_back(*i);
	}

	for (std::vector<MissionSite*>::const_iterator // get missionSites
			i = _game->getSavedGame()->getMissionSites()->begin();
			i != _game->getSavedGame()->getMissionSites()->end();
			++i)
	{
		if (targetNear(*i, x,y) == true)
			targets.push_back(*i);
	}

	for (std::vector<AlienBase*>::const_iterator // get aLienBases
			i = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
			++i)
	{
		if ((*i)->isDiscovered() == true
			&& targetNear(*i, x,y) == true)
		{
			targets.push_back(*i);
		}
	}

	return targets;
}

/**
 * Takes care of pre-calculating all the polygons currently visible
 * on the globe and caching them so they only need to be recalculated
 * when the globe is actually moved.
 */
void Globe::cachePolygons()
{
	cache(
		_rules->getPolygons(),
		&_cacheLand);
}

/**
 * Caches a set of polygons.
 * @param polygons	- pointer to list of polygons
 * @param cache		- pointer to cache list
 */
void Globe::cache(
		std::list<Polygon*>* polygons,
		std::list<Polygon*>* cache)
{
	for (std::list<Polygon*>::const_iterator // Clear existing cache
			i = cache->begin();
			i != cache->end();
			++i)
	{
		delete *i;
	}

	cache->clear();

	for (std::list<Polygon*>::const_iterator // Pre-calculate values to cache
			i = polygons->begin();
			i != polygons->end();
			++i)
	{
		double
			closest = 0., // Is quad on the back face?
			furthest = 0.,
			z;

		for (size_t
				j = 0;
				j != (*i)->getPoints();
				++j)
		{
			z = std::cos(_cenLat) * std::cos((*i)->getLatitude(j)) * std::cos((*i)->getLongitude(j) - _cenLon)
			  + std::sin(_cenLat) * std::sin((*i)->getLatitude(j));

			if (z > closest)
				closest = z;
			else if (z < furthest)
				furthest = z;
		}

		if (-furthest > closest)
			continue;

		Polygon* const poly = new Polygon(**i);

		for (size_t // Convert coordinates
				j = 0;
				j != poly->getPoints();
				++j)
		{
			Sint16
				x,y;
			polarToCart(
					poly->getLongitude(j),
					poly->getLatitude(j),
					&x,&y);
			poly->setX(j,x);
			poly->setY(j,y);
		}

		cache->push_back(poly);
	}
}

/**
 * Replaces a certain amount of colors in the palette of the globe.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void Globe::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_texture->setPalette(colors, firstcolor, ncolors);
	_markerSet->setPalette(colors, firstcolor, ncolors);

	_countries->setPalette(colors, firstcolor, ncolors);
	_markers->setPalette(colors, firstcolor, ncolors);
	_radars->setPalette(colors, firstcolor, ncolors);
}

/**
 * Keeps the animation & rotation timers running.
 */
void Globe::think()
{
	if (kL_reCenter == true)
	{
		kL_reCenter = false;
		center(
			_game->getSavedGame()->getGlobeLongitude(),
			_game->getSavedGame()->getGlobeLatitude());
	}

	_blinkTimer->think(NULL, this);
	_rotTimer->think(NULL, this);
}

/**
 * Makes the globe markers blink.
 */
void Globe::blink()
{
	_blink = -_blink;

	for (size_t
			i = 0;
			i != _markerSet->getTotalFrames();
			++i)
	{
		if (i != GLM_CITY)
			_markerSet->getFrame(static_cast<int>(i))->offset(_blink);
	}

	drawMarkers();
}

/**
 * Rotates the globe by a set amount. Necessary since the
 * globe keeps rotating while a button is pressed down.
 */
void Globe::rotate()
{
	_cenLon += _rotLon * (static_cast<double>(110 - Options::geoScrollSpeed) / 100.) / static_cast<double>(_zoom + 1);
	_cenLat += _rotLat * (static_cast<double>(110 - Options::geoScrollSpeed) / 100.) / static_cast<double>(_zoom + 1);

	_game->getSavedGame()->setGlobeLongitude(_cenLon);
	_game->getSavedGame()->setGlobeLatitude(_cenLat);

	invalidate();
}

/**
 * Draws the whole globe part by part.
 */
void Globe::draw()
{
	if (_redraw == true)
		cachePolygons();

	Surface::draw();

	drawOcean();
	drawLand();
	drawRadars();
	drawShadow();
	drawMarkers();
	drawDetail();
	drawFlights();
}

/**
 * Renders the ocean shading it according to the time of day.
 */
void Globe::drawOcean()
{
	lock();
	drawCircle(
			_cenX + 1,
			_cenY,
			static_cast<Sint16>(_radius) + 20,
			CLO_OCEAN);
//	ShaderDraw<Ocean>(ShaderSurface(this));
	unlock();
}

/**
 * Renders the land taking all the visible world polygons
 * and texturing and shading them accordingly.
 */
void Globe::drawLand()
{
	Sint16
		x[4],
		y[4];

	for (std::list<Polygon*>::const_iterator
			i = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
	{
		for (size_t // Convert coordinates
				j = 0;
				j != (*i)->getPoints();
				++j)
		{
			x[j] = (*i)->getX(j);
			y[j] = (*i)->getY(j);
		}

		drawTexturedPolygon( // Apply textures according to zoom and shade
						x,y,
						(*i)->getPoints(),
						_texture->getFrame(static_cast<int>((*i)->getPolyTexture() + _zoomTexture)),
						0,0);
	}
}

/**
 * Gets position of sun from point on globe.
 * @param lon - longitude of position
 * @param lat - latitude of position
 * @return, position of sun
 */
Cord Globe::getSunDirection(
		double lon,
		double lat) const
{
	double sun;

	if (Options::globeSeasons == true)
	{
		const int
			monthDays1[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
			monthDays2[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366},

			year = _game->getSavedGame()->getTime()->getYear(),
			month = _game->getSavedGame()->getTime()->getMonth() - 1,
			day = _game->getSavedGame()->getTime()->getDay() - 1;

		const double
			tm = static_cast<double>( // day fraction is also taken into account
						(((_game->getSavedGame()->getTime()->getHour() * 60)
							+ _game->getSavedGame()->getTime()->getMinute()) * 60)
							+ _game->getSavedGame()->getTime()->getSecond())
						/ 86400.;

		double today;
		if (year %4 == 0 // spring equinox (start of astronomic year)
			&& !
				(year %100 == 0
					&& year %400 != 0))
		{
			today = (static_cast<double>(monthDays2[month] + day) + tm) / 366. - 0.219;
		}
		else
			today = (static_cast<double>(monthDays1[month] + day) + tm) / 365. - 0.219;

		if (today < 0.)
			today += 1.;

		sun = -0.261 * std::sin(today * M_PI * 2.);
	}
	else
		sun = 0.;

	const double rot = _game->getSavedGame()->getTime()->getDaylight() * M_PI * 2.;
	Cord sun_direction(std::cos(rot + lon), // init.
					   std::sin(rot + lon) * -std::sin(lat),
					   std::sin(rot + lon) * std::cos(lat));

	if (sun > 0)
		 sun_direction *= 1. - sun;
	else
		 sun_direction *= 1. + sun;

	Cord pole (0, // init.
			   std::cos(lat),
			   std::sin(lat));
	pole *= sun;
	sun_direction += pole;
	double norm = sun_direction.norm();
	norm = 1. / norm; // norm should always be greater than 0 here
	sun_direction *= norm;

	return sun_direction;
}

/**
 *
 */
void Globe::drawShadow()
{
	ShaderMove<Cord> earth = ShaderMove<Cord>(
										_earthData[_zoom],
										getWidth(),
										getHeight());
	ShaderRepeat<Sint16> noise = ShaderRepeat<Sint16>(
											_randomNoiseData,
											static_data.random_surf_size,
											static_data.random_surf_size);

	earth.setMove(
			_cenX-getWidth() / 2,
			_cenY-getHeight() / 2);

	lock();
	ShaderDraw<CreateShadow>(
						ShaderSurface(this),
						earth,
						ShaderScalar(getSunDirection(
												_cenLon,
												_cenLat)),
						noise);
	unlock();
}

/**
 * Draws a XuLine!
 * @param surface	-
 * @param src		-
 * @param x1		-
 * @param y1		-
 * @param x2		-
 * @param y2		-
 * @param shade		-
 * @param color		- (default 0)
 */
void Globe::XuLine(
		Surface* surface,
		Surface* src,
		double x1,
		double y1,
		double x2,
		double y2,
		int shade,
		Uint8 color)
{
	if (_clipper->LineClip(
						&x1,&y1,
						&x2,&y2) != 1) // empty line
	{
		return;
	}

	bool inv;
	Uint8 tcol;
	double
		delta_x = x2 - x1,
		delta_y = y2 - y1,
		len,
		x0,y0,
		SX,SY;

	if (std::abs(static_cast<int>(y2) - static_cast<int>(y1)) > std::abs(static_cast<int>(x2) - static_cast<int>(x1)))
	{
		len = std::abs(static_cast<int>(y2) - static_cast<int>(y1));
		inv = false;
	}
	else
	{
		len = std::abs(static_cast<int>(x2) - static_cast<int>(x1));
		inv = true;
	}

	if (y2 < y1)
		SY = -1;
	else if (AreSame(delta_y, 0.))
		SY = 0;
	else
		SY = 1;

	if (x2 < x1)
		SX = -1;
	else if (AreSame(delta_x, 0.))
		SX = 0;
	else
		SX = 1;

	x0 = x1;
	y0 = y1;

	if (inv == true)
		SY = (delta_y / len);
	else
		SX = (delta_x / len);

	while (len > 0.)
	{
//		if (x0 > 0 && y0 > 0 && x0 < surface->getWidth() && y0 < surface->getHeight())
		tcol = src->getPixelColor(
							static_cast<int>(x0),
							static_cast<int>(y0));
		if (tcol != 0)
		{
			if (color != 0)
				tcol = color; // flight path or craft radar
			else
			{
				const Uint8 colorBlock = (tcol & helper::ColorGroup);

				if (colorBlock == CLO_OCEAN
					|| colorBlock == CLO_OCEAN + 16)
				{
					tcol = CLO_OCEAN + static_cast<Uint8>(shade) + 8; // this pixel is ocean
				}
				else // this pixel is land
				{
					const Uint8 colorShaded = tcol + static_cast<Uint8>(shade);

					if (colorShaded > colorBlock + helper::ColorShade)
						tcol = colorBlock + helper::ColorShade;
					else
						tcol = colorShaded;
				}
			}

			surface->setPixelColor(
								static_cast<int>(x0),
								static_cast<int>(y0),
								tcol);
		}

		x0 += SX;
		y0 += SY;
		len -= 1.;
	}
}

/**
 * Draws the radar ranges of player's Bases and Craft on the globe.
 * kL Rewrite!
 */
void Globe::drawRadars()
{
	_radars->clear();

	if (Options::globeRadarLines == false)
		return;

	double
		x,y,
		range,
		lat,
		lon;

	_radars->lock();
	if (_hover == true
		&& Options::globeAllRadarsOnBaseBuild == true)
	{
		const std::vector<std::string>& facilities = _game->getRuleset()->getBaseFacilitiesList();
		for (std::vector<std::string>::const_iterator
				i = facilities.begin();
				i != facilities.end();
				++i)
		{
			range = static_cast<double>(_game->getRuleset()->getBaseFacility(*i)->getRadarRange());
			if (range > 0.)
			{
				polarToCart(
						_hoverLat,
						_hoverLon,
						&x,&y);

				range *= unitToRads;
				drawGlobeCircle( // placing new Base.
							_hoverLat,
							_hoverLon,
							range,
							48);
//							CLO_RADAR1);
			}
		}
	}

	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		lat = (*i)->getLatitude();
		lon = (*i)->getLongitude();

		if (!
			(AreSame(lon, 0.) && AreSame(lat, 0.)))
		{
			polarToCart(
					lon,
					lat,
					&x,&y);

			for (std::vector<BaseFacility*>::const_iterator
					j = (*i)->getFacilities()->begin();
					j != (*i)->getFacilities()->end();
					++j)
			{
				if ((*j)->getBuildTime() == 0)
				{
					range = static_cast<double>((*j)->getRules()->getRadarRange());
					if (range > 0.)
					{
						range *= unitToRads;
						drawGlobeCircle( // Base radars.
									lat,
									lon,
									range,
									48);
//									CLO_RADAR1);
					}
				}
			}
		}

		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getStatus() == "STR_OUT"
				&& (*j)->getTakeoff() == true)
			{
				range = static_cast<double>((*j)->getRules()->getRadarRange());
				if (range > 0.)
				{
					lat = (*j)->getLatitude();
					lon = (*j)->getLongitude();

					polarToCart(
							lon,
							lat,
							&x,&y);

					range *= unitToRads;
					drawGlobeCircle( // Craft radars.
								lat,
								lon,
								range,
								24,
								CLO_RADAR2);
				}
			}
		}
	}
	_radars->unlock();
}

/**
 * Draws globe range circle.
 * @param lat		-
 * @param lon		-
 * @param radius	-
 * @param segments	-
 * @param color		- (default 0)
 */
void Globe::drawGlobeCircle(
		double lat,
		double lon,
		double radius,
		int segments,
		Uint8 color)
{
	double
		x,y,
		x2 = 0.,
		y2 = 0.,
		lat1,
		lon1;

	for (double // 48 circle segments
			az = 0.;
			az <= M_PI * 2. + 0.01;
			az += M_PI * 2. / static_cast<double>(segments))
	{
		// calculating sphere-projected circle
		lat1 = asin(std::sin(lat) * std::cos(radius) + std::cos(lat) * std::sin(radius) * std::cos(az));
		lon1 = lon + std::atan2(
						std::sin(az) * std::sin(radius) * std::cos(lat),
						std::cos(radius) - std::sin(lat) * std::sin(lat1));

		polarToCart(
				lon1,
				lat1,
				&x,&y);

		if (AreSame(az, 0.)) // first vertex is for initialization only
		{
			x2 = x;
			y2 = y;

			continue;
		}

		if (pointBack(lon1, lat1) == false)
			XuLine(
				_radars,
				this,
				x,y,
				x2,y2,
				4,
				color);

		x2 = x;
		y2 = y;
	}
}

/**
 *
 */
void Globe::setNewBaseHover(void)
{
	_hover = true;
}

/**
 *
 */
void Globe::unsetNewBaseHover(void)
{
	_hover = false;
}

/**
 *
 */
bool Globe::getNewBaseHover(void)
{
	return _hover;
}

/**
 *
 * @param lon -
 * @param lat -
 */
void Globe::setNewBaseHoverPos(
		double lon,
		double lat)
{
	_hoverLon = lon;
	_hoverLat = lat;
}

/**
 * Draws a VHLine!
 * @param surface	- pointer to a Surface
 * @param lon1		-
 * @param lat1		-
 * @param lon2		-
 * @param lat2		-
 * @param color		- (default 0)
 */
void Globe::drawVHLine(
		Surface* surface,
		double lon1,
		double lat1,
		double lon2,
		double lat2,
		Uint8 color)
{
	double
		sx = lon2 - lon1,
		sy = lat2 - lat1,
		ln1,lt1,
		ln2,lt2;
	Sint16
		x1,y1,
		x2,y2;
	int
		seg;

	if (sx < 0.)
		sx += M_PI * 2.;

	if (std::fabs(sx) < 0.01)
	{
		seg = static_cast<int>(std::abs(sy / (M_PI * 2.) * 48.));
		if (seg == 0)
			++seg;
	}
	else
	{
		seg = static_cast<int>(std::abs(sx / (M_PI * 2.) * 96.));
		if (seg == 0)
			++seg;
	}

	sx /= seg;
	sy /= seg;

	for (int
			i = 0;
			i != seg;
			++i)
	{
		ln1 = lon1 + sx * static_cast<double>(i);
		lt1 = lat1 + sy * static_cast<double>(i);
		ln2 = lon1 + sx * static_cast<double>(i + 1);
		lt2 = lat1 + sy * static_cast<double>(i + 1);

		if (pointBack(ln2, lt2) == false
			&& pointBack(ln1, lt1) == false)
		{
			polarToCart(ln1, lt1, &x1, &y1);
			polarToCart(ln2, lt2, &x2, &y2);

			surface->drawLine(
							x1,y1,
							x2,y2,
							color);
		}
	}
}

/**
 * Draws the details of the countries on the globe based on the current zoom level.
 */
void Globe::drawDetail()
{
	_countries->clear();
//	if (!Options::globeDetail) return;

	double
		lon,
		lat;

	if (Options::globeDetail == true // draw the country borders
		&& _zoom > 0)
	{
		double
			lon1,
			lat1;
		Sint16
			x[2],
			y[2];

		_countries->lock();
		for (std::list<Polyline*>::const_iterator
				i = _rules->getPolylines()->begin();
				i != _rules->getPolylines()->end();
				++i)
		{
			for (int
					j = 0;
					j != (*i)->getPoints() - 1;
					++j)
			{
				lon = (*i)->getLongitude(j),
				lat = (*i)->getLatitude(j);
				lon1 = (*i)->getLongitude(j + 1),
				lat1 = (*i)->getLatitude(j + 1);

				if (pointBack( // don't draw if polyline is facing back
							lon,
							lat) == false
					&& pointBack(
							lon1,
							lat1) == false)
				{
					polarToCart( // convert coordinates
							lon,
							lat,
							&x[0],
							&y[0]);
					polarToCart(
							lon1,
							lat1,
							&x[1],
							&y[1]);

					_countries->drawLine(
									x[0],
									y[0],
									x[1],
									y[1],
									CLO_LINE);
				}
			}
		}
		_countries->unlock();
	}

	Sint16
		x,y;

	if (_zoom > 1) // draw the city markers
	{
		for (std::vector<Region*>::const_iterator
				i = _game->getSavedGame()->getRegions()->begin();
				i != _game->getSavedGame()->getRegions()->end();
				++i)
		{
			for (std::vector<RuleCity*>::const_iterator
					j = (*i)->getRules()->getCities()->begin();
					j != (*i)->getRules()->getCities()->end();
					++j)
			{
				drawTarget(
						*j,
						_countries);
			}
		}
	}

	if (Options::globeDetail == true)
	{
		Text* const label = new Text(100,9);

		label->setPalette(getPalette());
		label->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
		label->setAlign(ALIGN_CENTER);

		if (_zoom > 2)
		{
			label->setColor(CLO_LABELCOUNTRY); // draw the country labels

			for (std::vector<Country*>::const_iterator
					i = _game->getSavedGame()->getCountries()->begin();
					i != _game->getSavedGame()->getCountries()->end();
					++i)
			{
				lon = (*i)->getRules()->getLabelLongitude(),
				lat = (*i)->getRules()->getLabelLatitude();

				if (pointBack( // don't draw if label is facing back
							lon,
							lat) == false)
				{
					polarToCart( // convert coordinates
							lon,
							lat,
							&x,&y);

					label->setX(x - 50);
					label->setY(y);
					label->setText(_game->getLanguage()->getString((*i)->getRules()->getType()));

					label->blit(_countries);
				}
			}
		}

		label->setColor(CLO_LABELCITY); // draw the city labels
		int offset_y;

		for (std::vector<Region*>::const_iterator
				i = _game->getSavedGame()->getRegions()->begin();
				i != _game->getSavedGame()->getRegions()->end();
				++i)
		{
			for (std::vector<RuleCity*>::const_iterator
					j = (*i)->getRules()->getCities()->begin();
					j != (*i)->getRules()->getCities()->end();
					++j)
			{
				lon = (*j)->getLongitude(),
				lat = (*j)->getLatitude();

				if (pointBack( // label is on front of globe
							lon,
							lat) == false)
				{
					if (_zoom >= (*j)->getZoomLevel())
					{
						polarToCart( // convert coordinates
								lon,
								lat,
								&x,&y);

						if ((*j)->getLabelTop() == true)
							offset_y = -10;
						else
							offset_y = 2;

						label->setX(x - 50);
						label->setY(y + offset_y);
						label->setText((*j)->getName(_game->getLanguage()));

						label->blit(_countries);
					}
				}
			}
		}

		label->setColor(CLO_LABELBASE); // draw xCom base labels
		label->setAlign(ALIGN_LEFT);

		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			lon = (*i)->getLongitude(),
			lat = (*i)->getLatitude();

			if ((*i)->getMarker() != -1	// cheap hack to hide a base when it hasn't been placed yet
				&& pointBack(			// city is on front of globe
						lon,
						lat) == false)
			{
				polarToCart( // convert coordinates
						lon,
						lat,
						&x,&y);

				label->setX(x - 3);
				label->setY(y - 10);
				label->setText((*i)->getName());

				label->blit(_countries);
			}
		}

		delete label;
	}


	// Debug stuff follows ...
	static bool canSwitchDebugType = false;

	if (_game->getSavedGame()->getDebugMode() == true)
	{
		canSwitchDebugType = true;
		int color = 0;

		int
			cycleCur = _game->getDebugCycle(),
			area = 0;

		if (_debugType == 0) // Country rects.
		{
			if (cycleCur >= static_cast<int>(_game->getSavedGame()->getCountries()->size()))
			{
				cycleCur = -1;
				_game->setDebugCycle(-1);
			}

			for (std::vector<Country*>::const_iterator
					i = _game->getSavedGame()->getCountries()->begin();
					i != _game->getSavedGame()->getCountries()->end();
					++i,
						++area)
			{
				if (area == cycleCur
					|| cycleCur == -1)
				{
					color += 10;

					for (size_t
							j = 0;
							j != (*i)->getRules()->getLonMin().size();
							++j)
					{
						const double
							lon1 = (*i)->getRules()->getLonMin().at(j),
							lon2 = (*i)->getRules()->getLonMax().at(j),
							lat1 = (*i)->getRules()->getLatMin().at(j),
							lat2 = (*i)->getRules()->getLatMax().at(j);

						drawVHLine(_countries, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
						drawVHLine(_countries, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
						drawVHLine(_countries, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
						drawVHLine(_countries, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
					}
				}

				if (area == cycleCur)
				{
					if (_game->getSavedGame()->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
						_game->getSavedGame()->setDebugArg((*i)->getType());

					break;
				}
				else
					_game->getSavedGame()->setDebugArg("");
			}
		}
		else if (_debugType == 1) // Region rects.
		{
			if (cycleCur >= static_cast<int>(_game->getSavedGame()->getRegions()->size()))
			{
				cycleCur = -1;
				_game->setDebugCycle(-1);
			}

			for (std::vector<Region*>::const_iterator
					i = _game->getSavedGame()->getRegions()->begin();
					i != _game->getSavedGame()->getRegions()->end();
					++i,
						++area)
			{
				if (area == cycleCur
					|| cycleCur == -1)
				{
					color += 10;

					for (size_t
							j = 0;
							j != (*i)->getRules()->getLatMax().size();
							++j)
					{
						const double
							lon1 = (*i)->getRules()->getLonMin().at(j),
							lon2 = (*i)->getRules()->getLonMax().at(j),
							lat1 = (*i)->getRules()->getLatMin().at(j),
							lat2 = (*i)->getRules()->getLatMax().at(j);

						drawVHLine(_countries, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
						drawVHLine(_countries, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
						drawVHLine(_countries, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
						drawVHLine(_countries, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
					}
				}

				if (area == cycleCur)
				{
					if (_game->getSavedGame()->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
						_game->getSavedGame()->setDebugArg((*i)->getType());

					break;
				}
				else
					_game->getSavedGame()->setDebugArg("");
			}
		}
		else if (_debugType == 2) // MissionZone rects.
		{
			int limit = 0;
			for (std::vector<Region*>::const_iterator
					i = _game->getSavedGame()->getRegions()->begin();
					i != _game->getSavedGame()->getRegions()->end();
					++i)
			{
				for (std::vector<MissionZone>::const_iterator
						j = (*i)->getRules()->getMissionZones().begin();
						j != (*i)->getRules()->getMissionZones().end();
						++j)
				{
					++limit;
				}
			}

			if (cycleCur >= limit)
			{
				cycleCur = -1;
				_game->setDebugCycle(-1);
			}

			for (std::vector<Region*>::const_iterator
					i = _game->getSavedGame()->getRegions()->begin();
					i != _game->getSavedGame()->getRegions()->end();
					++i)
			{
				color = -1;

				int zoneType = 0;
				for (std::vector<MissionZone>::const_iterator
						j = (*i)->getRules()->getMissionZones().begin();
						j != (*i)->getRules()->getMissionZones().end();
						++j,
							++area,
							++zoneType)
				{
					if (area == cycleCur
						|| cycleCur == -1)
					{
						color += 2;

						for (std::vector<MissionArea>::const_iterator
								k = (*j).areas.begin();
								k != (*j).areas.end();
								++k)
						{
							const double
								lon1 = (*k).lonMin, // * M_PI / 180.,
								lon2 = (*k).lonMax, // * M_PI / 180.,
								lat1 = (*k).latMin, // * M_PI / 180.,
								lat2 = (*k).latMax; // * M_PI / 180.;

							drawVHLine(_countries, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
							drawVHLine(_countries, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
							drawVHLine(_countries, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
							drawVHLine(_countries, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
						}
					}

					if (area == cycleCur)
					{
						if (_game->getSavedGame()->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
						{
							std::ostringstream oststr;
							oststr << (*i)->getType() << " [" << zoneType << "]";
							_game->getSavedGame()->setDebugArg(oststr.str());
						}
						break;
					}
					else
						_game->getSavedGame()->setDebugArg("");
				}

				if (area == cycleCur)
					break;
			}
		}
	}
	else // toggles debugMode.
	{
		if (canSwitchDebugType == true)
		{
			++_debugType;
			if (_debugType > 2)
				_debugType = 0;

			canSwitchDebugType = false;
		}
	}
}

/**
 * Draws flight paths.
 * @param surface	- pointer to a Surface
 * @param lon1		-
 * @param lat1		-
 * @param lon2		-
 * @param lat2		-
 */
void Globe::drawPath(
		Surface* surface,
		double lon1,
		double lat1,
		double lon2,
		double lat2)
{
	double
		dist,
		x1,y1,
		x2,y2;
	Sint16 qty;
	CordPolar
		p1,
		p2;
	Cord
		a (CordPolar(lon1, lat1)), // init.
		b (CordPolar(lon2, lat2)); // init.

	if (-b == a)
		return;

	b -= a;

	// longer paths have more parts
	dist = b.norm();
	dist *= dist * 15;

	qty = static_cast<Sint16>(dist) + 1;
	b /= qty;

	p1 = CordPolar(a);
	polarToCart(
				p1.lon,
				p1.lat,
				&x1,&y1);
	for (Sint16
			i = 0;
			i != qty;
			++i)
	{
		a += b;
		p2 = CordPolar(a);
		polarToCart(
					p2.lon,
					p2.lat,
					&x2,&y2);

		if (   pointBack(p1.lon, p1.lat) == false
			&& pointBack(p2.lon, p2.lat) == false)
		{
			XuLine(
				surface,
				this,
				x1,y1,
				x2,y2,
				8,
				CLO_FLIGHT);
		}

		p1 = p2;
		x1 = x2;
		y1 = y2;
	}
}

/**
 * Draws the flight paths of player Craft flying on the globe.
 */
void Globe::drawFlights()
{
//	_radars->clear();
	if (Options::globeFlightPaths == false)
		return;

	_radars->lock();
	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getStatus() == "STR_OUT"
				&& (*j)->getDestination() != NULL)
			{
				const double
					lon1 = (*j)->getLongitude(),
					lon2 = (*j)->getDestination()->getLongitude(),
					lat1 = (*j)->getLatitude(),
					lat2 = (*j)->getDestination()->getLatitude();
				drawPath(
						_radars,
						lon1,lat1,
						lon2,lat2);
			}
		}
	}
	_radars->unlock();
}

/**
 * Draws the marker for a specified target on the globe.
 * @param target	- pointer to globe Target
 * @param surface	- pointer to globe Surface
 */
void Globe::drawTarget(
		Target* target,
		Surface* surface)
{
	if (target->getMarker() != -1)
	{
		const double
			lon = target->getLongitude(),
			lat = target->getLatitude();

		if (pointBack(
					lon,
					lat) == false)
		{
			Sint16
				x,y;
			polarToCart(
					lon,
					lat,
					&x,&y);

			Surface* const marker = _markerSet->getFrame(target->getMarker());
			marker->setX(x - 1);
			marker->setY(y - 1);

			marker->blit(surface);
		}
	}
}

/**
 * Draws the markers of all the various things going on around the world
 * except Cities.
 */
void Globe::drawMarkers()
{
	// code for using SurfaceSet for markers:
	_markers->clear();

	for (std::vector<Base*>::const_iterator // Draw the Base markers
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		drawTarget(
				*i,
				_markers);
	}

	for (std::vector<Waypoint*>::const_iterator // Draw the Waypoint markers
			i = _game->getSavedGame()->getWaypoints()->begin();
			i != _game->getSavedGame()->getWaypoints()->end();
			++i)
	{
		drawTarget(
				*i,
				_markers);
	}

	for (std::vector<MissionSite*>::const_iterator // Draw the MissionSite markers
			i = _game->getSavedGame()->getMissionSites()->begin();
			i != _game->getSavedGame()->getMissionSites()->end();
			++i)
	{
		drawTarget(
				*i,
				_markers);
	}

	for (std::vector<AlienBase*>::const_iterator // Draw the AlienBase markers
			i = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
			++i)
	{
		drawTarget(
				*i,
				_markers);
	}

	for (std::vector<Ufo*>::const_iterator // Draw the Ufo markers
			i = _game->getSavedGame()->getUfos()->begin();
			i != _game->getSavedGame()->getUfos()->end();
			++i)
	{
		drawTarget(
				*i,
				_markers);
	}

	for (std::vector<Base*>::const_iterator // Draw the Craft markers
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			drawTarget(
					*j,
					_markers);
		}
	}
}
/*	_markers->clear();

	Sint16
		x,
		y;

	for (std::vector<Base*>::const_iterator // Draw the base markers
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		// Cheap hack to hide bases when they haven't been placed yet
		if (((*i)->getLongitude() != 0.0
				|| (*i)->getLatitude() != 0.0)
			&& !pointBack(
						(*i)->getLongitude(),
						(*i)->getLatitude()))
		{
			polarToCart(
					(*i)->getLongitude(),
					(*i)->getLatitude(),
					&x,
					&y);

			_mkXcomBase->setX(x - 1);
			_mkXcomBase->setY(y - 1);

			_mkXcomBase->blit(_markers);
		}
	}

	for (std::vector<Waypoint*>::const_iterator // Draw the waypoint markers
			i = _game->getSavedGame()->getWaypoints()->begin();
			i != _game->getSavedGame()->getWaypoints()->end();
			++i)
	{
		if (pointBack(
					(*i)->getLongitude(),
					(*i)->getLatitude()))
		{
			continue;
		}

		polarToCart(
				(*i)->getLongitude(),
				(*i)->getLatitude(),
				&x,
				&y);

		_mkWaypoint->setX(x - 1);
		_mkWaypoint->setY(y - 1);

		_mkWaypoint->blit(_markers);
	}

	for (std::vector<TerrorSite*>::const_iterator // Draw the terror site markers
			i = _game->getSavedGame()->getTerrorSites()->begin();
			i != _game->getSavedGame()->getTerrorSites()->end();
			++i)
	{
		if (pointBack(
					(*i)->getLongitude(),
					(*i)->getLatitude()))
		{
			continue;
		}

		polarToCart(
				(*i)->getLongitude(),
				(*i)->getLatitude(),
				&x,
				&y);

		_mkAlienSite->setX(x - 1);
		_mkAlienSite->setY(y - 1);

		_mkAlienSite->blit(_markers);
	}

	for (std::vector<AlienBase*>::const_iterator // Draw the Alien Base markers
			i = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
			++i)
	{
		if (pointBack(
					(*i)->getLongitude(),
					(*i)->getLatitude()))
		{
			continue;
		}

		polarToCart(
				(*i)->getLongitude(),
				(*i)->getLatitude(),
				&x,
				&y);

		if ((*i)->isDiscovered())
		{
			_mkAlienBase->setX(x - 1);
			_mkAlienBase->setY(y - 1);

			_mkAlienBase->blit(_markers);
		}
	}

	for (std::vector<Ufo*>::const_iterator // Draw the UFO markers
			i = _game->getSavedGame()->getUfos()->begin();
			i != _game->getSavedGame()->getUfos()->end();
			++i)
	{
		if (pointBack(
					(*i)->getLongitude(),
					(*i)->getLatitude()))
		{
			continue;
		}

		Surface* marker = NULL;

		switch ((*i)->getStatus())
		{
			case Ufo::DESTROYED:
			continue;
			case Ufo::FLYING:
				if (!(*i)->getDetected())
					continue;

				marker = _mkFlyingUfo;
			break;
			case Ufo::LANDED:
				if (!(*i)->getDetected())
					continue;

				marker = _mkLandedUfo;
			break;
			case Ufo::CRASHED:
				marker = _mkCrashedUfo;
			break;
		}

		polarToCart(
				(*i)->getLongitude(),
				(*i)->getLatitude(),
				&x,
				&y);

		marker->setX(x - 1);
		marker->setY(y - 1);

		marker->blit(_markers);
	}

	for (std::vector<Base*>::const_iterator // Draw the craft markers
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getStatus() != "STR_OUT" // Hide crafts docked at base
				|| pointBack(
						(*j)->getLongitude(),
						(*j)->getLatitude()))
			{
				continue;
			}

			polarToCart(
					(*j)->getLongitude(),
					(*j)->getLatitude(),
					&x,
					&y);

			_mkCraft->setX(x - 1);
			_mkCraft->setY(y - 1);

			_mkCraft->blit(_markers);
		}
	} */

/**
 * Blits the globe onto another surface.
 * @param surface - pointer to another Surface
 */
void Globe::blit(Surface* surface)
{
	Surface::blit(surface);

	_radars->blit(surface);
	_countries->blit(surface);
	_markers->blit(surface);
}

/**
 * Ignores any mouse hovers that are outside the globe.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Globe::mouseOver(Action* action, State* state)
{
	double
		lon,
		lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteXMouse())),
			static_cast<Sint16>(std::floor(action->getAbsoluteYMouse())),
			&lon,
			&lat);

	if (_isMouseScrolling == true
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if ((SDL_GetMouseState(0,0) & SDL_BUTTON(Options::geoDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				center(
					_lonBeforeMouseScrolling,
					_latBeforeMouseScrolling);
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
//			stopScrolling(action); // newScroll

			return;
		}

		_isMouseScrolled = true;

/*kL
		// Set the mouse cursor back ( or not )
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_IGNORE);
		SDL_WarpMouse(
					static_cast<Uint16>(_xBeforeMouseScrolling),
					static_cast<Uint16>(_yBeforeMouseScrolling));
//		SDL_WarpMouse( // newScroll
//					(_game->getScreen()->getWidth() - 100) / 2,
//					_game->getScreen()->getHeight() / 2);
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_ENABLE); */


		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_mouseOverThreshold == false) // check threshold
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
							   || std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;


		if (Options::geoDragScrollInvert) // scroll
		{
			const double
				newLon = (static_cast<double>(_totalMouseMoveX) / action->getXScale()) * ROTATE_LONGITUDE / static_cast<double>(_zoom + 1) / 2.,
				newLat = (static_cast<double>(_totalMouseMoveY) / action->getYScale()) * ROTATE_LATITUDE / static_cast<double>(_zoom + 1) / 2.;
			center(
				_lonBeforeMouseScrolling + newLon / static_cast<double>(Options::geoScrollSpeed), //kL / 10.0,
				_latBeforeMouseScrolling + newLat / static_cast<double>(Options::geoScrollSpeed)); //kL / 10.0);
		}
		else
		{
			const double
				newLon = static_cast<double>(-action->getDetails()->motion.xrel) * ROTATE_LONGITUDE / static_cast<double>(_zoom + 1) / 2.,
				newLat = static_cast<double>(-action->getDetails()->motion.yrel) * ROTATE_LATITUDE / static_cast<double>(_zoom + 1) / 2.;
			center(
				_cenLon + newLon / static_cast<double>(Options::geoScrollSpeed), //kL / 10.0,
				_cenLat + newLat / static_cast<double>(Options::geoScrollSpeed)); //kL / 10.0);
		}

/*kL
		// We don't want to look the mouse-cursor jumping :)
		action->setMouseAction( // newScroll
						_xBeforeMouseScrolling,
						_yBeforeMouseScrolling,
						getX(),
						getY());
		action->getDetails()->motion.x = static_cast<Uint16>(_xBeforeMouseScrolling);
		action->getDetails()->motion.y = static_cast<Uint16>(_yBeforeMouseScrolling); */

		_game->getCursor()->handle(action);
	}

/*kL
	if (_isMouseScrolling // newScroll
		&& (action->getDetails()->motion.x != static_cast<Uint16>(_xBeforeMouseScrolling)
			|| action->getDetails()->motion.y != static_cast<Uint16>(_yBeforeMouseScrolling)))
	{
		action->setMouseAction(
						_xBeforeMouseScrolling,
						_yBeforeMouseScrolling,
						getX(),
						getY());
		action->getDetails()->motion.x = static_cast<Uint16>(_xBeforeMouseScrolling);
		action->getDetails()->motion.y = static_cast<Uint16>(_yBeforeMouseScrolling);
	} */

	if (lat == lat // Check for errors
		&& lon == lon)
	{
		InteractiveSurface::mouseOver(action, state);
	}
}

/**
 * Ignores any mouse clicks that are outside the globe.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Globe::mousePress(Action* action, State* state)
{
	double
		lon,
		lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteXMouse())),
			static_cast<Sint16>(std::floor(action->getAbsoluteYMouse())),
			&lon,
			&lat);

	if (action->getDetails()->button.button == Options::geoDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		SDL_GetMouseState(
					&_xBeforeMouseScrolling,
					&_yBeforeMouseScrolling);

		_lonBeforeMouseScrolling = _cenLon;
		_latBeforeMouseScrolling = _cenLat;

		_totalMouseMoveX =
		_totalMouseMoveY = 0;

		_mouseOverThreshold = false;
		_mouseScrollingStartTime = SDL_GetTicks();
	}

	if (lat == lat // Check for errors
		&& lon == lon)
	{
		InteractiveSurface::mousePress(action, state);
	}
}

/**
 * Ignores any mouse clicks that are outside the globe.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Globe::mouseRelease(Action* action, State* state)
{
	double
		lon,
		lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteXMouse())),
			static_cast<Sint16>(std::floor(action->getAbsoluteYMouse())),
			&lon,
			&lat);

//	if (action->getDetails()->button.button == Options::geoDragScrollButton)
//		stopScrolling(action);

	if (lat == lat // Check for errors
		&& lon == lon)
	{
		InteractiveSurface::mouseRelease(action, state);
	}
}

/**
 * Ignores any mouse clicks that are outside the globe
 * and handles globe rotation and zooming.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Globe::mouseClick(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		zoomIn();
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		zoomOut();

	double
		lon,
		lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteXMouse())),
			static_cast<Sint16>(std::floor(action->getAbsoluteYMouse())),
			&lon,
			&lat);

	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now another button is used)
	if (_isMouseScrolling)
	{
		if (action->getDetails()->button.button != Options::geoDragScrollButton
			&& (SDL_GetMouseState(0,0) & SDL_BUTTON(Options::geoDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (!_mouseOverThreshold
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				center(
					_lonBeforeMouseScrolling,
					_latBeforeMouseScrolling);
			}

			_isMouseScrolled = _isMouseScrolling = false;
//			stopScrolling(action); // newScroll
		}
	}

	if (_isMouseScrolling) // DragScroll-Button release: release mouse-scroll-mode
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::geoDragScrollButton)
		{
			_isMouseScrolling = false;
//			stopScrolling(action); // newScroll
		}
		else
			return;

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (!_mouseOverThreshold
			&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
			center(
				_lonBeforeMouseScrolling,
				_latBeforeMouseScrolling);
		}

		if (_isMouseScrolled)
			return;
	}

	if (lat == lat // Check for errors
		&& lon == lon)
	{
		InteractiveSurface::mouseClick(action, state);

		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
			center(lon, lat);
	}
}

/**
 * Handles globe keyboard shortcuts.
 * @param action, Pointer to an action.
 * @param state, State that the action handlers belong to.
 */
void Globe::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);

	if (action->getDetails()->key.keysym.sym == Options::keyGeoToggleDetail)
		toggleDetail();

	if (action->getDetails()->key.keysym.sym == Options::keyGeoToggleRadar)
		toggleRadarLines();
}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * @param action - pointer to an Action
 */
/*void Globe::stopScrolling(Action* action)
{
	SDL_WarpMouse(
			static_cast<Uint16>(_xBeforeMouseScrolling),
			static_cast<Uint16>(_yBeforeMouseScrolling));
	action->setMouseAction(
					_xBeforeMouseScrolling,
					_yBeforeMouseScrolling,
					getX(),
					getY());
} */

/**
 * Gets the polygon's texture & shadeLevel at a given point.
 * @param lon		- longitude of the point
 * @param lat 		- latitude of the point
 * @param texture	- pointer to texture ID (returns -1 if polygon not found)
 * @param shade		- pointer to shade level
 */
void Globe::getPolygonTextureAndShade(
		double lon,
		double lat,
		int* texture,
		int* shade) const
{
	// this is shade conversion from 0..31 levels of geoscape to battlescape levels 0..15
/*	int worldshades[32] =
	{
		 0,  0,  0,  0,  1,  1,  2,  2,
		 3,  3,  4,  4,  5,  5,  6,  6,
		 7,  7,  8,  8,  9,  9, 10, 11,
		11, 12, 12, 13, 13, 14, 15, 15
	}; */
	const int worldshades[32] =
	{
		0, 1, 1, 1, 2, 2, 2, 3,
		3, 3, 4, 4, 4, 5, 5, 5,
		6, 6, 6, 7, 7, 7, 8, 8,
		9, 9,10,11,12,13,14,15
	};
/*	int worldshades[32] =
	{
		 0, 1, 2, 3, 4, 5, 6, 7,
		 8, 9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,
		24,25,26,27,28,29,30,31
	}; */ // terminator @ 25.

	*shade = worldshades[CreateShadow::getShadowValue(
													0,
													Cord (0.,0.,1.), // init.
													getSunDirection(lon, lat),
													0)];

	// This only temporarily changes cenLon/cenLat so the "const" is actually preserved.
/*	Globe* const globe = const_cast<Globe* const>(this); // WARNING: BAD CODING PRACTICE

	double
		oldLon = _cenLon,
		oldLat = _cenLat;
	globe->_cenLon = lon;
	globe->_cenLat = lat; */

	*texture = -1;
	for (std::list<Polygon*>::const_iterator
			i = _rules->getPolygons()->begin();
			i != _rules->getPolygons()->end();
			++i)
	{
		if (insidePolygon(
						lon,
						lat,
						*i) == true)
		{
			*texture = (*i)->getPolyTexture();
			break;
		}
	}

/*	globe->_cenLon = oldLon;
	globe->_cenLat = oldLat; */
}

/**
 * Gets the polygon's texture at a given point.
 * @param lon		- longitude of the point
 * @param lat 		- latitude of the point
 * @param texture	- pointer to texture ID (returns -1 if polygon not found)
 */
void Globe::getPolygonTexture(
		double lon,
		double lat,
		int* texture) const
{
	*texture = -1;
	for (std::list<Polygon*>::const_iterator
			i = _rules->getPolygons()->begin();
			i != _rules->getPolygons()->end();
			++i)
	{
		if (insidePolygon(
						lon,
						lat,
						*i) == true)
		{
			*texture = (*i)->getPolyTexture();
			break;
		}
	}
}

/**
 * Gets the polygon's shade at a given point.
 * @param lon	- longitude of the point
 * @param lat 	- latitude of the point
 * @param shade	- pointer to shade level
 */
void Globe::getPolygonShade(
		double lon,
		double lat,
		int* shade) const
{
	// this is shade conversion from 0..31 levels of geoscape to battlescape levels 0..15
	const int worldshades[32] =
	{
		0, 1, 1, 1, 2, 2, 2, 3,
		3, 3, 4, 4, 4, 5, 5, 5,
		6, 6, 6, 7, 7, 7, 8, 8,
		9, 9,10,11,12,13,14,15
	};

	*shade = worldshades[CreateShadow::getShadowValue(
													0,
													Cord (0.,0.,1.), // init.
													getSunDirection(lon, lat),
													0)];
}

/**
 * Turns Radar lines on or off.
 */
void Globe::toggleRadarLines()
{
	Options::globeRadarLines = !Options::globeRadarLines;
	drawRadars();
}

/**
 * Resizes the Globe.
 */
void Globe::resize()
{
	Surface* surfaces[4] =
	{
		this,
		_markers,
		_countries,
		_radars
	};

	int
		width = Options::baseXGeoscape - 64,
		height = Options::baseYGeoscape;

	for (size_t
			i = 0;
			i != 4;
			++i)
	{
		surfaces[i]->setWidth(width);
		surfaces[i]->setHeight(height);
		surfaces[i]->invalidate();
	}

	_clipper->Wxrig = width;
	_clipper->Wybot = height;
	_cenX = static_cast<Sint16>(width) / 2;
	_cenY = static_cast<Sint16>(height / 2);

	setupRadii(width, height);

	invalidate();
}

/**
 * Gets the current debugType for Geoscape messages.
 * @return, the debug type
 */
int Globe::getDebugType() const
{
	return _debugType;
}

}
