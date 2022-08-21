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


void TextureBuffer::setTexture(u8 index, core::dimension2du size, const std::string &name, video::ECOLOR_FORMAT format)
{
	assert(index != NO_DEPTH_TEXTURE);

	if (m_definitions.size() <= index)
		m_definitions.resize(index + 1);

	if (m_depth_texture_index == index)
		m_depth_texture_index = NO_DEPTH_TEXTURE;
	
	auto &definition = m_definitions[index];
	definition.valid = true;
	definition.dirty = true;
	definition.fixed_size = true;
	definition.size = size;
	definition.name = name;
	definition.format = format;
}

void TextureBuffer::setTexture(u8 index, v2f scale_factor, const std::string &name, video::ECOLOR_FORMAT format)
{
	assert(index != NO_DEPTH_TEXTURE);

	if (m_definitions.size() <= index)
		m_definitions.resize(index + 1);
	
	if (m_depth_texture_index == index)
		m_depth_texture_index = NO_DEPTH_TEXTURE;

	auto &definition = m_definitions[index];
	definition.valid = true;
	definition.dirty = true;
	definition.fixed_size = false;
	definition.scale_factor = scale_factor;
	definition.name = name;
	definition.format = format;
}

void TextureBuffer::setDepthTexture(u8 index, core::dimension2du size, const std::string &name, video::ECOLOR_FORMAT format)
{
	assert(index != NO_DEPTH_TEXTURE);
	setTexture(index, size, name, format);
	m_depth_texture_index = index;
}

void TextureBuffer::setDepthTexture(u8 index, v2f scale_factor, const std::string &name, video::ECOLOR_FORMAT format)
{
	assert(index != NO_DEPTH_TEXTURE);
	setTexture(index, scale_factor, name, format);
	m_depth_texture_index = index;
}

void TextureBuffer::reset(PipelineContext &context)
{
	if (!m_driver)
		m_driver = context.device->getVideoDriver();

	// remove extra textures
	if (m_textures.size() > m_definitions.size()) {
		for (unsigned i = m_definitions.size(); i < m_textures.size(); i++)
			if (m_textures[i])
				m_driver->removeTexture(m_textures[i]);

		m_textures.set_used(m_definitions.size());
	}

	// add placeholders for new definitions
	while (m_textures.size() < m_definitions.size())
		m_textures.push_back(nullptr);

	// change textures to match definitions
	bool modified = false;
	for (u32 i = 0; i < m_definitions.size(); i++) {
		video::ITexture **ptr = &m_textures[i];
		if (i == m_depth_texture_index) {
			if (*ptr) {
				m_driver->removeTexture(*ptr);
				*ptr = nullptr;
			}
			ptr = &m_depth_texture;
		}

		if (ensureTexture(ptr, m_definitions[i], context))
			modified = true;
		m_definitions[i].dirty = false;
	}
	
	// make sude depth texture is removed and reset
	if (m_depth_texture_index == NO_DEPTH_TEXTURE && m_depth_texture) {
		m_driver->removeTexture(m_depth_texture);
		m_depth_texture = nullptr;
	}

	if (!m_render_target)
		m_render_target = m_driver->addRenderTarget();

	if (modified)
		m_render_target->setTexture(m_textures, m_depth_texture);

	RenderTarget::reset(context);
}

void TextureBuffer::activate(PipelineContext &context)
{
	m_driver->setRenderTargetEx(m_render_target, m_clear ? video::ECBF_DEPTH | video::ECBF_COLOR : 0, context.clear_color);
	RenderTarget::activate(context);
}

bool TextureBuffer::ensureTexture(video::ITexture **texture, const TextureDefinition& definition, PipelineContext &context)
{
	bool modify;
	core::dimension2du size;
	if (definition.valid) {
		if (definition.fixed_size)
			size = definition.size;
		else
			size = core::dimension2du(
					(u32)(context.target_size.X * definition.scale_factor.X),
					(u32)(context.target_size.Y * definition.scale_factor.Y));
		
		modify = definition.dirty || (*texture == nullptr) || (*texture)->getSize() != size;
	}
	else {
		modify = (*texture != nullptr);
	}

	if (!modify)
		return false;

	if (*texture)
		m_driver->removeTexture(*texture);

	if (definition.valid)
		*texture = m_driver->addRenderTargetTexture(size, definition.name.c_str(), definition.format);
	else
		*texture = nullptr;

	return true;
}

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, u8 _texture_index)
	: buffer(_buffer), texture_index(_texture_index)
{}

void TextureBufferOutput::activate(PipelineContext &context)
{
	auto texture = buffer->getTexture(texture_index);
	auto driver = context.device->getVideoDriver();
	driver->setRenderTarget(texture, m_clear, m_clear, context.clear_color);
	driver->OnResize(texture->getSize());

	RenderTarget::activate(context);
}

u8 DynamicSource::getTextureCount()
{
	assert(isConfigured());
	return upstream->getTextureCount();
}

video::ITexture *DynamicSource::getTexture(u8 index)
{
	assert(isConfigured());
	return upstream->getTexture(index);
}

void ScreenTarget::activate(PipelineContext &context)
{
	auto driver = context.device->getVideoDriver();
	driver->setRenderTarget(nullptr, m_clear, m_clear, context.clear_color);
	driver->OnResize(size);
	RenderTarget::activate(context);
}

void DynamicTarget::activate(PipelineContext &context)
{
	if (!isConfigured())
		throw std::logic_error("Dynamic render target is not configured before activation.");
	upstream->activate(context);
}

void ScreenTarget::reset(PipelineContext &context)
{
	RenderTarget::reset(context);
	size = context.device->getVideoDriver()->getScreenSize();
}

SetRenderTargetStep::SetRenderTargetStep(RenderStep *_step, RenderTarget *_target)
	: step(_step), target(_target)
{
}

void SetRenderTargetStep::run(PipelineContext &context)
{
	step->setRenderTarget(target);
}

RenderSource *RenderPipeline::getInput()
{
	return &m_input;
}

RenderTarget *RenderPipeline::getOutput()
{
	return &m_output;
}

void RenderPipeline::run(PipelineContext &context)
{
	v2u32 original_size = context.target_size;
	context.target_size = v2u32(original_size.X * scale.X, original_size.Y * scale.Y);

	for (auto &object : m_objects)
		object->reset(context);

	for (auto &step: m_pipeline)
		step->run(context);
	
	context.target_size = original_size;
}

void RenderPipeline::setRenderSource(RenderSource *source)
{
	m_input.setRenderSource(source);
}

void RenderPipeline::setRenderTarget(RenderTarget *target)
{
	m_output.setRenderTarget(target);
}
