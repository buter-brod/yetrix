#pragma once

#include <string>
#include <vector>

// time in microseconds since epoch
typedef unsigned long long time_us;

namespace Utils {
	float rnd01();
	float rnd0xf(const float x);
	unsigned int rnd0xi(const unsigned int x);
	bool  rndYesNo();
	float rndfMinMax(const float min, const float max);

	/*
	rnd01  is random value 0..1
	rnd0xf  is random value 0..x
	rnd0xi is random integer value 0..x
	*/
}
