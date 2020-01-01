#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <string.h>
#include <inttypes.h>
#include "video_debugging.h"

typedef struct StreamingParams {
  char copy_video;
  char copy_audio;
} StreamingParams;

typedef struct StreamingContext {
  AVFormatContext *avfc;
  AVCodec *video_avc;
  AVCodec *audio_avc;
  AVStream *video_avs;
  AVStream *audio_avs;
  AVCodecContext *video_avcc;
  AVCodecContext *audio_avcc;
  int video_index;
  int audio_index;
  char *filename;
} StreamingContext;

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

int open_media(const char *in_filename, AVFormatContext **avfc) {
  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gac7a91abf2f59648d995894711f070f62
  // Allocate an AVFormatContext.
  *avfc = avformat_alloc_context();
  if (!*avfc) {logging("failed to alloc memory for format"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaff468dcc45289542f4c30d311bc2a201
  // Open an input stream and read the header.
  if (avformat_open_input(avfc, in_filename, NULL, NULL) != 0) {logging("failed to open input file %s", in_filename); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
  // Read packets of a media file to get stream information.
  if (avformat_find_stream_info(*avfc, NULL) < 0) {logging("failed to get stream info"); return -1;}
  return 0;
}

int prepare_decoder(StreamingContext *sc) {
  // iterating through all the input streams (audio, video, subtitles and etc)
  // if we have multiples streams of audio and video we're going to take the last
  for (int i = 0; i < sc->avfc->nb_streams; i++) {
    if (sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      // keeping a pointer to the video av stream
      sc->video_avs = sc->avfc->streams[i];
      sc->video_index = i;

      if (fill_stream_info(sc->video_avs, &sc->video_avc, &sc->video_avcc)) {return -1;}
      print_timing("Video timming info from the decoder preparation", sc->avfc, sc->video_avcc, sc->video_avs);
    } else if (sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      // keeping a pointer to the audio av stream
      sc->audio_avs = sc->avfc->streams[i];
      sc->audio_index = i;

      if (fill_stream_info(sc->audio_avs, &sc->audio_avc, &sc->audio_avcc)) {return -1;}
      print_timing("Audio timming info from the decoder preparation", sc->avfc, sc->audio_avcc, sc->audio_avs);
    } else {
      logging("skipping streams other than audio and video");
    }
  }

  return 0;
}

int prepare_encoder(StreamingContext *sc, AVCodecParameters *decoder_par, AVRational input_framerate) {
  /*
   * Prepping the encoder - video stream to be transcoded
   */
  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37
  // Find a registered encoder with the specified name.
  sc->video_avc = avcodec_find_encoder_by_name("libx264");
  if (!sc->video_avc) {logging("could not find the proper codec"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315
  // Allocate an AVCodecContext and set its fields to default values.
  sc->video_avcc = avcodec_alloc_context3(sc->video_avc);
  if (!sc->video_avcc) {logging("could not allocated memory for codec context"); return -1;}

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
  // Fill the codec context based on the values from the supplied codec parameters.
  if(avcodec_parameters_to_context(sc->video_avcc, decoder_par) < 0){logging("failed to copy codec parameters from the decoder"); return -1;}
  print_timing("Video encoder 1", sc->avfc, sc->video_avcc, sc->video_avs);

  av_opt_set(sc->video_avcc->priv_data, "preset", "slow", 0);
  //av_opt_set(sc->video_avcc->priv_data, "x264opts", "keyint=60:min-keyint=60:scenecut=-1", 0);

  AVDictionary *encoder_private_options = NULL;
  //av_dict_set(&encoder_private_options , "b", "2.0M", 0);

  sc->video_avcc->time_base = input_framerate;

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
  // Initialize the AVCodecContext to use the given AVCodec. (does it resets the avcc?)
  if (avcodec_open2(sc->video_avcc, sc->video_avc, &encoder_private_options) < 0) {logging("could not open the codec"); return -1;}
  print_timing("Video encoder 2", sc->avfc, sc->video_avcc, sc->video_avs);

  sc->video_avs = avformat_new_stream(sc->avfc, NULL);
  //sc->video_avs->time_base = (AVRational){1, input_framerate};

  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga6d02e640ccc12c783841ce51d09b9fa7
  // Any allocated fields in dst are freed and replaced with newly allocated duplicates of the corresponding fields in src.
  avcodec_parameters_copy(sc->video_avs->codecpar, decoder_par);
  print_timing("Video timming info from the encoder preparation", sc->avfc, sc->video_avcc, sc->video_avs);
  print_timing("Video encoder 3", sc->avfc, sc->video_avcc, sc->video_avs);

  return 0;
}

int prepare_copy(AVFormatContext *avfc, AVStream **avs, AVCodecParameters *decoder_par) {
  *avs = avformat_new_stream(avfc, NULL);
  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga6d02e640ccc12c783841ce51d09b9fa7
  // Any allocated fields in dst are freed and replaced with newly allocated duplicates of the corresponding fields in src.
  avcodec_parameters_copy((*avs)->codecpar, decoder_par);
  return 0;
}

int remux(AVPacket **pkt, AVFormatContext **avfc, AVRational decoder_tb, AVRational encoder_tb) {
  //https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e
  // Convert valid timing fields (timestamps / durations) in a packet from one timebase to another.
  av_packet_rescale_ts(*pkt, decoder_tb, encoder_tb);

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
  // Write a packet to an output media file ensuring correct interleaving
  if (av_interleaved_write_frame(*avfc, *pkt) < 0) { logging("error while copying stream packet"); return -1; }
  av_packet_unref(*pkt);
  return 0;
}


int main(int argc, char *argv[])
{
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 1;

  StreamingContext *decoder = (StreamingContext*) calloc(1, sizeof(StreamingContext));
  decoder->filename = argv[1];

  StreamingContext *encoder = (StreamingContext*) calloc(1, sizeof(StreamingContext));
  encoder->filename = argv[2];

  /*
   * Opening the decodable media file
   */
  if (open_media(decoder->filename, &decoder->avfc)) return -1;

  /*
   * Prepping the decoder
   */
  if (prepare_decoder(decoder)) return -1;

  /*
   * Opening the output media file
   */
  //https://www.ffmpeg.org/doxygen/trunk/avformat_8h.html#a0234fa1116af3c0a72edaa08a2ba304f
  // Allocate an AVFormatContext for an output format.
  avformat_alloc_output_context2(&encoder->avfc, NULL, NULL, encoder->filename);
  if (!encoder->avfc) {logging("could not allocate memory for output format");return -1;}

  /*
   * Prepping the encoder
   */

  // for video
  if (!sp.copy_video) {
    // transcoding
    // TODO:
  } else {
    // just copying
    if (prepare_copy(encoder->avfc, &encoder->video_avs, decoder->video_avs->codecpar)) {return -1;}
  }

  // for audio
  if (!sp.copy_audio) {
    // transcoding
    // TODO:
  } else {
    // just copying
    if (prepare_copy(encoder->avfc, &encoder->audio_avs, decoder->audio_avs->codecpar)) {return -1;}
  }

  /*
   * Prepping the output media file
   */
  if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
    //https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga9ed82634c59b339786575827c47a8f68
    // Place global headers in extradata instead of every keyframe.
    encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(encoder->avfc->oformat->flags & AVFMT_NOFILE)) {
    //https://www.ffmpeg.org/doxygen/trunk/aviobuf_8c.html#ab1b99c5b70aa59f15ab7cd4cbb40381e
    // Create and initialize a AVIOContext for accessing the resource indicated by url.
    if (avio_open(&encoder->avfc->pb, encoder->filename, AVIO_FLAG_WRITE) < 0) {
      logging("could not open the output file");
      return -1;
    }
  }
  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga18b7b10bb5b94c4842de18166bc677cb
  // Allocate the stream private data and write the stream header to an output media file.
  if (avformat_write_header(encoder->avfc, NULL) < 0) {logging("an error occurred when opening output file"); return -1;}

  /*
   * Reading the streams packets from the input media file
   */
  AVFrame *input_frame = av_frame_alloc();
  if (!input_frame) {logging("failed to allocated memory for AVFrame"); return -1;}

  AVPacket *input_packet = av_packet_alloc();
  if (!input_packet) {logging("failed to allocated memory for AVPacket"); return -1;}

  int response = 0;

  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
  // Return the next frame of a stream.
  while (av_read_frame(decoder->avfc, input_packet) >= 0)
  {
    if (decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (!sp.copy_video) {
        // transcoding
        // TODO
      } else {
        // just copying
        if (remux(&input_packet, &encoder->avfc, decoder->video_avs->time_base, encoder->video_avs->time_base)) return -1;
      }
    } else if (decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)  {
      if (!sp.copy_audio) {
        // transcoding
        // TODO
      } else {
        // just copying
        if (remux(&input_packet, &encoder->avfc, decoder->audio_avs->time_base, encoder->audio_avs->time_base)) return -1;
      }
    } else {
      logging("ignoring all non video or audio packets");
    }
  }
  //https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13
  // Write the stream trailer to an output media file and free the file private data.
  av_write_trailer(encoder->avfc);

  // TODO: we should free everything!
  return 0;
}

