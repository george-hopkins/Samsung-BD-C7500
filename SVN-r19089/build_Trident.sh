chmod -R 755 $OPENSOURCE_DIR/OP_Product/Codec/FFMpeg/Src/SVN-r19089
make distclean;

./configure  --prefix=$OPENSOURCE_DIR/OP_Product/Codec/FFMpeg/Src/SVN-r19089   --cc=mips24ke_nfp_be-gcc   --cross-prefix=mips24ke_nfp_be-   --enable-cross-compile   --disable-ffmpeg --disable-ffplay --disable-ffserver --enable-pthreads  --enable-mpegaudio-hp  --disable-memalign-hack  --enable-optimizations  --enable-stripping  --enable-hardcoded-tables  --target-os=linux --extra-cflags=-Wall --extra-cflags=-O2 --arch=mips --disable-debug    --disable-small   --disable-gprof  --extra-ldflags=-lm   --enable-fastdiv  --disable-altivec  --disable-extra-warnings   --disable-zlib   --extra-libs=-lmms   --enable-protocol=mms  --extra-cflags=-DMMS_PROTO  --extra-cflags=-I$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/MMS/Inc     --extra-ldflags=-L$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/MMS/Lib/TRIDENT     --extra-libs=-lglib-2.0  --enable-protocol=dlna  --extra-cflags=-DDLNA_PROTO  --extra-cflags=-I$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/DLNA/Inc    --extra-ldflags=-L$ROSE_DIR/AP_RoseSS/PlaySystem/Protocol/DLNA/Lib/TRIDENT   --enable-shared   --extra-cflags=-DUniplayer_DRM  --extra-cflags=-DNEW_JPG_PROCESSOR  --enable-protocol=https    --extra-libs=-lssl  --extra-libs=-lcrypto    --extra-ldflags=-L$OPENSOURCE_DIR/OP_Product/Protocol/OPENSSL/LIB/TRIDENT    --extra-cflags=-I$OPENSOURCE_DIR/OP_Product/Protocol/OPENSSL/openssl-0.9.8k/include  --enable-protocol=vudu  --extra-cflags=-DVUDU --enable-protocol=wvine  --extra-cflags=-DWIDEVINE --extra-cflags=-DWMDRM --extra-cflags=-DSIMPLE_TRY_DECODE_IN_PARSER  --extra-cflags=-D__TRIDENT_SPECIFIC



make clean;
make all;
make install;

cp -vf ./lib/* ./Release/TRIDENT/


