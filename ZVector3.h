#pragma once

#include <iostream>

class ZMatrix;

class ZVector3 {
public:
	double x, y, z;

	ZVector3();
	ZVector3(double x = 0.0, double y = 0.0, double z = 0.0);
	ZVector3(const ZVector3& other);
	~ZVector3();

	// ��� �Լ�
	double Dot(const ZVector3& rhs) const;
	ZVector3 Cross(const ZVector3& rhs) const;
	ZVector3 Scale(double k) const;
	double Length() const;
	double radBetween(const ZVector3& a, const ZVector3& b) const;
	double degBetween(const ZVector3& a, const ZVector3& b) const;
	ZVector3 Normalize() const;
	ZVector3 Transform(const ZMatrix& matrix) const;

	// ����(static) �Լ�
	static double Dot(const ZVector3& u, const ZVector3& v); // ���� (���� ��� �Լ�)
	static ZVector3 Cross(const ZVector3& u, const ZVector3& v); // ���� (���� ��� �Լ�)

	ZVector3 operator+(const ZVector3& other) const;
	ZVector3 operator-(const ZVector3& other) const;
	void operator=(const ZVector3& rhs);
	ZVector3 operator*(const ZVector3& rhs) const; // ���к� ���� (Hadamard Product)
	ZVector3 operator*(double k) const;


	friend std::ostream& operator<<(std::ostream& os, const ZVector3& vec) {
		os << "ZVector3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
		return os;
	}
};