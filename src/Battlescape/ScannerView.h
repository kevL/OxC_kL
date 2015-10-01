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

#ifndef OPENXCOM_SCANNERVIEW_H
#define OPENXCOM_SCANNERVIEW_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class BattleUnit;
class Game;


/**
 * Displays a view of units movement.
 */
class ScannerView
	:
		public InteractiveSurface
{

private:
	bool _dotsDone;
	int _frame;

	const BattleUnit* _unit;
	const Game* _game;

	///
	void mouseClick(Action* action, State* state);


	public:
		/// Create a ScannerView.
		ScannerView(
				int w,
				int h,
				int x,
				int y,
				const Game* const game,
				const BattleUnit* const unit);

		/// Draw the scanner.
		void draw();
		/// Animate the blobs.
		void animate();
};

}

#endif
