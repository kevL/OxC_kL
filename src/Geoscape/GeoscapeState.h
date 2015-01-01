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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_GEOSCAPESTATE_H
#define OPENXCOM_GEOSCAPESTATE_H

//#include <list>

#include "../Engine/State.h"


namespace OpenXcom
{

extern size_t kL_currentBase;

extern const double
	earthRadius,
	unitToRads,
	greatCircleConversionFactor;


class Base;
class Craft;
class DogfightState;
class Globe;
class ImageButton;
//class InteractiveSurface;
class SavedGame;
class Sound;
class Surface;
class TerrorSite;
class Text;
class TextButton;
class Timer;
class Ufo;


/**
 * Geoscape screen which shows an overview of
 * the world and lets the player manage the game.
 */
class GeoscapeState
	:
		public State
{

private:

	bool
		_dogfightEnded,
		_pause,
		_startMusic,
		_dfZoomInDone,
		_dfZoomOutDone;
	int
		_day,
		_month,
		_year;
	size_t _minimizedDogfights;
	double
		_dfLon,
		_dfLat;

	std::string _debug;

	Globe* _globe;
	ImageButton
		* _btnIntercept,
		* _btnBases,
		* _btnGraphs,
		* _btnUfopaedia,
		* _btnOptions,
		* _btnFunding,

		* _btn5Secs,
		* _btn1Min,
		* _btn5Mins,
		* _btn30Mins,
		* _btn1Hour,
		* _btn1Day,

		* _btnDetail,
		* _timeSpeed;
//	InteractiveSurface* _btnRotateLeft, * _btnRotateRight, * _btnRotateUp, * _btnRotateDown, * _btnZoomIn, * _btnZoomOut;
	SavedGame* _savedGame;
	Surface
//		* _sidebar,
		* _sideLine,
//		* _bg,
		* _srfSpace,	// kL
		* _srfTime,		// kL
		* _srfDay1,
		* _srfDay2,
		* _srfMonth1,
		* _srfMonth2,
		* _srfYear1,
		* _srfYear2;
	Text
		* _txtDebug,
		* _txtFunds,
		* _txtHour,
		* _txtHourSep,
		* _txtMin,
//		* _txtMinSep,
		* _txtSec;
//		* _txtDate;
//		* _txtWeekday,
//		* _txtDay,
//		* _txtMonth,
//		* _txtYear;
//	TextButton
//		* _sideTop,
//		* _sideBottom;
	Timer
		* _gameTimer,
		* _dfZoomInTimer,
		* _dfZoomOutTimer,
		* _dfStartTimer;

	std::list<State*> _popups;
	std::list<DogfightState*>
		_dogfights,
		_dogfightsToStart;

	/// Handle alien mission generation.
	void determineAlienMissions(bool atGameStart = false);
	/// Handle Terror mission generation.
	void setupTerrorMission();


	public:
		static Sound* soundPop; // kL


		/// Creates the Geoscape state.
		GeoscapeState();
		/// Cleans up the Geoscape state.
		~GeoscapeState();

		/// Handle keypresses.
		void handle(Action* action);

		/// Updates the palette and timer.
		void init();
		/// Runs the timer.
		void think();

		/// Displays the game time/date. (+Funds)
		void timeDisplay();
		/// Advances the game timer.
		void timeAdvance();
		/// Trigger whenever 5 seconds pass.
		void time5Seconds();
		/// Trigger whenever 10 minutes pass.
		void time10Minutes();
		/// Trigger whenever 30 minutes pass.
		void time30Minutes();
		/// Trigger whenever 1 hour passes.
		void time1Hour();
		/// Trigger whenever 1 day passes.
		void time1Day();
		/// Trigger whenever 1 month passes.
		void time1Month();

		/// Resets the timer to minimum speed.
		void timerReset();

		/// Displays a popup window.
		void popup(State* state);

		/// Gets the Geoscape globe.
		Globe* getGlobe() const;

		/// Handler for clicking the globe.
		void globeClick(Action* action);
		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Bases button.
		void btnBasesClick(Action* action);
		/// Handler for clicking the Graph button.
		void btnGraphsClick(Action* action);
		/// Handler for clicking the Ufopaedia button.
		void btnUfopaediaClick(Action* action);
		/// Handler for clicking the Options button.
		void btnOptionsClick(Action* action);
		/// Handler for clicking the Funding button.
		void btnFundingClick(Action* action);

		/// Handler for clicking the Detail area.
		void btnDetailPress(Action* action);

		/// Handler for pressing the Rotate Left arrow.
		void btnRotateLeftPress(Action* action);
		/// Handler for releasing the Rotate Left arrow.
		void btnRotateLeftRelease(Action* action);
		/// Handler for pressing the Rotate Right arrow.
		void btnRotateRightPress(Action* action);
		/// Handler for releasing the Rotate Right arrow.
		void btnRotateRightRelease(Action* action);
		/// Handler for pressing the Rotate Up arrow.
		void btnRotateUpPress(Action* action);
		/// Handler for releasing the Rotate Up arrow.
		void btnRotateUpRelease(Action* action);
		/// Handler for pressing the Rotate Down arrow.
		void btnRotateDownPress(Action* action);
		/// Handler for releasing the Rotate Down arrow.
		void btnRotateDownRelease(Action* action);
		/// Handler for left-clicking the Zoom In icon.
		void btnZoomInLeftClick(Action* action);
		/// Handler for right-clicking the Zoom In icon.
//		void btnZoomInRightClick(Action* action);
		/// Handler for left-clicking the Zoom Out icon.
		void btnZoomOutLeftClick(Action* action);
		/// Handler for right-clicking the Zoom Out icon.
//		void btnZoomOutRightClick(Action* action);

		/// Blit method - renders the state and dogfights.
		void blit();

		/// Gets the Timer for dogfight zoom-ins.
		Timer* getDogfightZoomInTimer() const;
		/// Gets the Timer for dogfight zoom-outs.
		Timer* getDogfightZoomOutTimer() const;
		/// Globe zoom in effect for dogfights.
		void dfZoomIn();
		/// Globe zoom out effect for dogfights.
		void dfZoomOut();
		/// Sets current dogfight's current Globe coordinates.
		void setDogfightCoords(Craft* craft);

		/// Multi-dogfights logic handling.
		void handleDogfights();
		/// Gets the number of minimized dogfights.
		int minimizedDogfightsCount() const;
		/// Starts a new dogfight.
		void startDogfight();
		/// Gets first free dogfight slot.
		int getNextDogfightSlot() const;

		/// Gets the dogfights.
		std::list<DogfightState*>& getDogfights();

		/// Handler for clicking the timer button.
		void btnTimerClick(Action* action);

		/// Process a terror site
		bool processTerrorSite(TerrorSite* ts) const;

		/// Handles base defense
		void handleBaseDefense(
				Base* base,
				Ufo* ufo);

		/// Update the resolution settings, we just resized the window.
		void resize(
				int& dX,
				int& dY);
};

}

#endif
