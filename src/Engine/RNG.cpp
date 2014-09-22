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

#include "RNG.h"

//#include <math.h>
//#include <time.h>
#include <cmath>
#include <ctime>


namespace OpenXcom
{

namespace RNG
{

/*	Written in 2014 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/*	This is a good generator if you're short on memory, but otherwise we
	rather suggest to use a xorshift128+ (for maximum speed) or
	xorshift1024* (for speed and very long period) generator. */



uint64_t x = time(0); /* The state must be seeded with a nonzero value. */


uint64_t next()
{
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c

	return x * 2685821657736338717LL;
}

/**
* Returns the current seed in use by the generator.
* @return, current seed
*/
uint64_t getSeed()
{
	return x;
}

/**
* Changes the current seed in use by the generator.
* @param n - new seed
*/
void setSeed(uint64_t n)
{
	x = n;
}

/**
 * Generates a random integer number within a certain range.
 * @param min - minimum number, inclusive
 * @param max - maximum number, inclusive
 * @return, generated number
 */
int generate(
		int min,
		int max)
{
	uint64_t rand = next();

	if (max < min)	// CyberAngel
		max = min;	// http://openxcom.org/bugs/openxcom/issues/736

	return (static_cast<int>(rand %(max - min + 1)) + min);
}

/**
 * Generates a random decimal number within a certain range.
 * @param min - minimum number
 * @param max - maximum number
 * @return, generated number
 */
double generate(
		double min,
		double max)
{
//kL	double rand = static_cast<double>(next());

	// kL_begin:
	double diff = max - min;
	if (AreSame(diff, 0.0))	// kL
		return min;			// kL

//	diff = (static_cast<double>(UINT64_MAX) / diff);
	diff = (static_cast<double>(std::numeric_limits<uint64_t>::max()) / diff);
	if (AreSame(diff, 0.0))	// kL
		return min;			// kL

	double rand = static_cast<double>(next());

	return ((rand / diff) + min);
	// kL_end.


//kL	return static_cast<double>(rand / (static_cast<double>(UINT64_MAX) / (max - min)) + min);
//		return (double)(rand / ((double)std::numeric_limits<uint64_t>::max() / (max - min)) + min); // AMDmi3 note.

//	return (rand / (static_cast<double>(UINT64_MAX) / (max - min)) + min); // kL
}

/**
 * Normal random variate generator.
 * @param mean				- mean
 * @param standardDeviation	- standard deviation
 * @return, normally distributed value
 */
double boxMuller(
		double mean,
		double standardDeviation)
{
//kL	static int use_last = 0;
	static bool use_last = false; // kL

	static double y2;
	double y1;

	if (use_last) /* use value from previous call */
	{
		y1 = y2;
//kL	use_last = 0;
		use_last = false; // kL
	}
	else
	{
		double
			x1,
			x2,
			w;

		do
		{
			x1 = 2.0 * generate(0.0, 1.0) - 1.0;
			x2 = 2.0 * generate(0.0, 1.0) - 1.0;
			w = (x1 * x1) + (x2 * x2);
		}
		while (w >= 1.0);

		w = sqrt((-2.0 * log(w)) / w);
		y1 = x1 * w;
		y2 = x2 * w;

//kL	use_last = 1;
		use_last = true; // kL
	}

	return (mean + (y1 * standardDeviation));
}

/**
 * Generates a random percent chance of an event occuring and returns the result.
 * @param value - value percentage (0-100%)
 * @return, true if the chance succeeded
 */
bool percent(int value)
{
	if (value < 1)
		return false; // kL
	else if (value > 99)
		return true; // kL

	return (generate(0, 99) < value);
}

/**
 * Generates a random positive integer up to a number.
 * @param max - maximum number, exclusive
 * @return, generated number
 */
int generateEx(int max)
{
	if (max < 2)	// kL
		return 0;	// kL

	uint64_t rand = next();

	return static_cast<int>(rand %max);
}

}

}
