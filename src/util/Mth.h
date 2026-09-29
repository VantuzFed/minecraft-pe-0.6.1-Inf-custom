#ifndef MTH_H__
#define MTH_H__

#include <vector>
#include <set>
#include <algorithm>
#include <cmath>

namespace Mth {
	constexpr float PI = 3.1415926535897932384626433832795028841971f; // exactly!
	constexpr float TWO_PI = 2.0f * PI; // exactly!
	constexpr float DEGRAD = PI / 180.0f;
	const float RADDEG = 180.0f / PI;

	void initMth();

	float sqrt(float x);
	inline double sqrt(double x) { return std::sqrt(x); }
	float invSqrt(float x);

	int floor(float x);
	inline int floor(double x) { return (int)std::floor(x); }

	float sin(float x);
	inline double sin(double x) { return std::sin(x); }
	float cos(float x);
	inline double cos(double x) { return std::cos(x); }

	float atan(float x);
	inline double atan(double x) { return std::atan(x); }
	float atan2(float dy, float dx);
	inline double atan2(double dy, double dx) { return std::atan2(dy, dx); }

	float random();
	int random(int n);

	float abs(float a);
	inline double abs(double a) { return a >= 0.0 ? a : -a; }
	float Min(float a, float b);
	inline double Min(double a, double b) { return a <= b ? a : b; }
	inline double Min(double a, float b) { return a <= b ? a : b; }
	inline double Min(float a, double b) { return a <= b ? a : b; }
	float Max(float a, float b);
	inline double Max(double a, double b) { return a >= b ? a : b; }
	inline double Max(double a, float b) { return a >= b ? a : b; }
	inline double Max(float a, double b) { return a >= b ? a : b; }
	int abs(int a);
	int Min(int a, int b);
	int Max(int a, int b);

	int   clamp(int v, int low, int high);
	float clamp(float v, float low, float high);
	inline double clamp(double v, double low, double high) { return v < low ? low : (v > high ? high : v); }
	inline double clamp(float v, double low, float high) { return v < low ? low : (v > high ? high : v); }
	inline double clamp(float v, double low, double high) { return v < low ? low : (v > high ? high : v); }
	inline double clamp(double v, float low, double high) { return v < low ? low : (v > high ? high : v); }
	inline double clamp(double v, double low, float high) { return v < low ? low : (v > high ? high : v); }
	float lerp(float src, float dst, float alpha);
	inline double lerp(double src, double dst, double alpha) { return src + (dst - src) * alpha; }
	inline double lerp(float src, double dst, float alpha) { return src + (dst - src) * alpha; }
	inline double lerp(double src, float dst, float alpha) { return src + (dst - src) * alpha; }
	int   lerp(int src, int dst, float alpha);

	///@param value The original signed value
	///@param with  The (possibly signed) value to "abs-decrease" <value> with
	///@param min   The minimum value
	float absDecrease(float value, float with, float min);
	//float absIncrease(float value, float with, float max);

	float absMax(float a, float b);
	inline double absMax(double a, double b) {
		double aa = abs(a);
		double ab = abs(b);
		return aa >= ab ? aa : ab;
	}
	float absMaxSigned(float a, float b);

	int intFloorDiv(int a, int b);
};

namespace Util
{
	template <class T>
	int removeAll(std::vector<T>& superset, const std::vector<T>& toRemove) {
		int subSize = (int)toRemove.size();
		int removed = 0;

		for (int i = 0; i < subSize; ++i) {
			T elem = toRemove[i];
			int size = (int)superset.size();
			for (int j = 0; j < size; ++j) {
				if (elem == superset[j]) {
					superset.erase( superset.begin() + j, superset.begin() + j + 1);
					++removed;
					break;
				}
			}
		}
		return removed;
	}

	template <class T>
	bool remove(std::vector<T>& list, const T& instance) {
		typename std::vector<T>::iterator it = std::find(list.begin(), list.end(), instance);
		if (it == list.end())
			return false;

		list.erase(it);
		return true;
	}

	// Could perhaps do a template<template ..>
	template <class T>
	bool remove(std::set<T>& list, const T& instance) {
		typename std::set<T>::iterator it = std::find(list.begin(), list.end(), instance);
		if (it == list.end())
			return false;

		list.erase(it);
		return true;
	}
};

#endif // MTH_H__
