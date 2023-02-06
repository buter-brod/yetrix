#pragma once

#include <random>
#include <string>

// time in microseconds since epoch
typedef unsigned long long time_us;

typedef std::string IDType;
constexpr float timeNotDefined = std::numeric_limits<float>::min();

template <typename ValueType> struct Vec2DBase {

	inline static ValueType notDefined = std::numeric_limits<ValueType>::min();

	ValueType x = notDefined;
	ValueType y = notDefined;

	Vec2DBase(){}
	Vec2DBase(const ValueType xx, const ValueType yy) : x(xx), y(yy){}

	bool operator==(const Vec2DBase& second) const {
		return x == second.x && y == second.y;
	}

	bool operator!=(const Vec2DBase& second) const {
		return !(*this == second);
	}

	bool operator< (const Vec2DBase& second) const {
		if (y != second.y)
			return y < second.y;

		return x < second.x;
	}
};

template <typename ValueType> Vec2DBase<ValueType> operator+(const Vec2DBase<ValueType>& first, const Vec2DBase<ValueType>& second) {

	return {first.x + second.x, first.y + second.y};
}

template <typename ValueType> Vec2DBase<ValueType> operator-(const Vec2DBase<ValueType>& first, const Vec2DBase<ValueType>& second) {

	return {first.x - second.x, first.y - second.y};
}

typedef Vec2DBase<int> Vec2D;

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

	std::mt19937& localRnd();
	std::string NewID();
	static const std::string emptyID = "";
}
