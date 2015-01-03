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

#include "OptionsNoAudioState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Audio Options screen.
 * @param origin Game section that originated this state.
 */
OptionsNoAudioState::OptionsNoAudioState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnAudio);

	_txtError = new Text(218, 136, 94, 8);

	add(_txtError, "text", "audioMenu");

	centerAllSurfaces();


	_txtError->setAlign(ALIGN_CENTER);
	_txtError->setVerticalAlign(ALIGN_MIDDLE);
	_txtError->setBig();
	_txtError->setWordWrap();
	_txtError->setText(tr("STR_NO_AUDIO_HARDWARE_DETECTED"));
}

/**
 * dTor.
 */
OptionsNoAudioState::~OptionsNoAudioState()
{}

}
