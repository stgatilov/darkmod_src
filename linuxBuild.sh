#!/bin/bash
mkdir -p ~/.doom3/darkmod
#scons -j2 BUILD_GAMEPAK=1 && cp game01-base.pk4 ~/.doom3/darkmod/
scons BUILD_GAMEPAK=1 && cp game01-base.pk4 ~/.doom3/darkmod/

