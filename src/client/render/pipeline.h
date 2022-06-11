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
#pragma once

#include "irrlichttypes_extrabloated.h"

#include <vector>
#include <memory>
#include <optional>

class RenderSource;
class RenderTarget;
class RenderStep;

/// Represents a source of rendering information
class RenderSource
{
public:
    /// Virtual destructor to ensure correct release of resources
    virtual ~RenderSource() = default;

    /// Return the number of texture in the source.
    virtual u8 getTextureCount() = 0;

    /// Get a texture by index. 
    /// Returns nullptr is the texture does not exist.
    virtual video::ITexture *getTexture(u8 index) = 0;

    /// Resets the state of the object for the next pipeline iteration
    virtual void reset() {};
};

/// Represents a render target (screen or FBO)
class RenderTarget
{
public:
    /// Virtual destructor to ensure correct release of resources
    virtual ~RenderTarget() = default;

    /// Activate the render target and configure OpenGL state
    virtual void activate()
    {
        m_clear = false;
    };

    /// Resets the state of the object for the next pipeline iteration
    virtual void reset()
    {
        m_clear = true;
    };
protected:
    bool m_clear {true};
};

/// Texture buffer represents set of textures
/// that are used as both render target and render source
class TextureBuffer : public RenderSource, public RenderTarget
{
public:

    TextureBuffer(video::IVideoDriver *driver)
            : m_driver(driver)
    {}

    virtual ~TextureBuffer() override;

    void setTexture(u8 index, u16 width, u16 height, u16 format);

    virtual u8 getTextureCount() override { return m_textures.size(); }
    virtual video::ITexture *getTexture(u8 index) override;
    virtual void activate() override;
private:
    void ensureRenderTarget();

    core::array<video::ITexture *> m_textures;
    video::IRenderTarget *m_render_target { nullptr };
    video::IVideoDriver *m_driver;
};

class RemappingSource : RenderSource
{
public:
    RemappingSource(RenderSource *source)
            : m_source(source)
    {}

    void setMapping(u8 index, u8 target_index)
    {
        if (index >= m_mappings.size()) {
            u8 start = m_mappings.size();
            m_mappings.resize(index);
            for (u8 i = start; i < m_mappings.size(); ++i)
                m_mappings[i] = i;
        }

        m_mappings[index] = target_index;
    }

    virtual u8 getTextureCount() override
    {
        return m_mappings.size();
    }

    virtual video::ITexture *getTexture(u8 index) override
    {
        if (index < m_mappings.size())
            index = m_mappings[index];

        return m_source->getTexture(index);
    }
public:
    RenderSource *m_source;
    std::vector<u8> m_mappings;
};

/// Enables direct output to screen buffer
class ScreenTarget : RenderTarget
{
public:
    ScreenTarget(video::IVideoDriver *driver) :
            m_driver(driver)
    {}

    virtual void activate() override;
    virtual void reset() override;
private:
    video::IVideoDriver *m_driver;
};

class RenderStep
{
public:
    virtual ~RenderStep() = default;
    virtual void setRenderSource(RenderSource *source) = 0;
    virtual void setRenderTarget(RenderTarget *target) = 0;
    virtual void reset() = 0;
    virtual void run() = 0;
};

class RenderPipeline : public RenderStep
{
public:
    void addStep(RenderStep *step)
    {
        m_pipeline.push_back(step);
    }

    RenderStep *own(RenderStep *step)
    {
        m_steps.push_back(std::unique_ptr<RenderStep>(step));
        return step;
    }

    RenderSource *own(RenderSource *source)
    {
        m_sources.push_back(std::unique_ptr<RenderSource>(source));
        return source;
    }

    RenderTarget *own(RenderTarget *target)
    {
        m_targets.push_back(std::unique_ptr<RenderTarget>(target));
        return target;
    }

    virtual void reset() override {}

    virtual void run() override
    {
        for (auto &target : m_targets)
            target->reset();

        for (auto &step : m_pipeline)
            step->reset();

        for (auto &step: m_pipeline)
            step->run();
    }

    virtual void setRenderSource(RenderSource *source) override {}
    virtual void setRenderTarget(RenderTarget *target) override {}
private:
    std::vector<RenderStep *> m_pipeline;
    std::vector< std::unique_ptr<RenderStep> > m_steps;
    std::vector< std::unique_ptr<RenderSource> > m_sources;
    std::vector< std::unique_ptr<RenderTarget> > m_targets;
};
