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

#ifndef MRT_H
#define MRT_H

#include <irrlicht.h>

class Mrt
{
	public:
		Mrt(irr::IrrlichtDevice* device);
		~Mrt();

		void updateMRT();
		void setRenderTarget(irr::video::SColor color);

        irr::video::ITexture* getColorRTT() const
        { return m_colorRTT; }

		irr::video::ITexture* getDepthRTT() const
		{ return m_depthRTT; }

        irr::video::ITexture* getNormalRTT() const
		{ return m_normalRTT; }

    private:
		irr::IrrlichtDevice* m_device;
		irr::video::IVideoDriver *m_driver;
		irr::core::dimension2d<irr::u32> m_resolution;

		irr::video::ITexture* m_colorRTT;
		irr::video::ITexture* m_depthRTT;
		irr::video::ITexture* m_normalRTT;

		irr::core::array<irr::video::IRenderTarget> m_MRT;
};
#endif
