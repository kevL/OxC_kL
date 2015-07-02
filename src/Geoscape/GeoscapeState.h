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

extern size_t kL_curBase;
extern bool
	kL_geoMusicPlaying,
	kL_geoMusicReturnState;

extern const double
	earthRadius,
	unitToRads,
	greatCircleConversionFactor;


class Base;
class Craft;
class DogfightState;
class Globe;
class ImageButton;
class InteractiveSurface;
class MissionSite;
class Ruleset;
class SavedGame;
class Sound;
class Surface;
class Text;
class TextButton;
class Timer;
class Ufo;


/**
 * Geoscape screen shows an overview of the world.
 */
class GeoscapeState
	:
		public State
{

private:
	static const size_t INDICATORS = 16;

	bool
		_dfCenterCurrentCoords,
		_dfZoomInDone,
		_dfZoomOutDone,
		_dfZoomOut,
		_pause,
		_pauseHard;
	int
		_5secIterForMusic,
		_day,
		_month,
		_year;
	int64_t _windowPops;
	size_t _minimizedDogfights;
	double
		_dfCCC_lon,
		_dfCCC_lat;

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
	InteractiveSurface
		* _isfVisibleUfo[INDICATORS],
		* _isfTime;
//	NumberText* _numVisibleUfo[INDICATORS];
	Ruleset* _rules;
	SavedGame* _gameSave;
	Surface
//		* _bg,
//		* _sidebar,
		* _sideBarBlack,
		* _srfSpace;
//		* _srfDay1,
//		* _srfDay2,
//		* _srfMonth1,
//		* _srfMonth2,
//		* _srfYear1,
//		* _srfYear2;
	Text
		* _txtDebug,
		* _txtFunds,
		* _txtScore,

		* _txtHour,
		* _txtHourSep,
		* _txtMin,
		* _txtSec,
		* _txtDay,
		* _txtMonth,
		* _txtYear,
//		* _txtMinSep,
//		* _txtDate;
//		* _txtWeekday,
		* _ufoDetected;
	TextButton
		* _sideTop,
		* _sideBottom;
	Timer
		* _gameTimer,
		* _dfZoomInTimer,
		* _dfZoomOutTimer,
		* _dfStartTimer,
		* _dfTimer;
	Ufo* _visibleUfo[INDICATORS];

	std::list<State*> _popups;
	std::list<DogfightState*>
		_dogfights,
		_dogfightsToStart;

	/// Handle alien mission generation.
	void determineAlienMissions(bool atGameStart = false);
	/// Handle land mission generation.
	void setupLandMission();

	/// Handler for clicking the timer button.
	void btnTimerPress(Action* action);
	/// Handler for clicking pause.
	void btnPausePress(Action* action);
	/// Handler for clicking a visible UFO button.
	void btnVisibleUfoPress(Action* action);


	public:
		static const int _ufoBlobs[8][13][13]; // used also by DogfightState

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

		/// Draws the UFO indicators for known UFOs.
		void drawUfoIndicators();

		/// Displays the game time/date. (+Funds)
		void updateTimeDisplay();
		/// Converts the date to a month string.
		std::wstring convertDateToMonth(int date);
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
		void resetTimer();
		/// Gets if time compression is set to 5 second intervals.
		bool is5Sec() const;

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
		void blit(); // OoO

		/// Gets the Timer for dogfight zoom-ins.
		Timer* getDfZoomInTimer() const;
		/// Gets the Timer for dogfight zoom-outs.
		Timer* getDfZoomOutTimer() const;
		/// Globe zoom in effect for dogfights.
		void dfZoomIn();
		/// Globe zoom out effect for dogfights.
		void dfZoomOut();
		/// Stores current Globe coordinates and zoom before a dogfight.
		void storePreDfCoords();
		/// Tells Dogfight zoom-out to ignore stored DF coordinates.
		void setDfCCC(
				double lon,
				double lat);
		/// Gets whether Dogfight zoom-out should ignore stored DF coordinates.
		bool getDfCCC() const;
		/// Gets the number of minimized dogfights.
		size_t getMinimizedDfCount() const;
		/// Multi-dogfights logic handling.
		void thinkDogfights();
		/// Starts a new dogfight.
		void startDogfight();
		/// Updates interceptions windows for all Dogfights.
		void resetInterceptPorts();
		/// Gets first free dogfight slot.
		size_t getOpenDfSlot() const;

		/// Gets the dogfights.
		std::list<DogfightState*>& getDogfights();

		/// Handles base defense
		void handleBaseDefense(
				Base* base,
				Ufo* ufo);

		/// Process a mission site
		bool processMissionSite(MissionSite* site) const; // OoO

		/// Update the resolution settings, the window was resized.
		void resize(
				int& dX,
				int& dY);

		/// Examines the quantity of remaining UFO-detected popups.
		void assessUfoPopups();
		/// Sets pause.
		void setPause();
		/// Gets pause.
		bool getPause() const;
};

}

#endif
