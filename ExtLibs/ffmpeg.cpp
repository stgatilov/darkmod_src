#include "ffmpeg.h"

namespace ExtLibs {

	double av_q2d(AVRational a) {
		return ::av_q2d(a);
	}

	const char *av_get_media_type_string( enum AVMediaType media_type ) {
		return ::av_get_media_type_string( media_type );
	}

	void av_malloc_attrib av_alloc_size( 1 ) *av_malloc( size_t size ) {
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

	int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags) {
		return ::av_dict_set_int(pm, key, value, flags);
	}

	void av_dict_free(AVDictionary **m) {
		::av_dict_free(m);
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

	void av_packet_unref(AVPacket *pkt) {
		::av_packet_unref(pkt);
	}

	void av_packet_move_ref(AVPacket *dst, AVPacket *src) {
		::av_packet_move_ref(dst, src);
	}

	AVCodec *avcodec_find_decoder( enum AVCodecID id ) {
		return ::avcodec_find_decoder( id );
	}

	int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, const AVPacket *avpkt) {
		return ::avcodec_decode_video2( avctx, picture, got_picture_ptr, avpkt );
	}

	int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, const AVPacket *avpkt) {
		return ::avcodec_decode_audio4(avctx, frame, got_frame_ptr, avpkt);
	}

	enum AVMediaType avcodec_get_type( enum AVCodecID codec_id ) {
		return ::avcodec_get_type( codec_id );
	}

	const char *avcodec_get_name( enum AVCodecID id ) {
		return ::avcodec_get_name( id );
	}

	AVRational av_codec_get_pkt_timebase (const AVCodecContext *avctx) {
		return ::av_codec_get_pkt_timebase(avctx);
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

	AVAudioFifo *av_audio_fifo_alloc(enum AVSampleFormat sample_fmt, int channels, int nb_samples) {
		return ::av_audio_fifo_alloc(sample_fmt, channels, nb_samples);
	}

	void av_audio_fifo_free(AVAudioFifo *af) {
		return ::av_audio_fifo_free(af);
	}

	int av_audio_fifo_write(AVAudioFifo *af, void **data, int nb_samples) {
		return ::av_audio_fifo_write(af, data, nb_samples);
	}

	int av_audio_fifo_read(AVAudioFifo *af, void **data, int nb_samples) {
		return ::av_audio_fifo_read(af, data, nb_samples);
	}

	int av_audio_fifo_size(AVAudioFifo *af) {
		return ::av_audio_fifo_size(af);
	}

	int av_audio_fifo_drain(AVAudioFifo *af, int nb_samples) {
		return ::av_audio_fifo_drain(af, nb_samples);
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

	struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
		int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
		int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
		int log_offset, void *log_ctx
	) {
		return ::swr_alloc_set_opts(s,
			out_ch_layout, out_sample_fmt, out_sample_rate,
			in_ch_layout, in_sample_fmt, in_sample_rate,
			log_offset, log_ctx
		);
	}

	void swr_free(struct SwrContext **s) {
		return ::swr_free(s);
	}

	int swr_init(struct SwrContext *s) {
		return ::swr_init(s);
	}

	int swr_is_initialized(struct SwrContext *s) {
		return ::swr_is_initialized(s);
	}

	int swr_convert_frame(SwrContext *swr, AVFrame *output, const AVFrame *input) {
		return ::swr_convert_frame(swr, output, input);
	}

	int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count) {
		return ::swr_convert(s, out, out_count, in, in_count);
	}

}