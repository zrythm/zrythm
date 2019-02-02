/* ffmpeg compatibility wrappers
 *
 * Copyright 2012,2013 Robin Gareus <robin@gareus.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FFCOMPAT_H
#define FFCOMPAT_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(50, 0, 0)
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 2, 0)
static inline int avformat_open_input(AVFormatContext **ps, const char *filename, void *fmt, void **options)
{
	return av_open_input_file(ps, filename, NULL, 0, NULL);
}
#endif /* avformat < 53.2.0 */

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 5, 0)
static inline AVCodecContext *
avcodec_alloc_context3(AVCodec *codec __attribute__((unused)))
{
	return avcodec_alloc_context();
}

static inline AVStream *
avformat_new_stream(AVFormatContext *s, AVCodec *c) {
	return av_new_stream(s,0);
}

static inline int
avcodec_get_context_defaults3(AVCodecContext *s, AVCodec *codec)
{
	avcodec_get_context_defaults(s);
	return 0;
}

#endif /* < 53.5.0 */

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 5, 6)
static inline int
avcodec_open2(AVCodecContext *avctx, AVCodec *codec, void **options __attribute__((unused)))
{
	return avcodec_open(avctx, codec);
}
#endif /* <= 53.5.6 */

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 5, 0)
static inline int
avformat_find_stream_info(AVFormatContext *ic, void **options)
{
	return av_find_stream_info(ic);
}

static inline void
avformat_close_input(AVFormatContext **s)
{
	av_close_input_file(*s);
}

#endif /* < 53.5.0 */

#endif /* FFCOMPAT_H */
