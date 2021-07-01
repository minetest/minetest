/*
Minetest
Copyright (C) 2010-2021

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "client/shader/shader_pass.h"
#include "log.h"

u64 ShaderPass::GetVariantKey( const std::vector<std::string> featuresUsed ) const {
	u64 r = 0;
	// For each requested feature, light up the appropriate bit in the bitmask.
	for ( const auto &str : featuresUsed ) {
		if ( STL_CONTAINS( featurePositions, str ) )
			r |= ( 1 << featurePositions.at( str ) );
	}
	return r;
}

// Stage names for error printing.
const std::string stageNames[5] = {
	"vertex",
	"tess control",
	"tess eval",
	"geometry",
	"fragment",
};
void ShaderPass::GenVariants( const ShaderSource &sources, const std::vector<std::string> &features ) {
	// Since variants can be expressed as bitmasks where each bit is a feature,
	// it follows that each variant can be enumerated by simply iterating all the
	// numbers from 0 to 2^n where n is the feature count.
	u32 featureCount = features.size();
	variantCount = 1 << featureCount;
	for ( u64 i = 0 ; i < variantCount ; i++ ) {
		// Then, for each number/bitmask representing a variant,
		// we construct a header where each set bit produces
		// the relevant #define.
		std::stringstream variantFeatures;
		for ( u64 j = 0 ; j < featureCount ; j++ ) {
			if ( ( 1 << j ) & i )
				variantFeatures << "#define " << features[j] << " = 1\n";
		}
		// ShaderSource variantSources = sources; // Copy-assign
		// variantSources.header = variantFeatures.str();
		ShaderProgram program = ShaderProgram( sources, variantFeatures.str() );
		if ( !program.IsLinked() ) {
			errorLog << "GLSL shader pass build failure.\n";
			errorLog << "Compiling with features:\n\n";
			errorLog << variantFeatures.str();
			errorLog << "\n";
			for ( u32 i = 0; i < 5; i++ ) {
				if ( program.GetCompileStatus(static_cast<StageIndex>(i)) == StageCompileStatus::Failure ) {
					errorLog
						<< "\tError log for " << stageNames[i] << " shader:\n\n"
						<<  program.GetCompileLog(static_cast<StageIndex>(i)) << "\n";
				}
			}
			errorstream << errorLog.str();
			buildFailed = true;
			variants.clear();
			return;
		}
		variants.push_back( program );
	}
}

const u32 maxShaderFeatures = 64; // bits of a u64
const u32 shaderFeatureWarnThreshold = 12;

ShaderPass::ShaderPass( const ShaderSource &sources, const std::vector<std::string> &features ) {
	if ( features.size() > maxShaderFeatures ) {
		errorstream << "Shader pass feature count exceeds 64, the pass may not be compiled.\n";
		return;
	}
	if ( features.size() > shaderFeatureWarnThreshold ) {
		warningstream << "Shader pass has " << features.size() << " features. "
			<< "This will result in " << ( 1 << features.size() ) << " generated variants! "
			<< "Consider reducing the feature count.";
	}
	GenVariants( sources, features );
}
const ShaderProgram& ShaderPass::GetProgram( const u64 programKey ) const {
	return variants[programKey];
}
