/********************************************************
 /                   3D math                             *
 /                                                       *
 / author: Krzysztof Marczak                             *
 / contact: buddhi1980@gmail.com                         *
 / licence: GNU GPL                                      *
 ********************************************************/

#ifndef ALGEBRA_H_
#define ALGEBRA_H_

#include <math.h>

/************************* vector 3D **********************/
class CVector3
{
public:
	inline CVector3()
	{
		x = 0;
		y = 0;
		z = 0;
	}
	inline CVector3(double x_init, double y_init, double z_init)
	{
		x = x_init;
		y = y_init;
		z = z_init;
	}
	inline CVector3(double alfa, double beta)
	{
		x = cos(beta) * cos(alfa);
		y = cos(beta) * sin(alfa);
		z = sin(beta);
	}
	inline CVector3(const CVector3 &vector)
	{
		x = vector.x;
		y = vector.y;
		z = vector.z;
	}
	inline CVector3 operator+(const CVector3 &vector) const
	{
		return CVector3(x + vector.x, y + vector.y, z + vector.z);
	}
	inline CVector3 operator-(const CVector3 &vector) const
	{
		return CVector3(x - vector.x, y - vector.y, z - vector.z);
	}
	inline CVector3 operator*(const double &scalar) const
	{
		return CVector3(x * scalar, y * scalar, z * scalar);
	}
	inline CVector3& operator=(const CVector3 &vector)
	{
		x = vector.x;
		y = vector.y;
		z = vector.z;
		return *this;
	}
	inline CVector3& operator+=(const CVector3 &vector)
	{
		x += vector.x;
		y += vector.y;
		z += vector.z;
		return *this;
	}
	inline CVector3& operator-=(const CVector3 &vector)
	{
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		return *this;
	}
	inline CVector3& operator*=(const double &scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}
	inline double Length() const
	{
		return sqrt(x * x + y * y + z * z);
	}
	inline double Dot(const CVector3& vector) const
	{
		return x * vector.x + y * vector.y + z * vector.z;
	}
	inline CVector3 Cross(const CVector3& v)
	{
		CVector3 c;
		c.x =  y*v.z - z*v.y;
	  c.y = -x*v.z + z*v.x;
	  c.z =  x*v.y - y*v.x;
	  return c;
	}
	inline double Normalize() //returns normalization factor
	{
		double norm = 1.0 / Length();
		x = x * norm;
		y = y * norm;
		z = z * norm;
		return norm;
	}
	inline double GetAlfa() const
	{
		return atan2(y, x);
	}
	inline double GetBeta() const
	{
		return atan2(z, sqrt(x * x + y * y));
	}
	double x;
	double y;
	double z;
};

/************************* matrix 3x3 (fast) *****************/
class CMatrix33
{
public:
	CMatrix33();
	CMatrix33(const CMatrix33 &matrix);
	CMatrix33 operator*(const CMatrix33 &matrix) const;
	CVector3 operator*(const CVector3 &vector) const;
	CMatrix33& operator=(const CMatrix33&);
	double m11;
	double m12;
	double m13;
	double m21;
	double m22;
	double m23;
	double m31;
	double m32;
	double m33;
};

/************************* rotation matrix *******************/
class CRotationMatrix
{
public:
	CRotationMatrix();
	void RotateX(double angle);
	void RotateY(double angle);
	void RotateZ(double angle);
	void Null();
	CVector3 RotateVector(const CVector3& vector) const;
	double GetAlfa() const;
	double GetBeta() const;
	double GetGamma() const;
	void SetRotation(double angles[3]);
	void SetRotation(double alfa, double beta, double gamma);
	CRotationMatrix Transpose(void) const;
	CMatrix33 GetMatrix() {return matrix;}
private:
	CMatrix33 matrix;
	bool zero;
};

#endif /* ALGEBRA_H_ */
