/*
 * Copyright (c) 2007-2014 James Le Cuirot
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
#include <stdlib.h>

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

#ifdef HAVE_AVFORMAT_CLOSE_INPUT
		avformat_close_input(&fmt);
#else
		av_close_input_file(fmt);
#endif
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

#ifdef HAVE_AV_DICT_GET

static VALUE ffrf_get_metadata_str(VALUE self, const char *key)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	AVDictionaryEntry *tag = av_dict_get(fmt->metadata, key, NULL, 0);
	return (tag && *tag->value) ? rb_str_new2(tag->value) : Qnil;
}

static VALUE ffrf_get_metadata_int(VALUE self, const char *key)
{
	AVFormatContext *fmt = ffrf_get_fmt(self);
	AVDictionaryEntry *tag = av_dict_get(fmt->metadata, key, NULL, 0);

	if (tag)
	{
		int value = atoi(tag->value);
		if (value != 0) return INT2NUM(value);
	}

	return Qnil;
}

#define METADATA_STR(key) ffrf_get_metadata_str(self, # key)
#define METADATA_INT(key) ffrf_get_metadata_int(self, # key)

#else

static VALUE ffrf_get_metadata_str(char* value)
{
	return (value && *value) ? rb_str_new2(value) : Qnil;
}

static VALUE ffrf_get_metadata_int(int value)
{
	return value != 0 ? INT2NUM(value) : Qnil;
}

#define METADATA_STR(key) ffrf_get_metadata_str(ffrf_get_fmt(self)->key)
#define METADATA_INT(key) ffrf_get_metadata_int(ffrf_get_fmt(self)->key)

#endif

/* Returns the title string. */
static VALUE ffrf_title(VALUE self)
{
	return METADATA_STR(title);
}

/* Returns the artist/author string. */
static VALUE ffrf_artist(VALUE self)
{
#ifdef HAVE_AV_DICT_GET
	return METADATA_STR(artist);
#else
	return METADATA_STR(author);
#endif
}

/* Returns the copyright string. */
static VALUE ffrf_copyright(VALUE self)
{
	return METADATA_STR(copyright);
}

/* Returns the comment string. */
static VALUE ffrf_comment(VALUE self)
{
	return METADATA_STR(comment);
}

/* Returns the album string. */
static VALUE ffrf_album(VALUE self)
{
	return METADATA_STR(album);
}

/* Returns the genre string. */
static VALUE ffrf_genre(VALUE self)
{
	return METADATA_STR(genre);
}

/* Returns the year. */
static VALUE ffrf_year(VALUE self)
{
	return METADATA_INT(year);
}

/* Returns the track number. */
static VALUE ffrf_track(VALUE self)
{
	return METADATA_INT(track);
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
#ifdef HAVE_CONST_AVMEDIA_TYPE_UNKNOWN
				case AVMEDIA_TYPE_VIDEO:
#else
				case CODEC_TYPE_VIDEO:
#endif
					rb_ary_push(streams, rb_class_new_instance(2, args, cFFrubyVideoStream));
					break;
#ifdef HAVE_CONST_AVMEDIA_TYPE_UNKNOWN
				case AVMEDIA_TYPE_AUDIO:
#else
				case CODEC_TYPE_AUDIO:
#endif
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

	for (i = 0; i < RARRAY_LEN(streams); i++)
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
	VALUE exception;
	AVFormatContext *fmt = NULL;

	VALUE filename_str = rb_funcall(filename, rb_intern("to_s"), 0);
	char* filename_ptr = RSTRING_PTR(filename_str);

#ifdef HAVE_AVFORMAT_OPEN_INPUT
	if (avformat_open_input(&fmt, filename_ptr, NULL, NULL) != 0)
#else
	if (av_open_input_file(&fmt, filename_ptr, NULL, 0, NULL) != 0)
#endif
	{
		len = strlen("Cannot open file !") + strlen(filename_ptr) + 1;
		msg = ALLOC_N(char, len);
		snprintf(msg, len, "Cannot open file %s!", filename_ptr);
		exception = rb_exc_new2(rb_eIOError, msg);
		free(msg);
		rb_exc_raise(exception);
	}

#ifdef HAVE_AVFORMAT_FIND_STREAM_INFO
	if (avformat_find_stream_info(fmt, NULL) < 0)
#else
	if (av_find_stream_info(fmt) < 0)
#endif
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
	rb_define_method(cFFrubyFile, "artist", ffrf_artist, 0);
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
	rb_define_alias(cFFrubyFile, "author", "artist");
}
