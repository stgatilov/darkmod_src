import os, platform, sys

def execute(cmd):
    print("CMD: " + cmd)
    assert os.system(cmd) == 0


sysname = platform.system().lower()
if 'windows' not in sysname:
    # this is enough for 64 bit 
    os.system('sudo apt-get update')
    os.system('sudo apt-get -y install mesa-common-dev libglu1-mesa-dev xorg-dev libxcb-*-dev libx11-xcb-dev libxxf86vm-dev libxext-dev uuid-dev')
    # workaround for FLTK build broken due to: https://github.com/appveyor/ci/issues/3842
    os.system('sudo rm -f /usr/local/bin/doxygen')
