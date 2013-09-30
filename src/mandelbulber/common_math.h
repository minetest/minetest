/*
 * common_math.h
 *
 *  Created on: 2010-01-23
 *      Author: krzysztof marczak
 */

#ifndef COMMON_MATH_H_
#define COMMON_MATH_H_

#include "algebra.hpp"

struct sVectorsAround
{
	double alpha;
	double beta;
	CVector3 v;
	int R;
	int G;
	int B;
	bool notBlack;
};

struct sVector
{
	double x;
	double y;
	double z;
};

struct sSortZ
{
	float z;
	int i;
};

enum enumPerspectiveType
{
	threePoint = 0, fishEye = 1, equirectangular = 2, fishEyeCut = 3
};

//int abs(int v);
int Random(int max);
double dMax(double a, double b, double c);
double dMin(double a, double b, double c);
void QuickSortZBuffer(sSortZ *dane, int l, int p);
CVector3 Projection3D(CVector3 point, CVector3 vp, CRotationMatrix mRot, enumPerspectiveType perspectiveType, double fov, double zoom);
inline double SmoothConditionAGreaterB(double a, double b, double sharpness) {return 1.0 / (1.0 + exp(sharpness * (b - a)));}
inline double SmoothConditionALessB(double a, double b, double sharpness) {return 1.0 / (1.0 + exp(sharpness * (a - b)));}

#endif /* COMMON_MATH_H_ */
