require "bundler/gem_tasks"
require "rake/extensiontask"
require "rdoc/task"

spec = Gem::Specification.load(File.expand_path("../ffruby.gemspec", __FILE__))
Rake::ExtensionTask.new("ffruby", spec)

RDoc::Task.new do |rdoc|
  rdoc.main = "README.rdoc"
  rdoc.rdoc_dir = "rdoc"
  rdoc.title = "FFruby " + File.read(File.expand_path("../VERSION", __FILE__)).chomp
  rdoc.rdoc_files.include *`git ls-files \*.{c,h,rdoc}`.split("\n")
end
