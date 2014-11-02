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

#ifndef OPENXCOM_GLOBE_H
#define OPENXCOM_GLOBE_H

#include <list>
#include <vector>

#include "Cord.h"

#include "../Engine/FastLineClip.h"
#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

extern bool kL_reCenter;


class Game;
class LocalizedText;
class Polygon;
class RuleGlobe;
class SurfaceSet;
class Target;
class Timer;


/**
 * Interactive globe view of the world.
 * Takes a flat world map made out of land polygons with
 * polar coordinates and renders it as a 3D-looking globe
 * with cartesian coordinates that the player can interact with.
 */
class Globe
	:
		public InteractiveSurface
{

private:
	static const int
		NUM_LANDSHADES	= 48,
		NUM_SEASHADES	= 72,
		NEAR_RADIUS		= 25,
		CITY_MARKER		= 8;
//		NUM_TEXTURES	= 13;
//kL	static const size_t DOGFIGHT_ZOOM = 5; // kL, was 3

	static const double ROTATE_LONGITUDE;
	static const double ROTATE_LATITUDE;

	bool
//		_blink, // they typed it to (int)
		_hover,
		_isMouseScrolled,
		_isMouseScrolling,
		_mouseOverThreshold;
	int
		_blink,
//		_blinkVal,
		_totalMouseMoveX,
		_totalMouseMoveY,
		_xBeforeMouseScrolling,
		_yBeforeMouseScrolling;
	double
		_cenLat,
		_cenLon,
		_hoverLat,
		_hoverLon,
		_lonBeforeMouseScrolling,
		_latBeforeMouseScrolling,
		_rotLat,
		_rotLon,
		_radius,
		_radiusStep;
	Sint16
		_cenX,
		_cenY;
	Uint32 _mouseScrollingStartTime;
	size_t
		_dfPreZoom,
		_zoom,
		_zoomTexture;

	FastLineClip* _clipper;
	Game* _game;
	RuleGlobe* _rules;
	Surface
		* _countries,
		* _markers,
		* _radars;
/*		* _mkAlienBase,		// kL_begin:
		* _mkAlienSite,
		* _mkCity,
		* _mkCraft,
		* _mkCrashedUfo,
		* _mkFlyingUfo,
		* _mkLandedUfo,
		* _mkWaypoint,
		* _mkXcomBase; */	// kL_end.
	SurfaceSet
		* _markerSet,
		* _texture;
	Timer
		* _blinkTimer,
		* _rotTimer;

	std::list<Polygon*> _cacheLand;
	/// normal of each pixel in earth globe per zoom level
	std::vector<std::vector<Cord> > _earthData;
	/// data sample used for noise in shading
	std::vector<Sint16> _randomNoiseData;
	/// list of dimension of earth on screen per zoom level
	std::vector<double> _zoomRadii;


	/// Sets the globe zoom factor.
	void setZoom(size_t zoom);

	/// Checks if a point is behind the globe.
	bool pointBack(
			double lon,
			double lat) const;
	/// Return latitude of last visible-to-player point on given longitude.
	double lastVisibleLat(double lon) const;
	/// Checks if a point is inside a polygon.
	bool insidePolygon(
			double lon,
			double lat,
			Polygon* poly) const;
	/// Checks if a target is near a point.
	bool targetNear(
			Target* target,
			int x,
			int y) const;
	/// Caches a set of polygons.
	void cache(
			std::list<Polygon*>* polygons,
			std::list<Polygon*>* cache);
	/// Get position of sun relative to given position in polar cords and date.
	Cord getSunDirection(
			double lon,
			double lat) const;
	/// Draw globe range circle.
	void drawGlobeCircle(
			double lat,
			double lon,
			double radius,
			int segments,
			Uint8 color = 0); // kL_add.
	/// Special "transparent" line.
	void XuLine(
			Surface* surface,
			Surface* src,
			double x1,
			double y1,
			double x2,
			double y2,
			int shade,
			Uint8 color = 0); // kL_add.
	void drawVHLine(
			Surface* surface,
			double lon1,
			double lat1,
			double lon2,
			double lat2,
			Uint8 color);
	/// Draw flight path.
	void drawPath(
			Surface* surface,
			double lon1,
			double lat1,
			double lon2,
			double lat2);
	/// Draw target marker.
	void drawTarget(Target* target);


	public:
		static Uint8 oceanColor1;
		static Uint8 oceanColor2;

		/// Creates a new globe at the specified position and size.
		Globe(
				Game* game,
				int cenX,
				int cenY,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the globe.
		~Globe();

		/// Converts polar coordinates to cartesian coordinates.
		void polarToCart(
				double lon,
				double lat,
				Sint16* x,
				Sint16* y) const;
		/// Converts polar coordinates to cartesian coordinates.
		void polarToCart(
				double lon,
				double lat,
				double* x,
				double* y) const;
		/// Converts cartesian coordinates to polar coordinates.
		void cartToPolar(
				Sint16 x,
				Sint16 y,
				double* lon,
				double* lat) const;

		/// Sets the texture set for the globe's polygons.
		void setTexture(SurfaceSet* texture);

		/// Starts rotating the globe left.
		void rotateLeft();
		/// Starts rotating the globe right.
		void rotateRight();
		/// Starts rotating the globe up.
		void rotateUp();
		/// Starts rotating the globe down.
		void rotateDown();
		/// Stops rotating the globe.
		void rotateStop();
		/// Stops longitude rotation of the globe.
		void rotateStopLon();
		/// Stops latitude rotation of the globe.
		void rotateStopLat();

		/// Zooms the globe in.
		void zoomIn();
		/// Zooms the globe out.
		void zoomOut();
		/// Zooms the globe minimum.
//		void zoomMin();
		/// Zooms the globe maximum.
//		void zoomMax();

		/// Sets the zoom level before a dogfight.
		void setPreDogfightZoom();
		/// Gets the zoom level from before a dogfight.
		size_t getPreDogfightZoom() const;
		/// Zooms the globe in for dogfights.
		bool zoomDogfightIn();
		/// Zooms the globe out for dogfights.
		bool zoomDogfightOut();
		/// Gets the current zoom.
		size_t getZoom() const;
		/// kL. Gets the number of zoom levels available.
		size_t getZoomLevels() const; // kL

		/// Centers the globe on a point.
		void center(
				double lon,
				double lat);

		/// Checks if a point is inside land.
		bool insideLand(
				double lon,
				double lat) const;

		/// Turns on/off the globe detail.
		void toggleDetail();

		/// Gets all the targets near a point on the globe.
		std::vector<Target*> getTargets(
				int x,
				int y,
				bool craftOnly) const;

		/// Caches visible globe polygons.
		void cachePolygons();
		/// Sets the palette of the globe.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256);

		/// Handles the timers.
		void think();
		/// Blinks the markers.
		void blink();
		/// Rotates the globe.
		void rotate();

		/// Draws the whole globe.
		void draw();
		/// Draws the ocean of the globe.
		void drawOcean();
		/// Draws the land of the globe.
		void drawLand();
		/// Draws the shadow.
		void drawShadow();
		/// Draws the radar ranges of the globe.
		void drawRadars();
		/// Draws the flight paths of the globe.
		void drawFlights();
		/// Draws the country details of the globe.
		void drawDetail();
		/// Draws all the markers over the globe.
		void drawMarkers();
		/// Blits the globe onto another surface.
		void blit(Surface* surface);

		/// Special handling for mouse hover.
		void mouseOver(Action* action, State* state);
		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state);
		/// Special handling for mouse releases.
		void mouseRelease(Action* action, State* state);
		/// Special handling for mouse clicks.
		void mouseClick(Action* action, State* state);
		/// Special handling for key presses.
		void keyboardPress(Action* action, State* state);

		/// Move the mouse back to where it started after we finish drag scrolling.
		void stopScrolling(Action* action);

		/// Get the polygons texture and shade at the given point.
		void getPolygonTextureAndShade(
				double lon,
				double lat,
				int* texture,
				int* shade) const;

		/// Get the localized text.
		const LocalizedText& tr(const std::string& id) const;
		/// Get the localized text.
		LocalizedText tr(
				const std::string& id,
				unsigned n) const;

		/// Sets hover base position.
		void setNewBaseHoverPos(
				double lon,
				double lat);
		/// Turns on new base hover mode.
		void setNewBaseHover(void);
		/// Turns off new base hover mode.
		void unsetNewBaseHover(void);
		/// Gets state of base hover mode
		bool getNewBaseHover(void);

		/// Gets _detail variable
//		bool getShowRadar(void);
		/// set the _radarLines variable
		void toggleRadarLines();

		/// Update the resolution settings, we just resized the window.
		void resize();
		/// Set up the radius of earth and stuff.
		void setupRadii(
				int width,
				int height);
};

}

#endif
