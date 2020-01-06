#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <string.h>
#include <inttypes.h>

void logging(const char *fmt, ...);
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs);
