#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>

#ifdef __ANDROID__
	#include <gl3.h>
#else
	#include <glcorearb.h>
#endif

/*
	# Shaders and Materials

	A model is rendered using a specific Material instance.
	Materials contain a reference to a Shader, and a collection
	of uniform state, texture references and feature toggles.

	Shaders are objects representing a single complete GLSL shader.
	A shader is logically organized into passes, which are composed of
	program variants, which are composed of stages.
	(what we call a stage is what GL calls a "shader")

	By specifying a combination of a pass and a feature matrix,
	a single GPU program is selected for rendering.
*/

const u32 DONT_CARE = 0xFFFFFFFF;

enum class CompareTest : u32 {
	DontCare		= DONT_CARE,
	Never			= GL_NEVER,
	Always			= GL_ALWAYS,
	Less			= GL_LESS,
	LessEqual		= GL_LEQUAL,
	Equal			= GL_EQUAL,
	GreaterEqual	= GL_GEQUAL,
	Greater			= GL_GREATER,
	NotEqual		= GL_NOTEQUAL,
};
enum class BlendOp : u32 {
	DontCare		= DONT_CARE,
	Add				= GL_FUNC_ADD,
	Subtract		= GL_FUNC_SUBTRACT,
	RevSubtract		= GL_FUNC_REVERSE_SUBTRACT,
};
enum class BlendFactor : u32 {
	DontCare		= DONT_CARE,
	Zero			= GL_ZERO,
	One				= GL_ONE,
	Source			= GL_SRC_COLOR,
	Dest			= GL_DST_COLOR,
	SourceAlpha		= GL_SRC_ALPHA,
	DestAlpha		= GL_DST_ALPHA,
	NegSourceAlpha	= GL_ONE_MINUS_SRC_ALPHA,
	NegDestAlpha	= GL_ONE_MINUS_DST_ALPHA,
	NegSource		= GL_ONE_MINUS_SRC_COLOR,
	NegDest			= GL_ONE_MINUS_DST_COLOR,
};
enum class StencilOp : u32 {
	DontCare 		= DONT_CARE,
	Keep			= GL_KEEP,
	Zero			= GL_ZERO,
	Replace			= GL_REPLACE,
	Increment		= GL_INCR,
	IncrementWrap	= GL_INCR_WRAP,
	Decrement		= GL_DECR,
	DecrementWrap	= GL_DECR_WRAP,
	Invert			= GL_INVERT,
};

// Non-programmable state of a shader or material.
// This struct is simple enough to allow easy hashing and equality comparison.
struct FixedFunctionState {
	bool		useBlending;
	BlendFactor	srcBlend;
	BlendFactor	dstBlend;

	// Alpha clip/test.
	// Note that it must be emulated with discard on some obscure drivers,
	// and thus we have to add a variant for that.
	bool		alphaTest;

	// See GLES 2.0 reference 4.1.4 "Stencil Test"
	CompareTest stencilTest;
	StencilOp	stencilFail;
	StencilOp	stencilZFail;
	StencilOp	stencilPass;

	// Disable for passes that should never apply AA,
	// like deferred rendering data.
	bool		allowAntiAliasing = true;

	CompareTest	depthTest;
	bool		depthWrite = true; // Disable for transparent shaders.

	float		brightness;
};

struct PassSources {
	std::string header; // Common header pasted right above each shader stage.

	std::string stages[5];
}
struct ProgramUniform {
	u32				type;
	u32				arrayLength;
	s32				location;
};

// There are five shader stage types in OpenGL and there likely won't be more any soon.
// We order stages by their order of appearance in the pipeline; if all five are used
// by a program, they will be executed in this order.
enum class StageIndex : u8 {
	Vertex			= 0,
	TessControl		= 1,	// since GL4.0 or GLES3.2
	TessEval		= 2,
	Geometry		= 3,	// since GL3.2 or GLES3.2
	Fragment		= 4,
};
// Extensions that introduced each stage.
std::string stageExtensions[5] = {
	"ARB_shader_object",
	"ARB_tessellation_shader",
	"ARB_tessellation_shader",
	"ARB_geometry_shader"
	"ARB_shader_object",
};
// GL enums to select each stage.
u32 stageTypes[5] = {
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,
};
enum class StageCompileStatus : u32 {
	Failure = 0,	// Stage failed to compile.
	Success = 1,	// Stage compiled successfully.
	NoSource = 2,	// Stage source was not provided.
};

class ShaderProgram {
	/*
		An object encapsulating a single OpenGL program object.
		Refer to the OpenGL reference for details.

		For our purposes, a single ShaderProgram is equivalent
		to one variant of a given pass.
	*/

	StageCompileStatus compileStatus[5];
	std::string compileLog[5];

	// True only after successful linkage.
	bool linked;

	u32 programHandle;

	static u32 lastBoundProgram = 0;
public:
	// Uniform list queried from OpenGL.
	// These are the uniforms that you *can* set on this program.
	std::unordered_map<std::string,ProgramUniform> uniforms;

	// This blob of text prefixes every single shader source.
	// Use it to set platform specific or init-time defines.
	static std::stringstream globalHeader;

	inline StageCompileStatus GetCompileStatus( StageIndex i ) {
		return compileStatus[i];
	}
	inline bool IsLinked() { return linked; }

	inline void Bind() {
		// Temp kludge until state can be reduced in some smarter way.
		if ( programHandle != lastBoundProgram ) {
			glUseProgram( programHandle );
			lastBoundProgram = programHandle;
		}
	}

	/* Programs are compiled and analyzed on construction. */
	ShaderProgram( PassSources sources );
	~ShaderProgram();
};

// variant.uniform[pass.uniforms[variantkey][shader.uniforms[passname][name]]]

// Is 'ext' an extension supported by the current device?
bool ExtensionPresent( const std::string &ext ) {
	return true; // dummy
}

ShaderProgram::ShaderProgram( PassSources sources, std::string extraHeader = "" ) {
	u32 stages[5];
	handle = glCreateProgram();

	if ( sources.stages[static_cast<u32>(StageIndex::Vertex)].length() == 0 ) {
		errorstream << "Vertex shader source missing.";
		return;
	}
	if ( sources.stages[static_cast<u32>(StageIndex::Fragment)].length() == 0 ) {
		errorstream << "Fragment shader source missing.";
		return;
	}

	for ( int i = 0; i < 5; i++ ) {
		if ( sources.stages[i].length() > 0 && ExtensionPresent( stageExtension[i] ) ) {
			stages[i] = glCreateShader( stageTypes[i] );
			if ( glIsShader( stages[i] ) ) {
				glShaderSource(
					stages[i], 3,
					{
						globalHeader.str(),
						sources.header.c_str(),
						sources.stages[i].c_str()
					},
					{ -1, -1, -1 } ); // C-strings need no lengths.
				glCompileShader( stages[i] );
				glGetShaderiv( stages[i], GL_COMPILE_STATUS, &compileStatus[i] );
			}
			if ( compileStatus[i] == StageCompileStatus::Success ) {
				glAttachShader( handle, stages[i] );
			}
		} else {
			compileStatus[i] = StageCompileStatus::NoSource;
		}
	}

	glLinkProgram( programHandle );
	glGetProgramiv( programHandle, GL_LINK_STATUS, &linked );

	if ( linked ) {
		u32 uniformCount;
		glGetProgramiv( programHandle, GL_ACTIVE_UNIFORMS, &uniformCount );

		u32 nameLength;
		glGetProgramiv( programHandle, ACTIVE_UNIFORM_MAX_LENGTH, &nameLength );

		u8 *nameBuffer = new u8[nameLength];
		for ( int i = 0; i < uniformCount ; i++ ) {
			u32 arrayLength;
			u32 type;
			u32 actualNameLength;
			glGetActiveUniform( programHandle, i, nameLength, &actualNameLength, &type, &arrayLength, nameBuffer );

			u32 location;
			glGetUniformLocation( programHandle, nameBuffer );

			uniforms[std::string( nameBuffer )] = {
				type,
				arrayLength,
				location,
			};
		}
	}

	// Regardless of how linking went, the shaders can be discarded now.
	for (int i = 0; i < 5; i++ ) {
		if ( stages[i] ) {
			if ( programHandle )
				glDetachShader( programHandle, stages[i] );
			glDeleteShader( stages[i] );
		}
	}
}
ShaderProgram::~ShaderProgram() {
	glDeleteProgram( programHandle )
}

/*
	A single utility pass of a shader.
	Utility passes allow a shader to provide
	different shader programs for a given rendering technique.

	Examples of utility passes:
	- G-buffer setup
	- Shadow caster
	- Plain forward render
	- Outline or other highlight
	- Damage overlay or similar
*/
class ShaderPass {
	PassSources sources;

	// Each shader feature is assigned a bit position for use in a bitmask.
	// Then, each unique bitmask is a key to a map of variant programs.
	std::unordered_map<std::string,int> featurePositions;
	std::vector<std::unique_ptr<ShaderProgram>> variants;

	u32 variantCount;

	std::stringstream errorLog;

	bool buildFailed;

	// Generate all variants recursively from a list of features.
	void GenVariants( std::vector<std::string> &featureStack );
public:

	FixedFunctionState fixedState;

	inline u32 GetVariantCount() { return variantCount; }

	inline bool IsValid() { return !buildFailed; }

	// Build a bitmask based on a list of features.
	const u64 GetVariantKey( std::vector<std::string> featuresUsed ) const;
	// Get a program based on a variant bitmask obtained from GetVariantKey.
	const ShaderProgram* GetProgram( const u64 programKey ) const;

	ShaderPass( PassSources sources, std::vector<std::string> features );
};
u64 ShaderPass::GetVariantKey( std::vector<std::string> featuresUsed ) const {
	u64 r = 0;
	auto end = featurePositions.end();
	// For each requested feature, light up the appropriate bit in the bitmask.
	for ( const std::string &str : featuresUsed ) {
		const auto itr = featurePositions.find( str );
		if ( itr != end )
			r |= ( 1 << featurePositions.at(str) );
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
	int featureCount = features.count();
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
				if ( program->compileStatus[i] == StageCompileStatus::Failure ) {
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

ShaderPass::ShaderPass( PassSources sources, std::vector<std::string> features ) {
	this->sources = sources;
	int i = 0;
	if ( features.count() > 12 ) {
		warningstream << "Shader pass has " << features.count() << " features. "
			<< "This will result in " << ( 1 << features.count() ) << " generated variants! "
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
const ShaderProgram* ShaderPass::GetProgram( const u64 programKey ) const {
	return variants[programKey].get();
}

std::unordered_map< u32, u32 > uniformTypeStrides = {
	{ GL_FLOAT, 		sizeof( GLfloat ) },
	{ GL_FLOAT_VEC2,	sizeof( GLfloat ) * 2 },
	{ GL_FLOAT_VEC3,	sizeof( GLfloat ) * 3 },
	{ GL_FLOAT_VEC4,	sizeof( GLfloat ) * 4 },
	{ GL_FLOAT_MAT2,	sizeof( GLfloat ) * 4 },
	{ GL_FLOAT_MAT3,	sizeof( GLfloat ) * 9 },
	{ GL_FLOAT_MAT4,	sizeof( GLfloat ) * 16 },
	{ GL_INT, 			sizeof( GLint ) },
	{ GL_INT_VEC2,		sizeof( GLint ) * 2 },
	{ GL_INT_VEC3,		sizeof( GLint ) * 3 },
	{ GL_INT_VEC4,		sizeof( GLint ) * 4 },
	{ GL_BOOL, 			sizeof( GLbool ) },
	{ GL_BOOL_VEC2,		sizeof( GLbool ) * 2 },
	{ GL_BOOL_VEC3,		sizeof( GLbool ) * 3 },
	{ GL_BOOL_VEC4,		sizeof( GLbool ) * 4 },
	{ GL_SAMPLER_2D,	sizeof( GLuint ) },
	{ GL_SAMPLER_3D,	sizeof( GLuint ) },
	{ GL_SAMPLER_CUBE,	sizeof( GLuint ) },
};

class Shader {
	std::unordered_map<std::string, u32> passMap;
	std::vector<ShaderPass> passes;

	u32 uniformCount;
	std::unordered_map<std::string, u32> uniformIndexMap;
	// GL type of each uniform
	std::vector<u32> uniformTypes;
	// Size in bytes of a contiguous buffer that can store all the uniform state
	// required by this shader. It is a sum of all strides.
	size_t uniformBufferSize;
	// Offset of each uniform in the aforementioned buffer.
	std::vector<uintptr_t> uniformMemoryOffsets;
	/*
		The Location Matrix

		This is a 3-dimensional jagged array of uniform locations.
		The first two indices are pass and variant ID, and
		the third index is the uniform ID as understood by this class.

		To utilize this, you retrieve the appropriate row using the
		pass and variant, and then bring up this row to the material's
		list of uniform values. Then you iterate one by one, setting uniforms
		to the locations retrieved from the row, types known by the shader,
		and values known by the material, skipping wherever the location
		equals -1 (which means this particular Program did not contain
		this specific uniform after linking).
	*/
	std::vector<std::vector<std::vector<s32>>> locationMatrix;
	std::vector<u32> uniformTypes; // as understood by GL

	void BuildUniformData();

	// For force-enabling features globally
	u64 enableMask;
	// For force-disabling features globally
	u64 disableMask;


public:
	inline size_t GetUniformBufferSize() { return uniformBufferSize; }
	inline ShaderProgram* GetProgram( u32 passId, u64 variant ) {
		return passes[passId].GetProgram((variant & disableMask ) | enableMask);
	}
};

void Shader::BuildUniformData() {
	std::unordered_set<std::string> alreadyWarned;
	std::unordered_map<std::string,u32> allUniformTypes;
	for ( auto &pass : passes ) {
		for ( int i = 0; i < pass.GetVariantCount(); i++ ) {
			auto *program = pass.GetProgram( i );
			for ( auto &uniform : program->uniforms ) {
				if ( allUniformTypes.at(uniform.first) == allUniformTypes.end() ) {
					allUniformTypes[uniform.first] = uniform.second.type;
				} else {
					if ( allUniformTypes.at(uniform.first) != uniform.second.type ) {
						&& alreadyWarned.find(uniform.first) == alreadyWarned.end() ) {
							warningstream << "Type of uniform " << uniform.first
								<< " is not the same between passes or variants."
								<< " This is unsupported and may result"
								<< " in undefined behavior.\n";
							alreadyWarned.emplace(uniform.first);
						}
					}
				}
			}
		}
	}

	uniformCount = 0;
	for ( auto &pair : allUniformTypes ) {
		uniformTypes.push_back( pair.second );
		uniformIndexMap[pair.first] = uniformCount;
		uniformCount++;
	}
	
	// Iterate over all uniforms again.
	for ( auto &pass : passes ) {
		for ( int i = 0; i < pass.GetVariantCount(); i++ ) {
			auto *program = pass.GetProgram( i );
			for ( auto &uniform : program->uniforms ) {
				
			}
		}
	}

	uniformMemoryOffsets.clear();
	uniformMemoryOffsets.reserve( uniformCount );

	u32 position = 0;
	for ( u32 i = 0; i < uniformCount ; i++ ) {
		uniformMemoryOffsets.push_back( position );
		position += uniformTypeStrides[uniformTypes[i]];
	}
	uniformMemorySize = position;
}

class ShaderCache {
	std::unordered_map<std::string, std::shared_ptr<Shader>> cache;
};

class Material {
	Shader *shader;

	// Contiguous buffer of memory for all the uniforms.
	u8 *uniformMemory;

public:
	std::unordered_map<std::string, std::bool> features;

	void SetFloat( std::string name, float v );

	void SetFloat2( std::string name, float v[2] );
	void SetFloat2( std::string name, v2f v );
	void SetFloat2( std::string name, v2s16 v );

	void SetFloat3( std::string name, float v[3] );
	void SetFloat3( std::string name, v3f v );
	void SetFloat3( std::string name, v3s16 v );

	void SetFloat4( std::string name, float v[4] );

	void SetMatrix( std::string name, float v[16] );
	void SetMatrix( std::string name, Matrix4f v );

	void SetTexture( std::string name, Texture tex );

	void SetShader( const Shader &shader );

	u64 currentVariant;

	// Retrieve the program, bind it and set all the uniforms.
	void BindForRendering( u32 passId );

	Material( const Shader &shader ) { SetShader( shader ); }
};


// This function assumes that all the necessary safety checks
// been already performed, and 'value' is a pointer to an
// appropriate value for the uniform with the index 'i'.
void Material::SetUniformUnsafe( uint i, void *value ) {
	uintptr_t destAddress = ((uintptr_t)uniformMemory) + uniformMemoryOffsets[i];
	std::memcpy( (void*)destAddress, value, shader->uniformTypeStrides[shader->uniformTypes[i]];
}

void Material::BindForRendering( u32 passId ) {
	auto &program = shader->GetProgram( passId, currentVariant );
}

void Material::SetShader( const Shader* newShader ) {
	u8 *newUniformMemory = new u8[newShader->GetUniformBufferSize()];
	if ( uniformMemory ) {
		// Copy what we can from the old shader
		for ( auto &pair : newShader->uniformIndexMap ) {
			// Does the old shader have a uniform with this name?
			auto &oldIndex = shader->uniformIndexMap.at( pair.first );
			if ( oldIndex != shader.uniformIndexMap.end() ) {
				// Is it the same type?
				auto oldT = shader->uniformTypes[oldIndex];
				auto newT = newShader->uniformTypes[pair.second];
				if ( oldT == newT ) {
					// Blit the value
					std::memcpy(
						newUniformMemory + newShader->uniformMemoryOffsets[pair.second],
						uniformMemory + shader->uniformMemoryOffsets[oldIndex],
						uniformTypeStrides[oldT]
					);
				}
			}
		}
		delete uniformMemory;
	}

	uniformMemory = newUniformMemory;
	this->shader = newShader;
}
