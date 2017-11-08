[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

# Intro

I was looking for a tutorial/book that would teach me how to start to use [FFmpeg](https://www.ffmpeg.org/) as a library (a.k.a. libav) and then I found the ["How to write a video player in less than 1k lines"](http://dranger.com/ffmpeg/) tutorial but it was deprecated and I decided to write this one.

Most of the code in here will be in c **but don't worry** you can easily understand and apply it to your preferred language. FFmpeg libav has lots of bindings for many languages like: [python](https://mikeboers.github.io/PyAV/), [go](https://github.com/imkira/go-libav) and even if your language doesn't have it, you can still support it through the `ffi`, here's an example with [Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua).

We'll start with a quick lesson about what is video, audio, codec and container and then we'll go to a crash course on how to use `ffmpeg` command line and finally we'll write code, feel free to skip directly to[ ](http://newmediarockstars.com/wp-content/uploads/2015/11/nintendo-direct-iwata.jpg)the section [Learn FFmpeg libav the Hard Way.](#learn-ffmpeg-libav-the-hard-way)

Some people used to say that the Internet video streaming is the future of the traditional TV, in any case, the FFmpeg is something that worths to be studied.

## video - what you see!

If you have a sequence series of images and change them at a given frequency, let's say [24 images per second](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/), you will create an [illusion of movement](https://en.wikipedia.org/wiki/Persistence_of_vision), in summary this is the very basic idea behind a video, **a series of pictures / frames running at a given rate**.

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)

## audio - what you listen!

Although a muted video can express a quite amount of feelings, adding sound to it brings more pleasure to the experience.

Sound is the vibration that propagates as a wave of pressure, through the air (or a transmission medium such as a gas, liquid or solid).

> In a digital audio system, a microphone converts sound to an analog electrical signal, then an analog-to-digital converter (ADC)—typically using [pulse-code modulation—converts (PCM)](https://en.wikipedia.org/wiki/Pulse-code_modulation) the analog signal into a digital signal.

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")
https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg

## codec - shrinking data

> CODEC is an electronic circuit or software that **compresses or decompresses digital audio/video.** It converts raw (uncompressed) digital audio/video to a compressed format or vice versa.
> https://en.wikipedia.org/wiki/Video_codec

But if we chose to pack millions of images in a single file and called it a movie, we might ending up with huge file. Let's do the math:

Suppose we are creating a video with a resolution of `1080 x 1920` (height x width) and that we'll spend `3 bytes` per pixel (the minimal point at a screen) to encode the color (or [24 bit color](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29), what gives us 16,777,215 different colors) and this video runs at `24 frames per second` and it is `30 minutes` long.

```c
toppf = 1080 * 1920 //total_of_pixels_per_frame
cpp = 3 //cost_per_pixel
tis = 30 * 60 //time_in_seconds
fps = 24 //frames_per_second

required_storage = tis * fps * toppf * cpp
```

This video would requires us a storage of approximately `250.28GB` or a bandwidth of `1.11Gbps` that's why we need to use a [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work).

## container - a comfy place for audio and video

> A container or wrapper format is a metafile format whose specification describes how different elements of data and metadata coexist in a computer file.
> https://en.wikipedia.org/wiki/Digital_container_format

A **single file that contains all the streams** (mostly the audio and video) and it also provides **synchronization and general metadata**, such as title, resolution and etc. Usually we can infer the format of a file by looking at its extension, for instance a `video.webm` is probably a video using the container [`webm`](https://www.webmproject.org/).

![container](/img/container.png)

# FFmpeg - command line

> A complete, cross-platform solution to record, convert and stream audio and video.

To work with multimedia we can use the AMAZING tool/library called [FFmpeg](https://www.ffmpeg.org/), chances are you know / use directly or indirectly it. (do you use [Chrome?](https://www.chromium.org/developers/design-documents/video)).

It has a command line program called `ffmpeg`, it's a very simple yet powerful binary. For instance, you can convert from `mp4` to the container `avi` just by typing the follow command.

```bash
$ ffmpeg -i input.mp4 output.avi
```

We just made a **remuxing** which is converting from one container to another one, technically here FFmpeg could be also be doing a transcoding but we'll talk about that later.

## FFmpeg command line tool 101

FFmpeg does have a [documentation](https://www.ffmpeg.org/ffmpeg.html) that explains greatly how it works. To make things short the FFmpeg command line program expects the following argument format to perform its actions `ffmpeg {1} {2} -i {3} {4} {5}`, where:

1. global options
2. input file options
3. input url
4. output file options
5. output url

The parts 2, 3, 4 and 5 can be as many as you need. It's easier to understand this argument format in action:

``` bash
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # global options
-c:a libfdk_aac -c:v libx264 \ # input options
-i bunny_1080p_60fps.mp4 \ # input url
-c:v libvpx-vp9 -c:a libvorbis \ # output options
bunny_1080p_60fps_vp9.webm # output url
```
This command takes an input file `mp4` containing two streams, an audio encoded with `aac` CODEC and a video encoded using `h264` CODEC and convert it to `webm` changing its audio and video CODECs too.

We could simplify the command above but then be aware that FFmpeg will adopt or guess the default values for you, for instance when you just type `ffmpeg -i input.avi output.mp4` what audio/video CODEC does it use to produce the `output.mp4`?

Werner Robitza wrote a must read/execute [tutorial about encoding and editing with FFmpeg](http://slhck.info/ffmpeg-encoding-course/#/).

# Common video operations

While working with audio/video we usually do a set of tasks with the media.

## Transcoding

![transcoding](/img/transcoding.png)

**What?** the act of converting one of the streams (audio or video) from one CODEC to another one.

**Why?** sometimes some devices (TVs, smart phones, console and etc) doesn't support X but Y and newer CODECs provide better compression rate.

**How?** converting an `H264` (AVC) video to an `H265` (HEVC).
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## Transmuxing

![transmuxing](/img/transmuxing.png)

**What?** the act of converting from one format (container) to another one.

**Why?** sometimes some devices (TVs, smart phones, console and etc) doesn't support X but Y and sometimes newer containers provide modern required features.

**How?** converting a `mp4` to a `webm`.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # just saying to ffmpeg to skip encoding
bunny_1080p_60fps.webm
```

## Transrating

![transrating](/img/transrating.png)

**What?** the act of changing the bit rate, or producing other renditions.

**Why?** people will try to watch your video in a `2G` (edge) connection using a less powerful smart phone or in a `fiber` Internet connection on their 4K TVs therefore you should offer more than on rendition of the same video with different bit rate.

**How?** producing a rendition with bit rate between 3856K and 2000K.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

Usually we'll be using transrating with transsizing. Werner Robitza wrote another must read/execute [series of posts about FFmpeg rate control](http://slhck.info/posts/).

## Transsizing

![transsizing](/img/transsizing.png)

**What?** the act of converting from one resolution to another one, as said before transsizing is often used with transrating.

**Why?** reasons are about the same as for the transrating.

**How?** converting a `1080p` to a `480p` resolution.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## Bonus Round: Adaptive Streaming

![adaptive streaming](/img/adaptive-streaming.png)

**What?** the act of producing many resolutions (bit rates) and split the media into chunks and serve them via http.

**Why?** to provide a flexible media that can be watched on a low end smart phone or on a 4K TV, it's also easy to scale and deploy but it can add latency.

**How?** creating an adaptive WebM using DASH.
```bash
# video streams
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 160x90 -b:v 250k -keyint_min 150 -g 150 -an -f webm -dash 1 video_160x90_250k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 320x180 -b:v 500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_320x180_500k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 750k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_750k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 1000k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_1000k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 1280x720 -b:v 1500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_1280x720_1500k.webm

# audio streams
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:a libvorbis -b:a 128k -vn -f webm -dash 1 audio_128k.webm

# the DASH manifest
$ ffmpeg \
 -f webm_dash_manifest -i video_160x90_250k.webm \
 -f webm_dash_manifest -i video_320x180_500k.webm \
 -f webm_dash_manifest -i video_640x360_750k.webm \
 -f webm_dash_manifest -i video_640x360_1000k.webm \
 -f webm_dash_manifest -i video_1280x720_500k.webm \
 -f webm_dash_manifest -i audio_128k.webm \
 -c copy -map 0 -map 1 -map 2 -map 3 -map 4 -map 5 \
 -f webm_dash_manifest \
 -adaptation_sets "id=0,streams=0,1,2,3,4 id=1,streams=5" \
 manifest.mpd
```

PS: I stole this example from the [Instructions to playback Adaptive WebM using DASH](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)

## Going beyond

There are [many and many other usages for FFmpeg](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly), I use it in conjunction with *iMovie* to produce/edit some videos for YouTube, you can certainly use it professionally.

# Learn FFmpeg libav the Hard Way

> Don't you wonder sometimes 'bout sound and vision?
> **David Robert Jones**

Since [FFmpeg](#ffmpeg---command-line) is so useful as a command line tool to do tons of tasks over media files, how can we use (embedded) it in our programs?

It turns out that FFmpeg itself is [composed of several libraries](https://www.ffmpeg.org/doxygen/trunk/index.html) that can be used to be integrated into our own programs.

Usually when you install FFmpeg, it installs automatically all these libraries, I'll be referring to the set of these libraries as **FFmpeg libav**.

> This title is a homage to Zed Shaw's series [Learn X the Hard Way](https://learncodethehardway.org/) specially his book Learn C the Hard Way.

## Chapter 0 - The infamous hello world

This hello world actually won't show the message `"hello world"` in the terminal :tongue: instead we're going to **print out information about the video**, things like its format (container), duration, resolution, audio channels and in the end we'll **decode some frames and save them as image files**.

### FFmpeg libav architecture

But before we start to code, let's learn how **FFmpeg libav architecture** works, how its components communicate with others. For instance, here's a diagram of a process of decoding a video.

![ffmpeg libav architecture - decoding process](/img/decoding.png)

You'll first need to load your media file into a component called `AVFormatContext` (the video container is also known as format), it actually doesn't fully load the whole file, it only loads the header.

Once we loaded the minimal header of our container we can access its streams, think of them as a rudimentary audio and video data. Each stream will be available in a component called `AVStream`.

> Stream is a fancy name for a continuous flow of data.

Suppose our video has two streams: an audio encoded with AAC CODEC and a video encoded with H264 CODEC. From each stream we can extract pieces (slices) of data called packets that will be loaded into components named `AVPacket`.

The data inside the packets are still coded (compressed) and in order to decode the packets we need to pass them to a specific `AVCodec`.

The `AVCodec` will decoded them into `AVFrame` and finally this component gives us the uncompressed frame.  Noticed that the same terminology is used either for audio and video stream.

### Chapter 0 - code walkthrough

But let's talk code here, we'll skip some details but don't worry the file is available for you to play with it.
