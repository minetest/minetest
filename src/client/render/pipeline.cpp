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
    if (index == m_depth_texture_index)
        return m_depth_texture;
    if (index >= m_textures.size())
        return nullptr;
    return m_textures[index];
}

void TextureBuffer::activate()
{
    ensureRenderTarget();
    m_driver->setRenderTargetEx(m_render_target, m_clear ? video::ECBF_DEPTH | video::ECBF_COLOR : 0, getClearColor());
    RenderTarget::activate();
}

void TextureBuffer::ensureRenderTarget()
{
    if (!m_render_target)
    {
        m_render_target = m_driver->addRenderTarget();
        m_render_target->setTexture(m_textures, m_depth_texture);
    }
}

void TextureBuffer::setTexture(u8 index, u16 width, u16 height, const std::string &name, video::ECOLOR_FORMAT format)
{
    if (m_render_target) {
        m_driver->removeRenderTarget(m_render_target);
        m_render_target = nullptr;
    }

    if (m_depth_texture_index == index) {
        if (m_depth_texture)
            m_driver->removeTexture(m_depth_texture);
        m_depth_texture = nullptr;
        m_depth_texture_index = 255; /* unused */
    }
    if (m_textures.size() > index && m_textures[index])
        m_driver->removeTexture(m_textures[index]);

    m_textures.reallocate(index + 1);
    while (m_textures.size() < index + 1)
        m_textures.push_back(nullptr);

    m_textures[index] = m_driver->addRenderTargetTexture({width, height}, name.c_str(), format);
}

void TextureBuffer::setDepthTexture(u8 index, u16 width, u16 height, const std::string &name, video::ECOLOR_FORMAT format)
{
    if (m_render_target) {
        m_driver->removeRenderTarget(m_render_target);
        m_render_target = nullptr;
    }

    if (m_textures.size() > index && m_textures[index]) {
        m_driver->removeTexture(m_textures[index]);
        m_textures[index] = nullptr;
    }
    
    if (m_depth_texture)
        m_driver->removeTexture(m_depth_texture);
    
    if (index == m_textures.size() - 1)
        m_textures.reallocate(index);
    
    m_depth_texture_index = index;
    m_depth_texture = m_driver->addRenderTargetTexture({width, height}, name.c_str(), format);
}

TextureBufferOutput::TextureBufferOutput(video::IVideoDriver *_driver, TextureBuffer *_buffer, u8 _texture_index)
    : driver(_driver), buffer(_buffer), texture_index(_texture_index)
{}

void TextureBufferOutput::activate()
{
    auto texture = buffer->getTexture(texture_index);
    driver->setRenderTarget(texture, m_clear, m_clear, getClearColor());
    driver->OnResize(texture->getSize());

    RenderTarget::activate();
}

void ScreenTarget::activate()
{
    m_driver->setRenderTarget(nullptr, m_clear, m_clear, getClearColor());
    m_driver->OnResize(m_screen_size);
    RenderTarget::activate();
}

void ScreenTarget::reset()
{
    m_clear = true;
    m_screen_size = m_driver->getScreenSize();
}

RenderStep *createRenderingPipeline(const std::string &stereo_mode, video::IVideoDriver *driver,
		Client *client, Hud *hud)
{
    RenderPipeline *pipeline = new RenderPipeline();

    return pipeline;
};
