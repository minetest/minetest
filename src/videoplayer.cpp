/*
Minetest
Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

#include "videoplayer.h"
#include "log.h"

extern "C" {
#ifdef _WIN32
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/imgutils.h>
#include <ffmpeg/swscale.h>
#else
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#endif
}

struct SwsContext  *SwsCtx       = nullptr;
AVCodecParameters  *CodecParam   = nullptr;
AVFormatContext    *FormatCtx    = nullptr;
AVCodecContext     *CodecCtxOrig = nullptr;
AVCodecContext     *CodecCtx     = nullptr;
AVCodec            *Codec        = nullptr;
AVFrame            *Frame        = nullptr;
AVFrame            *FrameRGB     = nullptr;
AVPacket            Packet;

// Copy data from decoded frame to Irrlicht image
void writeVideoTexture(AVFrame *Frame, video::IImage *image)
{
	if (!Frame->data[0])
		return;

	u32 width = image->getDimension().Width;
	u32 height = image->getDimension().Height;

	uint8_t *frame_data = Frame->data[0];
	u8 *data = (u8 *)(image->lock());

	// Decoded format: R8G8B8
	memcpy(data, frame_data, width * height * 3);
	image->unlock();
}

video::ITexture *VideoPlayer::getFrameTexture()
{
	driver->removeTexture(texture);
	texture = driver->addTexture("tmp", image);
	return texture;
}

s32 VideoPlayer::init(const char *name, IrrlichtDevice *device)
{
	this->device = device;
	driver = device->getVideoDriver();
	m_frame_idx = 0;
	m_filename = name;

	// Open video file
	if (avformat_open_input(&FormatCtx, name, NULL, NULL) != 0)
		return -1;

	// Silence warning output
	av_log_set_level(AV_LOG_ERROR);

	// Retrieve stream info
	if (avformat_find_stream_info(FormatCtx, NULL) < 0)
		return -1;

	// Dump info about file
	//av_dump_format(FormatCtx, 0, name, 0);

	// Video duration
	m_duration = (FormatCtx->duration / AV_TIME_BASE);

	// Find the 1st video stream
	stream = av_find_best_stream(FormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	// Get a pointer to the codec params for the video stream
	AVCodecParameters *CodecCtxOrig = FormatCtx->streams[stream]->codecpar;

	// Find the decoder for the video stream
	Codec = avcodec_find_decoder(CodecCtxOrig->codec_id);

	if (!Codec) {
		errorstream << "Unsupported codec!" << std::endl;
		return -1;
	}

	// Copy params, then assign to context
	CodecParam = avcodec_parameters_alloc();
	CodecCtx = avcodec_alloc_context3(Codec);

	if (avcodec_parameters_copy(CodecParam, CodecCtxOrig) != 0) {
		errorstream << "Couldn't copy codec params" << std::endl;
		return -1;
	}

	// Fill the codec context from codec params
	avcodec_parameters_to_context(CodecCtx, CodecParam);

	// Open codec
	if (avcodec_open2(CodecCtx, Codec, NULL) < 0)
		return -1;

	// Allocate video frame
	Frame = av_frame_alloc();

	// Allocate a AVFrame structure
	FrameRGB = av_frame_alloc();
	if (!FrameRGB)
		return -1;

	m_width = CodecCtx->width;
	m_height = CodecCtx->height;

	s32 bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_width, m_height, 1);
	buffer = (uint8_t *)av_malloc(bytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in FrameRGB
	av_image_fill_arrays(FrameRGB->data, FrameRGB->linesize,
		buffer, AV_PIX_FMT_RGB24, m_width, m_height, 1);

	// Init SWS context for scaling
	SwsCtx = sws_getContext(
		CodecCtx->width,
		CodecCtx->height,
		CodecCtx->pix_fmt,
		m_width,
		m_height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	// Video framerate
	m_framerate = ((float)CodecCtx->framerate.num) / CodecCtx->framerate.den;

	// Creating the image holding the decoded video frame data
	image = driver->createImage(video::ECOLOR_FORMAT::ECF_R8G8B8,
		core::dimension2du(m_width, m_height));
	
	avcodec_flush_buffers(CodecCtx);
	then = device->getTimer()->getTime();
	return 0;
}

s32 VideoPlayer::decodeFrame()
{
	u32 i = 0;
	while (true) {
		now = device->getTimer()->getTime();
		// Decode next frame only if the current's finished
		if ((now - then) >= (1.0f / (m_framerate + 1.5f)) * 1000) {
			if ((i = decodeFrameInternal()) != 0) {
				if (texture)
					driver->removeTexture(texture);
				continue;
			}
			then = now;
		}
		break;
	}

	return i;
}

s32 VideoPlayer::decodeFrameInternal()
{
	u32 ret = 0;
	if ((ret = av_read_frame(FormatCtx, &Packet) >= 0)) {
		// Whether a packet is from the stream
		if (Packet.stream_index == stream) {
			// Decode video frame
			avcodec_send_packet(CodecCtx, &Packet);
			avcodec_receive_frame(CodecCtx, Frame);

			// Whether we get a frame
			if (finished) {
				// Convert the image from its native format to RGB
				sws_scale(SwsCtx, (uint8_t const * const *)Frame->data,
					Frame->linesize, 0, CodecCtx->height,
					FrameRGB->data, FrameRGB->linesize);

				writeVideoTexture(FrameRGB, image);
				m_frame_idx++;
				ret = 0;
			}
		}

		// Free the packet allocated by av_read_frame()
		av_packet_unref(&Packet);
	}

	return ret;
}

void VideoPlayer::goToFrame(u32 frame_idx)
{
        av_seek_frame(FormatCtx, stream, frame_idx, AVSEEK_FLAG_BACKWARD);
        m_frame_idx = frame_idx;
}

void VideoPlayer::pause()
{
	av_seek_frame(FormatCtx, stream, m_frame_idx--, AVSEEK_FLAG_BACKWARD);
}

VideoPlayer::~VideoPlayer()
{
	if (image)
		image->drop();

	// Free the RGB image
	av_free(buffer);
	av_frame_free(&FrameRGB);

	// Free the YUV frame
	av_frame_free(&Frame);

	// Close the codecs
	avcodec_close(CodecCtx);
	avcodec_close(CodecCtxOrig);

	// Close the video file
	avformat_close_input(&FormatCtx);
}
