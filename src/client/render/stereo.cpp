/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "stereo.h"
#include "client/client.h"
#include "client/camera.h"
#include "constants.h"
#include "settings.h"

OffsetCameraStep::OffsetCameraStep(float eye_offset)
{
	move.setTranslation(core::vector3df(eye_offset, 0.0f, 0.0f));
}


OffsetCameraStep::OffsetCameraStep(bool right_eye)
{
	float eye_offset = BS * g_settings->getFloat("3d_paralax_strength", -0.087f, 0.087f) * (right_eye ? 1 : -1);
	move.setTranslation(core::vector3df(eye_offset, 0.0f, 0.0f));
}

void OffsetCameraStep::reset(PipelineContext &context)
{
	base_transform = context.client->getCamera()->getCameraNode()->getRelativeTransformation();
}

void OffsetCameraStep::run(PipelineContext &context)
{
	context.client->getCamera()->getCameraNode()->setPosition((base_transform * move).getTranslation());
}
