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

const int
	DIST_ENGAGE		= 650,
	DIST_STANDOFF	= 580,
	TIMEOUT			= 38;

enum ColorNames
{
	CRAFT_MIN,	// 0
	CRAFT_MAX,	// 1
	RADAR_MIN,	// 2
	RADAR_MAX,	// 3
	DAMAGE_MIN,	// 4
	DAMAGE_MAX,	// 5
	BLOB_MIN,	// 6
	RANGE_METER	// 7
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
 * Shows a dogfight (interception) between a player craft and a UFO.
 */
class DogfightState
	:
		public State
{

private:
	static const int _projectileBlobs[4][6][3];

	bool
		_end,
		_destroyUfo,
		_destroyCraft,
		_ufoBreakingOff,
		_weapon1Enabled,
		_weapon2Enabled,
		_minimized,
		_endDogfight,
		_animatingHit;
	int
		_diff,
		_timeout,
		_dist,
		_targetDist,

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
		_intercept,
		_interceptQty;
	Uint8
		_colors[8], // see ColorNames enum above^
		_currentCraftDamageColor;

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
		* _btnUfo;
	ImageButton* _mode;
	InteractiveSurface
		* _btnMinimize,
		* _preview,
		* _weapon1,
		* _weapon2;
	InteractiveSurface* _btnMinimizedIcon;
	SavedGame* _savedGame;
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
		* _txtStatus,
		* _txtInterception;
	Timer* _craftDamageAnimTimer;
	Ufo* _ufo;

	/// Ends the dogfight.
	void endDogfight();

	/// Gets the globe texture icon to display for the interception.
	const std::string getTextureIcon();


	public:
		/// Creates the Dogfight state.
		DogfightState(
					Globe* globe,
					Craft* craft,
					Ufo* ufo,
					GeoscapeState* geo);
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
		/// Sets the Craft to minimum distance.
		void minimumDistance();
		/// Sets the Craft to maximum distance.
		void maximumDistance();

		/// Changes the status text.
		void setStatus(const std::string& status);

		/// Handler for clicking the Minimize button.
		void btnMinimizeClick(Action* action);
		/// Handler for pressing the Standoff button.
		void btnStandoffPress(Action* action);
		/// Handler for pressing the Cautious Attack button.
		void btnCautiousPress(Action* action);
		/// Handler for pressing the Standard Attack button.
		void btnStandardPress(Action* action);
		/// Handler for pressing the Aggressive Attack button.
		void btnAggressivePress(Action* action);
		/// Handler for pressing the Disengage button.
		void btnDisengagePress(Action* action);
		/// Handler for clicking the Ufo button.
		void btnUfoClick(Action* action);
		/// Handler for clicking the Preview graphic.
		void previewPress(Action* action);

		/// Draws UFO.
		void drawUfo();
		/// Draws projectiles.
		void drawProjectile(const CraftWeaponProjectile* proj);
		/// Animates craft damage.
		void animateCraftDamage();
		/// Updates craft damage.
		void drawCraftDamage();

		/// Toggles usage of weapon 1.
		void weapon1Click(Action* action);
		/// Toggles usage of weapon 2.
		void weapon2Click(Action* action);

		/// Changes colors of weapon icons, range indicators and ammo texts base on current weapon state.
		void recolor(
				const int weaponPod,
				const bool enabled);

		/// Returns true if state is minimized.
		bool isMinimized() const;
		/// Sets state minimized or maximized.
		void setMinimized(const bool minimized);
		/// Handler for clicking the minimized interception window icon.
		void btnMinimizedIconClick(Action* action);

		/// Gets interception number.
		size_t getInterceptSlot() const;
		/// Sets interception number.
		void setInterceptSlot(const size_t intercept);
		/// Sets interceptions count.
		void setInterceptQty(const size_t intercepts);

		/// Calculates window position according to opened interception windows.
		void calculateWindowPosition();
		/// Moves window to new position.
		void moveWindow();

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
