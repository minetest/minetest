/*
Minetest
Copyright (C) 2016 RealBadAngel, Maciej Kasatkin <maciej.kasatkin@o2.pl>

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

#include "mrt.h"
#include "log.h"
#include "debug.h"

Mrt::Mrt(irr::IrrlichtDevice* device)
	:m_device(device),
	m_resolution(irr::core::dimension2d<irr::u32> (0,0))
{
	m_driver = device->getVideoDriver();
	sanity_check(m_driver);
	updateMRT();
}

Mrt::~Mrt()
{
	if (m_colorRTT)
		m_driver->removeTexture(m_colorRTT);
	if (m_depthRTT)
		m_driver->removeTexture(m_depthRTT);
	if (m_normalRTT)
		m_driver->removeTexture(m_normalRTT);
	m_MRT.clear();
}

void Mrt::updateMRT()
{
	irr::core::dimension2d<irr::u32> resolution = 
		m_driver->getCurrentRenderTargetSize();
	if (resolution == m_resolution)
		return;
	m_resolution = resolution;

	if (m_colorRTT)
		m_driver->removeTexture(m_colorRTT);
	if (m_depthRTT)
		m_driver->removeTexture(m_depthRTT);
	if (m_normalRTT)
		m_driver->removeTexture(m_normalRTT);
		
	m_colorRTT = m_driver->addRenderTargetTexture(m_resolution,
		"mrt_colorRTT", irr::video::ECF_A8R8G8B8);
	sanity_check(m_colorRTT != NULL);
	m_depthRTT = m_driver->addRenderTargetTexture(m_resolution,
		"mrt_depthRTT", irr::video::ECF_A8R8G8B8);
	sanity_check(m_depthRTT  != NULL);
	m_normalRTT = m_driver->addRenderTargetTexture(m_resolution,
		"mrt_normalRTT", irr::video::ECF_A8R8G8B8);
	sanity_check(m_normalRTT != NULL);

	m_MRT.clear();
	m_MRT.push_back(irr::video::IRenderTarget(m_colorRTT));
	m_MRT.push_back(irr::video::IRenderTarget(m_depthRTT));
	m_MRT.push_back(irr::video::IRenderTarget(m_normalRTT));
}

void Mrt::setRenderTarget(irr::video::SColor color)
{
	m_driver->setRenderTarget(m_MRT, true, true, color);
}
