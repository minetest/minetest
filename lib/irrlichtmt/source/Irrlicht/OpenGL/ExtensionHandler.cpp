// Copyright (C) 2015 Patryk Nadrowski
// Copyright (C) 2009-2010 Amundis
// 2017 modified by Michael Zeilfelder (unifying extension handlers)
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "ExtensionHandler.h"

#include "irrString.h"
#include "SMaterial.h"
#include "fast_atof.h"
#include <mt_opengl.h>

namespace irr
{
namespace video
{
	void COpenGL3ExtensionHandler::initExtensionsOld()
	{
		auto extensions_string = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
		const char *pos = extensions_string;
		while (const char *next = strchr(pos, ' ')) {
			addExtension(std::string{pos, next});
			pos = next + 1;
		}
		addExtension(pos);
		updateLegacyExtensionList();
	}

	void COpenGL3ExtensionHandler::initExtensionsNew()
	{
		int ext_count = GetInteger(GL_NUM_EXTENSIONS);
		for (int k = 0; k < ext_count; k++)
			addExtension(reinterpret_cast<const char *>(GL.GetStringi(GL_EXTENSIONS, k)));
		updateLegacyExtensionList();
	}

	void COpenGL3ExtensionHandler::addExtension(std::string name) {
		Extensions.emplace(std::move(name));
	}

	bool COpenGL3ExtensionHandler::queryExtension(const std::string &name) const{
		return Extensions.find(name) != Extensions.end();
	}

	void COpenGL3ExtensionHandler::updateLegacyExtensionList() {
		for (size_t j = 0; j < IRR_OGLES_Feature_Count; ++j)
			FeatureAvailable[j] = queryExtension(getFeatureString(j));
	}

} // end namespace video
} // end namespace irr
