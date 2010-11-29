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
#include <string.h>
#include <stdio.h>

VALUE cFFrubyFile;

#ifdef RDOC
	/* Satisfy stupid RDoc. */
	static void rdoc() { mFFruby = rb_define_module("FFruby"); }
#endif

static void ffrf_free(AVFormatContext *fmt)
{
	unsigned int i;

	if (fmt != NULL)
	{
		for (i = 0; i < fmt->nb_streams; i++)
		{
			if (fmt->streams[i]->codec->codec != NULL)
				avcodec_close(fmt->streams[i]->codec);
		}

		av_close_input_file(fmt);
	}
}

static VALUE ffrf_alloc(VALUE klass)
{
	return Data_Wrap_Struct(klass, 0, ffrf_free, NULL);
}

static AVFormatContext* ffrf_get_fmt(VALUE self)
{
	AVFormatContext *fmt;
	Data_Get_Struct(self, AVFormatContext, fmt);
	return fmt;
}

/* Returns the title string. */
static VALUE ffrf_title(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->title);
}

/* Returns the author string. */
static VALUE ffrf_author(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->author);
}

/* Returns the copyright string. */
static VALUE ffrf_copyright(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->copyright);
}

/* Returns the comment string. */
static VALUE ffrf_comment(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->comment);
}

/* Returns the album string. */
static VALUE ffrf_album(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->album);
}

/* Returns the genre string. */
static VALUE ffrf_genre(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->genre);
}

/* Returns the year. */
static VALUE ffrf_year(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return INT2NUM(fmt->year);
}

/* Returns the track number. */
static VALUE ffrf_track(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return INT2NUM(fmt->track);
}

/* Returns the duration in seconds as a float. */
static VALUE ffrf_duration(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_float_new((double) fmt->duration / (double) AV_TIME_BASE);
}

/* Returns the bit rate. */
static VALUE ffrf_bit_rate(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return INT2NUM(fmt->bit_rate);
}

/* Returns the format name. */
static VALUE ffrf_format(VALUE self)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	return rb_str_new2(fmt->iformat->name);
}

/* Returns an array of streams contained within this file. */
static VALUE ffrf_streams(VALUE self)
{
	unsigned int i;
	AVFormatContext *fmt;
	VALUE streams;
	VALUE* args;

	if ((streams = rb_iv_get(self, "@streams")) == Qnil)
	{
		fmt = ffrf_get_fmt(self);
		streams = rb_ary_new();
		rb_iv_set(self, "@streams", streams);

		args = (VALUE*) ALLOCA_N(VALUE*, 2);
		args[0] = self;

		for (i = 0; i < fmt->nb_streams; i++)
		{
			args[1] = INT2FIX(i);

			switch (fmt->streams[i]->codec->codec_type)
			{
				case CODEC_TYPE_VIDEO:
					rb_ary_push(streams, rb_class_new_instance(2, args, cFFrubyVideoStream));
					break;
				case CODEC_TYPE_AUDIO:
					rb_ary_push(streams, rb_class_new_instance(2, args, cFFrubyAudioStream));
					break;
				default:
					break;
			}
		}
	}

	return streams;
}

static VALUE ffrf_av_streams(VALUE self, VALUE klass)
{
	VALUE stream;
	VALUE streams;
	VALUE typed_streams;
	unsigned int i;

	streams = rb_funcall(self, rb_intern("streams"), 0);
	typed_streams = rb_ary_new();
	Check_Type(streams, T_ARRAY);

	for (i = 0; i < RARRAY(streams)->len; i++)
	{
		if (rb_obj_is_kind_of((stream = rb_ary_entry(streams, i)), klass))
			rb_ary_push(typed_streams, stream);
	}

	return typed_streams;
}

/* Returns an array of video streams contained within this file. */
static VALUE ffrf_video_streams(VALUE self)
{
	return ffrf_av_streams(self, cFFrubyVideoStream);
}

/* Returns an array of audio streams contained within this file. */
static VALUE ffrf_audio_streams(VALUE self)
{
	return ffrf_av_streams(self, cFFrubyAudioStream);
}

/* call-seq:
 *   new(filename) -> FFruby::File
 *
 * Creates an FFruby::File instance using the given filename. */
static VALUE ffrf_initialize(VALUE self, VALUE filename)
{
	size_t len;
	char* msg;
	AVFormatContext *fmt;
	VALUE exception;

	VALUE filename_str = rb_funcall(filename, rb_intern("to_s"), 0);
    char* filename_ptr = RSTRING(filename_str)->ptr;

	if (av_open_input_file(&fmt, filename_ptr, NULL, 0, NULL) != 0)
	{
		len = strlen("Cannot open file !") + strlen(filename_ptr) + 1;
		msg = ALLOC_N(char, len);
		snprintf(msg, len, "Cannot open file %s!", filename_ptr);
		exception = rb_exc_new2(rb_eIOError, msg);
		free(msg);
		rb_exc_raise(exception);
	}

	if (av_find_stream_info(fmt) < 0)
	{
		len = strlen("Problem reading file !") + strlen(filename_ptr) + 1;
		msg = ALLOC_N(char, len);
		snprintf(msg, len, "Problem reading file %s!", filename_ptr);
		exception = rb_exc_new2(rb_eIOError, msg);
		free(msg);
		rb_exc_raise(exception);
	}

	DATA_PTR(self) = fmt;
	return self;
}

/* Document-class: FFruby::File
 *
 * An interface to FFmpeg on existing files. Provides access to
 * metadata and stream instances. */
void Init_ffrf()
{
	cFFrubyFile = rb_define_class_under(mFFruby, "File", rb_cObject);
	rb_define_alloc_func(cFFrubyFile, ffrf_alloc);
	rb_define_method(cFFrubyFile, "initialize", ffrf_initialize, 1);
	rb_define_method(cFFrubyFile, "title", ffrf_title, 0);
	rb_define_method(cFFrubyFile, "author", ffrf_author, 0);
	rb_define_method(cFFrubyFile, "copyright", ffrf_copyright, 0);
	rb_define_method(cFFrubyFile, "comment", ffrf_comment, 0);
	rb_define_method(cFFrubyFile, "album", ffrf_album, 0);
	rb_define_method(cFFrubyFile, "genre", ffrf_genre, 0);
	rb_define_method(cFFrubyFile, "year", ffrf_year, 0);
	rb_define_method(cFFrubyFile, "track", ffrf_track, 0);
	rb_define_method(cFFrubyFile, "duration", ffrf_duration, 0);
	rb_define_method(cFFrubyFile, "bit_rate", ffrf_bit_rate, 0);
	rb_define_method(cFFrubyFile, "format", ffrf_format, 0);
	rb_define_method(cFFrubyFile, "streams", ffrf_streams, 0);
	rb_define_method(cFFrubyFile, "video_streams", ffrf_video_streams, 0);
	rb_define_method(cFFrubyFile, "audio_streams", ffrf_audio_streams, 0);
}
