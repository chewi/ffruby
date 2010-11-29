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

extern void Init_ffrf();
extern void Init_ffrs();

VALUE mFFruby;

/* Returns an array of the input and output formats supported by
 * FFmpeg. This method will hopefully be expanded to return more than
 * just names in the future. */
static VALUE ffruby_formats(VALUE self, VALUE klass)
{
	AVInputFormat *ifmt;
	AVOutputFormat *ofmt;
	VALUE array = rb_ary_new();

#ifdef HAVE_AV_IFORMAT_NEXT
	for (ifmt = av_iformat_next(NULL); ifmt; ifmt = av_iformat_next(ifmt))
#else
	for (ifmt = first_iformat; ifmt; ifmt = ifmt->next)
#endif
		rb_ary_push(array, rb_str_new2(ifmt->name));

#ifdef HAVE_AV_OFORMAT_NEXT
	for (ofmt = av_oformat_next(NULL); ofmt; ofmt = av_oformat_next(ofmt))
#else
	for (ofmt = first_oformat; ofmt; ofmt = ofmt->next)
#endif
		rb_ary_push(array, rb_str_new2(ofmt->name));

	return rb_funcall(rb_funcall(array, rb_intern("uniq"), 0), rb_intern("sort"), 0);
}

/* Returns an array of the codecs supported by FFmpeg. This method
 * will hopefully be expanded to return more than just names in the
 * future. */
static VALUE ffruby_codecs(VALUE self, VALUE klass)
{
	AVCodec *codec;
	VALUE array = rb_ary_new();

#ifdef HAVE_AV_CODEC_NEXT
	for (codec = av_codec_next(NULL); codec; codec = av_codec_next(codec))
#else
	for (codec = first_avcodec; codec; codec = codec->next)
#endif
		rb_ary_push(array, rb_str_new2(codec->name));

	return rb_funcall(rb_funcall(array, rb_intern("uniq"), 0), rb_intern("sort"), 0);
}

/* Top level module for the FFruby classes. Provides access to
 * information about FFmpeg itself. */
void Init_ffruby()
{
	av_register_all();

	mFFruby = rb_define_module("FFruby");
	rb_define_module_function(mFFruby, "formats", ffruby_formats, 0);
	rb_define_module_function(mFFruby, "codecs", ffruby_codecs, 0);

	Init_ffrf();
	Init_ffrs();
}
