scons -c
scons BUILD="release" MACOSX_TARGET_ARCH="i386"
cp tdmlauncher.macosx tdmlauncher.i386.macosx
scons BUILD="release" MACOSX_TARGET_ARCH="ppc"
cp tdmlauncher.macosx tdmlauncher.ppc.macosx
lipo -arch i386 tdmlauncher.i386.macosx -arch ppc tdmlauncher.ppc.macosx -create -output tdmlauncher.macosx
