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
	// For each requested feature, light up the appropriate bits in the bitmask.
	// If multiple mutually exclusive features are submitted, which one ends up
	// enabled is undefined.
	for ( const auto &str : featuresUsed ) {
		if ( STL_CONTAINS( featureMap, str ) ) {
			featurePositions.at( str ).EnableFeature( &r );
		}
	}
	return r;
}

// Split a string into tokens using whitespace.
std::vector<std::string> SplitString( const std::string &str ) {
	std::istringstream stream(str);
	std::vector<std::string> split{std::istream_iterator<std::string>(stream),std::istream_iterator<std::string>()};
	return split;
}
// Figure out the minimum amount of bits necessary to represent 'number'.
u32 BitsToContain( u32 number ) {
	u32 r = 0;
	while ( ( 1 << r ) <= number ) r++;
	return std::max( 1, r );
}

// Stage names for error printing.
const std::string stageNames[5] = {
	"vertex",
	"tess control",
	"tess eval",
	"geometry",
	"fragment",
};

void ShaderPass::DeleteVariants() {
	for ( u32 i = 0; i < variants.size(); i++ )
		if ( variants[i] )
			delete variants[i];
	variants.clear();
	featureMap.clear();
}

void ShaderPass::GenVariants( const ShaderSource &sources, const std::vector<std::string> &features ) {
	std::vector<bool> variantValidity;
	bool warnedAboutAmount;
	u32 bitPosition = 0;
	u32 actualVariantCount = 1;
	infostream << "Compiling shader pass variants.\n";
	for ( const auto &str : features ) {
		// Rings are represented by whitespace-separated lists of define tokens.
		std::vector<std::string> ring = SplitString( str );
		u32 ringOrder = ring.size() + 1;
		actualVariantCount *= ringOrder;
		if ( actualVariantCount > 2048 && !warnedAboutAmount ) {
			warnedAboutAmount = true;
			warningstream << "Shader pass exceeds 2048 generated variants! "
					<< "If compiling them takes too long, consider reducing the feature count.";
		}
		u32 bitCount = BitsToContain( ringOrder );
		u32 bitMask = ( ( 1u << bitCount ) - 1 ) << bitPosition;
		for ( u32 i = 0; i < ( 1u << bitCount ); i++ ) {
			if ( i < ringOrder ) {
				variantValidity.push_back( true );
				// Start feature indices at 1 while also keeping 0 valid
				// for the absence of the feature
				if ( i > 0 ) {
					featureMap[ring[i-1]] = {
						.index = i,
						.shift = bitPosition,
						.mask = bitMask,
					};
				}
			} else {
				// Variant does not exist: the index
				// is bigger than the ring's feature count.
				variantValidity.push_back( false );
			}
		}
		bitPosition += bitCount;
	}
	assert( variantValidity.size() == ( 1 << bitPosition ) );
	for ( u64 i = 0 ; i < ( 1 << bitPosition ) ; i++ ) {
		if ( variantValidity[i] ) {
			std::stringstream variantFeatures;
			for ( auto &pair : featureMap ) {
				auto &str = pair.first;
				auto &bits = pair.second;
				// Compare the number at the ring's position.
				if ( ( bits.index << bits.shift ) == ( i & bits.mask ) )
					variantFeatures << "#define " << pair.first << " = 1\n";
			}
			ShaderProgram *program = new ShaderProgram( sources, variantFeatures.str() );
			if ( !program->IsLinked() ) {
				errorstream << "GLSL shader pass build failure.\n";
				errorstream << "Compiling with features:\n\n";
				errorstream << variantFeatures.str();
				errorstream << "\n";
				for ( u32 i = 0; i < 5; i++ ) {
					if ( program->GetCompileStatus(static_cast<StageIndex>(i)) == StageCompileStatus::Failure ) {
						errorstream
							<< "\tError log for " << stageNames[i] << " shader:\n\n"
							<<  program->GetCompileLog(static_cast<StageIndex>(i)) << "\n";
					}
				}
				buildFailed = true;
				DeleteVariants();
				return;
			}
			variants.push_back( program );
		} else {
			variants.push_back( nullptr );
		}
	}
}

ShaderPass::ShaderPass( const ShaderSource &sources, const std::vector<std::string> &features ) {
	GenVariants( sources, features );
}
const ShaderProgram& ShaderPass::GetProgram( const u64 programKey ) const {
	assert( programKey < variants.size() && variants[programKey] );
	return *variants[programKey];
}
