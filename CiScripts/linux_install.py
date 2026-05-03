import os, platform, sys

def execute(cmd):
    print("CMD: " + cmd)
    os.system(cmd)  # errors ignored


sysname = platform.system().lower()
if 'windows' not in sysname:
    # this is enough for 64 bit 
    execute('sudo apt-get update')
    execute('sudo apt-get -y install xorg-dev libwayland-dev libxkbcommon-dev')
    execute('sudo apt-get -y install mesa-common-dev libglu1-mesa-dev')
    execute('sudo apt-get -y install libxcb-*-dev libx11-xcb-dev libxxf86vm-dev libxext-dev uuid-dev')
