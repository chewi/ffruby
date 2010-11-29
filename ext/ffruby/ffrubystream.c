/*
 * Copyright (c) 2007-2010 James Le Cuirot
 *
 * This file is part of FFruby.
 *
 * FFruby is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFruby is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ffruby.h"

VALUE cFFrubyStream;
VALUE cFFrubyVideoStream;
VALUE cFFrubyAudioStream;

/* Satisfy stupid RDoc. This hopefully gets optimised away. */
static void rdoc() { mFFruby = rb_define_module("FFruby"); }

static VALUE ffrs_alloc(VALUE klass)
{
	return Data_Wrap_Struct(klass, 0, 0, NULL);
}

static AVStream* ffrs_get_stream(VALUE self)
{
	AVStream *stream;
	Data_Get_Struct(self, AVStream, stream);
	return stream;
}

static void ffrs_open_codec(AVStream* stream)
{
	AVCodec *codec;

	if (stream->codec->codec == NULL)
	{
		if ((codec = avcodec_find_decoder(stream->codec->codec_id)) == NULL)
			rb_raise(rb_eIOError, "Cannot find codec!");

		if (avcodec_open(stream->codec, codec) < 0)
			rb_raise(rb_eIOError, "Cannot open codec!");
	}
}

/* Returns the codec name. */
static VALUE ffrs_codec(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	ffrs_open_codec(stream);
	return rb_str_new2(stream->codec->codec->name);
}

/* Returns the FourCC tag. */
static VALUE ffrs_tag(VALUE self)
{
	char tag[4];
	signed int i, j;
	AVStream *stream = ffrs_get_stream(self);

	i = stream->codec->codec_tag;

	for (j = 3; j >= 0; j -= 1)
	{
		tag[j] = i >> (j * 8);
		i -= tag[j] << (j * 8);
	}

	for (j = 3; j >= 0; j -= 1)
	{
		if (tag[j] != '\0')
			break;
	}

	return rb_str_new(tag, j + 1);
}

/* Returns the bit rate. */
static VALUE ffrs_bit_rate(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	return INT2NUM(stream->codec->bit_rate);
}

/* Returns the frame width. */
static VALUE ffrvs_width(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	return INT2NUM(stream->codec->width);
}

/* Returns the frame height. */
static VALUE ffrvs_height(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	return INT2NUM(stream->codec->height);
}

/* Returns the frame aspect ratio. */
static VALUE ffrvs_frame_aspect_ratio(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);

	if (stream->codec->width == 0 || stream->codec->height == 0)
		return Qnil;
	else
		return rb_funcall(rb_mKernel, rb_intern("Rational"), 2, INT2NUM(stream->codec->width), INT2NUM(stream->codec->height));
}

/* Returns the sample aspect ratio. */
static VALUE ffrvs_sample_aspect_ratio(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);

	if (stream->codec->sample_aspect_ratio.den == 0 || stream->codec->sample_aspect_ratio.num == 0)
		return rb_funcall(rb_mKernel, rb_intern("Rational"), 2, INT2FIX(1), INT2FIX(1));
	else
		return rb_funcall(rb_mKernel, rb_intern("Rational"), 2, INT2NUM(stream->codec->sample_aspect_ratio.num), INT2NUM(stream->codec->sample_aspect_ratio.den));
}

/* Returns the real aspect ratio. */
static VALUE ffrvs_real_aspect_ratio(VALUE self)
{
	VALUE x, y;
	x = ffrvs_frame_aspect_ratio(self);
	y = ffrvs_sample_aspect_ratio(self);

	if (x == Qnil || y == Qnil)
		return Qnil;
	else
		return rb_funcall(x, rb_intern("*"), 1, y);
}

/* Returns the frame rate as a Rational. */
static VALUE ffrvs_frame_rate(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	ffrs_open_codec(stream);	

	if (stream->r_frame_rate.den && stream->r_frame_rate.num)
		return rb_funcall(rb_mKernel, rb_intern("Rational"), 2, INT2NUM(stream->r_frame_rate.num), INT2NUM(stream->r_frame_rate.den));
	else
		return rb_funcall(rb_mKernel, rb_intern("Rational"), 2, INT2NUM(stream->codec->time_base.den), INT2NUM(stream->codec->time_base.num));
}

/* Returns the number of audio channels. */
static VALUE ffras_channels(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	return INT2FIX(stream->codec->channels);
}

/* Returns the sample rate. */
static VALUE ffras_sample_rate(VALUE self)
{
	AVStream *stream = ffrs_get_stream(self);
	return INT2NUM(stream->codec->sample_rate);
}

/* call-seq:
 *   new(file, index) -> FFruby::Stream
 *
 * Creates an FFruby::Stream instance using the given FFruby::File and
 * stream index. This should usually only be called by FFruby
 * itself. Access stream instances using FFruby::File#streams
 * instead. */
static VALUE ffrs_initialize(VALUE self, VALUE file, VALUE index)
{
	AVFormatContext *fmt;
	AVStream *stream;
	unsigned int i = FIX2UINT(index);

	Data_Get_Struct(file, AVFormatContext, fmt);

	if (i < 0 || i >= fmt->nb_streams)
		rb_raise(rb_eIndexError, "Stream index out of range!");

	DATA_PTR(self) = fmt->streams[i];
	stream = ffrs_get_stream(self);

	return self;
}

void Init_ffrs()
{
/* Document-class: FFruby::Stream
 *
 * An interface to FFmpeg on any type of stream. Provides access
 * to generic metadata. */
	cFFrubyStream = rb_define_class_under(mFFruby, "Stream", rb_cObject);
	rb_define_alloc_func(cFFrubyStream, ffrs_alloc);
	rb_define_method(cFFrubyStream, "initialize", ffrs_initialize, 2);
	rb_define_method(cFFrubyStream, "codec", ffrs_codec, 0);
	rb_define_method(cFFrubyStream, "tag", ffrs_tag, 0);
	rb_define_method(cFFrubyStream, "bit_rate", ffrs_bit_rate, 0);

/* Document-class: FFruby::VideoStream
 *
 * An interface to FFmpeg on video streams. Provides access to
 * video metadata. */
	cFFrubyVideoStream = rb_define_class_under(mFFruby, "VideoStream", cFFrubyStream);
	rb_define_method(cFFrubyVideoStream, "width", ffrvs_width, 0);
	rb_define_method(cFFrubyVideoStream, "height", ffrvs_height, 0);
	rb_define_method(cFFrubyVideoStream, "frame_aspect_ratio", ffrvs_frame_aspect_ratio, 0);
	rb_define_method(cFFrubyVideoStream, "sample_aspect_ratio", ffrvs_sample_aspect_ratio, 0);
	rb_define_method(cFFrubyVideoStream, "real_aspect_ratio", ffrvs_real_aspect_ratio, 0);
	rb_define_method(cFFrubyVideoStream, "frame_rate", ffrvs_frame_rate, 0);

/* Document-class: FFruby::AudioStream
 *
 * An interface to FFmpeg on audio streams. Provides access to
 * audio metadata. */
	cFFrubyAudioStream = rb_define_class_under(mFFruby, "AudioStream", cFFrubyStream);
	rb_define_method(cFFrubyAudioStream, "channels", ffras_channels, 0);
	rb_define_method(cFFrubyAudioStream, "sample_rate", ffras_sample_rate, 0);
}
