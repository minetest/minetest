/*
 * fractparams.h
 *
 *  Created on: 2010-05-03
 *      Author: krzysztof
 */

#ifndef FRACTPARAMS_H_
#define FRACTPARAMS_H_

#include "fractal.h"
//#include "texture.hpp"

struct sParamRenderD
{
	double zoom; //zoom
	double DE_factor; //factor for distance estimation steps
	double resolution; //resolution of image in fractal coordinates
	double persp; //perspective factor
	double quality; //DE threshold factor
	double smoothness;
	double alpha; //rotation of fractal
	double beta; //
	double gamma;
	double DOFFocus;
	double DOFRadius;
	double mainLightAlpha;
	double mainLightBeta;
	double auxLightIntensity;
	double auxLightMaxDist;
	double auxLightDistributionRadius;
	double auxLightVisibility;
	double auxLightPre1intensity;
	double auxLightPre2intensity;
	double auxLightPre3intensity;
	double auxLightPre4intensity;
	double stereoEyeDistance;
	double viewDistanceMin;
	double viewDistanceMax;
	double volumetricLightIntensity[5];
	double fogDensity;
	double fogColour1Distance;
	double fogColour2Distance;
	double fogDistanceFactor;
	double colourSaturation;
	double fastAoTune;
	double iterFogOpacity;
	double iterFogOpacityTrim;
	double fakeLightsIntensity;
	double fakeLightsVisibility;
	double fakeLightsVisibilitySize;
	double shadowConeAngle;
	double primitivePlaneReflect;
	double primitiveBoxReflect;
	double primitiveInvertedBoxReflect;
	double primitiveSphereReflect;
	double primitiveInvertedSphereReflect;
	double primitiveWaterReflect;

//	sImageAdjustments imageAdjustments;

	CVector3 vp; //view point
	CVector3 auxLightPre1;
	CVector3 auxLightPre2;
	CVector3 auxLightPre3;
	CVector3 auxLightPre4;
	CVector3 auxLightRandomCenter;
};

struct sParamRender
{
	sParamRenderD doubles;

	sFractal fractal;
	int image_width; //image width
	int image_height; //image height
	int globalIlumQuality; //ambient occlusion quality
	int reflectionsMax;
	int coloring_seed; //colouring random seed
	int auxLightRandomSeed;
	int auxLightNumber;
	int SSAOQuality;
	int startFrame;
	int endFrame;
	int framesPerKeyframe;
	int imageFormat;
	int noOfTiles;
	int tileCount;

	int OpenCLEngine;
	int OpenCLPixelsPerJob;

	enumPerspectiveType perspectiveType;

	bool shadow; //enable shadows
	bool global_ilumination; //enable global ilumination
	bool fastGlobalIllumination; //enable fake global ilumination
	bool slowShading; //enable fake gradient calculation for shading
	bool textured_background; //enable testured background
	bool background_as_fulldome;
	bool recordMode; //path recording mode
	bool continueRecord; //continue recording mode
	bool playMode; //play mode
	bool animMode; //animation mode
	bool SSAOEnabled;
	bool DOFEnabled;
	bool auxLightPre1Enabled;
	bool auxLightPre2Enabled;
	bool auxLightPre3Enabled;
	bool auxLightPre4Enabled;
	bool volumetricLightEnabled[5];
	bool penetratingLights;
	bool stereoEnabled;
	bool quiet;
	bool fishEyeCut;
	bool fakeLightsEnabled;
/*
	sImageSwitches imageSwitches;

	sRGB background_color1; //background colour
	sRGB background_color2;
	sRGB background_color3;
	sRGB auxLightPre1Colour;
	sRGB auxLightPre2Colour;
	sRGB auxLightPre3Colour;
	sRGB auxLightPre4Colour;
	sRGB fogColour1;
	sRGB fogColour2;
	sRGB fogColour3;
	sRGB primitivePlaneColour;
	sRGB primitiveBoxColour;
	sRGB primitiveInvertedBoxColour;
	sRGB primitiveSphereColour;
	sRGB primitiveInvertedSphereColour;
	sRGB primitiveWaterColour;
	sEffectColours effectColours;

	sRGB palette[256];

	char file_destination[1000];
	char file_envmap[1000];
	char file_background[1000];
	char file_lightmap[1000];
	char file_path[1000];
	char file_keyframes[1000];

	cTexture *backgroundTexture;
	cTexture *envmapTexture;
	cTexture *lightmapTexture;
*/
	std::vector<enumFractalFormula> formulaSequence;
	std::vector<double> hybridPowerSequence;

	double settingsVersion;
};

#endif /* FRACTPARAMS_H_ */
