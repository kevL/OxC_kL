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

#ifndef OPENXCOM_DOGFIGHTSTATE_H
#define OPENXCOM_DOGFIGHTSTATE_H

//#include <vector>
//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

enum ColorDf
{
	CRAFT_MIN,			//  0
	CRAFT_MAX,			//  1
	RADAR_MIN,			//  2
	RADAR_MAX,			//  3
	DAMAGE_MIN,			//  4
	DAMAGE_MAX,			//  5
	BLOB_MIN,			//  6
	RANGE_METER,		//  7
	DISABLED_WEAPON,	//  8
	DISABLED_AMMO,		//  9
	DISABLED_RANGE		// 10
};


class Craft;
class CraftWeaponProjectile;
class GeoscapeState;
class Globe;
class ImageButton;
class InteractiveSurface;
class SavedGame;
class Surface;
class Text;
class Timer;
class Ufo;


/**
 * Shows a dogfight (interception) between a player's Craft and a UFO.
 */
class DogfightState
	:
		public State
{

private:
	static const int
		_projectileBlobs[4][6][3],

		DST_ENGAGE		= 635,
		DST_STANDOFF	= 595,
		DST_CLOSE		= 64,
		MSG_TIMEOUT		= 45;


	bool
		_end,
		_destroyUfo,
		_destroyCraft,
		_ufoBreakingOff,
		_w1Enabled,
		_w2Enabled,
		_minimized,
		_endDogfight,
		_animatingHit;
	int
		_cautionLevel,
		_diff,
		_timeout,
		_dist,
		_desired,

		_ufoSize,
		_craftHeight,
		_craftHeight_pre,

		_x,
		_y,
		_minimizedIconX,
		_minimizedIconY,

		_w1FireCountdown,
		_w2FireCountdown,
		_w1FireInterval,
		_w2FireInterval;
	size_t
		_slot,
		_totalIntercepts;
	Uint8 _colors[11]; // see ColorDf enum above^
//		_currentCraftDamageColor;

	std::vector<CraftWeaponProjectile*> _projectiles;

	Craft* _craft;
	GeoscapeState* _geo;
	Globe* _globe;
	ImageButton
		* _btnStandoff,
		* _btnCautious,
		* _btnStandard,
		* _btnAggressive,
		* _btnDisengage,
		* _btnUfo,
		* _craftStance;
	InteractiveSurface
		* _btnMinimize,
		* _preview,
		* _weapon1,
		* _weapon2;
	InteractiveSurface* _btnMinimizedIcon;
	SavedGame* _gameSave;
	Surface
		* _battle,
		* _damage,
		* _range1,
		* _range2,
		* _texture,
		* _window;
	Text
		* _txtAmmo1,
		* _txtAmmo2,
		* _txtDistance,
		* _txtInterception,
		* _txtStatus,
		* _txtTitle;
	Timer* _craftDamageAnimTimer;
	Ufo* _ufo;

	/// Moves window to new position.
	void placePort();
	/// Ends the dogfight.
	void endDogfight();

	/// Gets the globe texture icon to display for the interception.
	const std::string getTextureIcon();


	public:
		/// Creates the Dogfight state.
		DogfightState(
					Globe* const globe,
					Craft* const craft,
					Ufo* const ufo,
					GeoscapeState* const geo);
		/// Cleans up the Dogfight state.
		~DogfightState();

		/// Runs the timers.
		void think();
		/// Animates the window.
		void animate();
		/// Updates the craft.
		void updateDogfight();

		/// Fires the first Craft weapon.
		void fireWeapon1();
		/// Fires the second Craft weapon.
		void fireWeapon2();
		/// Fires UFO weapon.
		void ufoFireWeapon();
		/// Sets the Craft to maximum distance.
		void maximumDistance();
		/// Sets the Craft to minimum distance.
		void minimumDistance();

		/// Changes the status text.
		void setStatus(const std::string& status);

		/// Handler for the escape/cancel keyboard button.
		void keyEscape(Action* action);
		/// Handler for pressing the Standoff button.
		void btnStandoffClick(Action* action);
		/// Handler for pressing the Cautious Attack button.
		void btnCautiousClick(Action* action);
		/// Handler for pressing the Standard Attack button.
		void btnStandardClick(Action* action);
		/// Handler for pressing the Aggressive Attack button.
		void btnAggressiveClick(Action* action);
		/// Handler for pressing the Disengage button.
		void btnDisengageClick(Action* action);
		/// Handler for clicking the Ufo button.
		void btnUfoClick(Action* action);
		/// Handler for clicking the Preview graphic.
		void previewClick(Action* action);
		/// Handler for clicking the Minimize dogfight button.
		void btnMinimizeDfClick(Action* action);
		/// Handler for clicking the Maximize dogfight icon.
		void btnMaximizeDfPress(Action* action);

		/// Returns true if state is minimized.
		bool isMinimized() const;
		/// Returns true if Craft stance is in stand-off.
		bool isStandingOff() const;

		/// Draws UFO.
		void drawUfo();
		/// Draws projectiles.
		void drawProjectile(const CraftWeaponProjectile* const prj);
		/// Animates craft damage.
		void animateCraftDamage();
		/// Updates craft damage.
		void drawCraftDamage(bool init = false);

		/// Toggles usage of weapon 1.
		void weapon1Click(Action* action);
		/// Toggles usage of weapon 2.
		void weapon2Click(Action* action);

		/// Changes colors of weapon stuffs.
		void recolor(
				const int hardPt,
				const bool enabled);

		/// Gets interception slot.
		size_t getInterceptSlot() const;
		/// Sets interception slot.
		void setInterceptSlot(const size_t intercept);
		/// Sets total interceptions in progress.
		void setTotalIntercepts(const size_t intercepts);
		/// Calculates and positions this interception's view window.
		void resetInterceptPort(
				size_t dfOpen,
				size_t dfOpenTotal);

		/// Checks if the dogfight should be ended.
		bool dogfightEnded() const;

		/// Gets pointer to the UFO in this dogfight.
		Ufo* getUfo() const;
		/// Gets pointer to the xCom Craft in this dogfight.
		Craft* getCraft() const;

		/// Gets the current distance between UFO and Craft.
		int getDistance() const;
};

}

#endif
