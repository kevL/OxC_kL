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

#ifndef OPENXCOM_CONFIRMLANDINGSTATE_H
#define OPENXCOM_CONFIRMLANDINGSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class RuleCity;
class RuleTerrain;
class RuleTexture;
class Text;
class TextButton;
class Window;


/**
 * Window that asks the player to confirm a Craft landing at its destination.
 */
class ConfirmLandingState
	:
		public State
{

private:
	int _shade;

	RuleCity* _city;
	Craft* _craft;
	RuleTerrain* _terrainRule;
	RuleTexture* _texture;
	Text
		* _txtBegin,
		* _txtMessage,
		* _txtMessage2,

		* _txtBase,
		* _txtShade,
		* _txtTexture;
	TextButton
		* _btnNo,
		* _btnYes;
	Window* _window;


	public:
		/// Creates the Confirm Landing state.
		ConfirmLandingState(
				Craft* const craft,
				RuleTexture* texture = NULL,
				const int shade = -1);
		/// Cleans up the Confirm Landing state.
		~ConfirmLandingState();

		/// initialize the state, make a sanity check.
		void init();

		/// Selects a terrain type for crashed or landed UFOs.
//		RuleTerrain* selectTerrain(const double lat);
		/// Selects a terrain type for missions at cities.
//		RuleTerrain* selectCityTerrain(const double lat);

		/// Handler for clicking the Yes button.
		void btnYesClick(Action* action);
		/// Handler for clicking the No button.
		void btnNoClick(Action* action);
};

}

#endif
