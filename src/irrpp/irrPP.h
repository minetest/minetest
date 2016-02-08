#ifndef IRRPP_H
#define IRRPP_H

#include <irrlicht.h>
#include "camera.h"

#define IRRPP_VERSION 0.1

#include "IQuadSceneNode.h"
#include "CPostProcessingEffect.h"
#include "CPostProcessingEffectChain.h"
#include "IShaderDefaultPostProcessCallback.h"
#include "E_POST_PROCESSING_EFFECT.h"

namespace irr
{
    namespace video
	{
		class irrPP;
	}
}

//! Use this function to create a new instance of irrPP
/**
 * @param device irrlicht device to use
 * @param quality default quality of an effect, the RTT1 and RTT2 textures are created at this quality
 * @param shaderDir directory which holds shader source files, relative to the executable
 * @return New instance of irrPP
 */
irr::video::irrPP* createIrrPP(
		irr::IrrlichtDevice* device,
		Camera *camera,
		irr::video::E_POSTPROCESSING_EFFECT_QUALITY quality = irr::video::EPQ_FULL,
		const irr::io::path shaderDir = "postprocess/");

namespace irr
{
namespace video
{

class irrPP
{
    public:
		irrPP(irr::IrrlichtDevice* device, Camera* camera,
				irr::video::E_POSTPROCESSING_EFFECT_QUALITY quality,
				const irr::io::path shaderDir);
		~irrPP();

		void render(irr::video::ITexture* input,
				irr::video::ITexture* output = 0);

		/**
		 * Creates a new post-processing effect chain.
		 * @return Pointer to the newly created chain.
		 */
		irr::video::CPostProcessingEffectChain* createEffectChain();

		/**
		 * Adds a new post processing effect into the chain
		 * @param effectShader shader to use fur this effect
		 * @param callback custom callback to use for this shader, if any
		 * @return A pointer to the newly created post processing effect
		 */
		irr::video::CPostProcessingEffect* createEffect(irr::core::stringc sourceFrag);

		/**
		 * Adds a new post processing effect into the chain using one of the included shaders
		 * @param type type of the shader
		 * @return A pointer to the newly created post processing effect
		 */
		irr::video::CPostProcessingEffect* createEffect(irr::video::E_POSTPROCESSING_EFFECT type);
	
		void setQuality(irr::video::E_POSTPROCESSING_EFFECT_QUALITY quality);

		irr::video::ITexture* getRTT1() const;

		irr::video::ITexture* getRTT2() const;

		irr::u32 getActiveEffectCount() const;

		/**
		 * Returns the root post-processing effect chain. Effects which don't belong to a specific chain are managed by a "root" chain.
		 * @return Pointer to the root post-processing effect chain.
		 */
		irr::video::CPostProcessingEffectChain* getRootEffectChain() const;

		irr::scene::IQuadSceneNode* getQuadNode() const;

		core::stringc getDebugString() const;

	private:
		irr::IrrlichtDevice* Device;
		Camera *camera;
		irr::video::E_POSTPROCESSING_EFFECT_QUALITY Quality;
		irr::io::path ShaderDir;
		
		irr::video::ITexture* RTT1, *RTT2;
		irr::scene::IQuadSceneNode* Quad;
		irr::core::array<irr::video::CPostProcessingEffectChain*> Chains;

		struct EffectEntry
		{
		    irr::video::CPostProcessingEffect* effect;
		    irr::video::ITexture* source;
		    irr::video::ITexture* target;
		    bool keepSource;
		};
};

}
}

#endif // IRRPP_H
