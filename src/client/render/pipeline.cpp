/*
Minetest
Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>

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

#include "pipeline.h"
#include "client/client.h"
#include "client/hud.h"

#include <vector>
#include <memory>


TextureBuffer::~TextureBuffer()
{
    if (m_render_target)
        m_driver->removeRenderTarget(m_render_target);
    m_render_target = nullptr;
    for (u32 index = 0; index < m_textures.size(); index++)
        m_driver->removeTexture(m_textures[index]);
    m_textures.clear();
}

video::ITexture *TextureBuffer::getTexture(u8 index)
{
    if (index >= m_textures.size())
        return nullptr;
    return m_textures[index];
}

void TextureBuffer::activate()
{
    ensureRenderTarget();
    m_driver->setRenderTargetEx(m_render_target, m_clear ? video::ECBF_DEPTH | video::ECBF_COLOR : 0);
    RenderTarget::activate();
}

void TextureBuffer::ensureRenderTarget()
{
    if (!m_render_target)
    {
        m_render_target = m_driver->addRenderTarget();
        m_render_target->setTexture(m_textures, nullptr);
    }
}

void ScreenTarget::activate()
{
    m_driver->setRenderTarget(nullptr, m_clear, m_clear);
    RenderTarget::activate();
}

void ScreenTarget::reset()
{
    m_clear = true;
}

RenderStep *createRenderingPipeline(const std::string &stereo_mode, video::IVideoDriver *driver,
		Client *client, Hud *hud)
{
    RenderPipeline *pipeline = new RenderPipeline();

    return pipeline;
};
