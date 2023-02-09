#include "Utils.h"

#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>

static const float RANDOM_STRENGTH = 5000.f;

namespace Utils {

	
	std::mt19937& localRnd() {

		static std::random_device rd;
		static std::mt19937 rnd(rd());
		return rnd;
	}

	std::string NewID()
	{
		static const std::string base64symbols = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
		static std::uniform_int_distribution<int> distribution(0, base64symbols.size() - 1);

		// 1 billion unique ids is good enough
		constexpr size_t idSize = 5;

		std::string result;
		result.reserve(idSize);

		while (result.size() < idSize)
		{
			const auto index = distribution(localRnd());
			result += base64symbols.at(index);
		}

		return result;
	}

	bool rndYesNo() {
		return rnd0xi(2) == 0;
	}
	
	const std::string& rndStr(const std::vector<std::string>& vec) {

		if (vec.empty()) {
			static const std::string errorStr = "";
			return errorStr; // sad but error.
		}

		return vec[Utils::rnd0xi((unsigned int)vec.size())];
	}

	size_t rnd() {

		static std::default_random_engine rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(0, RANDOM_STRENGTH);
		return (size_t)dist(rng);
	}

	float rnd01() { return float(rnd()) / RANDOM_STRENGTH; }
	float rnd0xf(const float x) { return rnd01() * x; }
	unsigned int rnd0xi(const unsigned int x) { return rnd() % x; }

	float rndfMinMax(const float min, const float max) { return min + rnd01() * (max - min); }
}
