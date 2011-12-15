clear;
echo ""
echo ">>> Build FFmpeg for SystemTD.."
echo ">>> Disabled protocol : MMS, HTTPS, DLNA, VUDU, WVINE"
echo ">>> Disabled Divx-DRM"
echo ">>> Enabled JPEG for Uniplayer"
echo ""
echo ""
chmod -R 755 $OPENSOURCE_DIR/OP_Product/Codec/FFMpeg/Src/SVN-r19089
make distclean;


./configure --prefix=$OPENSOURCE_DIR/OP_Product/Codec/FFMpeg/Src/SVN-r19089  --cross-prefix=arm_v7_vfp_le-  --cc=arm_v7_vfp_le-gcc      --enable-armvfp  --arch=arm    --enable-pthreads     --enable-cross-compile  --target-os=linux   --extra-cflags=-Wall --extra-cflags=-O3 --disable-ffserver        --disable-extra-warnings --disable-mmx2    --disable-ssse3  --disable-mmx   --disable-iwmmxt       --disable-debug    --enable-stripping   --disable-small   --disable-gprof   --enable-optimizations   --disable-memalign-hack     --disable-altivec  --enable-mpegaudio-hp    --enable-network --disable-ffmpeg  --disable-ffplay  --disable-zlib    --enable-muxers   --enable-encoders    --enable-protocols      --enable-decoders        --enable-demuxers      --enable-parsers    --extra-cflags=-DNEW_JPG_PROCESSOR   --enable-fastdiv   --enable-hardcoded-tables  --disable-protocol=mms --disable-protocol=dlna --disable-protocol=vudu --disable-protocol=https  --disable-protocol=wvine


make clean;
make all;
make install;

