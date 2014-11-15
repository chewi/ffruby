#!/usr/bin/env ruby

require "mkmf"
$DEFLIBPATH = []

dir_config("ffmpeg")
libs = %w( avutil avformat avcodec )
headers = []

libs.each do |lib|
  have_library(lib)
end

libs.each do |lib|
  header = "#{lib}.h"
  result = false

  [ "lib#{lib}", "ffmpeg" ].each do |dir|
    path = File.join(dir, header)

    if checking_for(path) { try_cpp(cpp_include(path)) }
      headers << result = path
      break
    end
  end

  $defs.push "-D#{lib.upcase}_H_PATH=\"<#{result || header}>\""
end

%w( iformat oformat codec ).each do |func|
  have_func("av_#{func}_next", headers)
end

%w( av_dict_get avformat_open_input avformat_close_input avformat_find_stream_info avcodec_open2 ).each do |func|
  have_func(func, headers)
end

have_const("AVMEDIA_TYPE_UNKNOWN", headers)
create_makefile("ffruby")
