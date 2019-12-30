#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <string.h>
#include <inttypes.h>

static void logging(const char *fmt, ...)
{
  va_list args;
  fprintf( stderr, "LOG: " );
  va_start( args, fmt );
  vfprintf( stderr, fmt, args );
  va_end( args );
  fprintf( stderr, "\n" );
}

void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs) {
  logging("=================================================");
  logging("%s", name);

  logging("\tAVFormatContext");
  if (avf != NULL) {
    logging("\t\tstart_time=%d duration=%d bit_rate=%d start_time_realtime=%d", avf->start_time, avf->duration, avf->bit_rate, avf->start_time_realtime);
  } else {
    logging("\t\t->NULL");
  }

  logging("\tAVCodecContext");
  if (avc != NULL) {
    logging("\t\tbit_rate=%d ticks_per_frame=%d width=%d height=%d gop_size=%d keyint_min=%d sample_rate=%d profile=%d level=%d ",
        avc->bit_rate, avc->ticks_per_frame, avc->width, avc->height, avc->gop_size, avc->keyint_min, avc->sample_rate, avc->profile, avc->level);
    logging("\t\tavc->time_base=num/den %d/%d", avc->time_base.num, avc->time_base.den);
    logging("\t\tavc->framerate=num/den %d/%d", avc->framerate.num, avc->framerate.den);
    logging("\t\tavc->pkt_timebase=num/den %d/%d", avc->pkt_timebase.num, avc->pkt_timebase.den);
  } else {
    logging("\t\t->NULL");
  }

  logging("\tAVStream");
  if (avs != NULL) {
    logging("\t\tindex=%d start_time=%d duration=%d ", avs->index, avs->start_time, avs->duration);
    logging("\t\tavs->time_base=num/den %d/%d", avs->time_base.num, avs->time_base.den);
    logging("\t\tavs->sample_aspect_ratio=num/den %d/%d", avs->sample_aspect_ratio.num, avs->sample_aspect_ratio.den);
    logging("\t\tavs->avg_frame_rate=num/den %d/%d", avs->avg_frame_rate.num, avs->avg_frame_rate.den);
    logging("\t\tavs->r_frame_rate=num/den %d/%d", avs->r_frame_rate.num, avs->r_frame_rate.den);
  } else {
    logging("\t\t->NULL");
  }

  logging("=================================================");
}

int fill_stream_info(AVStream *avs, AVCodec **avc, AVCodecContext **avcc) {
  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
  // Find a registered decoder with a matching codec ID.
  *avc = avcodec_find_decoder(avs->codecpar->codec_id);
  if (!*avc) {logging("failed to find the codec"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315
  // Allocate an AVCodecContext and set its fields to default values.
  *avcc = avcodec_alloc_context3(*avc);
  if (!*avcc) {logging("failed to alloc memory for codec context"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
  // Fill the codec context based on the values from the supplied codec parameters.
  if (avcodec_parameters_to_context(*avcc, avs->codecpar) < 0) {logging("failed to fill codec context"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
  // Initialize the AVCodecContext to use the given AVCodec.
  if (avcodec_open2(*avcc, *avc, NULL) < 0) {logging("failed to open codec"); return -1;}
  return 0;
}

int main(int argc, char *argv[])
{
  char *in_filename = argv[1];
  char *out_filename =argv[2];
  //av_log_set_level(AV_LOG_DEBUG);

  AVFormatContext *decoder_avfc = NULL;
  AVFormatContext *encoder_avfc = NULL;

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gac7a91abf2f59648d995894711f070f62
  // allocating memory for enc avformat
  decoder_avfc = avformat_alloc_context();
  if (!decoder_avfc) {logging("failed to alloc memory for format"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaff468dcc45289542f4c30d311bc2a201
  // opening the file for the format
  if (avformat_open_input(&decoder_avfc, in_filename, NULL, NULL) != 0) {logging("failed to open input file %s", in_filename); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
  // reading bytes from file to get stream info
  if (avformat_find_stream_info(decoder_avfc, NULL) < 0) {logging("failed to get stream info"); return -1;}

  /*
   * Prepping the decoder
   */
  AVCodec *decoder_video_codec, *decoder_audio_codec;
  AVStream *decoder_video_avs, *decoder_audio_avs;
  AVCodecContext *decoder_video_avcc, *decoder_audio_avcc;

  // iterating through all the input streams (audio, video, subtitles and etc)
  // if we have multiples streams of audio and video we're going to take the last
  for (int i = 0; i < decoder_avfc->nb_streams; i++) {
    if (decoder_avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      // keeping a pointer to the video av stream
      decoder_video_avs = decoder_avfc->streams[i];

      fill_stream_info(decoder_video_avs, &decoder_video_codec, &decoder_video_avcc);
      print_timing("Video timming info from the decoder preparation", decoder_avfc, decoder_video_avcc, decoder_video_avs);
    } else if (decoder_avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      // keeping a pointer to the audio av stream
      decoder_audio_avs = decoder_avfc->streams[i];

      fill_stream_info(decoder_audio_avs, &decoder_audio_codec, &decoder_audio_avcc);
      print_timing("Audio timming info from the decoder preparation", decoder_avfc, decoder_audio_avcc, decoder_audio_avs);
    } else {
      logging("skipping streams other than audio and video");
    }
  }

  /*
   * Prepping the encoder
   */
  AVCodec *encoder_video_codec = NULL, *encoder_audio_codec = NULL;
  AVStream *encoder_video_avs = NULL, *encoder_audio_avs = NULL;
  AVCodecContext *encoder_video_avcc = NULL, *encoder_audio_avcc = NULL;

  //https://www.ffmpeg.org/doxygen/trunk/avformat_8h.html#a0234fa1116af3c0a72edaa08a2ba304f
  // Allocate an AVFormatContext for an output format.
  avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);
  if (!encoder_avfc) {logging("could not allocate memory for output format");return -1;}

  // prepping video to be copied
  encoder_video_avs = avformat_new_stream(encoder_avfc, NULL);
  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga6d02e640ccc12c783841ce51d09b9fa7
  // Any allocated fields in dst are freed and replaced with newly allocated duplicates of the corresponding fields in src.
  avcodec_parameters_copy(encoder_video_avs->codecpar, decoder_video_avs->codecpar);
  print_timing("Video timming info from the encoder preparation", encoder_avfc, encoder_video_avcc, encoder_video_avs);

  // prepping audio to be copied
  encoder_audio_avs = avformat_new_stream(encoder_avfc, NULL);
  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga6d02e640ccc12c783841ce51d09b9fa7
  // Any allocated fields in dst are freed and replaced with newly allocated duplicates of the corresponding fields in src.
  avcodec_parameters_copy(encoder_audio_avs->codecpar, decoder_audio_avs->codecpar);
  print_timing("Audio timming info from the encoder preparation", encoder_avfc, encoder_audio_avcc, encoder_audio_avs);


  if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
    //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga9ed82634c59b339786575827c47a8f68
    // Place global headers in extradata instead of every keyframe.
    encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(encoder_avfc->oformat->flags & AVFMT_NOFILE)) {
    //https://www.ffmpeg.org/doxygen/trunk/aviobuf_8c.html#ab1b99c5b70aa59f15ab7cd4cbb40381e
    // Create and initialize a AVIOContext for accessing the resource indicated by url.
    if (avio_open(&encoder_avfc->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
      logging("could not open the output file");
      return -1;
    }
  }
  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga18b7b10bb5b94c4842de18166bc677cb
  // Allocate the stream private data and write the stream header to an output media file.
  if (avformat_write_header(encoder_avfc, NULL) < 0) {logging("an error occurred when opening output file"); return -1;}


  AVFrame *input_frame = av_frame_alloc();
  if (!input_frame) {logging("failed to allocated memory for AVFrame"); return -1;}

  AVPacket *input_packet = av_packet_alloc();
  if (!input_packet) {logging("failed to allocated memory for AVPacket"); return -1;}

  int response = 0;

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
  // Return the next frame of a stream.
  while (av_read_frame(decoder_avfc, input_packet) >= 0)
  {
    if (decoder_avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      //https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e
      // Convert valid timing fields (timestamps / durations) in a packet from one timebase to another.
      av_packet_rescale_ts(input_packet, decoder_avfc->streams[input_packet->stream_index]->time_base, encoder_avfc->streams[input_packet->stream_index]->time_base);

      //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
      // Write a packet to an output media file ensuring correct interleaving
      if (av_interleaved_write_frame(encoder_avfc, input_packet) < 0) { logging("error while copying stream packet"); return -1; }
    } else if (decoder_avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)  {
      //https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e
      // Convert valid timing fields (timestamps / durations) in a packet from one timebase to another.
      av_packet_rescale_ts(input_packet, decoder_avfc->streams[input_packet->stream_index]->time_base, encoder_avfc->streams[input_packet->stream_index]->time_base);

      //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
      // Write a packet to an output media file ensuring correct interleaving
      if (av_interleaved_write_frame(encoder_avfc, input_packet) < 0) { logging("error while copying stream packet"); return -1; }
    } else {
      logging("ignoring all non video or audio packets");
    }
  }
  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13
  // Write the stream trailer to an output media file and free the file private data.
  av_write_trailer(encoder_avfc);

  // TODO: we should free everything!
  //avformat_free_context(decoder_avfc);
  //avformat_free_context(encoder_avfc);
  return 0;
}

