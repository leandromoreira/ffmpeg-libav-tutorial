[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

# Sound and Vision

> Don't you wonder sometimes 'bout sound and vision?
> **David Robert Jones**

Some people used to say that the Internet video streaming is the future of the traditional TV, in any case, the core of this evolution worths to be studied.

## video - what you see!

If you show a series of images in a given frequency, let's say [24 images per second](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/), you will create an [illusion of movement](https://en.wikipedia.org/wiki/Persistence_of_vision), in summary this is the very basic idea behind a video, **a series of pictures / frames running at a given rate**.

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

> Stream is a fancy name for a continuous set of data.

![container](/img/container.png)

# FFmpeg - command line

# Common video operations

## Transcoding
## Transmuxing
## Transrating
## Transsizing

# Libav - hello world
