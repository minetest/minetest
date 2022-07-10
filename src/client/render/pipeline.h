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
#include <string>

class RenderSource;
class RenderTarget;
class RenderStep;
class Client;
class Hud;

struct PipelineContext
{
    PipelineContext(IrrlichtDevice *_device, Client *_client, Hud *_hud, video::SColor _color, v2u32 _target_size)
        : device(_device), client(_client), hud(_hud), clear_color(_color), target_size(_target_size)
    {
    }

    IrrlichtDevice *device;
    Client *client;
    Hud *hud;
    video::SColor clear_color;
    v2u32 target_size;
};

/**
 * Represents a source of rendering information such as textures
 */
class RenderSource
{
public:
    virtual ~RenderSource() = default;

    /**
     * Return the number of textures in the source.
     */
    virtual u8 getTextureCount() = 0;

    /**
     * Get a texture by index. 
     * Returns nullptr is the texture does not exist.
     */
    virtual video::ITexture *getTexture(u8 index) = 0;

    /**
     * Resets the state of the object for the next pipeline iteration
     */
    virtual void reset() {}

    /**
     * Resets the state of the object for the next pipeline iteration
     * 
     * @param context Execution context of the pipeline
     */
    virtual void reset(PipelineContext *context) { reset(); }
};

/**
 *  Represents a render target (screen or framebuffer)
 */
class RenderTarget
{
public:
    virtual ~RenderTarget() = default;

    /**
     * Set the clear color of the render target
     * 
     * @param color color use when clearing the render target
     */
    void setClearColor(video::SColor *color) { m_clear_color = color; }

    /**
     * Activate the render target and configure OpenGL state for the output.
     * This is usually done by @see RenderStep implementations.
     */
    virtual void activate()
    {
        m_clear = false;
    }

    /**
     * Activate the render target and configure OpenGL state for the output.
     * This is usually done by @see RenderStep implementations.
     */
    virtual void activate(PipelineContext *context)
    {
        activate();
    }

    /**
     * Resets the state of the object for the next pipeline iteration
     */
    virtual void reset()
    {
        m_clear = true;
    }

    /**
     * Resets the state of the object for the next pipeline iteration
     */
    virtual void reset(PipelineContext *context)
    {
        reset();
    }

protected:
    bool m_clear {true};
    video::SColor *m_clear_color { nullptr };

    video::SColor getClearColor() { return m_clear_color ? *m_clear_color : video::SColor(0x0u); }
};

/**
 * Texture buffer represents a framebuffer with a multiple attached textures.
 *
 * @note Use of TextureBuffer requires use of gl_FragData[] in the shader
 */
class TextureBuffer : public RenderSource, public RenderTarget
{
public:

    TextureBuffer(video::IVideoDriver *driver)
            : m_driver(driver)
    {}

    virtual ~TextureBuffer() override;

    /**
     * Configure texture for the specific index
     * 
     * @param index index of the texture
     * @param width width of the texture in pixels
     * @param height height of the texture in pixels
     * @param name unique name of the texture
     * @param format color format
     */
    void setTexture(u8 index, u16 width, u16 height, const std::string& name, video::ECOLOR_FORMAT format);

    /**
     * @Configure depth texture and assign index
     * 
     * @param index index to use for the depth texture
     * @param width width of the texture in pixels
     * @param height height of the texture in pixels
     * @param name unique name for the texture
     * @param format color format
     */
    void setDepthTexture(u8 index, u16 width, u16 height, const std::string& name, video::ECOLOR_FORMAT format);

    virtual u8 getTextureCount() override { return m_textures.size(); }
    virtual video::ITexture *getTexture(u8 index) override;
    virtual void activate() override;
private:
    void ensureRenderTarget();

    video::IVideoDriver *m_driver;
    core::array<video::ITexture *> m_textures;
    video::ITexture *m_depth_texture { nullptr };
    u8 m_depth_texture_index { 255 /* assume unused */ };
    video::IRenderTarget *m_render_target { nullptr };
};

/**
 * Targets output to designated texture in texture buffer
 */
class TextureBufferOutput : public RenderTarget
{
public:
    TextureBufferOutput(TextureBuffer *buffer, u8 texture_index);
    void activate(PipelineContext *context) override;
private:
    TextureBuffer *buffer;
    u8 texture_index;
};

/**
 * Allows remapping texture indicies in another RenderSource.
 * 
 * @note all unmapped indexes are passed through to the underlying render source.
 */
class RemappingSource : RenderSource
{
public:
    RemappingSource(RenderSource *source)
            : m_source(source)
    {}

    /**
     * Maps texture index to a different index in the dependent source.
     * 
     * @param index texture index as requested by the @see RenderStep.
     * @param target_index matching texture index in the underlying @see RenderSource.
     */
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

/**
 * Implements direct output to screen framebuffer.
 */
class ScreenTarget : public RenderTarget
{
public:
    ScreenTarget(video::IVideoDriver *driver) :
            m_driver(driver)
    {}

    virtual void activate() override;
    virtual void reset() override;
private:
    video::IVideoDriver *m_driver;
    core::dimension2du m_screen_size { 0, 0 };
};

/**
 * Base class for rendering steps in the pipeline
 */
class RenderStep
{
public:
    virtual ~RenderStep() = default;

    /**
     * Assigns render source to this step.
     * 
     * @param source source of rendering information
     */
    virtual void setRenderSource(RenderSource *source) = 0;

    /**
     * Assigned render target to this step.
     * 
     * @param target render target to send output to.
     */
    virtual void setRenderTarget(RenderTarget *target) = 0;

    /**
     * Resets the step state before executing the pipeline.
     * 
     * Steps may carry state between invocations in the pipeline.
     */
    virtual void reset() {}

    /**
     * Resets the step state before executing the pipeline.
     * 
     * Steps may carry state between invocations in the pipeline.
     */
    virtual void reset(PipelineContext *context) { reset(); }

    /**
     * Runs the step. This method is invoked by the pipeline.
     */
    virtual void run() {}

    /**
     * Runs the step. This method is invoked by the pipeline.
     */
    virtual void run(PipelineContext *context) { run(); }
};

/**
 * Provides default empty implementation of supporting methods in a rendering step.
 */
class TrivialRenderStep : public RenderStep
{
public:
    virtual void setRenderSource(RenderSource *source) override {}
    virtual void setRenderTarget(RenderTarget *target) override {}
    virtual void reset() override {}
};

/**
 * Implements a trivial step by calling a method on another object (rendering core)
 * 
 * @tparam _Target type of the object to invoke the method for.
 */
template <class _Target>
class TrampolineStep : public TrivialRenderStep
{
public:
    /**
     * Describes the prototype of supported method
     */
    typedef void (_Target::*_Call)();


    TrampolineStep(_Target *core, _Call call) :
        core(core), call(call)
    {}

    virtual void run() override 
    {
        (core->*call)();
    }
private:
    _Target *core;
    _Call call;
};

/**
 * Render Pipeline provides a flexible way to execute rendering steps in the engine.
 * 
 * RenderPipeline also implements @see RenderStep, allowing for nesting of the pipelines.
 */
class RenderPipeline : public RenderStep
{
public:
    /**
     * Add a step to the end of the pipeline
     * 
     * @param step reference to a @see RenderStep implementation.
     */
    void addStep(RenderStep *step)
    {
        m_pipeline.push_back(step);
    }

    /**
     * Capture ownership of a dynamically created @see RenderStep instance.
     * 
     * RenderPipeline will delete the instance when the pipeline is destroyed.
     * 
     * @param step reference to the instance.
     * @return RenderStep* value of the 'step' parameter.
     */
    RenderStep *own(RenderStep *step)
    {
        m_steps.push_back(std::unique_ptr<RenderStep>(step));
        return step;
    }

    /**
     * Capture ownership of a dynamically created @see RenderSource instance.
     * 
     * RenderPipeline will delete the instance when the pipeline is destroyed.
     * 
     * @param source reference to the instance.
     * @return RenderSource* value of the 'source' parameter.
     */
    RenderSource *own(RenderSource *source)
    {
        m_sources.push_back(std::unique_ptr<RenderSource>(source));
        return source;
    }

    /**
     * Capture ownership of a dynamically created @see RenderTarget instance.
     * 
     * @param target reference to the instance
     * @return RenderTarget* value of the 'target' parameter
     */
    RenderTarget *own(RenderTarget *target)
    {
        m_targets.push_back(std::unique_ptr<RenderTarget>(target));
        return target;
    }

    virtual void reset(PipelineContext *context) override {}

    virtual void run(PipelineContext *context) override
    {
        for (auto &target : m_targets)
            target->reset(context);

        for (auto &step : m_pipeline)
            step->reset(context);

        for (auto &step: m_pipeline)
            step->run(context);
    }

    virtual void setRenderSource(RenderSource *source) override {}
    virtual void setRenderTarget(RenderTarget *target) override {}
private:
    std::vector<RenderStep *> m_pipeline;
    std::vector< std::unique_ptr<RenderStep> > m_steps;
    std::vector< std::unique_ptr<RenderSource> > m_sources;
    std::vector< std::unique_ptr<RenderTarget> > m_targets;
};
