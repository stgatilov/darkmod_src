scons -c
scons BUILD="release" TARGET_ARCH="x86"
cp tdm_update.macosx tdm_update.i386.macosx
scons BUILD="release" TARGET_ARCH="ppc"
cp tdm_update.macosx tdm_update.ppc.macosx
lipo -arch i386 tdm_update.i386.macosx -arch ppc tdm_update.ppc.macosx -create -output tdm_update.macosx
