require 'mkmf'

extension_name = 'llvmruby'

dir_config(extension_name)
dir_config('llvm', `llvm-config --includedir`.strip, `llvm-config --libdir`.strip)

have_library('stdc++')
have_library('pthread')

# This is a workaround for a dependency bug in llvm-config. See:
# <https://mail.gna.org/public/etoile-discuss/2008-10/msg00014.html>
# <https://mail.gna.org/public/etoile-discuss/2008-10/msg00015.html>
# <http://svn.gna.org/viewcvs/etoile?rev=3694&view=rev>
components = `llvm-config --libs all`.strip
components = components.split(/\s+/).reject { |lib| lib =~ /LLVMCBase/ }.join(' ')

with_ldflags([`llvm-config --ldflags`.strip, components].join(' ').strip) do
  create_makefile(extension_name)
end
