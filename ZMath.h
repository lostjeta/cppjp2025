// ZMath.h
// 수학 유틸리티 함수 및 상수 정의
// Mathematical utility functions and constants

#pragma once
#include <math.h>

// 원주율 상수 (float 정밀도)
// PI constant (float precision)
constexpr float PI = 3.14159265f;

// 원주율 상수 (double 정밀도)
// PI constant (double precision)
constexpr double PI_D = 3.1415926535897932;

// 제곱 함수
// Square function
// @param x: 제곱할 값 (value to square)
// @return: x의 제곱 (x squared)
template <typename T>
constexpr auto sqroot(const T& x)
{
    return x * x;
}

// 각도를 [-PI, PI] 범위로 정규화
// Normalize angle to [-PI, PI] range
// @param theta: 정규화할 각도 (라디안) (angle to normalize in radians)
// @return: [-PI, PI] 범위로 정규화된 각도 (normalized angle in [-PI, PI] range)
template<typename T>
T wrap_angle(T theta)
{
    const T modded = fmod(theta, (T)2.0 * (T)PI_D);
    return (modded > (T)PI_D) ?
        (modded - (T)2.0 * (T)PI_D) :
        modded;
}

// 선형 보간 함수 (Linear Interpolation, LERP)
// Linear interpolation function (LERP)
// @param src: 시작 값 (source value)
// @param dst: 목표 값 (destination value)
// @param alpha: 보간 계수 [0.0, 1.0] (interpolation factor [0.0, 1.0])
// @return: src와 dst 사이의 보간된 값 (interpolated value between src and dst)
template<typename T>
constexpr T lerp(const T& src, const T& dst, float alpha)
{
    return src + (dst - src) * alpha;
}

// 각도를 도(degree)에서 라디안(radian)으로 변환
// Convert angle from degrees to radians
// @param deg: 도 단위 각도 (angle in degrees)
// @return: 라디안 단위 각도 (angle in radians)
template<typename T>
constexpr T to_rad(T deg)
{
    return deg * PI / (T)180.0;
}
