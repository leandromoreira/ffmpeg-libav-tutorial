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
  int video_stream_index;

  AVStream *stream[2];
  AVCodec *codec[2];
  AVCodecContext *codec_context[2];
} TranscodeContext;

static void logging(const char *fmt, ...);
static int decode_packet(AVPacket *i_packet, AVCodecContext *decoder_codec_context, AVFrame *i_frame);
static int encode_frame(AVFormatContext *o_format_context, AVCodecContext *encoder_codec_context, AVFrame *frame, int stream_index);

static int prepare_input(TranscodeContext *input_transcode);
static int prepare_output(TranscodeContext *input_transcode, TranscodeContext *output_transcode);

int main(const int argc, char *argv[])
{
  if (argc < 3) {
    logging("Usage: %s <input file name> <output filename>", argv[0]);
    return -1;
  }

  TranscodeContext *decoder = malloc(sizeof(TranscodeContext));
  TranscodeContext *encoder = malloc(sizeof(TranscodeContext));

  decoder->file_name = argv[1];
  encoder->file_name = argv[2];

  av_register_all();

  decoder->format_context = avformat_alloc_context();
  if (!decoder->format_context) {
    logging("could not allocate memory for Format Context");
    return -1;
  }

  if (prepare_input(decoder)) {
    logging("error on prepare input");
    return -1;
  }

  avformat_alloc_output_context2(&encoder->format_context, NULL, NULL, encoder->file_name);
  if (!encoder->format_context) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (prepare_output(decoder, encoder)) {
    logging("error on prepare output");
    return -1;
  }

  if (avio_open(&encoder->format_context->pb, encoder->file_name, AVIO_FLAG_WRITE) < 0) {
    logging("could not open output file");
    return -1;
  }

  if (avformat_write_header(encoder->format_context, NULL) < 0) {
    logging("error while writing the header of the output");
    return -1;
  }

  // decode input to encode output
  AVFrame *i_frame = av_frame_alloc();
  if (!i_frame) {
    logging("failed to allocated memory for AVFrame");
    return -1;
  }

  AVPacket *i_packet = av_packet_alloc();
  if (!i_packet) {
    logging("failed to allocated memory for AVPacket");
    return -1;
  }

  int response = 0;
  int how_many_packets_to_process = 8;

  while (av_read_frame(decoder->format_context, i_packet) >= 0)
  {
    logging("AVPacket->pts %" PRId64, i_packet->pts);
    response = decode_packet(i_packet, decoder->codec_context[i_packet->stream_index], i_frame);
    if (response < 0)
      break;

    i_frame->format = decoder->codec_context[i_packet->stream_index]->pix_fmt;
    i_frame->width = decoder->codec_context[i_packet->stream_index]->width;
    i_frame->height = decoder->codec_context[i_packet->stream_index]->height;

    encode_frame(
        encoder->format_context,
        encoder->codec_context[i_packet->stream_index],
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

  while (ret >= 0) {
    ret = avcodec_receive_packet(encoder_codec_context, o_packet);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      logging("Error while receiving a packet from the decoder: %s", av_err2str(ret));
      return -1;
    } else if (ret < 0) {
      logging("Error while receiving a packet from the decoder: %s", av_err2str(ret));
      return -1;
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

static int prepare_output(TranscodeContext *input_transcode, TranscodeContext *output_transcode)
{
  for (int i = 0; i < input_transcode->format_context->nb_streams; i++)
  {
    AVCodec *encoder = NULL;
    AVCodecContext *encoder_context = NULL;
    AVCodecContext *decoder_context = input_transcode->codec_context[i];
    AVStream *stream = avformat_new_stream(output_transcode->format_context, NULL);

    if (!stream) {
      logging("it could not create a new stream for the output");
      return -1;
    }

    if (decoder_context->codec_type == AVMEDIA_TYPE_VIDEO) {
      encoder = avcodec_find_encoder_by_name("libx264");
    } else {
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

    AVDictionary *encoder_options = NULL;

    if (decoder_context->codec_type == AVMEDIA_TYPE_VIDEO) {
      // This is -x264-params keyint=60:min-keyint=60:no-scenecut=1 command line part
      // https://en.wikibooks.org/wiki/MeGUI/x264_Settings
      av_opt_set(&encoder_options, "keyint", "60", 0);
      av_opt_set(&encoder_options, "min-keyint", "60", 0);
      av_opt_set(&encoder_options, "no-scenecut", "1", 0);

      encoder_context->height = decoder_context->height;
      encoder_context->width = decoder_context->width;
      encoder_context->time_base = av_inv_q(decoder_context->framerate);
      encoder_context->sample_aspect_ratio = decoder_context->sample_aspect_ratio;

      if (encoder->pix_fmts)
        encoder_context->pix_fmt = encoder->pix_fmts[0];
      else
        encoder_context->pix_fmt = decoder_context->pix_fmt;

    } else if (decoder_context->codec_type == AVMEDIA_TYPE_AUDIO) {
      //review if this is necessary while just copying the stream
      encoder_context->sample_rate = decoder_context->sample_rate;
      encoder_context->channel_layout = decoder_context->channel_layout;
      encoder_context->channels = av_get_channel_layout_nb_channels(encoder_context->channel_layout);
      encoder_context->sample_fmt = encoder->sample_fmts[0];
      encoder_context->time_base = (AVRational){1, encoder_context->sample_rate};
    }

    if (avcodec_open2(encoder_context, encoder, &encoder_options) < 0) {
      logging("failed to open codec through avcodec_open2");
      return -1;
    }

    if (avcodec_parameters_from_context(stream->codecpar, encoder_context) < 0) {
      logging("failed to copy encoder parameters to output stream");
      return -1;
    }

    stream->time_base = encoder_context->time_base;

    if (encoder_context->codec_type == AVMEDIA_TYPE_VIDEO) {
      output_transcode->video_stream_index = i;
    } else if (encoder_context->codec_type == AVMEDIA_TYPE_AUDIO) {
      output_transcode->audio_stream_index = i;
    }

    output_transcode->codec[i] = encoder;
    output_transcode->stream[i] = stream;
    output_transcode->codec_context[i] = encoder_context;
  }

  return 0;
}

static int prepare_input(TranscodeContext *input_transcode)
{
  if (avformat_open_input(&input_transcode->format_context, input_transcode->file_name, NULL, NULL) != 0) {
    logging("ERROR could not open the file %s.", input_transcode->file_name);
    return -1;
  }

  if (avformat_find_stream_info(input_transcode->format_context,  NULL) < 0) {
    logging("ERROR could not get the stream info");
    return -1;
  }

  for (int i = 0; i < input_transcode->format_context->nb_streams; i++)
  {
    AVStream *stream = input_transcode->format_context->streams[i];
    AVCodecParameters *codec_parameters = input_transcode->format_context->streams[i]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);

    if (!codec) {
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

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
      logging("failed to open codec through avcodec_open2");
      return -1;
    }

    if (codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
      codec_context->framerate = av_guess_frame_rate(input_transcode->format_context, stream, NULL);
      input_transcode->video_stream_index = i;
    } else if (codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
      input_transcode->audio_stream_index = i;
    }

    input_transcode->codec[i] = codec;
    input_transcode->stream[i] = stream;
    input_transcode->codec_context[i] = codec_context;
  }

  av_dump_format(input_transcode->format_context, 0, input_transcode->file_name, 0);
  return 0;
}
