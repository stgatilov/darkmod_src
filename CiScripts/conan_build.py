import os, platform, sys

def execute(cmd):
    print("CMD: " + cmd)
    assert os.system(cmd) == 0

try:
    bitness = sys.argv[1]
except:
    bitness = os.environ['PLATFORM']
assert bitness in ['64', '32']

sysname = platform.system().lower()
if 'windows' in sysname:
    osname = 'windows'
else:
    osname = 'linux'


execute('conan config install global.conf')

os.chdir('../ThirdParty')
execute('python ./1_export_custom.py --unattended')

for config in ['release', 'debug']:
    cmd = 'conan build .'
    cmd += f' -pr:b profiles/base_{osname}'
    cmd += f' -pr profiles/os_{osname}'
    cmd += f' -pr profiles/arch_{bitness}'
    cmd += f' -pr profiles/build_{config}' # note: third-party are built as full Debug
    cmd += f' -of build_{osname}_{bitness}_{config}'
    cmd += ' -b missing'
    cmd += ' -o thedarkmod/*:build_game=True'
    cmd += ' -o thedarkmod/*:build_installer=True'
    cmd += ' -o thedarkmod/*:build_packager=True'
    execute(cmd)

os.chdir('../CiScripts')
