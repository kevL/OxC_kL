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

#ifndef OPENXCOM_RNG_H
#define OPENXCOM_RNG_H

#include <algorithm>

#define __STDC_LIMIT_MACROS

//#include <stdint.h>
#include <cstdint>


namespace OpenXcom
{

/**
 * Random Number Generator used throughout the game
 * for all your random needs. Uses a 64-bit xorshift
 * pseudorandom number generator.
 */
namespace RNG
{

	/// Gets the seed in use.
	uint64_t getSeed();
	/// Sets the seed in use.
	void setSeed(uint64_t seed);

	/// Generates a random integer number, inclusive.
	int generate(
			int minRand,
			int maxRand);
	/// Generates a random floating-point number.
	double generate(
			double minRand,
			double maxRand);

	/// Get normally distributed value.
	double boxMuller(
			double mean = 0,
			double deviation = 1);

	/// Generates a percentage chance.
	bool percent(int value);

	/// Generates a random integer number, exclusive.
	int generateEx(int maxRand);


	/// Shuffles a list randomly.
	/**
	 * Randomly changes the orders of the elements in a list.
	 * @param container - the container to randomize
	 */
	template <typename T>
	void shuffle(T& container)
	{
		std::random_shuffle(
						container.begin(),
						container.end(),
						generateEx);
	}
}

}

#endif
