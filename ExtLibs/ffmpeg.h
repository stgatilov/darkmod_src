#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
}

#ifdef MAKEDLL
#  define EXTLIB __declspec(dllexport)
#else
#  define EXTLIB __declspec(dllimport)
#endif

namespace ExtLibs {
	EXTLIB const char *av_get_media_type_string( enum AVMediaType media_type );
	EXTLIB void *av_malloc( size_t size ) av_malloc_attrib av_alloc_size( 1 );
	EXTLIB void av_free( void *ptr );
	EXTLIB void av_free_packet( AVPacket *pkt );
	EXTLIB void av_log_set_callback( void( *callback )(void*, int, const char*, va_list) );
	EXTLIB void av_log_default_callback( void *avcl, int level, const char *fmt, va_list vl );
	EXTLIB int av_dict_set( AVDictionary **pm, const char *key, const char *value, int flags );
	EXTLIB int64_t av_frame_get_best_effort_timestamp( const AVFrame *frame );
	EXTLIB AVFrame *av_frame_alloc( void );
	EXTLIB void av_frame_free( AVFrame **frame );
	EXTLIB int avcodec_open2( AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options );
	EXTLIB int avcodec_close( AVCodecContext *avctx );
	EXTLIB void av_init_packet( AVPacket *pkt );
	EXTLIB AVCodec *avcodec_find_decoder( enum AVCodecID id );
	EXTLIB int avcodec_decode_video2( AVCodecContext *avctx, AVFrame *picture,
		int *got_picture_ptr,
		const AVPacket *avpkt );
	EXTLIB enum AVMediaType avcodec_get_type( enum AVCodecID codec_id );
	EXTLIB const char *avcodec_get_name( enum AVCodecID id );
	EXTLIB AVIOContext *avio_alloc_context(
		unsigned char *buffer,
		int buffer_size,
		int write_flag,
		void *opaque,
		int( *read_packet )(void *opaque, uint8_t *buf, int buf_size),
		int( *write_packet )(void *opaque, uint8_t *buf, int buf_size),
		int64_t( *seek )(void *opaque, int64_t offset, int whence) );
	EXTLIB void av_register_all( void );
	EXTLIB AVFormatContext *avformat_alloc_context( void );
	EXTLIB int avformat_open_input( AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options );
	EXTLIB int avformat_find_stream_info( AVFormatContext *ic, AVDictionary **options );
	EXTLIB int av_find_best_stream( AVFormatContext *ic,
		enum AVMediaType type,
		int wanted_stream_nb,
		int related_stream,
		AVCodec **decoder_ret,
		int flags );
	EXTLIB int av_read_frame( AVFormatContext *s, AVPacket *pkt );
	EXTLIB void avformat_close_input( AVFormatContext **s );
	EXTLIB void sws_freeContext( struct SwsContext *swsContext );
	EXTLIB struct SwsContext *sws_getContext( int srcW, int srcH, enum AVPixelFormat srcFormat,
		int dstW, int dstH, enum AVPixelFormat dstFormat,
		int flags, SwsFilter *srcFilter,
		SwsFilter *dstFilter, const double *param );
	EXTLIB int sws_scale( struct SwsContext *c, const uint8_t *const srcSlice[],
		const int srcStride[], int srcSliceY, int srcSliceH,
		uint8_t *const dst[], const int dstStride[] );
}