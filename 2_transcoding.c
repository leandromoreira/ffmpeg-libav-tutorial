// -codecs
// decoder = avcodec_find_decoder_by_name("h264_qsv");
// find_codec_or_die
//  \/
// codec = encoder ?
// 680         avcodec_find_encoder_by_name(name) :
// 681         avcodec_find_decoder_by_name(name);
// ffmpeg -hide_banner -codecs | grep vp9
// ffmpeg -hide_banner -codecs | grep opus
// https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html#a8
// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169
// https://ffmpeg.org/doxygen/trunk/transcoding_8c-example.html

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

typedef struct TrancodeContext {
  char *file_name;
  AVFormatContext *format_context;

  int audio_stream_index;
  AVStream *audio_stream;
  AVCodecContext *audio_codec;
  AVCodecContext *audio_codec_context;
  AVCodecParameters *audio_codec_parameters;

  int video_stream_index;
  AVStream *video_stream;
  AVCodecContext *video_codec;
  AVCodecContext *video_codec_context;
  AVCodecParameters *video_codec_parameters;
} TranscodeContext;

static void logging(const char *fmt, ...);
static int decode_packet(AVPacket *i_packet, AVCodecContext *decoder_codec_context, AVFrame *i_frame);
static int encode_frame(AVFormatContext *o_format_context, AVCodecContext *encoder_codec_context, AVFrame *frame, int stream_index);
static int prepare_input(char *input_file_name, AVFormatContext *format_context);
static int prepare_output(AVFormatContext *i_format_context, AVFormatContext *o_format_context, TranscodeContext *transcode_context);

TranscodeContext *transcode_context = NULL;

int main(const int argc, const char *argv[])
{
  if (argc < 3) {
    logging("Usage: %s <input file name> <output filename>", argv[0]);
    return -1;
  }

  const TranscodeContext *input_transcode = malloc(sizeof(TranscodeContext));
  const TranscodeContext *output_transcode = malloc(sizeof(TranscodeContext));

  const char *input_file_name = argv[1];
  const char *output_file_name = argv[2];

  av_register_all();

  const AVFormatContext *i_format_context = avformat_alloc_context();
  if (!i_format_context) {
    logging("could not allocate memory for Format Context");
    return -1;
  }

  if (prepare_input(input_file_name, i_format_context) != 0) {
    logging("error on prepare input");
    return -1;
  }

  const AVFormatContext *o_format_context = NULL;
  avformat_alloc_output_context2(&o_format_context, NULL, NULL, output_file_name);

  if (!o_format_context) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }


  logging("prepare_output");
  if (prepare_output(i_format_context, o_format_context, transcode_context) != 0) {
    logging("error on prepare output");
    return -1;
  }

  if (avio_open(&o_format_context->pb, output_file_name, AVIO_FLAG_WRITE) < 0) {
    logging("could not open output file");
    return -1;
  }

  /* init muxer, write output file header */
  if (avformat_write_header(o_format_context, NULL) < 0) {
    logging("error while writing the header of the output");
    return -1;
  }


  // decode input to encode output
  AVFrame *i_frame = av_frame_alloc();
  if (!i_frame)
  {
    logging("failed to allocated memory for AVFrame");
    return -1;
  }
  AVPacket *i_packet = av_packet_alloc();
  if (!i_packet)
  {
    logging("failed to allocated memory for AVPacket");
    return -1;
  }

  int response = 0;
  int how_many_packets_to_process = 8;

  logging("looping");
  while (av_read_frame(i_format_context, i_packet) >= 0)
  {
    logging("AVPacket->pts %" PRId64, i_packet->pts);
    response = decode_packet(i_packet, transcode_context[i_packet->stream_index].decoder_codec_context, i_frame);
    if (response < 0)
      break;

    encode_frame(
        o_format_context,
        transcode_context[i_packet->stream_index].encoder_codec_context,
        i_frame,
        i_packet->stream_index
    );

    if (--how_many_packets_to_process <= 0) break;

    av_frame_unref(i_frame);
    av_packet_unref(i_packet);
  }

   /* flush the encoder */
    //encode(c, NULL, pkt, f);

  return 0;
}

static int encode_frame(AVFormatContext *o_format_context, AVCodecContext *encoder_codec_context, AVFrame *frame, int stream_index)
{
  AVPacket *o_packet = av_packet_alloc();
  if (!o_packet) {
    logging("could not allocate memory for output packet");
    return -1;
  }

  int ret;
  /* send the frame to the encoder */
  if (frame)
    logging("Send frame %3"PRId64"", frame->pts);
  ret = avcodec_send_frame(encoder_codec_context, frame);
  if (ret < 0) {
    logging("Error sending a frame for encoding\n");
    return -1;
  }
  while (ret >= 0) {
    ret = avcodec_receive_packet(encoder_codec_context, o_packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      return -1;
    else if (ret < 0) {
      fprintf(stderr, "Error during encoding\n");
      exit(1);
    }

    /* prepare packet for muxing */
    o_packet->stream_index = stream_index;
    //av_packet_rescale_ts(&enc_pkt,
    //    stream_ctx[stream_index].encoder_codec_context->time_base,
    //    ofmt_ctx->streams[stream_index]->time_base);
    //av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
    /* mux encoded frame */
    ret = av_interleaved_write_frame(o_format_context, o_packet);

    logging("Write packet %3"PRId64" (size=%5d)", o_packet->pts, o_packet->size);
    av_packet_unref(o_packet);
  }
  av_packet_free(&o_packet);
  return 0;
}

static void logging(const char *fmt, ...)
{
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

static int decode_packet(AVPacket *i_packet, AVCodecContext *decoder_codec_context, AVFrame *i_frame)
{
  int response = avcodec_send_packet(decoder_codec_context, i_packet);

  if (response < 0) {
    logging("Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  while (response >= 0)
  {
    response = avcodec_receive_frame(decoder_codec_context, i_frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }
  }
  return 0;
}

static int prepare_output(AVFormatContext *i_format_context, AVFormatContext *o_format_context, TranscodeContext *transcode_context)
{
  for (int i = 0; i < i_format_context->nb_streams; i++)
  {
    AVCodec *encoder = NULL;
    AVCodecContext *decoder_context = transcode_context[i].decoder_codec_context;

    AVCodecContext *encoder_context = NULL;

    AVStream *out_stream = avformat_new_stream(o_format_context, NULL);

    if (!out_stream) {
      logging("it could not create a new stream for the output");
      return -1;
    }

    if (decoder_context->codec_type == AVMEDIA_TYPE_VIDEO) {
      // let's simulate we'll use a different encoder solely for our video stream
      encoder = avcodec_find_encoder_by_name("libx264");
    } else {
      // for audio we'll use the same
      encoder = avcodec_find_encoder(decoder_context->codec_id);
    }

    if (!encoder) {
      logging("we couldn't find the decoder");
      return -1;
    }

    encoder_context = avcodec_alloc_context3(encoder);
    if (!encoder_context)
    {
      logging("failed to allocated memory for AVCodecContext");
      return -1;
    }

    // do we need this?
    if (avcodec_parameters_to_context(encoder_context, i_format_context->streams[i]->codecpar) < 0)
    {
      logging("failed to copy codec params to codec context");
      return -1;
    }


    // if it's our video stream we'll setup some specifics
    if (decoder_context->codec_type == AVMEDIA_TYPE_VIDEO) {

      // we can override the encoder context ie: encoder_context->gop_size ...
      // and we can also set the codec params
      // https://en.wikibooks.org/wiki/MeGUI/x264_Settings
      //
      // This is -x264-params keyint=60:min-keyint=60:no-scenecut=1 command line part
      av_opt_set(encoder_context->priv_data, "preset", "superfast", 0);
      av_opt_set(encoder_context->priv_data, "keyint", "60", 0);
      av_opt_set(encoder_context->priv_data, "min-keyint", "60", 0);
      av_opt_set(encoder_context->priv_data, "no-scenecut", "1", 0);

      // for some reason we need to manually inform these options
      encoder_context->time_base = av_inv_q(decoder_context->framerate);
      if (encoder->pix_fmts)
        encoder_context->pix_fmt = encoder->pix_fmts[0];
      else
        encoder_context->pix_fmt = decoder_context->pix_fmt;
    }

    logging("encoder time_base %d/%d", encoder_context->time_base.num, encoder_context->time_base.den);
    if (avcodec_open2(encoder_context, encoder, NULL) < 0)
    {
      logging("failed to open codec through avcodec_open2");
      return -1;
    }

    if (avcodec_parameters_from_context(out_stream->codecpar, encoder_context) < 0) {
      logging("failed to copy encoder parameters to output stream");
      return -1;
    }

    logging("out stream default time_base %d/%d", out_stream->time_base.num, out_stream->time_base.den);
    out_stream->time_base = encoder_context->time_base;
    logging("out stream encoder context time_base %d/%d", out_stream->time_base.num, out_stream->time_base.den);
    transcode_context[i].encoder_codec_context = encoder_context;
  }

  return 0;
}




static int prepare_input(char *input_file_name, AVFormatContext *format_context)
{
  if (!format_context) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&format_context, input_file_name, NULL, NULL) != 0) {
    logging("ERROR could not open the file %s.", input_file_name);
    return -1;
  }

  if (avformat_find_stream_info(format_context,  NULL) < 0) {
    logging("ERROR could not get the stream info");
    return -1;
  }
  // create an array of N transcode context, one per stream (audio and video)
  logging("sizeof transcode_context (%p)", transcode_context);
  transcode_context = av_mallocz_array(format_context->nb_streams, sizeof(*transcode_context));
  logging("sizeof transcode_context (%p)", transcode_context);

  for (int i = 0; i < format_context->nb_streams; i++)
  {
    AVStream *stream = format_context->streams[i];
    AVCodecParameters *codec_parameters = format_context->streams[i]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);

    if (codec==NULL) {
      logging("ERROR unsupported codec!");
      return -1;
    }

    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
      logging("failed to allocated memory for AVCodecContext");
      return -1;
    }

    if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0) {
      logging("failed to copy codec params to codec context");
      return -1;
    }

    if (codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
      logging("default framerate %d/%d", codec_context->framerate.num, codec_context->framerate.den);
      codec_context->framerate = av_guess_frame_rate(format_context, stream, NULL);
      logging("guessed framerate %d/%d", codec_context->framerate.num, codec_context->framerate.den);
    }

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
      logging("failed to open codec through avcodec_open2");
      return -1;
    }

    // store the decoder for each stream
    logging("transcode_context[%d] %p", i, transcode_context[i]);
    transcode_context[i].decoder_codec_context = codec_context;
    logging("transcode_context[%d] %p", i, transcode_context[i]);
  }
  return 0;
}
