# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name         = "ffruby"
  s.version      = File.read(File.expand_path("../VERSION", __FILE__)).chomp
  s.license      = "LGPL-2.1"
  s.authors      = ["James Le Cuirot"]
  s.email        = "chewi@aura-online.co.uk"
  s.homepage     = "https://github.com/chewi/ffruby"
  s.summary      = "A Ruby interface to FFmpeg's libavformat and libavcodec."
  s.description  = "A Ruby interface to FFmpeg's libavformat and libavcodec. It allows you to query metadata from a wide variety of media files through some simple classes."
  s.requirements = "FFmpeg libraries and also development headers if building."

  s.extensions       = ["ext/ffruby/extconf.rb"]
  s.extra_rdoc_files = `git ls-files \*.{c,h,rdoc}`.split("\n")
  s.files            = `git ls-files`.split("\n")

  s.add_development_dependency "bundler"
  s.add_development_dependency "rake"
  s.add_development_dependency "rake-compiler"
end
