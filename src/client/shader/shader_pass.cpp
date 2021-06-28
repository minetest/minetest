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

u64 ShaderPass::GetVariantKey( const std::vector<std::string> featuresUsed ) const {
	u64 r = 0;
	// For each requested feature, light up the appropriate bit in the bitmask.
	for ( const auto &str : featuresUsed ) {
		if ( STL_CONTAINS( featurePositions, str ) )
			r |= ( 1 << featurePositions.at( str ) );
	}
	return r;
}

const std::string const stageNames[5] = {
	"vertex",
	"tess control",
	"tess eval",
	"geometry",
	"fragment",
};
void ShaderPass::GenVariants( const std::vector<const std::string> &features ) {
	// Since variants can be expressed as bitmasks where each bit is a feature,
	// it follows that each variant can be enumerated by simply iterating all the
	// numbers from 0 to 2^n where n is the feature count.
	int featureCount = features.size();
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
		PassSources variantSources = sources; // Copy-assign
		variantSources.header = variantFeatures.str();
		auto *program = new ShaderProgram( variantSources );
		if ( !program->IsLinked() ) {
			errorLog << "GLSL shader pass build failure.\n";
			errorLog << "Compiling with features:\n\n";
			errorLog << variantFeatures.str();
			errorLog << "\n";
			for ( int i = 0; i < 5; i++ ) {
				if ( program->GetCompileStatus(static_cast<StageIndex>(i)) == StageCompileStatus::Failure ) {
					errorLog
						<< "\tError log for " << stageNames[i] << " shader:\n\n"
						<< program->compileLog[i] << "\n";
				}
			}
			errorstream << errorLog.str();
			buildFailed = true;
			delete program;
			variants.clear();
			return;
		}
		variants.push_back( std::make_unique( program ) );
	}
}

ShaderPass::ShaderPass( const PassSources sources, const std::vector<std::string> &features ) {
	this->sources = sources;
	int i = 0;
	if ( features.size() > 12 ) {
		warningstream << "Shader pass has " << features.size() << " features. "
			<< "This will result in " << ( 1 << features.size() ) << " generated variants! "
			<< "Consider reducing the feature count.";
	}
	for ( const std::string &str : features ) {
		featureMap[str] = i++;
		if ( 64 < i ) {
			errorstream << "Shader pass feature count exceeds 64, the pass may not be compiled.\n";
			return;
		}
	}
	GenVariants( features );
	if ( !buildFailed ) {
		AggregateUniforms();
	}
}
const ShaderProgram& ShaderPass::GetProgram( const u64 programKey ) const {
	return variants[programKey];
}
