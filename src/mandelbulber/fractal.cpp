/********************************************************
 /                   MANDELBULBER                        *
 /                                                       *
 / author: Krzysztof Marczak                             *
 / contact: buddhi1980@gmail.com                         *
 / licence: GNU GPL                                      *
 ********************************************************/

/*
 * fractal.cpp
 *
 *  Created on: 2010-01-23
 *      Author: krzysztof
 */

//#include "Render3D.h"
//#include "interface.h"
//#include "primitives.h"
#include <stdlib.h>

#include "fractal.h"
#include "algebra.cpp"
/**
 * Compute the fractal at the point, in one of the various modes
 *
 * Mode: normal: Returns distance
 *		 fake_ao: Returns minimum radius
 *		 colouring: Returns colour index
 *		 delta_DE1, delta_DE2: Returns radius
 */

unsigned int MixNumbers(double a, double b, double c)
{
	unsigned int result = int(a*12356312.0)^int(b*23564234.0)^int(c*35564353.0);
	//printf("%ud\n", result);
	return result;
}

int Noise(int seed)
{
	int x = seed;
	x = (x<<7) ^ x;
	return abs((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff);
}


template<int Mode>
double Compute(CVector3 z, const sFractal &par, int *iter_count)
{
	int L;
	double distance = 0;

	double w = 0;
	double constantw = 0;

	CVector3 dz(1.0, 0.0, 0.0);
	CVector3 one(1.0, 0.0, 0.0);
	double r_dz = 1;
	double ph_dz = 0;
	double th_dz = 0;
	double p = par.doubles.power; //mandelbulb power
	int N = par.doubles.N;

	CVector3 constant;

	double fixedRadius = par.mandelbox.doubles.foldingSphericalFixed;
	double fR2 = fixedRadius * fixedRadius;
	double minRadius = par.mandelbox.doubles.foldingSphericalMin;
	double mR2 = minRadius * minRadius;
	double tglad_factor1 = fR2 / mR2;

	double tgladDE = 1.0;

	double scale = par.mandelbox.doubles.scale;

	enumFractalFormula actualFormula = par.formula;
	if (actualFormula == kaleidoscopic || actualFormula == menger_sponge) 
		tgladDE = 1.0;

	double tgladColor = 1.0;

	if (par.juliaMode)
	{
		constant = par.doubles.julia;
	}
	else
	{
		constant = z * par.doubles.constantFactor;
	}

	bool hybridEnabled = false;
	if (actualFormula == hybrid) hybridEnabled = true;

	double r = z.Length();

	double min = 1e200;
	for (L = 0; L < N; L++)
	{
		if (hybridEnabled)
		{
			int tempL = L;
			if(tempL > (int)par.formulaSequence.size()-1) tempL = (int)par.formulaSequence.size()-1;
			actualFormula = par.formulaSequence[tempL];
			p = par.hybridPowerSequence[tempL];
			scale = p;
		}

		if (par.IFS.foldingMode)
		{
			if (par.IFS.absX) z.x = fabs(z.x);
			if (par.IFS.absY) z.y = fabs(z.y);
			if (par.IFS.absZ) z.z = fabs(z.z);

			for (int i = 0; i < IFS_VECTOR_COUNT; i++)
			{
				if (par.IFS.enabled[i])
				{
					z = par.IFS.rot[i].RotateVector(z);
					double length = z.Dot(par.IFS.doubles.direction[i]);

					if (length < par.IFS.doubles.distance[i])
					{
						z -= par.IFS.doubles.direction[i] * 2.0 * (length - par.IFS.doubles.distance[i]);
					}

				}
			}

			z = par.IFS.mainRot.RotateVector(z - par.IFS.doubles.offset) + par.IFS.doubles.offset;
			z *= par.IFS.doubles.scale;
			z -= par.IFS.doubles.offset * (par.IFS.doubles.scale - 1.0);

			r = z.Length();
		}

		if (par.tgladFoldingMode)
		{
			if (z.x > par.doubles.foldingLimit)
			{
				z.x = par.doubles.foldingValue - z.x;
				tgladColor *= 0.9;
			}
			else if (z.x < -par.doubles.foldingLimit)
			{
				z.x = -par.doubles.foldingValue - z.x;
				tgladColor *= 0.9;
			}
			if (z.y > par.doubles.foldingLimit)
			{
				z.y = par.doubles.foldingValue - z.y;
				tgladColor *= 0.9;
			}
			else if (z.y < -par.doubles.foldingLimit)
			{
				z.y = -par.doubles.foldingValue - z.y;
				tgladColor *= 0.9;
			}
			if (z.z > par.doubles.foldingLimit)
			{
				z.z = par.doubles.foldingValue - z.z;
				tgladColor *= 0.9;
			}
			else if (z.z < -par.doubles.foldingLimit)
			{
				z.z = -par.doubles.foldingValue - z.z;
				tgladColor *= 0.9;
			}
			r = z.Length();
		}

		if (par.sphericalFoldingMode)
		{
			double fR2_2 = par.doubles.foldingSphericalFixed * par.doubles.foldingSphericalFixed;
			double mR2_2 = par.doubles.foldingSphericalMin * par.doubles.foldingSphericalMin;
			double r2_2 = r * r;
			double tglad_factor1_2 = fR2_2 / mR2_2;

			if (r2_2 < mR2_2)
			{
				z = z * tglad_factor1_2;
				tgladDE *= tglad_factor1_2;
				tgladColor += 1;
			}
			else if (r2_2 < fR2_2)
			{
				double tglad_factor2_2 = fR2_2 / r2_2;
				z = z * tglad_factor2_2;
				tgladDE *= tglad_factor2_2;
				tgladColor += 10;
			}
			r = z.Length();
		}

		switch (actualFormula)
		{
			case trig_DE:
			{
				double r1 = pow(r, p - 1);
				double r2 = r1 * r;
				double th = z.GetAlfa();
				double ph = -z.GetBeta();
				if (Mode == 0)
				{
					double p_r1_rdz = p * r1 * r_dz;
					double ph_phdz = (p - 1.0) * ph + ph_dz;
					double th_thdz = (p - 1.0) * th + th_dz;
					CVector3 rot(th_thdz, ph_phdz);
					dz = rot * p_r1_rdz + one;
					r_dz = dz.Length();
					th_dz = dz.GetAlfa();
					ph_dz = -dz.GetBeta();
				}
				CVector3 rot(p * th, p * ph);
				z = rot * r2 + constant;
				r = z.Length();
				break;
			}
			case trig_optim:
			{
				//optimisation based on: http://www.fractalforums.com/mandelbulb-implementation/realtime-renderingoptimisations/
				double th0 = asin(z.z / r);
				double ph0 = atan2(z.y, z.x);
				double rp = pow(r, p - 1.0);
				double th = th0 * p;
				double ph = ph0 * p;
				double cth = cos(th);
				r_dz = rp * r_dz * p + 1.0;
				rp *= r;
				z = CVector3(cth * cos(ph), cth * sin(ph), sin(th)) * rp + constant;
				r = z.Length();
				break;
			}
			case mandelbulb2:
			{
				double temp, tempR;
				tempR = sqrt(z.x * z.x + z.y * z.y);
				z *= (1.0 / tempR);
				temp = z.x * z.x - z.y * z.y;
				z.y = 2.0 * z.x * z.y;
				z.x = temp;
				z *= tempR;

				tempR = sqrt(z.y * z.y + z.z * z.z);
				z *= (1.0 / tempR);
				temp = z.y * z.y - z.z * z.z;
				z.z = 2.0 * z.y * z.z;
				z.y = temp;
				z *= tempR;

				tempR = sqrt(z.x * z.x + z.z * z.z);
				z *= (1.0 / tempR);
				temp = z.x * z.x - z.z * z.z;
				z.z = 2.0 * z.x * z.z;
				z.x = temp;
				z *= tempR;

				z = z * r;
				z += constant;
				r = z.Length();
				break;
			}
			case mandelbulb3:
			{
				double temp, tempR;

				double sign = 1.0;
				double sign2 = 1.0;

				if (z.x < 0) sign2 = -1.0;
				tempR = sqrt(z.x * z.x + z.y * z.y);
				z *= (1.0 / tempR);
				temp = z.x * z.x - z.y * z.y;
				z.y = 2.0 * z.x * z.y;
				z.x = temp;
				z *= tempR;

				if (z.x < 0) sign = -1.0;
				tempR = sqrt(z.x * z.x + z.z * z.z);
				z *= (1.0 / tempR);
				temp = z.x * z.x - z.z * z.z;
				z.z = 2.0 * z.x * z.z * sign2;
				z.x = temp * sign;
				z *= tempR;

				z = z * r;
				z += constant;
				r = z.Length();
				break;
			}
			case mandelbulb4:
			{
				double rp = pow(r, p - 1);

				double angZ = atan2(z.y, z.x);
				double angY = atan2(z.z, z.x);
				double angX = atan2(z.z, z.y);

				CRotationMatrix rotM;
				rotM.RotateX(angX * (p - 1));
				rotM.RotateY(angY * (p - 1));
				rotM.RotateZ(angZ * (p - 1));

				z = rotM.RotateVector(z) * rp + constant;
				r = z.Length();
				break;
			}
			case xenodreambuie:
			{
				double rp = pow(r, p);
				double th = atan2(z.y, z.x);
				double ph = acos(z.z / r);
				if (ph > 0.5 * M_PI)
				{
					ph = M_PI - ph;
				}
				else if (ph < -0.5 * M_PI)
				{
					ph = -M_PI - ph;
				}
				z.x = rp * cos(th * p) * sin(ph * p);
				z.y = rp * sin(th * p) * sin(ph * p);
				z.z = rp * cos(ph * p);
				z = z + constant;

				r = z.Length();
				break;
			}
			case fast_trig:
			{
				double x2 = z.x * z.x;
				double y2 = z.y * z.y;
				double z2 = z.z * z.z;
				double temp = 1.0 - z2 / (x2 + y2);
				double newx = (x2 - y2) * temp;
				double newy = 2.0 * z.x * z.y * temp;
				double newz = -2.0 * z.z * sqrt(x2 + y2);
				z.x = newx + constant.x;
				z.y = newy + constant.y;
				z.z = newz + constant.z;
				r = z.Length();
				break;
			}
			case minus_fast_trig:
			{
				double x2 = z.x * z.x;
				double y2 = z.y * z.y;
				double z2 = z.z * z.z;
				double temp = 1.0 - z2 / (x2 + y2);
				double newx = (x2 - y2) * temp;
				double newy = 2.0 * z.x * z.y * temp;
				double newz = 2.0 * z.z * sqrt(x2 + y2);
				z.x = newx + constant.x;
				z.y = newy + constant.y;
				z.z = newz + constant.z;
				r = z.Length();
				break;
			}
			case hypercomplex:
			{
				CVector3 newz(z.x * z.x - z.y * z.y - z.z * z.z - w * w, 2.0 * z.x * z.y - 2.0 * w * z.z, 2.0 * z.x * z.z - 2.0 * z.y * w);
				double neww = 2.0 * z.x * w - 2.0 * z.y * z.z;
				z = newz + constant;
				w = neww;
				r = sqrt(z.x * z.x + z.y * z.y + z.z * z.z + w * w);
				break;
			}
			case quaternion:
			{
				CVector3 newz(z.x * z.x - z.y * z.y - z.z * z.z - w * w, 2.0 * z.x * z.y, 2.0 * z.x * z.z);
				double neww = 2.0 * z.x * w;
				z = newz + constant;
				w = neww;
				r = sqrt(z.x * z.x + z.y * z.y + z.z * z.z + w * w);
				break;
			}
			case menger_sponge:
			{
				double temp;
				z.x = fabs(z.x);
				z.y = fabs(z.y);
				z.z = fabs(z.z);
				if (z.x - z.y < 0)
				{
					temp = z.y;
					z.y = z.x;
					z.x = temp;
				}
				if (z.x - z.z < 0)
				{
					temp = z.z;
					z.z = z.x;
					z.x = temp;
				}
				if (z.y - z.z < 0)
				{
					temp = z.z;
					z.z = z.y;
					z.y = temp;
				}

				if (Mode == colouring)
				{
					double length2 = z.Length();
					if (length2 < min) min = length2;
				}

				z *= 3.0;

				z.x -= 2.0;
				z.y -= 2.0;
				if (z.z > 1.0) z.z -= 2.0;
				r = z.Length();
				tgladDE *= 3.0;
				break;
			}
			case tglad:
			{
				if (par.mandelbox.rotationsEnabled)
				{
					bool lockout = false;
					z = par.mandelbox.rot[0][0].RotateVector(z);
					if (z.x > par.mandelbox.doubles.foldingLimit)
					{
						z.x = par.mandelbox.doubles.foldingValue - z.x;
						tgladColor += par.mandelbox.doubles.colorFactorX;
						lockout = true;
					}
					z = par.mandelbox.rotinv[0][0].RotateVector(z);

					z = par.mandelbox.rot[1][0].RotateVector(z);
					if (!lockout && z.x < -par.mandelbox.doubles.foldingLimit)
					{
						z.x = -par.mandelbox.doubles.foldingValue - z.x;
						tgladColor += par.mandelbox.doubles.colorFactorX;
					}
					z = par.mandelbox.rotinv[1][0].RotateVector(z);

					lockout = false;
					z = par.mandelbox.rot[0][1].RotateVector(z);
					if (z.y > par.mandelbox.doubles.foldingLimit)
					{
						z.y = par.mandelbox.doubles.foldingValue - z.y;
						tgladColor += par.mandelbox.doubles.colorFactorY;
						lockout = true;
					}
					z = par.mandelbox.rotinv[0][1].RotateVector(z);

					z = par.mandelbox.rot[1][1].RotateVector(z);
					if (!lockout && z.y < -par.mandelbox.doubles.foldingLimit)
					{
						z.y = -par.mandelbox.doubles.foldingValue - z.y;
						tgladColor += par.mandelbox.doubles.colorFactorY;
					}
					z = par.mandelbox.rotinv[1][1].RotateVector(z);

					lockout = false;
					z = par.mandelbox.rot[0][2].RotateVector(z);
					if (z.z > par.mandelbox.doubles.foldingLimit)
					{
						z.z = par.mandelbox.doubles.foldingValue - z.z;
						tgladColor += par.mandelbox.doubles.colorFactorZ;
						lockout = true;
					}
					z = par.mandelbox.rotinv[0][2].RotateVector(z);

					z = par.mandelbox.rot[1][2].RotateVector(z);
					if (!lockout && z.z < -par.mandelbox.doubles.foldingLimit)
					{
						z.z = -par.mandelbox.doubles.foldingValue - z.z;
						tgladColor += par.mandelbox.doubles.colorFactorZ;
					}
					z = par.mandelbox.rotinv[1][2].RotateVector(z);
				}
				else
				{
					if (z.x > par.mandelbox.doubles.foldingLimit)
					{
						z.x = par.mandelbox.doubles.foldingValue - z.x;
						tgladColor += par.mandelbox.doubles.colorFactorX;
					}
					else if (z.x < -par.mandelbox.doubles.foldingLimit)
					{
						z.x = -par.mandelbox.doubles.foldingValue - z.x;
						tgladColor += par.mandelbox.doubles.colorFactorX;
					}
					if (z.y > par.mandelbox.doubles.foldingLimit)
					{
						z.y = par.mandelbox.doubles.foldingValue - z.y;
						tgladColor += par.mandelbox.doubles.colorFactorY;
					}
					else if (z.y < -par.mandelbox.doubles.foldingLimit)
					{
						z.y = -par.mandelbox.doubles.foldingValue - z.y;
						tgladColor += par.mandelbox.doubles.colorFactorY;
					}
					if (z.z > par.mandelbox.doubles.foldingLimit)
					{
						z.z = par.mandelbox.doubles.foldingValue - z.z;
						tgladColor += par.mandelbox.doubles.colorFactorZ;
					}
					else if (z.z < -par.mandelbox.doubles.foldingLimit)
					{
						z.z = -par.mandelbox.doubles.foldingValue - z.z;
						tgladColor += par.mandelbox.doubles.colorFactorZ;
					}
				}

				r = z.Length();
				double r2 = r * r;

				z += par.mandelbox.doubles.offset;

				if (r2 < mR2)
				{
					z *= tglad_factor1;
					tgladDE *= tglad_factor1;
					tgladColor += par.mandelbox.doubles.colorFactorSp1;
				}
				else if (r2 < fR2)
				{
					double tglad_factor2 = fR2 / r2;
					z *= tglad_factor2;
					tgladDE *= tglad_factor2;
					tgladColor += par.mandelbox.doubles.colorFactorSp2;
				}

				z -= par.mandelbox.doubles.offset;

				z = par.mandelbox.mainRot.RotateVector(z);

				z = z * scale + constant;
				tgladDE = tgladDE*fabs(scale)+1.0;

				r = z.Length();
				break;
			}

			case generalizedFoldBox:
			{
				//Reference: http://www.fractalforums.com/new-theories-and-research/generalized-box-fold/msg36503/#msg36503

				int i;
				const CVector3 *Nv;
				int sides;

				Nv = par.genFoldBox.Nv_tet;
				sides = par.genFoldBox.sides_tet;

				if (par.genFoldBox.type == foldCube)
				{
					Nv = par.genFoldBox.Nv_cube;
					sides = par.genFoldBox.sides_cube;
				}
				else if (par.genFoldBox.type == foldOct)
				{
					Nv = par.genFoldBox.Nv_oct;
					sides = par.genFoldBox.sides_oct;
				}
				else if (par.genFoldBox.type == foldDodeca)
				{
					Nv = par.genFoldBox.Nv_dodeca;
					sides = par.genFoldBox.sides_dodeca;
				}
				else if (par.genFoldBox.type == foldOctCube)
				{
					Nv = par.genFoldBox.Nv_oct_cube;
					sides = par.genFoldBox.sides_oct_cube;
				}
				else if (par.genFoldBox.type == foldIcosa)
				{
					Nv = par.genFoldBox.Nv_icosa;
					sides = par.genFoldBox.sides_icosa;
				}
				else if (par.genFoldBox.type == foldBox6)
				{
					Nv = par.genFoldBox.Nv_box6;
					sides = par.genFoldBox.sides_box6;
				}
				else if (par.genFoldBox.type == foldBox5)
				{
					Nv = par.genFoldBox.Nv_box5;
					sides = par.genFoldBox.sides_box5;
				}

        double melt = par.mandelbox.doubles.melt;
        double solid = par.mandelbox.doubles.solid;

				// Find the closest cutting plane if any that cuts the line between the origin and z.
				// Line is parameterized as X = Y + L*a;
				// Cutting plane is X.Dot(Nv) = Solid.
				// (Y + L*a).Dot(Nv) = solid.
				// a = (solid - Y.Dot(Nv))/L.Dot(Nv) = b/c
				CVector3 L = z;
				double a = 1;
				CVector3 Y; // Y is the origin in this case.
				int side = -1;
				double b, c;

				for (i = 0; i < sides; i++)
				{
					b = solid;
					c = L.Dot(Nv[i]);
					// A bit subtile here. a_r must be positive and I want to avoid divide by zero.
					if ((c > 0) && ((a * c) > b))
					{
						side = i;
						a = b / c;
					}
				}

				// If z is above the foldingValue we may have to fold. Else early out.
				if (side != -1)
				{ // mirror check
					int side_m = side;
					CVector3 Nv_m = Nv[side_m];
					CVector3 X_m = z - Nv_m * (z.Dot(Nv_m) - solid);

					// Find any plane (Nv_r) closest to X_m that cuts the line between Nv_m and X_m.
					// Nv_m cross Nv_r will define a possible rotation axis.
					// a = (solid - Y.Dot(Nv)/L.Dot(Nv) = b/c.
					L = X_m - Nv_m;
					Y = Nv_m;
					a = 1;
					side = -1;

					for (i = 0; i < sides; i++)
					{
						if (i != side_m)
						{
							b = solid - Y.Dot(Nv[i]);
							c = L.Dot(Nv[i]);
							// A bit subtile here. a_r must be positive and I want to avoid divide by zero.
							if ((c > 0) && ((a * c) > b))
							{
								side = i;
								a = b / c;
							}
						}
					}

					// Was a cutting plane found?
					if (side != -1)
					{ // rotation check
						CVector3 Xmr_intersect = Y + L * a;
						int side_r = side;
						CVector3 Nv_r = Nv[side_r];
						// The axis of rotation is define by the cross product of Nv_m and Nv_r and
						// the intersection of the line between Nv_m and Nv_r and  Xmr_intersect.
						CVector3 L_r = Nv_m.Cross(Nv_r);
						// The closest point betwee z and the line of rotation can be found by minimizing
						// the square of the distance (D) between z and the line
						// X = Xmr_intersect + L_r * a_rmin.
						// Setting dD/da_rmin equal to zero and solving for a_rmin.
						double a_rmin = (z.Dot(L_r) - Xmr_intersect.Dot(L_r)) / (L_r.Dot(L_r));

						// force a_rmin to be positive. I think I made an even number of sign errors here.
						if (a_rmin < 0)
						{
							a_rmin = -a_rmin;
							L_r = L_r * (-1);
						}
						CVector3 X_r = Xmr_intersect + L_r * a_rmin;

						// Find any plane (Nv_i) closest to Xmr_intersect that cuts the line between
						// Xmr_intersect and X_r. This will define a possible inversion point.
						// a = (solid - Y.Dot(Nv)/L.Dot(Nv) = b/c.
						L = X_r - Xmr_intersect;
						Y = Xmr_intersect;
						a = 1;
						side = -1;

						for (i = 0; i < sides; i++)
						{
							if ((i != side_m) && (i != side_r))
							{
								b = solid - Y.Dot(Nv[i]);
								c = L.Dot(Nv[i]);
								// A bit subtile here. a must be positive and I want to avoid divide by zero.
								if ((c > 0) && ((a * c) > b))
								{
									side = i;
									a = b / c;
								}
							}
						}

						if (side != -1)
						{ // inversion check
							// Only inversion point possible but still need to check for melt.

							CVector3 X_i = Y + L * a;
							CVector3 z2X = X_i - z;
							// Is z above the melt layer.
							if (z2X.Dot(z2X) > (melt * melt))
							{
								double z2X_mag = z2X.Length();
								z = z + z2X * (2 * (z2X_mag - melt) / (z2X_mag + .00000001));
								tgladColor += par.mandelbox.doubles.colorFactorZ;
							}
						}
						else
						{
							// Only rotation line possible but still need to check for melt.
							// Is z above the melt layer.
							CVector3 z2X = X_r - z;
							if (z2X.Dot(z2X) > (melt * melt))
							{
								double z2X_mag = z2X.Length();
								z = z + z2X * (2 * (z2X_mag - melt) / (z2X_mag + .00000001));
								tgladColor += par.mandelbox.doubles.colorFactorY;
							}
						}

					}
					else
					{
						// Only mirror plane possible but still need to check for melt.
						CVector3 z2X = X_m - z;
						if (z2X.Dot(z2X) > (melt * melt))
						{
							double z2X_mag = z2X.Length();
							z = z + z2X * (2 * (z2X_mag - melt) / (z2X_mag + .00000001));
							tgladColor += par.mandelbox.doubles.colorFactorX;
						}
					}
				} // outside solid

				r = z.Length();
				double r2 = r * r;

				z += par.mandelbox.doubles.offset;

				if (r2 < mR2)
				{
					z *= tglad_factor1;
					tgladDE *= tglad_factor1;
					tgladColor += par.mandelbox.doubles.colorFactorSp1;
				}
				else if (r2 < fR2)
				{
					double tglad_factor2 = fR2 / r2;
					z *= tglad_factor2;
					tgladDE *= tglad_factor2;
					tgladColor += par.mandelbox.doubles.colorFactorSp2;
				}

				z -= par.mandelbox.doubles.offset;

				z = par.mandelbox.mainRot.RotateVector(z);

				z = z * scale + constant;
				tgladDE = tgladDE * fabs(scale) + 1.0;

				r = z.Length();
				break;
			}

			case smoothMandelbox:
			{
				double sm = par.mandelbox.doubles.sharpness;

				double zk1 = SmoothConditionAGreaterB(z.x, par.mandelbox.doubles.foldingLimit,sm);
				double zk2 = SmoothConditionALessB(z.x, -par.mandelbox.doubles.foldingLimit,sm);
				z.x = z.x * (1.0 - zk1) + (par.mandelbox.doubles.foldingValue - z.x) * zk1;
				z.x = z.x * (1.0 - zk2) + (-par.mandelbox.doubles.foldingValue - z.x) * zk2;
				tgladColor += (zk1 + zk2) * par.mandelbox.doubles.colorFactorX;

				double zk3 = SmoothConditionAGreaterB(z.y, par.mandelbox.doubles.foldingLimit,sm);
				double zk4 = SmoothConditionALessB(z.y, -par.mandelbox.doubles.foldingLimit,sm);
				z.y = z.y * (1.0 - zk3) + (par.mandelbox.doubles.foldingValue - z.y) * zk3;
				z.y = z.y * (1.0 - zk4) + (-par.mandelbox.doubles.foldingValue - z.y) * zk4;
				tgladColor += (zk3 + zk4) * par.mandelbox.doubles.colorFactorY;

				double zk5 = SmoothConditionAGreaterB(z.z, par.mandelbox.doubles.foldingLimit,sm);
				double zk6 = SmoothConditionALessB(z.z, -par.mandelbox.doubles.foldingLimit,sm);
				z.z = z.z * (1.0 - zk5) + (par.mandelbox.doubles.foldingValue - z.z) * zk5;
				z.z = z.z * (1.0 - zk6) + (-par.mandelbox.doubles.foldingValue - z.z) * zk6;
				tgladColor += (zk5 + zk6) * par.mandelbox.doubles.colorFactorZ;

				r = z.Length();
				double r2 = r * r;
				double tglad_factor2 = fR2 / r2;
				double rk1 = SmoothConditionALessB(r2, mR2, sm);
				double rk2 = SmoothConditionALessB(r2, fR2, sm);
				double rk21 = (1.0 - rk1) * rk2;

				z = z * (1.0 - rk1) + z * (tglad_factor1 * rk1);
				z = z * (1.0 - rk21) + z * (tglad_factor2 * rk21);
				tgladDE = tgladDE * (1.0 - rk1) + tgladDE * (tglad_factor1 * rk1);
				tgladDE = tgladDE * (1.0 - rk21) + tgladDE * (tglad_factor2 * rk21);
				tgladColor += rk1 * par.mandelbox.doubles.colorFactorSp1;
				tgladColor += rk21 * par.mandelbox.doubles.colorFactorSp2;

				z = par.mandelbox.mainRot.RotateVector(z);
				z = z * scale + constant;

				tgladDE = tgladDE * fabs(scale) + 1.0;
				r = z.Length();
				break;
			}
			case foldingIntPow2:
			{
				if (z.x > par.doubles.FoldingIntPowFoldFactor) z.x = par.doubles.FoldingIntPowFoldFactor * 2.0 - z.x;
				else if (z.x < -par.doubles.FoldingIntPowFoldFactor) z.x = -par.doubles.FoldingIntPowFoldFactor * 2.0 - z.x;

				if (z.y > par.doubles.FoldingIntPowFoldFactor) z.y = par.doubles.FoldingIntPowFoldFactor * 2.0 - z.y;
				else if (z.y < -par.doubles.FoldingIntPowFoldFactor) z.y = -par.doubles.FoldingIntPowFoldFactor * 2.0 - z.y;

				if (z.z > par.doubles.FoldingIntPowFoldFactor) z.z = par.doubles.FoldingIntPowFoldFactor * 2.0 - z.z;
				else if (z.z < -par.doubles.FoldingIntPowFoldFactor) z.z = -par.doubles.FoldingIntPowFoldFactor * 2.0 - z.z;

				r = z.Length();

				double fR2_2 = 1.0;
				double mR2_2 = 0.25;
				double r2_2 = r * r;
				double tglad_factor1_2 = fR2_2 / mR2_2;

				if (r2_2 < mR2_2)
				{
					z = z * tglad_factor1_2;
				}
				else if (r2_2 < fR2_2)
				{
					double tglad_factor2_2 = fR2_2 / r2_2;
					z = z * tglad_factor2_2;
				}

				z = z * 2.0;
				double x2 = z.x * z.x;
				double y2 = z.y * z.y;
				double z2 = z.z * z.z;
				double temp = 1.0 - z2 / (x2 + y2);
				double newx = (x2 - y2) * temp;
				double newy = 2.0 * z.x * z.y * temp;
				double newz = -2.0 * z.z * sqrt(x2 + y2);
				z.x = newx + constant.x;
				z.y = newy + constant.y;
				z.z = newz + constant.z;
				z.z *= par.doubles.FoldingIntPowZfactor;
				r = z.Length();
				break;
			}
			case kaleidoscopic:
			{

				if (par.IFS.absX) z.x = fabs(z.x);
				if (par.IFS.absY) z.y = fabs(z.y);
				if (par.IFS.absZ) z.z = fabs(z.z);

				for (int i = 0; i < IFS_VECTOR_COUNT; i++)
				{
					if (par.IFS.enabled[i])
					{
						z = par.IFS.rot[i].RotateVector(z);
						double length = z.Dot(par.IFS.doubles.direction[i]);

						if (length < par.IFS.doubles.distance[i])
						{
							z -= par.IFS.doubles.direction[i] * (2.0 * (length - par.IFS.doubles.distance[i]) * par.IFS.doubles.intensity[i]);
						}

					}
				}
				z = par.IFS.mainRot.RotateVector(z - par.IFS.doubles.offset) + par.IFS.doubles.offset;

				if(par.IFS.doubles.edge.x > 0) z.x = par.IFS.doubles.edge.x - fabs(par.IFS.doubles.edge.x - z.x);
				if(par.IFS.doubles.edge.y > 0) z.y = par.IFS.doubles.edge.y - fabs(par.IFS.doubles.edge.y - z.y);
				if(par.IFS.doubles.edge.z > 0) z.z = par.IFS.doubles.edge.z - fabs(par.IFS.doubles.edge.z - z.z);

				if (Mode == colouring)
				{
					double length2 = z.Length();
					if (length2 < min) min = length2;
				}

				z *= par.IFS.doubles.scale;
				if(par.IFS.mengerSpongeMode)
				{
					z.x -= par.IFS.doubles.offset.x * (par.IFS.doubles.scale - 1.0);
					z.y -= par.IFS.doubles.offset.y * (par.IFS.doubles.scale - 1.0);
					if (z.z > 0.5 * par.IFS.doubles.offset.z * (par.IFS.doubles.scale - 1.0)) z.z -= par.IFS.doubles.offset.z * (par.IFS.doubles.scale - 1.0);
				}
				else
				{
					z -= par.IFS.doubles.offset * (par.IFS.doubles.scale - 1.0);
				}

				tgladDE *= par.IFS.doubles.scale;
				r = z.Length();

				break;
			}
			case mandelboxVaryScale4D:
			{
				scale = scale + par.mandelbox.doubles.vary4D.scaleVary * (fabs(scale) - 1.0);
				CVector3 oldz = z;
				z.x = fabs(z.x + par.mandelbox.doubles.vary4D.fold) - fabs(z.x - par.mandelbox.doubles.vary4D.fold) - z.x;
				z.y = fabs(z.y + par.mandelbox.doubles.vary4D.fold) - fabs(z.y - par.mandelbox.doubles.vary4D.fold) - z.y;
				z.z = fabs(z.z + par.mandelbox.doubles.vary4D.fold) - fabs(z.z - par.mandelbox.doubles.vary4D.fold) - z.z;
				w = fabs(w + par.mandelbox.doubles.vary4D.fold) - fabs(w - par.mandelbox.doubles.vary4D.fold) - w;
				if(z.x != oldz.x) tgladColor += par.mandelbox.doubles.colorFactorX;
				if(z.y != oldz.y) tgladColor += par.mandelbox.doubles.colorFactorY;
				if(z.z != oldz.z) tgladColor += par.mandelbox.doubles.colorFactorZ;
				double rr = pow(z.x * z.x + z.y * z.y + z.z * z.z + w * w, par.mandelbox.doubles.vary4D.rPower);
				double m = scale;
				if (rr < par.mandelbox.doubles.vary4D.minR * par.mandelbox.doubles.vary4D.minR)
				{
					m = scale / (par.mandelbox.doubles.vary4D.minR * par.mandelbox.doubles.vary4D.minR);
					tgladColor += par.mandelbox.doubles.colorFactorSp1;
				}
				else if (rr < 1.0)
				{
					m = scale / rr;
					tgladColor += par.mandelbox.doubles.colorFactorSp2;
				}
				z = z * m + constant;
				w = w * m + par.mandelbox.doubles.vary4D.wadd;
				tgladDE = tgladDE * fabs(m) + 1.0;
				r = sqrt(z.x * z.x + z.y * z.y + z.z * z.z + w * w);
				break;
			}
			case aexion:
			{
				if(L == 0)
				{
					double cx = fabs(constant.x + constant.y + constant.z) + par.doubles.cadd;
					double cy = fabs(-constant.x - constant.y + constant.z) + par.doubles.cadd;
					double cz = fabs(-constant.x + constant.y - constant.z) + par.doubles.cadd;
					double cw = fabs(constant.x - constant.y - constant.z) + par.doubles.cadd;
					constant.x = cx;
					constant.y = cy;
					constant.z = cz;
					constantw = cw;
					double tempx = fabs(z.x + z.y + z.z) + par.doubles.cadd;
					double tempy = fabs(-z.x - z.y + z.z) + par.doubles.cadd;
					double tempz = fabs(-z.x + z.y - z.z) + par.doubles.cadd;
					double tempw = fabs(z.x - z.y - z.z) + par.doubles.cadd;
					z.x = tempx;
					z.y = tempy;
					z.z = tempz;
					w = tempw;
				}
				double tempx = z.x * z.x - z.y * z.y + 2.0 * w * z.z + constant.x;
				double tempy = z.y * z.y - z.x * z.x + 2.0 * w * z.z + constant.y;
				double tempz = z.z * z.z - w * w + 2.0 * z.x * z.y + constant.z;
				double tempw = w * w - z.z * z.z + 2.0 * z.x * z.y + constantw;
				z.x = tempx;
				z.y = tempy;
				z.z = tempz;
				w = tempw;
				r = sqrt(z.x * z.x + z.y * z.y + z.z * z.z + w * w);
				break;
			}
			case benesi:
			{
				double r1 = z.y*z.y + z.z*z.z;
				double newx = 0;
				if(constant.x < 0 || z.x < sqrt(r1))
				{
					newx = z.x*z.x - r1;
				}
				else
				{
					newx = -z.x*z.x + r1;
				}
				r1 = - 1.0/sqrt(r1) * 2.0 * fabs(z.x);
				double newy = r1 * (z.y*z.y - z.z*z.z);
				double newz = r1 * 2.0 * z.y * z.z;

				z.x = newx + constant.x;
				z.y = newy + constant.y;
				z.z = newz + constant.z;

				r = z.Length();
				break;
			}
			case bristorbrot:
			{
				double newx = z.x*z.x - z.y*z.y - z.z*z.z;
				double newy = z.y * (2.0 * z.x - z.z);
				double newz = z.z * (2.0 * z.x + z.y);

				z.x = newx + constant.x;
				z.y = newy + constant.y;
				z.z = newz + constant.z;

				r = z.Length();
				break;
			}
			case invertX:
			{
				z.x = z.x >= 0.0 ? z.x*z.x/(fabs(z.x) + p) : -z.x*z.x/(fabs(z.x) + p);
				r = z.Length();
				break;
			}
			case invertY:
			{
				z.y = z.y >= 0.0 ? z.y*z.y/(fabs(z.y) + p) : -z.y*z.y/(fabs(z.y) + p);
				r = z.Length();
				break;
			}
			case invertZ:
			{
				z.z = z.z >= 0.0 ? z.z*z.z/(fabs(z.z) + p) : -z.z*z.z/(fabs(z.z) + p);
				r = z.Length();
				break;
			}
			case invertR:
			{
				double rInv = r*r/(r + p);
				z.x = z.x / r * rInv;
				z.y = z.y / r * rInv;
				z.z = z.z / r * rInv;
				r = z.Length();
				break;
			}
			case sphericalFold:
			{
				double rr = r*r;
				double pp = p*p;
				if (rr < pp)
				{
					z.x = 1.0 / pp;
					z.y = 1.0 / pp;
					z.z = 1.0 / pp;
				}
				else if (rr < pp*4.0)
				{
					z.x = 1.0 / rr;
					z.y = 1.0 / rr;
					z.z = 1.0 / rr;
				}
				r = z.Length();
				break;
			}
			case powXYZ:
			{
				z.x = z.x >= 0 ? pow(z.x,p) : -pow(-z.x,p);
				z.y = z.y >= 0 ? pow(z.y,p) : -pow(-z.y,p);
				z.z = z.z >= 0 ? pow(z.z,p) : -pow(-z.z,p);
				r = z.Length();
				break;
			}
			case scaleX:
			{
				z.x = z.x * p;
				r = z.Length();
				break;
			}
			case scaleY:
			{
				z.y = z.y * p;
				r = z.Length();
				break;
			}
			case scaleZ:
			{
				z.z = z.z * p;
				r = z.Length();
				break;
			}
			case offsetX:
			{
				z.x = z.x + p;
				r = z.Length();
				break;
			}
			case offsetY:
			{
				z.y = z.y + p;
				r = z.Length();
				break;
			}
			case offsetZ:
			{
				z.z = z.z + p;
				r = z.Length();
				break;
			}
			case angleMultiplyX:
			{
				double angle = atan2(z.z,z.y)*p;
				double tempR = sqrt(z.z*z.z + z.y*z.y);
				z.y = tempR * cos(angle);
				z.z = tempR * sin(angle);
				r = z.Length();
				break;
			}
			case angleMultiplyY:
			{
				double angle = atan2(z.z,z.x)*p;
				double tempR = sqrt(z.z*z.z + z.x*z.x);
				z.x = tempR * cos(angle);
				z.z = tempR * sin(angle);
				r = z.Length();
				break;
			}
			case angleMultiplyZ:
			{
				double angle = atan2(z.y,z.x)*p;
				double tempR = sqrt(z.x*z.x + z.y*z.y);
				z.x = tempR * cos(angle);
				z.y = tempR * sin(angle);
				r = z.Length();
				break;
			}
			case hybrid:
				break;
			case none:
				break;
		}

		//************************** iteration terminate conditions *****************
		if (Mode == deltaDE1)
		{
			if (r > 1e10)
				break;
		}
		else if (Mode == deltaDE2)
		{
			if (L == *iter_count)
				break;
		}

		if (Mode == orbitTrap)
		{
			CVector3 delta = z - par.doubles.fakeLightsOrbitTrap;
			distance = delta.Length();
			if (L >= par.fakeLightsMinIter && L <= par.fakeLightsMaxIter && distance < min) min = distance;
			if (distance > 1000)
			{
				distance = min;
				break;
			}
		}

		if (actualFormula == menger_sponge || actualFormula == kaleidoscopic)
		{
			if (r > 1000)
			{
				distance = (r - 2.0) / tgladDE;
				break;
			}
		}
		else if (actualFormula == tglad || actualFormula == smoothMandelbox || actualFormula == mandelboxVaryScale4D || actualFormula == generalizedFoldBox)
		{
			if (r > 1024)
			{
				distance = r / fabs(tgladDE);
				break;
			}
		}
		else
		{
			if (Mode == normal) //condition for all other trigonometric and hypercomplex fractals
			{
				if (r > 1e2)
				{
					distance = 0.5 * r * log(r) / r_dz;
					break;
				}
			}
			else if (Mode == fake_AO) //mode 2
			{
				if (r < min) min = r;
				if (r > 1e15)
				{
					distance = min;
					break;
				}
			}
			else if (Mode == colouring) //mode 1
			{
				if(par.primitives.onlyPlane)
				{
					distance = z.Length();
					if (distance > 1e15)
					{
						distance = (L - log(log(r) / log((double)N)) / log(p))/100.0;
						break;
					}
				}
				else
				{
					distance = z.Length();
					if (distance < min) min = distance;
					if (distance > 1e15)
					{
						distance = min;
						break;
					}
				}
			}
		}
	}

	//************ return values *****************

/*
	N_counter += L + 1;
	Loop_counter++;

	if (L < 64)
		histogram[L]++;
	else
		histogram[63]++;
*/

	if (iter_count != NULL)
		*iter_count = L;

	if (Mode == normal)
	{
		if (L == N)
			distance = 0;
		return distance;
	}

	if (Mode == deltaDE1 || Mode == deltaDE2)
		return r;

	if (Mode == fake_AO)
		return distance;

	if (Mode == orbitTrap)
		return distance;

	if (Mode == colouring)
	{
		if (par.formula == hybrid)
		{
			if (min > 100) 
				min = 100;
			if (distance > 20) 
				distance = 20;
			if (tgladColor > 1000) 
				tgladColor = 1000;

			return distance * 5000.0 + tgladColor * 100.0 + min * 1000.0;
		} 
		else if (actualFormula == tglad || actualFormula == smoothMandelbox || actualFormula == mandelboxVaryScale4D || actualFormula == generalizedFoldBox)
			return tgladColor * 100.0 + z.Length()*par.mandelbox.doubles.colorFactorR;
		else if (actualFormula == kaleidoscopic || actualFormula == menger_sponge)
			return min * 1000.0;
		else
			return distance * 5000.0;
	}
}

#if 0
//******************* Calculate distance *******************8

double CalculateDistance(CVector3 point, sFractal &params, bool *max_iter)
{
	int L;
	double distance;
	params.objectOut = objFractal;

	if (params.limits_enabled)
	{
		bool limit = false;
		double distance_a = 0;
		double distance_b = 0;
		double distance_c = 0;

		if (point.x < params.doubles.amin - params.doubles.detailSize)
		{
			distance_a = fabs(params.doubles.amin - point.x);
			limit = true;
		}
		if (point.x > params.doubles.amax + params.doubles.detailSize)
		{
			distance_a = fabs(params.doubles.amax - point.x);
			limit = true;
		}

		if (point.y < params.doubles.bmin - params.doubles.detailSize)
		{
			distance_a = fabs(params.doubles.bmin - point.y);
			limit = true;
		}
		if (point.y > params.doubles.bmax + params.doubles.detailSize)
		{
			distance_b = fabs(params.doubles.bmax - point.y);
			limit = true;
		}

		if (point.z < params.doubles.cmin - params.doubles.detailSize)
		{
			distance_c = fabs(params.doubles.cmin - point.z);
			limit = true;
		}
		if (point.z > params.doubles.cmax + params.doubles.detailSize)
		{
			distance_c = fabs(params.doubles.cmax - point.z);
			limit = true;
		}

		if (limit)
		{
			if (max_iter != NULL)
				*max_iter = false;
			distance = dMax(distance_a, distance_b, distance_c);
			return distance;
		}
	}

	if(!params.primitives.onlyPlane)
	{
		if (params.analitycDE)
		{
			distance = Compute<normal>(point, params, &L);
			if (max_iter != NULL)
			{
				if (L == (int)params.doubles.N) *max_iter = true;
				else *max_iter = false;
			}
			params.itersOut = L;

			if (L < params.minN && distance < params.doubles.detailSize) distance = params.doubles.detailSize;

			if (params.interiorMode)
			{
				if (distance < 0.5 * params.doubles.detailSize || L == (int)params.doubles.N)
				{
					distance = params.doubles.detailSize;
					if (max_iter != NULL) *max_iter = false;
				}
			}
			if (params.iterThresh)
			{
				if(distance < params.doubles.detailSize)
				{
					distance = params.doubles.detailSize * 1.01;
				}
			}
		}
		else
		{
			double deltaDE = 1e-10;

			double r = Compute<deltaDE1>(point, params, &L);
			int retval = L;
			params.itersOut = L;

			point.x += deltaDE;
			point.y += 0;
			point.z += 0;
			double r2 = Compute<deltaDE2>(point, params, &L);
			double dr1 = fabs(r2 - r) / deltaDE;

			point.x -= deltaDE;
			point.y += deltaDE;
			point.z += 0;
			r2 = Compute<deltaDE2>(point, params, &L);
			double dr2 = fabs(r2 - r) / deltaDE;

			point.x += 0;
			point.y -= deltaDE;
			point.z += deltaDE;
			r2 = Compute<deltaDE2>(point, params, &L);
			double dr3 = fabs(r2 - r) / deltaDE;

			double dr = sqrt(dr1 * dr1 + dr2 * dr2 + dr3 * dr3);

			if (params.linearDEmode)
			{
				distance = 0.5 * r / dr;
			}
			else
			{
				distance = 0.5 * r * log(r) / dr;
			}

			if (retval == (int)params.doubles.N)
			{
				if (max_iter != NULL) *max_iter = true;
				distance = 0;
			}
			else if (max_iter != NULL) *max_iter = false;

			if (L < params.minN && distance < params.doubles.detailSize) distance = params.doubles.detailSize;

			if (params.interiorMode)
			{
				if (distance < 0.5 * params.doubles.detailSize || retval == 256)
				{
					distance = params.doubles.detailSize;
					if (max_iter != NULL) *max_iter = false;
				}
			}

			if (params.iterThresh)
			{
				if(distance < params.doubles.detailSize)
				{
					distance = params.doubles.detailSize * 1.01;
				}
			}

		}
	}
	else
	{
		distance = 10.0;
		if (max_iter != NULL) *max_iter = false;
	}

	//plane
	if (params.primitives.planeEnable)
	{
		double planeDistance = PrimitivePlane(point, params.doubles.primitives.planeCentre, params.doubles.primitives.planeNormal);
		if(!params.primitives.onlyPlane && planeDistance < distance) 	params.objectOut = objPlane;
		distance = (planeDistance < distance) ? planeDistance : distance;

	}

	//box
	if (params.primitives.boxEnable)
	{
		double boxDistance = PrimitiveBox(point, params.doubles.primitives.boxCentre, params.doubles.primitives.boxSize);
		if(boxDistance < distance) 	params.objectOut = objBox;
		distance = (boxDistance < distance) ? boxDistance : distance;
	}

	//inverted box
	if (params.primitives.invertedBoxEnable)
	{
		double boxDistance = PrimitiveInvertedBox(point, params.doubles.primitives.invertedBoxCentre, params.doubles.primitives.invertedBoxSize);
		if(boxDistance < distance) 	params.objectOut = objBoxInv;
		distance = (boxDistance < distance) ? boxDistance : distance;
	}

	//sphere
	if (params.primitives.sphereEnable)
	{
		double sphereDistance = PrimitiveSphere(point, params.doubles.primitives.sphereCentre, params.doubles.primitives.sphereRadius);
		if(sphereDistance < distance) 	params.objectOut = objSphere;
		distance = (sphereDistance < distance) ? sphereDistance : distance;
	}

	//invertedSphere
	if (params.primitives.invertedSphereEnable)
	{
		double sphereDistance = PrimitiveInvertedSphere(point, params.doubles.primitives.invertedSphereCentre, params.doubles.primitives.invertedSphereRadius);
		if(sphereDistance < distance) 	params.objectOut = objSphereInv;
		distance = (sphereDistance < distance) ? sphereDistance : distance;
	}

	//water
	if (params.primitives.waterEnable)
	{
		double waterDistance = PrimitiveWater(point, params.doubles.primitives.waterHeight, params.doubles.primitives.waterAmplitude,
				params.doubles.primitives.waterLength, params.doubles.primitives.waterRotation, params.primitives.waterIterations, 0.1, params.frameNo);
		if(waterDistance < distance) 	params.objectOut = objWater;
		distance = (waterDistance < distance) ? waterDistance : distance;
	}

	if (distance < 0) distance = 0;
	if (max_iter != NULL)
	{

		if (*max_iter)
		{
			if (params.limits_enabled)
			{
				double distance_a1 = fabs(params.doubles.amin - point.x);
				double distance_a2 = fabs(params.doubles.amax - point.x);
				double distance_b1 = fabs(params.doubles.bmin - point.y);
				double distance_b2 = fabs(params.doubles.bmax - point.y);
				double distance_c1 = fabs(params.doubles.cmin - point.z);
				double distance_c2 = fabs(params.doubles.cmax - point.z);
				double min1 = dMin(distance_a1, distance_b1, distance_c1);
				double min2 = dMin(distance_a2, distance_b2, distance_c2);
				double min = MIN(min1, min2);
				if(min < params.doubles.detailSize)
				{
					distance = min;
				}
			}
			else
			{
				//distance = params.doubles.detailSize * 0.5;
				distance = 0.0;
			}
		}


	}
	return distance;
}
#endif

// force template instantiation
template double Compute<normal>(CVector3, const sFractal&, int*);
template double Compute<colouring>(CVector3, const sFractal&, int*);
template double Compute<fake_AO>(CVector3, const sFractal&, int*);
template double Compute<deltaDE1>(CVector3, const sFractal&, int*);
template double Compute<deltaDE2>(CVector3, const sFractal&, int*);
template double Compute<orbitTrap>(CVector3, const sFractal&, int*);
