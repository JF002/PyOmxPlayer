from distutils.core import setup, Extension

omxplayermodule = Extension('omxplayer', 
          define_macros = [('TARGET_RASPBERRY_PI', '1'),
                           ('__VIDEOCORE4__','1'),
                           ('GRAPHICS_X_VG','1'),
                           ('HAVE_OMXLIB','1'),
                           ('USE_EXTERNAL_FFMPEG','1'),
                           ('USE_EXTERNAL_OMX','1'),
                           ('HAVE_LIBAVCODEC_AVCODEC_H','1'),
                           ('HAVE_LIBAVUTIL_MEM_H','1'),
                           ('HAVE_LIBAVUTIL_OPT_H','1'),
                           ('HAVE_LIBAVUTIL_AVUTIL_H','1'),
                           ('HAVE_LIBAVFORMAT_AVFORMAT_H','1'),
                           ('HAVE_LIBAVFILTER_AVFILTER_H','1'),
                           ('OMX','1'),
                           ('OMX_SKIP64BIT','1'),
                           ('USE_EXTERNAL_LIBBCM_HOST','1'),
                           ('__STDC_CONSTANT_MACROS','1'),
                           ('STANDALONE','1'),
                           ('__STDC_LIMIT_MACROS','1'),
                           ('_LINUX','1'),
                           ('_REENTRANT','1'),
                           ('_LARGEFILE64_SOURCE','1'),
                           ],
					sources = ['omxplayermodule.cpp'],
					include_dirs = ['/home/pi/omxplayer/ffmpeg_compiled/usr/local/include',
                          '/home/pi/omxplayer',
                          '/usr/local/include'],
					library_dirs = ['/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib',
                          '/usr/local/lib',
                          '/opt/vc/lib'],
          extra_objects = ['/home/pi/omxplayer/linux/RBP.o',
                          '/home/pi/omxplayer/DynamicDll.o',
                          '/opt/vc/lib/libbcm_host.so',
                          '/home/pi/omxplayer/utils/log.o',
                          '/home/pi/omxplayer/OMXCore.o',
                          '/home/pi/omxplayer/File.o',
                          '/home/pi/omxplayer/OMXStreamInfo.o',
                          '/home/pi/omxplayer/OMXPlayerAudio.o',
                          '/home/pi/omxplayer/OMXThread.o',
                          '/home/pi/omxplayer/OMXAudio.o',
                          '/home/pi/omxplayer/OMXAudioCodecOMX.o',
                          '/opt/vc/lib/libopenmaxil.so',
                          '/home/pi/omxplayer/OMXClock.o',
                          '/home/pi/omxplayer/OMXReader.o',
                          '/home/pi/omxplayer/utils/PCMRemap.o',
                          '/home/pi/omxplayer/linux/XMemUtils.o',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libavcodec.so',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libavformat.so',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libavutil.so',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libavdevice.so',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libpostproc.so',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libswresample.so',
                          '/home/pi/omxplayer/ffmpeg_compiled/usr/local/lib/libswscale.so'])    

setup(name = 'omxplayer', version = '0.1', description = 'OMXPlayer', ext_modules = [omxplayermodule])