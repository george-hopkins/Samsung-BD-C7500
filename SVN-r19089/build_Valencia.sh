chmod -R 755 $OPENSOURCE_DIR/OP_Product/Codec/FFMpeg/Src/SVN-r19089
make distclean;

./configure --prefix=$OPENSOURCE_DIR/OP_Product/Codec/FFMpeg/Src/SVN-r19089    --cross-prefix=arm_v7_vfp_le-  --cc=arm_v7_vfp_le-gcc      --enable-armvfp  --arch=arm    --enable-pthreads     --enable-cross-compile  --target-os=linux   --extra-cflags=-Wall --extra-cflags=-O3 --disable-ffserver        --disable-hardcoded-tables  --disable-extra-warnings --disable-mmx2    --disable-ssse3  --disable-mmx   --disable-iwmmxt       --disable-debug    --enable-stripping   --disable-small   --disable-gprof   --enable-optimizations   --disable-memalign-hack     --disable-altivec  --enable-mpegaudio-hp    --enable-network --disable-ffmpeg  --disable-ffplay  --disable-zlib    --enable-muxers   --enable-encoders    --enable-protocols      --enable-decoders        --enable-demuxers      --enable-parsers   --extra-libs=-lmms   --enable-protocol=mms  --extra-cflags=-DMMS_PROTO  --extra-cflags=-I$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/MMS/Inc     --extra-ldflags=-L$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/MMS/Lib/VALENCIA    --extra-libs=-lglib-2.0         --enable-protocol=dlna  --extra-cflags=-DDLNA_PROTO  --extra-cflags=-I$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/DLNA/Inc   --extra-ldflags=-L$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/DLNA/Lib/VALENCIA   --enable-shared  --extra-cflags=-fPIC  --extra-cflags=-DUniplayer_DRM  --extra-cflags=-DNEW_JPG_PROCESSOR   --enable-protocol=https    --extra-libs=-lssl  --extra-libs=-lcrypto    --extra-ldflags=-L$OPENSOURCE_DIR/OP_Product/Protocol/OPENSSL/LIB/VALENCIA    --extra-cflags=-I$OPENSOURCE_DIR/OP_Product/Protocol/OPENSSL/openssl-0.9.8k/include --enable-protocol=wvine  --extra-cflags=-DWIDEVINE   --enable-protocol=vudu  --extra-cflags=-DVUDU --extra-cflags=-DWMDRM --extra-cflags=-DSIMPLE_TRY_DECODE_IN_PARSER




make clean;
make all;
make install;

cp -vf ./lib/* ./Release/VALENCIA/


