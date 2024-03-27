// Copyright (C) 2015 Patryk Nadrowski
// Copyright (C) 2009-2010 Amundis
// 2017 modified by Michael Zeilfelder (unifying extension handlers)
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "ExtensionHandler.h"

#include "irrString.h"
#include "SMaterial.h"
#include "fast_atof.h"
#include "os.h"
#include <mt_opengl.h>

// FIXME: this basically duplicates what mt_opengl.h already does

namespace irr
{
namespace video
{
void COpenGL3ExtensionHandler::initExtensionsOld()
{
	auto extensions_string = reinterpret_cast<const char *>(GL.GetString(GL_EXTENSIONS));
	const char *pos = extensions_string;
	while (const char *next = strchr(pos, ' ')) {
		addExtension(std::string{pos, next});
		pos = next + 1;
	}
	addExtension(pos);
	extensionsLoaded();
}

void COpenGL3ExtensionHandler::initExtensionsNew()
{
	int ext_count = GetInteger(GL_NUM_EXTENSIONS);
	for (int k = 0; k < ext_count; k++)
		addExtension(reinterpret_cast<const char *>(GL.GetStringi(GL_EXTENSIONS, k)));
	extensionsLoaded();
}

void COpenGL3ExtensionHandler::addExtension(std::string &&name)
{
	Extensions.emplace(std::move(name));
}

bool COpenGL3ExtensionHandler::queryExtension(const std::string &name) const noexcept
{
	return Extensions.find(name) != Extensions.end();
}

void COpenGL3ExtensionHandler::extensionsLoaded()
{
	os::Printer::log((std::string("Loaded ") + std::to_string(Extensions.size()) + " extensions:").c_str(), ELL_DEBUG);
	for (const auto &it : Extensions)
		os::Printer::log((std::string("  ") + it).c_str(), ELL_DEBUG);
	for (size_t j = 0; j < IRR_OGLES_Feature_Count; ++j)
		FeatureAvailable[j] = queryExtension(getFeatureString(j));
}

} // end namespace video
} // end namespace irr
