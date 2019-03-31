import os, platform

def build_arch(compiler, arch, *, libcxx = None, runtime = None, buildtype = 'Release'):
    cmd = 'conan install . -s compiler="%s" -s arch="%s"' % (compiler, arch)
    if libcxx is not None:
        cmd += ' -s compiler.libcxx="%s"' % libcxx
    if runtime is not None:
        cmd += ' -s compiler.runtime=%s' % runtime
    if buildtype is not None:
        cmd += ' -s build_type=%s' % buildtype
    cmd += ' --build outdated'
    print("CMD: %s" % cmd)
    os.system(cmd)

assert platform.machine().endswith('64'), "Use 64-bit OS for builds"

sysname = platform.system().lower()
if 'windows' in sysname:
    build_arch('Visual Studio', 'x86_64', runtime='MT') #, buildtype='RelWithDebInfo')
    build_arch('Visual Studio', 'x86', runtime='MT') #, buildtype='RelWithDebInfo')
else:
    build_arch('gcc', 'x86_64', libcxx='libstdc++')
    build_arch('gcc', 'x86', libcxx='libstdc++')
