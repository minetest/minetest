#include "irrPP.h"

irr::video::irrPP* createIrrPP(irr::IrrlichtDevice* device,
		irr::video::E_POSTPROCESSING_EFFECT_QUALITY quality,
		const irr::io::path shaderDir)
{
	return new irr::video::irrPP(device, quality, shaderDir);
}

irr::video::irrPP::irrPP(irr::IrrlichtDevice* device,
		irr::video::E_POSTPROCESSING_EFFECT_QUALITY quality,
		const irr::io::path shaderDir)
	:Device(device),
	Quality(quality),
	ShaderDir(shaderDir),
	RTT1(0), RTT2(0)
{
	Quad = new irr::scene::IQuadSceneNode(0, Device->getSceneManager());
	// create the root chain
	createEffectChain();

	setQuality(quality);
}

irr::video::irrPP::~irrPP()
{
	while (Chains.size() > 0) {
		delete Chains[0];
		Chains.erase(0);
	}
}

void irr::video::irrPP::render(irr::video::ITexture* input,
		irr::video::ITexture* output)
{
	irr::u32 numActiveEffects = getActiveEffectCount();

	// first, let's order all the active effects
	EffectEntry orderedEffects[numActiveEffects];

	irr::u32 effectCounter = 0;
	irr::video::ITexture* usedRTT = input, *freeRTT;

	irr::u32 numChains = Chains.size();

	for (irr::u32 chain = 0; chain < numChains; chain++) {
		if (Chains[chain]->isActive()) {
			irr::video::CPostProcessingEffectChain* thisChain = Chains[chain];
			irr::u32 numChainEffects = thisChain->getEffectCount();

			bool shouldveKeptLastRender = false;
			if (thisChain->getKeepOriginalRender())
				shouldveKeptLastRender = true;

			for (irr::u32 effect = 0; effect < numChainEffects; effect++) {
				if (thisChain->getEffectFromIndex(effect)->isActive()) {
					irr::video::CPostProcessingEffect* thisEffect =
						thisChain->getEffectFromIndex(effect);
					freeRTT = (usedRTT == RTT2 ? RTT1 : RTT2);

					orderedEffects[effectCounter].effect = thisEffect;
					orderedEffects[effectCounter].source = usedRTT;
					orderedEffects[effectCounter].target = freeRTT;
					usedRTT = freeRTT;

					// handle 'downscale your RTT' feature
					if (thisEffect->getCustomRTT()) {
						orderedEffects[effectCounter].target =
							thisEffect->getCustomRTT();
						usedRTT = thisEffect->getCustomRTT();
					}

					// handle 'keep original render' feature
					if (shouldveKeptLastRender && effectCounter > 0) {
						orderedEffects[effectCounter - 1].target =
							thisChain->getOriginalRender();
						orderedEffects[effectCounter].source =
							thisChain->getOriginalRender();

						shouldveKeptLastRender = false;
					}
					effectCounter++;
				}
			}
		}
	}

	// render the ordered effects
	for (irr::u32 i = 0; i < numActiveEffects; i++)
	{
		if (i < numActiveEffects - 1)
			Device->getVideoDriver()->setRenderTarget(orderedEffects[i].target);
		else // last effect
			Device->getVideoDriver()->setRenderTarget(output);

		Quad->setMaterialType(orderedEffects[i].effect->getMaterialType());
		Quad->setMaterialTexture(0, orderedEffects[i].source);

		irr::u32 numTexturesToPass =
			orderedEffects[i].effect->getTextureToPassCount();
		for (irr::u32 texToPass = 0; texToPass < numTexturesToPass; texToPass++)
			Quad->setMaterialTexture(texToPass + 1,
				orderedEffects[i].effect->getTextureToPass(texToPass));

		Quad->render();
	}
}





irr::video::CPostProcessingEffectChain* irr::video::irrPP::createEffectChain()
{
	irr::video::CPostProcessingEffectChain* chain =
		new irr::video::CPostProcessingEffectChain(Device, Quality, ShaderDir);
	Chains.push_back(chain);

	return chain;
}

irr::video::CPostProcessingEffect* irr::video::irrPP::createEffect(
		irr::core::stringc sourceFrag,
		irr::video::IShaderConstantSetCallBack* callback)
{
	return getRootEffectChain()->createEffect(sourceFrag, callback);
}

irr::video::CPostProcessingEffect* irr::video::irrPP::createEffect(
		irr::video::E_POSTPROCESSING_EFFECT type)
{
	return getRootEffectChain()->createEffect(type);
}

void irr::video::irrPP::setQuality(
		irr::video::E_POSTPROCESSING_EFFECT_QUALITY quality)
{
	irr::core::dimension2d<irr::u32> fullRes =
		Device->getVideoDriver()->getCurrentRenderTargetSize();
	irr::core::dimension2d<irr::u32> qRes = fullRes / (irr::u32)quality;

	if (RTT1)
		Device->getVideoDriver()->removeTexture(RTT1);

	if (RTT2)
		Device->getVideoDriver()->removeTexture(RTT2);

	RTT1 = Device->getVideoDriver()->addRenderTargetTexture(qRes, "irrPP-RTT1");
	RTT2 = Device->getVideoDriver()->addRenderTargetTexture(qRes, "irrPP-RTT2");

	Quality = quality;
}

void irr::video::irrPP::setQuality(irr::core::dimension2d<irr::u32> resolution)
{
	if (RTT1)
		Device->getVideoDriver()->removeTexture(RTT1);

	if (RTT2)
		Device->getVideoDriver()->removeTexture(RTT2);

	RTT1 = Device->getVideoDriver()->addRenderTargetTexture(resolution, "irrPP-RTT1");
	RTT2 = Device->getVideoDriver()->addRenderTargetTexture(resolution, "irrPP-RTT2");

	Quality = EPQ_CUSTOM;
}

irr::video::ITexture* irr::video::irrPP::getRTT1() const
{
	return RTT1;
}

irr::video::ITexture* irr::video::irrPP::getRTT2() const
{
	return RTT2;
}

irr::u32 irr::video::irrPP::getActiveEffectCount() const
{
	irr::u32 numChains = Chains.size();
	irr::u32 count = 0;

	for (irr::u32 chain = 0; chain < numChains; chain++)
	{
		count += Chains[chain]->getActiveEffectCount();
	}

	return count;
}

irr::video::CPostProcessingEffectChain* irr::video::irrPP::getRootEffectChain() const
{
	return Chains[0];
}

irr::scene::IQuadSceneNode* irr::video::irrPP::getQuadNode() const
{
	return Quad;
}

irr::core::stringc irr::video::irrPP::getDebugString() const
{
	irr::core::stringc out;
	out += "Active effects: ";
	out += getActiveEffectCount();
	out += "\n";

	irr::u32 counter = 0, numChains = Chains.size();
	for (irr::u32 chain = 0; chain < numChains; chain++) {
		if (Chains[chain]->isActive()) {
			irr::video::CPostProcessingEffectChain* thisChain = Chains[chain];
			irr::u32 numChainEffects = thisChain->getEffectCount();

			for (irr::u32 effect = 0; effect < numChainEffects; effect++) {
				if (thisChain->getEffectFromIndex(effect)->isActive()) {
					irr::video::CPostProcessingEffect* thisEffect =
						thisChain->getEffectFromIndex(effect);

					out += "#";
					out += counter;
					out += "\t";

					out += thisEffect->getName();
					out += "\t\t";

					if (thisEffect->getCustomRTT()) {
						out += thisEffect->getCustomRTT()->getSize().Width;
						out += "x";
						out += thisEffect->getCustomRTT()->getSize().Height;
					}
					else {
						out += RTT1->getSize().Width;
						out += "x";
						out += RTT1->getSize().Height;
					}

					out += "\n";

					counter++;
				}
			}
		}
	}
	return out;
}
