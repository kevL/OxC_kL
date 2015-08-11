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

#ifndef OPENXCOM_MEDIKITVIEW_H
#define OPENXCOM_MEDIKITVIEW_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class BattleUnit;
class Text;


/**
 * Display a view of unit wounds
 */
class MedikitView
	:
		public InteractiveSurface
{

private:
	static const std::string BODY_PARTS[];

	size_t _selectedPart;

	BattleUnit* _unit;
	Game* _game;
	Text
		* _txtPart,
		* _txtWound;

	/// Handles clicking on the body view.
	void mouseClick(Action* action, State* state);


	public:
		/// Creates the MedikitView.
		MedikitView(
				int w,
				int h,
				int x,
				int y,
				Game* game,
				BattleUnit* unit,
				Text* part,
				Text* wound);

		/// Draws the body view.
		void draw();

		/// Gets the selected body part.
		size_t getSelectedPart() const;
		/// Automatically selects a wounded body part.
		void autoSelectPart();
};

}

#endif
