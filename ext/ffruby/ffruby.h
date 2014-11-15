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

#ifndef _FFRUBY_H_
#define _FFRUBY_H_

#include <ruby.h>
#include AVUTIL_H_PATH
#include AVFORMAT_H_PATH
#include AVCODEC_H_PATH

extern VALUE mFFruby;
extern VALUE cFFrubyFile;
extern VALUE cFFrubyStream;
extern VALUE cFFrubyVideoStream;
extern VALUE cFFrubyAudioStream;

#endif
