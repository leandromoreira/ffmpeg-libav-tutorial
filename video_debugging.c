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

void logging(const char *fmt, ...)
{
  va_list args;
  fprintf( stderr, "LOG: " );
  va_start( args, fmt );
  vfprintf( stderr, fmt, args );
  va_end( args );
  fprintf( stderr, "\n" );
}

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    logging("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
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
