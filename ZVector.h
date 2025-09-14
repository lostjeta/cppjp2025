#pragma once

#include <iostream> // 

class ZVector3 {
public:
	double x, y, z;

	ZVector3(double x = 0.0, double y = 0.0, double z = 0.0)
		:x(x), y(y), z(z) { }

	ZVector3(const ZVector3& other) // 생성자 만들기
		: x(other.x), y(other.y), z(other.z) { }

	//r value, perfect forwarding 개념 알고있으면 좋다.

	ZVector3 operator + (const ZVector3& other) const {
		return ZVector3(x + other.x, y + other.y, z + other.z);
	} // 백터 더하기

	ZVector3 operator - (const ZVector3& other) const {
		return ZVector3(x - other.x, y - other.y, z - other.z);
	}

	double dot(const ZVector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	ZVector3 cross(const ZVector3& other) const {
		return ZVector3(
			y*other.z - z*other.y,
			z * other.x - 
		)
	}

	friend std::ostream& operator<<(std::ostream& os, const ZVector3& vec) {
		os << "ZVector3(" << vec.x << ", " << vec.y << ", " << vec.z << " )";
		return os;
	}

};