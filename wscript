# /usr/bin/env python
import re
import Options
import sys, os, shutil, platform
from Utils import cmd_output
from os.path import join, dirname, abspath
from logging import fatal

cwd = os.getcwd()
VERSION="0.5.6"
APPNAME="meguro"

srcdir = '.'
blddir = 'build'

def set_options(opt):
  # the gcc module provides a --debug-level option
  opt.tool_options('compiler_cxx')
  opt.tool_options('compiler_cc')
  opt.tool_options('misc')
  opt.add_option( '--debug'
                , action='store_true'
                , default=False
                , help='Build debug variant [Default: False]'
                , dest='debug'
                )
  opt.add_option( '--enable-gczeal'
                , action='store_true'
                , default=False
                , help='Enable zealous GC in Spidermonkey (useful for finding GC issues)'
                , dest='gczeal'
                )  

def mkdir_p(dir):
  if not os.path.exists (dir):
    os.makedirs (dir)

# Copied from Python 2.6 because 2.4.4 at least is broken by not using
# mkdirs
# http://mail.python.org/pipermail/python-bugs-list/2005-January/027118.html
def copytree(src, dst, symlinks=False, ignore=None):
    names = os.listdir(src)
    if ignore is not None:
        ignored_names = ignore(src, names)
    else:
        ignored_names = set()

    os.makedirs(dst)
    errors = []
    for name in names:
        if name in ignored_names:
            continue
        srcname = join(src, name)
        dstname = join(dst, name)
        try:
            if symlinks and os.path.islink(srcname):
                linkto = os.readlink(srcname)
                os.symlink(linkto, dstname)
            elif os.path.isdir(srcname):
                copytree(srcname, dstname, symlinks, ignore)
            else:
                shutil.copy2(srcname, dstname)
            # XXX What about devices, sockets etc.?
        except (IOError, os.error), why:
            errors.append((srcname, dstname, str(why)))
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except Error, err:
            errors.extend(err.args[0])
    try:
        shutil.copystat(src, dst)
    except OSError, why:
        if WindowsError is not None and isinstance(why, WindowsError):
            # Copying file access times may fail on Windows
            pass
        else:
            errors.extend((src, dst, str(why)))
    if errors:
        raise Error, errors

def configure_autoconf(conf):
  print("----- Configuring Autoconf -----")
  src = join(conf.srcdir,"deps/autoconf-2.13")
  default_tgt = join(conf.blddir, "autoconf")
  if not os.path.exists(default_tgt):
    copytree(src,default_tgt,True)

  if os.system("cd %s && ./configure && make" % (default_tgt)) != 0:
    conf.fatal("Building Autoconf failed.")

  conf.env["AUTOCONF_HOME"] = default_tgt
  conf.env["AUTOCONF_BIN"] = join(default_tgt,"autoconf")

  print(conf.env["AUTOCONF_BIN"])



def configure_nspr(conf):
  print("----- Configuring NSPR -----")
  src = join(conf.srcdir,"deps/nspr")

  default_tgt = join(conf.blddir, "default", "deps/nspr")
  if not os.path.exists(default_tgt):
    copytree(src,default_tgt,True)

  sixtyfour = ""
  if conf.env["IS_64BIT"]:
    sixtyfour = "--enable-64bit"

  if os.system("cd %s && ./configure %s" % (default_tgt,sixtyfour)) != 0:
    conf.fatal("Configuring NSPR failed.")

  if conf.env["USE_DEBUG"]:
    print("----- Configuring Debug NSPR -----")
    # Copy the configured nspr directory to debug
    debug_tgt = join(conf.blddir, "debug", "deps/nspr")
    if not os.path.exists(debug_tgt):
      copytree(src,debug_tgt,True)

    if os.system("cd %s && ./configure --enable-debug %s" % (debug_tgt,sixtyfour)) != 0:
      conf.fatal("Configuring Debug NSPR failed.")

def configure_tokyocabinet(conf):
  print("----- Configuring Tokyo Cabinet -----")
  tc_deps_dir = "deps/tokyocabinet"
  src = join(conf.srcdir,tc_deps_dir)
  default_tgt = join(conf.blddir, "default",tc_deps_dir)
  if not os.path.exists(default_tgt):
    copytree(src,default_tgt,True)

  configure_template = "cd %s && ./configure --disable-shared %s %s %s"

  #enable_static = "--enable-static"
  enable_static = ""
  if sys.platform.startswith('darwin'):
    enable_static = ""
  sixtyfour_offset = ""
  if not conf.env["IS_64BIT"]:
    sixtyfour_offset = "--enable-off64"

  configure_cmd = configure_template % (default_tgt,sixtyfour_offset,enable_static,"")

  if os.system(configure_cmd) != 0:
    conf.fatal("Configuring Tokyo Cabinet failed")

  if conf.env["USE_DEBUG"]:
    print("----- Configuring Debug Tokyo Cabinet -----")
    debug_tgt = join(conf.blddir, "debug", tc_deps_dir)
    if not os.path.exists(debug_tgt):
      copytree(src,debug_tgt,True)


    enable_debug = ""
    # If we are on darwin, can't compile a debuggable tokyo cabinet
    #if not sys.platform.startswith('darwin'):
    #  enable_debug = "--enable-debug"
      
    g_configure_cmd = configure_template % (debug_tgt,sixtyfour_offset,enable_static,enable_debug)
    if os.system(g_configure_cmd) != 0:
      conf.fatal("Configuring Debug Tokyo Cabinet failed")
  

# Can only partial configure NSPR since it depens on nspr's output
def configure_spidermonkey(conf):
  print("------ Configuring Spidermonkey -----")
  src = join(conf.srcdir, "deps/spidermonkey")

  nspr_include_dir = join(conf.blddir, "default", "deps/nspr/dist/include/nspr")
  nspr_lib_dir = join(conf.blddir, "default", "deps/nspr/dist/lib")
  autoconf_template = "cd %s && %s -m %s --macro-dir %s"
  configure_template = "cd %s && ./configure --enable-threadsafe --enable-static --with-nspr-cflags=-I%s --with-nspr-libs='-L%s -lplds4 -lplc4 -lnspr4' %s"

  gczeal = ''
  if conf.env["USE_GCZEAL"]:
    gczeal = '--enable-gczeal'

  default_tgt = join(conf.blddir, "default", "deps/spidermonkey")
  if not os.path.exists(default_tgt):
    copytree(src,default_tgt,True)

  print("------ Autoconfing Spidermonkey -----")
  cmd = autoconf_template % (default_tgt, conf.env.AUTOCONF_BIN, conf.env.AUTOCONF_HOME, conf.env.AUTOCONF_HOME)
  if os.system(cmd) != 0:
    conf.fatal("Autoconfing Spidermonkey failed.")

  if os.system(configure_template % (default_tgt,nspr_include_dir, nspr_lib_dir,gczeal)) != 0:
    conf.fatal("Configuring Spidermonkey failed.")

  if conf.env["USE_DEBUG"]:
    g_nspr_include_dir = join(conf.blddir, "debug", "deps/nspr/dist/include/nspr")
    g_nspr_lib_dir = join(conf.blddir, "debug", "deps/nspr/dist/lib")
    debug_tgt = join(conf.blddir, "debug", "deps/spidermonkey")
    if not os.path.exists(debug_tgt):
      copytree(src,debug_tgt,True)

    print("------ Autoconfing Debug Spidermonkey -----")
    cmd = autoconf_template % (debug_tgt, conf.env.AUTOCONF_BIN, conf.env.AUTOCONF_HOME, conf.env.AUTOCONF_HOME)
    print cmd
    if os.system(cmd) != 0:
      conf.fatal("Autoconfing Spidermonkey failed.")

    print("------ Configuring Debug Spidermonkey ------")

    cmd = (configure_template % (debug_tgt, 
                                 g_nspr_include_dir, 
                                 g_nspr_lib_dir,
                                 gczeal)) + " --enable-debug"
    if os.system(cmd) != 0:
      conf.fatal("Configuring Spidermonkey failed.")

def configure(conf):
  print("#================================================================")
  mode = "release"
  if Options.options.debug:
    mode = "release + debug"
  gc = "normal"
  if Options.options.gczeal:
    gc = "zeal"
  print("# Configuring Meguro Version %s (%s) (GC: %s)" % 
    ( VERSION, mode, gc)
  )
  print("#================================================================")
  conf.check_tool('osx')
  conf.check_tool('compiler_cxx')
  if not conf.env.CXX: conf.fatal('c++ compiler not found')
  conf.check_tool('compiler_cc')
  if not conf.env.CC: conf.fatal('c compiler not found')

  conf.env["USE_DEBUG"] = Options.options.debug
  conf.env["USE_GCZEAL"] = Options.options.gczeal

  if platform.architecture()[0] == '64bit':
    conf.env["IS_64BIT"] = True

  if sys.platform.startswith('darwin'):
    conf.env["FRAMEWORK_CORE"] = ['CoreFoundation','CoreServices']

  conf.check(lib='rt', uselib_store='RT')
  if not conf.check(lib='bz2', uselib_store='BZ2'): 
    conf.fatal('bz2 library not found')

  if not conf.check(lib='z', uselib_store='Z'):
    conf.fatal('z library not found')

  if not conf.check(lib='pthread', uselib_store='PTHREAD'):
    conf.fatal('pthread library not found')

  conf.check(lib='dl', uselib_store='DL')
  if not sys.platform.startswith("sunos") and not sys.platform.startswith("darwin"):
    conf.env.append_value("CCFLAGS", "-rdynamic")
    conf.env.append_value("LINKFLAGS_DL", "-rdynamic")


  #conf.find_program(['autoconf213','autoconf-2.13'], var='AUTOCONF', mandatory=True)

  conf.define("HAVE_CONFIG_H", 1)

  # LFS
  conf.env.append_value('CCFLAGS',  '-D_LARGEFILE_SOURCE')
  conf.env.append_value('CXXFLAGS', '-D_LARGEFILE_SOURCE')
  conf.env.append_value('CCFLAGS',  '-D_FILE_OFFSET_BITS=64')
  conf.env.append_value('CXXFLAGS', '-D_FILE_OFFSET_BITS=64')

  # platform
  platform_def = '-DPLATFORM=' + sys.platform
  conf.env.append_value('CCFLAGS', platform_def)
  conf.env.append_value('CXXFLAGS', platform_def)

  # Split off debug variant before adding variant specific defines
  debug_env = conf.env.copy()
  conf.set_env_name('debug', debug_env)

  # Configure debug variant
  conf.setenv('debug')
  debug_env.set_variant('debug')
  debug_env.append_value('CCFLAGS', ['-DDEBUG', '-g', '-O0', '-Wall', '-Wextra'])
  debug_env.append_value('CXXFLAGS', ['-DDEBUG', '-g', '-O0', '-Wall', '-Wextra'])
  conf.write_config_header("config.h")

  # Configure default variant
  conf.setenv('default')
  conf.env.append_value('CCFLAGS', ['-DNDEBUG', '-O3'])
  conf.env.append_value('CXXFLAGS', ['-DNDEBUG', '-O3'])
  conf.write_config_header("config.h")

  configure_autoconf(conf)
  configure_nspr(conf)
  configure_spidermonkey(conf)
  configure_tokyocabinet(conf)


# We need the NSPR library to have threading support in Spidermonkey
def build_nspr(bld):
  default_env = bld.env_of_name("default")
  default_build_dir = bld.srcnode.abspath(default_env)
  default_dir = join(default_build_dir, "deps/nspr")

  rule_template = "cd %s && make"

  static_lib = "dist/lib/libnspr4.a"
  target = join("deps/nspr", static_lib)

  nspr_bld = bld.new_task_gen(
    name = 'nspr',
    source = bld.path.ant_glob('deps/nspr/include/gencfg.c'),
    target = "deps/nspr/dist",
    rule   = rule_template % default_dir,
    before = 'spidermonkey',
    install_path = None
  )

  nspr_lib_dir = join(default_dir,"dist/lib")
  default_env["NSPR_PREFIX"] = join(default_dir,"dist/")
  default_env["CPPPATH_NSPR"] = join(default_dir,"dist/include/nspr")
  default_env["INCLUDE_PATH_NSPR"] = join(default_dir,"dist/include/nspr")
  default_env["LIB_PATH_NSPR"] = join(default_dir,"dist/lib")
  default_env["LINKFLAGS_NSPR"] = [join(nspr_lib_dir,"libnspr4.a"),join(nspr_lib_dir,"libplc4.a"), join(nspr_lib_dir,"libplds4.a")]

  if bld.env["USE_DEBUG"]:
    debug_env = bld.env_of_name("debug")
    debug_build_dir = bld.srcnode.abspath(debug_env)
    debug_dir = join(debug_build_dir, "deps/nspr")
    nspr_g = nspr_bld.clone('debug')
    nspr_g.rule = rule_template % debug_dir
    debug_nspr_lib_dir = join(debug_dir,"dist/lib")
    debug_env["NSPR_PREFIX"] = join(debug_dir,"dist/")
    debug_env["CPPPATH_NSPR"] = join(debug_dir,"dist/include/nspr")
    debug_env["INCLUDE_PATH_NSPR"] = join(debug_dir,"dist/include/nspr")
    debug_env["LIB_PATH_NSPR"] = join(debug_dir,"dist/lib")
    debug_env["LINKFLAGS_NSPR"] = [join(debug_nspr_lib_dir,"libnspr4.a"),join(debug_nspr_lib_dir,"libplc4.a"), join(debug_nspr_lib_dir,"libplds4.a")]

def build_spidermonkey(bld):
  default_env = bld.env_of_name("default")
  default_build_dir = bld.srcnode.abspath(default_env)
  default_dir = join(default_build_dir, "deps/spidermonkey")


  #rule_template = "cd %s && ./configure --enable-threadsafe --enable-static --with-nspr-cflags=-I%s --with-nspr-libs='-L%s -lplds4 -lplc4 -lnspr4' %s && make"
  rule_template = "cd %s && make"

  #static_lib = bld.env["staticlib_PATTERN"] % "spidermonkey"
  static_lib = "libjs_static.a"
  target = join("deps/spidermonkey",static_lib)

  sm = bld.new_task_gen(
    name = 'spidermonkey',
    source = bld.path.ant_glob('deps/spidermonkey/prmjtime.cpp'),
    target = target,
    rule   = rule_template % (default_dir),
    before = "cxx",
    install_path = None
  )

  bld.env_of_name('default')["CPPPATH_SPIDERMONKEY"] = default_dir
  bld.env_of_name('default')["LINKFLAGS_SPIDERMONKEY"] = join(default_dir,static_lib)
  bld.env_of_name('default')["SPIDERMONKEY_TARGET"] = join(default_dir,static_lib)

  if bld.env["USE_DEBUG"]:
    debug_env = bld.env_of_name("debug")
    debug_build_dir = bld.srcnode.abspath(debug_env)
    debug_dir = join(debug_build_dir, "deps/spidermonkey")
    sm_g = sm.clone('debug')
    sm_g.rule = rule_template % (debug_dir)
    bld.env_of_name('debug')["CPPPATH_SPIDERMONKEY"] = debug_dir
    bld.env_of_name('debug')["LINKFLAGS_SPIDERMONKEY"] = join(debug_dir,static_lib)
    bld.env_of_name('debug')["SPIDERMONKEY_TARGET"] = join(debug_dir,static_lib)

def build_tokyocabinet(bld):
  tc_deps_dir = "deps/tokyocabinet"
  default_env = bld.env_of_name("default")
  default_build_dir = bld.srcnode.abspath(default_env)
  default_dir = join(default_build_dir, tc_deps_dir)

  rule_template = "cd %s && make"
  static_lib = "libtokyocabinet.a"
  target = join(tc_deps_dir,static_lib)

  tc = bld(
    name = 'tokyocabinet',
    source = bld.path.ant_glob(join(tc_deps_dir),"tcadb.c"),
    target = target,
    rule = rule_template % (default_dir),
    before = "cxx",
    install_path = None
  )

  default_env["CPPPATH_TOKYOCABINET"] = default_dir
  default_env["LINKFLAGS_TOKYOCABINET"] = join(default_dir,static_lib)

  if bld.env["USE_DEBUG"]:
    debug_env = bld.env_of_name("debug")
    debug_build_dir = bld.srcnode.abspath(debug_env)
    debug_dir = join(debug_build_dir, tc_deps_dir)
    tc_g = tc.clone('debug')
    tc_g.rule = rule_template % (debug_dir)
    debug_env["CPPPATH_TOKYOCABINET"] = debug_dir
    debug_env["LINKFLAGS_TOKYOCABINET"] = join(debug_dir,static_lib)



def build_meguro(bld):
  meguro = bld.new_task_gen("cxx", "program")
  meguro.name = "meguro"
  meguro.target = "meguro"
  meguro.source = """
    src/base64.cpp
    src/line_iterator.cpp
    src/tokyo_cabinet_iterator.cpp
    src/tokyo_cabinet_hash_iterator.cpp
    src/progress.cpp
    src/shadow_key_map.cpp
    src/mapper.cpp
    src/meguro.cpp
    src/reducer.cpp
    src/thread_safe_queue.cpp
    src/tracemonkey_js_handle.cpp
    src/dictionary.cpp
    src/mgutil.cpp
  """
  meguro.includes = """
    src/
    deps/spidermonkey/
  """

  if sys.platform.startswith('darwin'):
    meguro.uselib = 'DL PTHREAD TOKYOCABINET SPIDERMONKEY NSPR CORE BZ2 Z'
  else:
    meguro.uselib = 'DL PTHREAD TOKYOCABINET SPIDERMONKEY NSPR BZ2 Z'

  meguro.install_path = '${PREFIX}/bin'
  meguro.chmod = 0755

  def versioning(meguro):
    if os.path.exists(join(cwd, ".git")):
      actual_version = cmd_output("git describe").strip()
    else:
      actual_version = VERSION
    x = { 'CCFLAGS'   : " ".join(meguro.env["CCFLAGS"])
        , 'CPPFLAGS'  : " ".join(meguro.env["CPPFLAGS"])
        , 'LIBFLAGS'  : " ".join(meguro.env["LIBFLAGS"])
        , 'VERSION'   : actual_version
        , 'PREFIX'    : meguro.env["PREFIX"]
        }
    return x

  meguro_version = bld(
      features='subst',
      before="cc",
      source="src/meguro_version.h.in",
      target = 'src/meguro_version.h',
      dict = versioning(meguro),
      install_path = None
  )

  if bld.env["USE_DEBUG"]:
    meguro_g = meguro.clone("debug")
    meguro_g.target = "meguro_g"
    meguro_version_g= meguro_version.clone("debug")
    meguro_version_g.dict = versioning(meguro_g)
    meguro_version_g.install_path = None
    
def build(bld):

  build_nspr(bld)
  build_spidermonkey(bld)
  build_tokyocabinet(bld)
  build_meguro(bld)

def shutdown():
  Options.options.debug
  # HACK from node.js
  if not Options.commands['clean']:
    if os.path.exists('build/default/meguro') and not os.path.exists('meguro'):
      os.symlink('build/default/meguro', 'meguro')
    if os.path.exists('build/debug/meguro_g') and not os.path.exists('meguro_g'):
      os.symlink('build/debug/meguro_g', 'meguro_g')
  else:
    if os.path.exists('meguro'): os.unlink('meguro')
    if os.path.exists('meguro_g'): os.unlink('meguro_g')
