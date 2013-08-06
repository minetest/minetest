/*
 * algebra.cpp
 *
 *  Created on: 2010-02-28
 *      Author: krzysztof
 */
#include "algebra.hpp"

/***************** class CMatrix33 ***********************/
CMatrix33::CMatrix33()
{
	m11 = 0;
	m12 = 0;
	m13 = 0;
	m21 = 0;
	m22 = 0;
	m23 = 0;
	m31 = 0;
	m32 = 0;
	m33 = 0;
}

CMatrix33::CMatrix33(const CMatrix33 &matrix)
{
	m11 = matrix.m11;
	m12 = matrix.m12;
	m13 = matrix.m13;
	m21 = matrix.m21;
	m22 = matrix.m22;
	m23 = matrix.m23;
	m31 = matrix.m31;
	m32 = matrix.m32;
	m33 = matrix.m33;
}

CMatrix33& CMatrix33::operator=(const CMatrix33 &matrix)
{
	m11 = matrix.m11;
	m12 = matrix.m12;
	m13 = matrix.m13;
	m21 = matrix.m21;
	m22 = matrix.m22;
	m23 = matrix.m23;
	m31 = matrix.m31;
	m32 = matrix.m32;
	m33 = matrix.m33;
	return *this;
}

CMatrix33 CMatrix33::operator*(const CMatrix33 &matrix) const
{
	CMatrix33 result;
	result.m11 = m11 * matrix.m11 + m12 * matrix.m21 + m13 * matrix.m31;
	result.m12 = m11 * matrix.m12 + m12 * matrix.m22 + m13 * matrix.m32;
	result.m13 = m11 * matrix.m13 + m12 * matrix.m23 + m13 * matrix.m33;
	result.m21 = m21 * matrix.m11 + m22 * matrix.m21 + m23 * matrix.m31;
	result.m22 = m21 * matrix.m12 + m22 * matrix.m22 + m23 * matrix.m32;
	result.m23 = m21 * matrix.m13 + m22 * matrix.m23 + m23 * matrix.m33;
	result.m31 = m31 * matrix.m11 + m32 * matrix.m21 + m33 * matrix.m31;
	result.m32 = m31 * matrix.m12 + m32 * matrix.m22 + m33 * matrix.m32;
	result.m33 = m31 * matrix.m13 + m32 * matrix.m23 + m33 * matrix.m33;
	return result;
}

CVector3 CMatrix33::operator*(const CVector3 &vector) const
{
	CVector3 result;
	result.x = m11 * vector.x + m12 * vector.y + m13 * vector.z;
	result.y = m21 * vector.x + m22 * vector.y + m23 * vector.z;
	result.z = m31 * vector.x + m32 * vector.y + m33 * vector.z;
	return result;
}

/**************** class RotarionMatrix **********************/
CRotationMatrix::CRotationMatrix()
{
	matrix.m11 = 1.0;
	matrix.m12 = 0.0;
	matrix.m13 = 0.0;
	matrix.m21 = 0.0;
	matrix.m22 = 1.0;
	matrix.m23 = 0.0;
	matrix.m31 = 0.0;
	matrix.m32 = 0.0;
	matrix.m33 = 1.0;
	zero = true;
}

void CRotationMatrix::RotateX(double angle)
{
	if (angle != 0.0)
	{
		CMatrix33 rot;
		double s = sin(angle);
		double c = cos(angle);
		rot.m11 = 1.0;
		rot.m22 = c;
		rot.m33 = c;
		rot.m23 = -s;
		rot.m32 = s;
		matrix = matrix * rot;
		zero = false;
	}
}

void CRotationMatrix::RotateY(double angle)
{
	if (angle != 0.0)
	{
		CMatrix33 rot;
		double s = sin(angle);
		double c = cos(angle);
		rot.m22 = 1.0;
		rot.m33 = c;
		rot.m11 = c;
		rot.m31 = -s;
		rot.m13 = s;
		matrix = matrix * rot;
		zero = false;
	}
}

void CRotationMatrix::RotateZ(double angle)
{
	if (angle != 0.0)
	{
		CMatrix33 rot;
		double s = sin(angle);
		double c = cos(angle);
		rot.m33 = 1.0;
		rot.m11 = c;
		rot.m22 = c;
		rot.m12 = -s;
		rot.m21 = s;
		matrix = matrix * rot;
		zero = false;
	}
}

void CRotationMatrix::SetRotation(double angles[3])
{
	Null();
	RotateZ(angles[2]);
	RotateY(angles[1]);
	RotateX(angles[0]);
}

void CRotationMatrix::SetRotation(double alfa, double beta, double gamma)
{
	Null();
	RotateZ(alfa);
	RotateY(beta);
	RotateX(gamma);
}

CVector3 CRotationMatrix::RotateVector(const CVector3& vector) const
{
	if (!zero)
	{
		CVector3 vector2 = matrix * vector;
		return vector2;
	}
	else
	{
		return vector;
	}
}

void CRotationMatrix::Null()
{
	//CRotationMatrix();
	matrix.m11 = 1.0;
	matrix.m12 = 0.0;
	matrix.m13 = 0.0;
	matrix.m21 = 0.0;
	matrix.m22 = 1.0;
	matrix.m23 = 0.0;
	matrix.m31 = 0.0;
	matrix.m32 = 0.0;
	matrix.m33 = 1.0;
	zero = true;
}

double CRotationMatrix::GetAlfa() const
{
	return atan2(matrix.m12,matrix.m22);
}

double CRotationMatrix::GetBeta() const
{
	return asin(-matrix.m32);
}

double CRotationMatrix::GetGamma() const
{
	return atan2(matrix.m31,matrix.m33);
}

CRotationMatrix CRotationMatrix::Transpose() const
{
	CRotationMatrix m;
	m.matrix.m11 = matrix.m11;
	m.matrix.m12 = matrix.m21;
	m.matrix.m13 = matrix.m31;
	m.matrix.m21 = matrix.m12;
	m.matrix.m22 = matrix.m22;
	m.matrix.m23 = matrix.m32;
	m.matrix.m31 = matrix.m13;
	m.matrix.m32 = matrix.m23;
	m.matrix.m33 = matrix.m33;
	m.zero = false;
	return m;
}
