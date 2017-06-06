#include "stdafx.h"
#include "ffmpeg.h"

namespace ExtLibs {

	const char *av_get_media_type_string( enum AVMediaType media_type ) {
		return ::av_get_media_type_string( media_type );
	}

	void *av_malloc( size_t size ) av_malloc_attrib av_alloc_size( 1 ) {
		return ::av_malloc( size );
	}

	void av_free( void *ptr ) {
		::av_free( ptr );
	}

	void av_log_default_callback( void *avcl, int level, const char *fmt,
		va_list vl ) {
		return ::av_log_default_callback( avcl, level, fmt, vl );
	}

	void av_free_packet( AVPacket *pkt ) {
		::av_free_packet( pkt );
	}

	void av_log_set_callback( void( *callback )(void*, int, const char*, va_list) ) {
		::av_log_set_callback( callback );
	}

	int av_dict_set( AVDictionary **pm, const char *key, const char *value, int flags ) {
		return ::av_dict_set( pm, key, value, flags );
	}

	int64_t av_frame_get_best_effort_timestamp( const AVFrame *frame ) {
		return ::av_frame_get_best_effort_timestamp( frame );
	}

	AVFrame *av_frame_alloc( void ) {
		return ::av_frame_alloc();
	}

	void av_frame_free( AVFrame **frame ) {
		return ::av_frame_free( frame );
	}

	int avcodec_open2( AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options ) {
		return  ::avcodec_open2( avctx, codec, options );
	}

	int avcodec_close( AVCodecContext *avctx ) {
		return ::avcodec_close( avctx );
	}

	void av_init_packet( AVPacket *pkt ) {
		return ::av_init_packet( pkt );
	}

	AVCodec *avcodec_find_decoder( enum AVCodecID id ) {
		return ::avcodec_find_decoder( id );
	}

	int avcodec_decode_video2( AVCodecContext *avctx, AVFrame *picture,
		int *got_picture_ptr,
		const AVPacket *avpkt ) {
		return ::avcodec_decode_video2( avctx, picture, got_picture_ptr, avpkt );
	}

	enum AVMediaType avcodec_get_type( enum AVCodecID codec_id ) {
		return ::avcodec_get_type( codec_id );
	}

	const char *avcodec_get_name( enum AVCodecID id ) {
		return ::avcodec_get_name( id );
	}

	AVIOContext *avio_alloc_context(
		unsigned char *buffer,
		int buffer_size,
		int write_flag,
		void *opaque,
		int( *read_packet )(void *opaque, uint8_t *buf, int buf_size),
		int( *write_packet )(void *opaque, uint8_t *buf, int buf_size),
		int64_t( *seek )(void *opaque, int64_t offset, int whence) ) {
		return ::avio_alloc_context( buffer, buffer_size, write_flag, opaque, read_packet, write_packet, seek );
	}

	void av_register_all( void ) {
		::av_register_all();
	}

	AVFormatContext *avformat_alloc_context( void ) {
		return ::avformat_alloc_context();
	}

	int avformat_open_input( AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options ) {
		return ::avformat_open_input( ps, filename, fmt, options );
	}

	int avformat_find_stream_info( AVFormatContext *ic, AVDictionary **options ) {
		return ::avformat_find_stream_info( ic, options );
	}

	int av_find_best_stream( AVFormatContext *ic,
	enum AVMediaType type,
		int wanted_stream_nb,
		int related_stream,
		AVCodec **decoder_ret,
		int flags ) {
		return ::av_find_best_stream( ic, type, wanted_stream_nb, related_stream, decoder_ret, flags );
	}

	int av_read_frame( AVFormatContext *s, AVPacket *pkt ) {
		return ::av_read_frame( s, pkt );
	}

	void avformat_close_input( AVFormatContext **s ) {
		return ::avformat_close_input( s );
	}

	void sws_freeContext( struct SwsContext *swsContext ) {
		return ::sws_freeContext( swsContext );
	}

	struct SwsContext *sws_getContext( int srcW, int srcH, enum AVPixelFormat srcFormat,
		int dstW, int dstH, enum AVPixelFormat dstFormat,
		int flags, SwsFilter *srcFilter,
		SwsFilter *dstFilter, const double *param ) {
		return ::sws_getContext( srcW, srcH, srcFormat,
			dstW, dstH, dstFormat,
			flags, srcFilter,
			dstFilter, param );
	}

	int sws_scale( struct SwsContext *c, const uint8_t *const srcSlice[],
		const int srcStride[], int srcSliceY, int srcSliceH,
		uint8_t *const dst[], const int dstStride[] ) {
		return ::sws_scale( c, srcSlice,
			srcStride, srcSliceY, srcSliceH,
			dst, dstStride );
	}
}