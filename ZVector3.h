#pragma once

#include <iostream> // 
class ZMatrix;
class ZVector3 {
public:
	double x, y, z;

	ZVector3();

	ZVector3(double x = 0.0, double y = 0.0, double z = 0.0)
		:x(x), y(y), z(z) { }

	ZVector3(const ZVector3& other) // ������ �����
		: x(other.x), y(other.y), z(other.z) { }
	~ZVector3();

	double Dot(const ZVector3& rhs) const;
	ZVector3 Cross(const ZVector3& rhs) const;
	ZVector3 Scale(double k) const;
	double Length() const;
	double radBetween(const ZVector3& a, const ZVector3& b) const;
	double degBetween(const ZVector3& a, const ZVector3& b) const;
	ZVector3 Normalize() const;
	ZVector3 Transform(const ZMatrix& matrix) const;

	//����(stataic) �Լ�
	static double Dot(const ZVector3& u, const ZVector3& v); // ���� (������� �Լ�)
	static ZVector3 Cross(const ZVector3& u, const ZVector3& v);

	//r value, perfect forwarding ���� �˰������� ����.

	ZVector3 operator + (const ZVector3& other) const;

	ZVector3 operator - (const ZVector3& other) const;
	void operator = (const ZVector3 & rhs);
	ZVector3 operator*(const ZVector3& rhs) const; // ���к� ����(Hadamard Product)
	ZVector3 operator* (double k) const;

	friend std::ostream& operator<<(std::ostream& os, const ZVector3& vec) {
		os << "ZVector3(" << vec.x << ", " << vec.y << ", " << vec.z << " )";
		return os;
	}

	double dot(const ZVector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	ZVector3 cross(const ZVector3& other) const;

	

};