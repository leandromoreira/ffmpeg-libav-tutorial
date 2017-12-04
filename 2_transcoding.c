#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <string.h>
#include <inttypes.h>

typedef struct _TranscodeContext {
  char *file_name;
  AVFormatContext *format_context;

  AVCodec *codec[2];
  AVStream *stream[2];
  AVCodecParameters *codec_parameters[2];
  AVCodecContext *codec_context[2];
  int video_stream_index;
  int audio_stream_index;
} TranscodeContext;

static void logging(const char *fmt, ...);
static int decode_packet(TranscodeContext *decoder_context, TranscodeContext *encoder_context, AVPacket *packet, AVFrame *frame, int stream_index);
static int encode_frame(TranscodeContext *decoder_context, TranscodeContext *encoder_context, AVFormatContext *format_context, AVCodecContext *codec_context, AVFrame *frame, int stream_index);
static int prepare_decoder(TranscodeContext *decoder_context);
static int prepare_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context);

int main(int argc, char *argv[])
{
  av_register_all();

  TranscodeContext *decoder_context = malloc(sizeof(TranscodeContext));
  decoder_context->file_name = argv[1];

  if (prepare_decoder(decoder_context)) {
    logging("error while preparing input");
    return -1;
  }

  TranscodeContext *encoder_context = malloc(sizeof(TranscodeContext));
  encoder_context->file_name = argv[2];

  if (prepare_encoder(encoder_context, decoder_context)) {
    logging("error while preparing output");
    return -1;
  }

  AVFrame *input_frame = av_frame_alloc();
  if (!input_frame) {
    logging("failed to allocated memory for AVFrame");
    return -1;
  }
  AVPacket *input_packet = av_packet_alloc();
  if (!input_packet) {
    logging("failed to allocated memory for AVPacket");
    return -1;
  }

  int response = 0;
  int how_many_packets_to_process = 20;

  while (av_read_frame(decoder_context->format_context, input_packet) >= 0)
  {
    logging("AVPacket->pts %" PRId64, input_packet->pts);

    if (input_packet->stream_index == decoder_context->video_stream_index) {
      response = decode_packet(
          decoder_context,
          encoder_context,
          input_packet,
          input_frame,
          input_packet->stream_index
          );

      if (response < 0)
        break;
      if (--how_many_packets_to_process <= 0) break;
      av_packet_unref(input_packet);
    } else {
      // just copying audio stream
      av_packet_rescale_ts(input_packet,
          decoder_context->stream[input_packet->stream_index]->time_base,
          encoder_context->stream[input_packet->stream_index]->time_base
          );

      if (av_interleaved_write_frame(encoder_context->format_context, input_packet) < 0) {
        logging("error while copying audio stream");
        return -1;
      }
      logging("\tfinish copying packets without reencoding");
    }
  }
  // flush all frames
  encode_frame(decoder_context, encoder_context, decoder_context->format_context, decoder_context->codec_context[encoder_context->video_stream_index], NULL, encoder_context->video_stream_index);

  av_write_trailer(encoder_context->format_context);

  logging("releasing all the resources");

  avformat_close_input(&decoder_context->format_context);
  avformat_free_context(decoder_context->format_context);
  av_packet_free(&input_packet);
  av_frame_free(&input_frame);
  avcodec_free_context(&decoder_context->codec_context[0]);
  avcodec_free_context(&decoder_context->codec_context[1]);
  free(decoder_context);
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

static int prepare_decoder(TranscodeContext *decoder_context) {
  decoder_context->format_context = avformat_alloc_context();
  if (!decoder_context->format_context) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&decoder_context->format_context, decoder_context->file_name, NULL, NULL) != 0) {
    logging("ERROR could not open the file");
    return -1;
  }

  if (avformat_find_stream_info(decoder_context->format_context,  NULL) < 0) {
    logging("ERROR could not get the stream info");
    return -1;
  }

  for (int i = 0; i < decoder_context->format_context->nb_streams; i++)
  {
    if (decoder_context->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      decoder_context->video_stream_index = i;
      logging("Video");
    } else if (decoder_context->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      decoder_context->audio_stream_index = i;
      logging("Audio");
    }
    decoder_context->codec_parameters[i] = decoder_context->format_context->streams[i]->codecpar;
    decoder_context->stream[i] = decoder_context->format_context->streams[i];

    logging("\tAVStream->time_base before open coded %d/%d", decoder_context->stream[i]->time_base.num, decoder_context->stream[i]->time_base.den);
    logging("\tAVStream->r_frame_rate before open coded %d/%d", decoder_context->stream[i]->r_frame_rate.num, decoder_context->stream[i]->r_frame_rate.den);
    logging("\tAVStream->start_time %" PRId64, decoder_context->stream[i]->start_time);
    logging("\tAVStream->duration %" PRId64, decoder_context->stream[i]->duration);

    decoder_context->codec[i] = avcodec_find_decoder(decoder_context->codec_parameters[i]->codec_id);
    if (!decoder_context->codec[i]) {
      logging("ERROR unsupported codec!");
      return -1;
    }

    decoder_context->codec_context[i] = avcodec_alloc_context3(decoder_context->codec[i]);
    if (!decoder_context->codec[i]) {
      logging("failed to allocated memory for AVCodecContext");
      return -1;
    }

    if (avcodec_parameters_to_context(decoder_context->codec_context[i], decoder_context->codec_parameters[i]) < 0) {
      logging("failed to copy codec params to codec context");
      return -1;
    }

    if (avcodec_open2(decoder_context->codec_context[i], decoder_context->codec[i], NULL) < 0) {
      logging("failed to open codec through avcodec_open2");
      return -1;
    }
  }

  return 0;
}

static int decode_packet(TranscodeContext *decoder_context, TranscodeContext *encoder_context, AVPacket *packet, AVFrame *frame, int stream_index)
{
  AVFormatContext *format_context = decoder_context->format_context;
  AVCodecContext *codec_context = decoder_context->codec_context[stream_index];

  int response = avcodec_send_packet(codec_context, packet);

  if (response < 0) {
    logging("Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  while (response >= 0)
  {
    response = avcodec_receive_frame(codec_context, frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }

    if (response >= 0) {
      if (codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
        logging(
            "\tEncoding VIDEO Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
            codec_context->frame_number,
            av_get_picture_type_char(frame->pict_type),
            frame->pkt_size,
            frame->pts,
            frame->key_frame,
            frame->coded_picture_number
            );
        encode_frame(decoder_context, encoder_context, format_context, codec_context, frame, stream_index);
      }
      av_frame_unref(frame);
    }
  }
  return 0;
}

static int encode_frame(TranscodeContext *decoder_context, TranscodeContext *encoder_context, AVFormatContext *format_context, AVCodecContext *codec_context, AVFrame *frame, int stream_index)
{
  AVPacket *output_packet = av_packet_alloc();
  if (!output_packet) {
    logging("could not allocate memory for output packet");
    return -1;
  }

  int ret;
  if (frame)
    logging("Send frame %3"PRId64"", frame->pts);
  ret = avcodec_send_frame(codec_context, frame);

  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_context, output_packet);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      logging("Error while receiving a packet from the decoder: %s", av_err2str(ret));
      return -1;
    } else if (ret < 0) {
      logging("Error while receiving a packet from the decoder: %s", av_err2str(ret));
      return -1;
    }

    /* prepare packet for muxing */
    output_packet->stream_index = stream_index;

    av_packet_rescale_ts(output_packet,
        decoder_context->stream[stream_index]->time_base,
        encoder_context->stream[stream_index]->time_base
        );
    /* mux encoded frame */
    ret = av_interleaved_write_frame(format_context, output_packet);

    logging("Write packet %3"PRId64" (size=%5d)", output_packet->pts, output_packet->size);
    av_packet_unref(output_packet);
  }
  av_packet_free(&output_packet);
  return 0;
}


static int prepare_video_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context) {
  int index = decoder_context->video_stream_index;
  encoder_context->stream[index] = avformat_new_stream(encoder_context->format_context, NULL);
  encoder_context->codec[index] = avcodec_find_encoder_by_name("libx264");

  if (!encoder_context->codec[index]) {
    logging("could not find the proper codec");
    return -1;
  }

  encoder_context->codec_context[index] = avcodec_alloc_context3(encoder_context->codec[index]);
  if (!encoder_context->codec_context[index]) {
    logging("could not allocated memory for codec context");
    return -1;
  }

  // how to free this?
  AVDictionary *encoder_options = NULL;
  av_opt_set(&encoder_options, "keyint", "60", 0);
  av_opt_set(&encoder_options, "min-keyint", "60", 0);
  av_opt_set(&encoder_options, "no-scenecut", "1", 0);

  encoder_context->codec_context[index]->height = decoder_context->codec_context[index]->height;
  encoder_context->codec_context[index]->width = decoder_context->codec_context[index]->width;
  encoder_context->codec_context[index]->time_base = av_inv_q(decoder_context->codec_context[index]->framerate);
  encoder_context->codec_context[index]->sample_aspect_ratio = decoder_context->codec_context[index]->sample_aspect_ratio;

  if (encoder_context->codec[index]->pix_fmts)
    encoder_context->codec_context[index]->pix_fmt = encoder_context->codec[index]->pix_fmts[0];
  else
    encoder_context->codec_context[index]->pix_fmt = decoder_context->codec_context[index]->pix_fmt;

  encoder_context->codec_context[index]->time_base = decoder_context->stream[index]->time_base;

  if (avcodec_open2(encoder_context->codec_context[index], encoder_context->codec[index], &encoder_options) < 0) {
    logging("could not open the codec");
    return -1;
  }

  if (avcodec_parameters_from_context(encoder_context->stream[index]->codecpar, encoder_context->codec_context[index]) < 0) {
    logging("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder_context->stream[index]->time_base = encoder_context->codec_context[index]->time_base;
  return 0;
}

static int prepare_audio_copy(TranscodeContext *encoder_context, TranscodeContext *decoder_context) {
  int index = decoder_context->audio_stream_index;
  encoder_context->codec[index] = avcodec_find_encoder(decoder_context->codec_context[index]->codec_id);

  encoder_context->codec_context[index] = avcodec_alloc_context3(encoder_context->codec[index]);
  encoder_context->codec_context[index]->sample_rate = decoder_context->codec_context[index]->sample_rate;
  encoder_context->codec_context[index]->sample_fmt = encoder_context->codec[index]->sample_fmts[0];
  encoder_context->codec_context[index]->channel_layout = decoder_context->codec_context[index]->channel_layout;
  encoder_context->codec_context[index]->channels = av_get_channel_layout_nb_channels(encoder_context->codec_context[index]->channel_layout);

  encoder_context->stream[index] = avformat_new_stream(encoder_context->format_context, NULL);
  //encoder_context->time_base = (AVRational){1, encoder_context->sample_rate};
  encoder_context->stream[index]->time_base = encoder_context->codec_context[index]->time_base;

  if (avcodec_open2(encoder_context->codec_context[index], encoder_context->codec[index], NULL) < 0) {
    logging("failed to open codec through avcodec_open2");
    return -1;
  }

  if (avcodec_parameters_from_context(encoder_context->stream[index]->codecpar, encoder_context->codec_context[index]) < 0) {
    logging("failed to copy encoder parameters to output stream");
    return -1;
  }
  return 0;
}

static int prepare_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context) {
  avformat_alloc_output_context2(&encoder_context->format_context, NULL, NULL, encoder_context->file_name);
  if (!encoder_context->format_context) {
    logging("could not allocate memory for output format");
    return -1;
  }

  if (prepare_video_encoder(encoder_context, decoder_context)) {
    logging("error while preparing video encoder");
    return -1;
  }

  if (prepare_audio_copy(encoder_context, decoder_context)) {
    logging("error while preparing audio copy");
    return -1;
  }

  if (encoder_context->format_context->oformat->flags & AVFMT_GLOBALHEADER)
    encoder_context->format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(encoder_context->format_context->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&encoder_context->format_context->pb, encoder_context->file_name, AVIO_FLAG_WRITE) < 0) {
      logging("could not open the output file");
      return -1;
    }
  }
  if (avformat_write_header(encoder_context->format_context, NULL) < 0) {
    logging("an error occurred when opening output file");
    return -1;
  }

  return 0;
}
