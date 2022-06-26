[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

起初我在寻找可以学习使用FFmpeg库(又名 libav)的教程或书籍，然后找到了名为["如何在1k行代码内实现视频播放器"](http://dranger.com/ffmpeg/)的指南。但该项目已经停止维护，因此我决定撰写此教程。

此项目主要使用C语言开发，**但请不用担心**：项目内容非常通俗易懂。FFmpeg libav具有许多其他语言的实现，例如[python](https://pyav.org/)，[go](https://github.com/imkira/go-libav)。即使其中没有你熟悉的编程语言，仍然可以通过  `ffi` 为它提供支持（这是一个 [Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua) 的示例）。

下文将会简单介绍什么是视频、音频、编解码和容器，然后我们将尝试使用 FFmpeg 命令行工具，最终使用代码实现一些功能。如果你拥有一些经验，可以随时跳过这些内容，直接阅读 [笨办法学 FFmpeg libav](#笨办法学-FFmpeg-libav) 章节。

许多人认为网络视频流媒体是传统 TV 的未来。无论如何，FFmpeg 值得我们深入学习。

__目录__

* [介绍](#介绍)
  * [视频 - 目光所见](#视频---目光所见)
  * [音频 - 耳朵所听](#音频---耳朵所听)
  * [编解码 - 压缩数据](#编解码---压缩数据)
  * [容器 - 整合音频和视频](#容器---整合音视频)
* [FFmpeg - 命令行](#FFmpeg---命令行)
  * [FFmpeg 命令行工具 101](#FFmpeg-命令行工具-101)
* [通用视频操作](#通用视频操作)
  * [转码](#转码)
  * [转封装](#转封装)
  * [转码率](#转码率)
  * [转分辨率](#转分辨率)
  * [自适应流](#自适应流)
  * [更多](#更多)
* [笨办法学 FFmpeg libav](#笨办法学-FFmpeg-libav)
  * [章节0 - 臭名昭著的 hello world](#章节0---臭名昭著的-hello-world)
    * [FFmpeg libav 架构](#FFmpeg-libav-架构)
  * [章节1 - 音视频同步](#章节-1---音视频同步)
  * [章节2 - 重新封装](#章节-2---重新封装)
  * [章节3 - 转码](#章节-3---转码)

# 介绍

## 视频 - 目光所见

如果以一定的频率播放一组图片([比如每秒24张图片](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/))，人将会产生[视觉暂留现象](https://en.wikipedia.org/wiki/Persistence_of_vision)。
概括来讲，视频的本质就是: **以给定频率播放的一系列图片/帧**.

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

当代插画 (1886)

## 音频 - 耳朵所听

尽管一个没有声音的视频也可以表达很多感受和情绪，但加入音频会带来更多的体验乐趣。

声音是指以压力波形式通过空气或其他介质（例如气体、液体或者固体）传播的振动。

> 在数字音频系统中，麦克风将声音转换为模拟电信号，然后通常使用脉冲编码调制（[PCM](https://en.wikipedia.org/wiki/Pulse-code_modulation)）的模数转换器（ADC）将模拟信号转换为数字信号。

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")

>[图片来源](https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg)

## 编解码 - 压缩数据

> CODEC是用于压缩或解压缩数字音频/视频的硬件或软件。 它提供将原始（未压缩的）数字音频/视频与压缩格式相互转换的能力。
>
> https://en.wikipedia.org/wiki/Video_codec

如果我们选择打包数百万张图片来生成一个视频文件，那么该文件的大小将会非常惊人。让我们来计算一下：

假如我们创建一个 `1080x1920` (高x宽)的视频，每个像素占用 `3 bytes` 对颜色进行编码(或使用 [24 bit](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29) 真色彩, 这可以提供 16,777,216 种不同的颜色)，每秒 24 帧，视频时长为 30 分钟。

```c
toppf = 1080 * 1920 // 每帧所有的像素点
cpp = 3 // 每个像素的大小(bytes)
tis = 30 * 60 // 时长(秒)
fps = 24 // 每秒帧数

required_storage = tis * fps * toppf * cpp
```

计算结果显示，此视频需要大约 `250.28G` 的存储空间或 `1.19Gbps` 的带宽。这就是我们为什么需要使用 [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work) 的原因。

## 容器 - 整合音视频

> 容器或者封装格式描述了不同的数据元素和元数据是如何在计算机文件中共存的。
> https://en.wikipedia.org/wiki/Digital_container_format

**单个这样的文件包含所有的流**（主要是音频和视频），并提供**同步和通用元数据**，比如标题、分辨率等等。

一般我们可以通过文件的后缀来判断文件格式：比如 video.webm 通常是一个使用 [`webm`](https://www.webmproject.org/) 容器格式的视频。

![container](/img/container.png)

# FFmpeg - 命令行

> 这是一个完整的跨平台解决方案，可用于音视频的录制、转换和流式传输等。

我们使用非常优秀的工具/库 [FFmpeg](https://www.ffmpeg.org/) 来处理多媒体文件。你可能对它有些了解，也可能已经直接或者间接的在使用它了（你用过 [Chrome](https://www.chromium.org/developers/design-documents/video) 吗？）

`ffmpeg` 是该方案中简单而强大的命令行工具。例如，可以通过以下命令将一个 `mp4` 文件转换成 `avi` 格式：

```bash
$ ffmpeg -i input.mp4 output.avi
```

通过上述步骤，我们做了一次重新封装，从一个容器转换为另外一个容器。FFmpeg 也可以用于转码，我们稍后再针对它进行讨论。

## **FFmpeg 命令行工具 101**

FFmpeg 有一个非常完善的[文档](https://www.ffmpeg.org/ffmpeg.html)来介绍它是如何工作的。

简单来说，FFmpeg 命令行程序需要以下参数格式来执行操作： `ffmpeg {1} {2} -i {3} {4} {5}`，分别是:

1. 全局参数
2. 输入文件参数
3. 输入文件
4. 输出文件参数
5. 输出文件

选项 2、3、4、5 可以可以根据自己的需求进行添加。以下是一个易于理解的示例：

``` bash
# 警告：这个文件大约 300MB
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # 全局参数
-c:a libfdk_aac \ # 输入文件参数
-i bunny_1080p_60fps.mp4 \ # 输入文件
-c:v libvpx-vp9 -c:a libvorbis \ # 输出文件参数
bunny_1080p_60fps_vp9.webm # 输出文件
```

这个命令行作用是将一个 `mp4` 文件（包含了 `aac` 格式的音频流，`h264` 编码格式的视频流）转换为 `webm`，同时改变了音视频的编码格式。

我们可以简化上述命令行，但请注意 FFmpeg 会猜测或采用默认值。例如我们仅输入 `ffmpeg -i input.avi output.mp4` 时，FFmpeg 会使用哪种音频/视频编码来生成 `output.mp4` 呢？

Werner Robitza 写了一篇 [关于 ffmpeg 编码和编辑的教程](https://slhck.info/ffmpeg-encoding-course/#/)。

# 通用视频操作

在处理音频/视频时，我们通常会执行一系列操作。

## 转码

![transcoding](/img/transcoding.png)

**是什么?** 将其中一个流（视频流或音频流）从一种编码格式转换成另一种

**为什么?** 有时候有些设备（TV，智能手机等等）不支持 X ，但是支持 Y 和一些更新的编码方式，这些方式能提供更好的压缩比

**如何做?** 转换 `H264`（AVC）视频为 `H265`（HEVC）

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## 转封装

![transmuxing](/img/transmuxing.png)

**是什么?** 将视频/音频从某一种格式（容器）转换成另一种

**为什么?** 有时候有些设备（TV，智能手机等等）不支持 X ，但是支持 Y 和一些新的容器，这些格式提供了更现代的功能/特征

**如何做?** 转换一个 `mp4` 为 `ts`

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # 令 ffmpeg 跳过编解码过程
bunny_1080p_60fps.ts
```

## 转码率

![transrating](/img/transrating.png)

**是什么?** 改变码率或生成其他版本。

**为什么?** 有的人使用用较为落后的智能手机通过 `2G` (edge) 的网络连接来观看视频，有些人使用 4K 电视及光纤网络来观看视频，因此我们需要提供不同的码率的视频来满足不同的需求。

**如何做?** 生成视频码率在 3856k 和 2000K 之间的版本。

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

我们通常会同时使用改变码率和分辨率的操作。Werner Robitza 写了另一篇关于 [FFmpeg 码率控制](https://slhck.info/posts/) 的必知必会系列文章。

## 转分辨率

![transsizing](/img/transsizing.png)

**是什么?** 将视频从一种分辨率转为其他分辨率的操作。正如上文所述，改变分辨率的操作通常与改变码率的操作同时使用。

**为什么?** 原因与转码率相同，需要满足不同情况下的不同需求。

**如何做?** 将视频从 `1080p` 转换为  `480p` 

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## 自适应流

![adaptive streaming](/img/adaptive-streaming.png)

**是什么?** 生成很多不同分辨率/码率的视频并分块，通过http进行传输。

**为什么?** 为了在不同的终端和网络环境下提供更加灵活的观看体验，比如低端智能手机或者4K电视。这也使得扩展和部署更为简单方便，但是会增加延迟。

**如何做?** 用 DASH 创建一个自适应的 WebM。

```bash
# 视频流
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 160x90 -b:v 250k -keyint_min 150 -g 150 -an -f webm -dash 1 video_160x90_250k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 320x180 -b:v 500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_320x180_500k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 750k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_750k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 1000k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_1000k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 1280x720 -b:v 1500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_1280x720_1500k.webm

# 音频流
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:a libvorbis -b:a 128k -vn -f webm -dash 1 audio_128k.webm

# DASH 格式
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

PS: 该样例借鉴自 [使用 DASH 播放自适应 WebM](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)

## 更多

FFmpeg 还有很多[其他用法](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly)。我会利用 FFmpeg 结合 iMovie 为 YouTube 编辑视频，你当然也可以更专业地使用它。

# 笨办法学 FFmpeg libav

> Don't you wonder sometimes 'bout sound and vision?
> **David Robert Jones**

既然 [FFmpeg](#ffmpeg---command-line) 作为命令行工具对多媒体文件进行基本处理这么有效，那么我们如何在自己的程序里使用它呢？

FFmpeg 是由几个可以集成到程序里的[lib库](https://www.ffmpeg.org/doxygen/trunk/index.html)组成的。通常在安装FFmpeg时，会自动安装这些库。我们将这些库统一叫做 **FFmpeg libav**。

> 这个标题是对 Zed Shaw 的[笨办法学XX](https://learncodethehardway.org/)系列丛书的致敬，特别是笨办法学C语言。

## 章节0 - 臭名昭著的 hello world

这里说的 hello world 实际上不是在终端里输出 “hello world” :tongue:，而是**输出视频信息**，例如：格式、时长、分辨率、音频轨道，最后我们将**解码一些帧，并保存为图片**。


### FFmpeg libav 架构

在我们开始之前，我们需要先了解一下**FFmpeg libav 架构**的工作流程和各个组件之间的工作方式。

下面是一张视频解码的处理流程图：

![ffmpeg libav architecture - decoding process](/img/decoding.png)

首先，我们需要加载媒体文件到 [AVFormatContext](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) 组件（为便于理解，容器看作是文件格式即可）。这个过程并不是加载整个文件，它通常只是加载了文件头。

我们加载**容器的头部信息**后，就可以访问媒体文件流（流可以认为是基本的音频和视频数据）。每个流在 [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html) 组件中可用。

> 流是数据流的一个昵称

假设我们的视频文件包含两个流：一个是 [AAC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding) 音频流，一个是 [H264（AVC）](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC)视频流。我们可以从每一个流中提取出被称为数据包的数据片段（切片），这些数据包将被加载到 [AVPacket](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) 组件中。

**数据包中的数据仍然是被编码的**（被压缩），为了解码这些数据，我们需要将这些数据给到 [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html)。

`AVCodec` 将解码这些数据到 [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html)，最后我们将得到**解码后的帧**。注意，视频流和音频流共用此处理流程。

### 构建要求

由于有些人编译或者运行示例时会遇到许多[问题](https://github.com/leandromoreira/ffmpeg-libav-tutorial/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+compiling)，因此我们使用 `Docker` 来构建开发/运行环境。我们将使用一个 Big Buck Bunny 的视频来作为示例，如果你没有这个视频，运行 `make fetch_small_bunny_video` 来获取。

### 章节 0 - 代码一览

> 展示[代码](/0_hello_world.c)并执行。
>
> ```bash
> $ make run_hello
> ```

我们将跳过一些细节，不过不用担心，[代码](https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/0_hello_world.c)都在Github上维护。

我们首先为 [`AVFormatContext`](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) 分配内存，利用它可以获得相关格式（容器）的信息。

```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```

我们将打开一个文件并读取文件的头信息，利用相关格式的简要信息填充 `AVFormatContext`（注意，编解码器通常不会被打开）。需要使用 [`avformat_open_input`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49) 函数，该函数需要 `AVFormatContext`、文件名和两个可选参数：[`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html)（如果为NULL，FFmpeg将猜测格式）、[`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html)（解封装参数）。

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```

可以输出视频的格式和时长：

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```

为了访问数据流，我们需要从媒体文件中读取数据。需要利用函数 [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb)完成此步骤。`pFormatContext->nb_streams` 将获取所有的流信息，并且通过  `pFormatContext->streams[i]` 获取到指定的 `i` 数据流（[`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html))。

```c
avformat_find_stream_info(pFormatContext,  NULL);
```

可以使用循环来获取所有流数据：

```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
  //
}
```

针对每个流维护一个对应的 [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html)，该结构体描述了被编码流的各种属性。

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```

通过codec id和 [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca) 函数可以找到对应已经注册的解码器，返回 [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html) 指针，该组件能让我们知道如何编解码这个流。

```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```

现在可以输出一些编解码信息。

```c
// 用于视频和音频
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
  printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
  printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
}
// 通用
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pCodecParameters->bit_rate);
```

利用刚刚获取的 `AVCodec` 为 [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html) 分配内存，它将维护解码/编码过程的上下文。 然后需要使用 [`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16)和被编码流的参数(`AVCodecParameters`) 来填充 `AVCodecContext`。

完成上下文填充后，使用 [`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) 来打开解码器。

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```

现在我们将从流中读取数据包并将它们解码为帧。但首先，需要为  [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) 和 [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html) 分配内存。

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```

使用函数 [`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61) 读取帧数据来填充数据包。

```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //...
}
```

使用函数 [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3) 来把**原始数据包**（未解压的帧）发送给解码器。

```c
avcodec_send_packet(pCodecContext, pPacket);
```

使用函数 [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c) 从解码器接受原始数据帧（解压后的帧）。

```c
avcodec_receive_frame(pCodecContext, pFrame);
```

可以输出 frame 编号、[PTS](https://en.wikipedia.org/wiki/Presentation_timestamp)、DTS、[frame 类型](https://en.wikipedia.org/wiki/Video_compression_picture_types)等其他信息。

```c
printf(
    "Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d]",
    av_get_picture_type_char(pFrame->pict_type),
    pCodecContext->frame_number,
    pFrame->pts,
    pFrame->pkt_dts,
    pFrame->key_frame,
    pFrame->coded_picture_number,
    pFrame->display_picture_number
);
```

最后，我们可以将解码后的帧保存为[灰度图](https://en.wikipedia.org/wiki/Netpbm#PGM_example)。处理过程非常简单，使用 `pFrame->data`，它的索引与 [Y, Cb 和 Cr 分量](https://en.wikipedia.org/wiki/YCbCr) 相关联。我们只选择 `0`（Y 分量）数据保存灰度图。

```c
save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // 编写 pgm 格式所需的最小文件头
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // 逐行写入
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
```

现在将得到一张2MB大小的灰度图：

![saved frame](/img/generated_frame.png)

## 章节 1 - 音视频同步

> **Be the player** - 一个年轻 JS 开发者开发的新 MSE 视频播放器。

在我们学习 [重新封装](#章节-2---重新封装) 之前，我们来谈谈timing（时机/时间点），或者说播放器如何知道在正确的时间来播放每一帧。

在上一个例子中，我们保存了一些帧：

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

当我们在设计一个播放器的时候，需要**以给定的速度播放每一帧**。否则，我们很难获得好的体验，因为在观看的过程中很可能播放得太快或者太慢。

因此我们需要引入一些机制来流畅地播放每一帧。每一帧都将拥有一个**播放时间戳**（PTS）。它是一个将**timebase**（时基，FFmpeg中一种特殊的时间度量单位，**timescale**可以认为是它的倒数）作为单位的递增数字。

我们来模仿几个场景，通过以下示例可以更迅速地理解。

例如 `fps=60/1` ， `timebase=1/60000`，PTS 将以 `timescale / fps = 1000` 进行递增，因此每一帧对应的 PTS 如下（假设开始为0）:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

相同情况下，将 timebase 修改为 `1/60`：

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
* `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

如 `fps=25`，`timebase=1/75`，PTS 将以 `timescale / fps = 3` 进行递增，因此每一帧对应的 PTS 如下（假设开始为0）：

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
* `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
* `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`

通过 `pts_time`， 我们可以找到一种渲染它和音频的 `pts_time` 或系统时钟进行同步的方式。FFmpeg libav 提供了获取这些信息的接口：

- fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
- tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
- tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

被保存的帧按照 DTS 顺序发送(frames：1,6,4,2,3,5)，按照 PTS 顺序播放(frames：1,2,3,4,5)。同时，我们可以发现B帧相对于P帧和I帧压缩率更高，更加节省空间。

```
LOG: AVStream->r_frame_rate 60/1
LOG: AVStream->time_base 1/60000
...
LOG: Frame 1 (type=I, size=153797 bytes) pts 6000 key_frame 1 [DTS 0]
LOG: Frame 2 (type=B, size=8117 bytes) pts 7000 key_frame 0 [DTS 3]
LOG: Frame 3 (type=B, size=8226 bytes) pts 8000 key_frame 0 [DTS 4]
LOG: Frame 4 (type=B, size=17699 bytes) pts 9000 key_frame 0 [DTS 2]
LOG: Frame 5 (type=B, size=6253 bytes) pts 10000 key_frame 0 [DTS 5]
LOG: Frame 6 (type=P, size=34992 bytes) pts 11000 key_frame 0 [DTS 1]
```

## 章节 2 - 重新封装

重新封装是将文件从一种格式转换为另一种格式。例如：我们可以非常容易地利用 FFmpeg 将 [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4_Part_14) 格式的视频 转换成 [MPEG-TS](https://en.wikipedia.org/wiki/MPEG_transport_stream) 格式。

```bash
ffmpeg input.mp4 -c copy output.ts
```

以上命令将在不编解码的情况下（`-c copy`）来对 mp4 做解封装，然重新后封装为 `mpegts` 文件。如果不用 `-f` 参数来指定格式的话，ffmpeg 会根据文件扩展名来进行猜测。

FFmpeg 或 libav 的一般用法遵循以下模式/架构或工作流：

* **[协议层](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** -  接收一个输入（例如一个文件，也可以是 `rtmp` 或 `http`）
* **[格式层](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - 解封装数据内容，暴露出元数据和流信息
* **[编码层](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - 解码原始数据流 <sup>*可选*</sup>
* **[像素层](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)** - 可以对原始帧应用一些 `filters`（例如调整大小）<sup>*可选*</sup>
* 然后反过来做相同的操作
* **[编码层](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - 编码（重新编码或者转码）原始帧<sup>*可选*</sup>
* **[格式层](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - 封装（或重新封装）原始数据流（压缩数据）
* **[协议层](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - 将封装后数据输出 (另外的文件或远程服务器)

![ffmpeg libav workflow](/img/ffmpeg_libav_workflow.jpeg)

> 这张图的灵感来自 [Leixiaohua's](https://leixiaohua1020.github.io/#ffmpeg-development-examples) 和 [Slhck's](https://slhck.info/ffmpeg-encoding-course/#/9) 的作品。

现在我们将使用 libav 编写一个示例，完成与此命令行相同的效果:  `ffmpeg input.mp4 -c copy output.ts`

我们读取一个输入文件（`input_format_context`)，并且将修改保存至输出（`output_format_context`)。

```c
AVFormatContext *input_format_context = NULL;
AVFormatContext *output_format_context = NULL;
```

通常我们的做法是分配内存并打开输入文件。对于这个示例，我们将打开一个输入文件并为一个输出文件分配内存。

```c
if ((ret = avformat_open_input(&input_format_context, in_filename, NULL, NULL)) < 0) {
  fprintf(stderr, "Could not open input file '%s'", in_filename);
  goto end;
}
if ((ret = avformat_find_stream_info(input_format_context, NULL)) < 0) {
  fprintf(stderr, "Failed to retrieve input stream information");
  goto end;
}

avformat_alloc_output_context2(&output_format_context, NULL, NULL, out_filename);
if (!output_format_context) {
  fprintf(stderr, "Could not create output context\n");
  ret = AVERROR_UNKNOWN;
  goto end;
}
```

我们将重新封装视频、音频、字幕流，因此需要将用到的这些流存入一个数组中。

```c
number_of_streams = input_format_context->nb_streams;
streams_list = av_mallocz_array(number_of_streams, sizeof(*streams_list));
```

分配完所需要的内存之后，我们将遍历所有的流，然后利用 [avformat_new_stream](https://ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827) 为每一个流创建一个对应的输出流。注意，当前只需要针对视频、音频、字幕流进行处理。

```c
for (i = 0; i < input_format_context->nb_streams; i++) {
  AVStream *out_stream;
  AVStream *in_stream = input_format_context->streams[i];
  AVCodecParameters *in_codecpar = in_stream->codecpar;
  if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
      in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
      in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
    streams_list[i] = -1;
    continue;
  }
  streams_list[i] = stream_index++;
  out_stream = avformat_new_stream(output_format_context, NULL);
  if (!out_stream) {
    fprintf(stderr, "Failed allocating output stream\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }
  ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
  if (ret < 0) {
    fprintf(stderr, "Failed to copy codec parameters\n");
    goto end;
  }
}
```

现在，我们需要创建一个输出文件。

```c
if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
  ret = avio_open(&output_format_context->pb, out_filename, AVIO_FLAG_WRITE);
  if (ret < 0) {
    fprintf(stderr, "Could not open output file '%s'", out_filename);
    goto end;
  }
}

ret = avformat_write_header(output_format_context, NULL);
if (ret < 0) {
  fprintf(stderr, "Error occurred when opening output file\n");
  goto end;
}
```

完成上述操作之后，我们就可以将输入流逐个数据包复制到输出流。我们通过（`av_read_frame`）循环读取每一个数据包。对于每一数据包，我们都要重新计算 PTS 和 DTS，最终通过 `av_interleaved_write_frame` 写入输出格式的上下文。

```c
while (1) {
  AVStream *in_stream, *out_stream;
  ret = av_read_frame(input_format_context, &packet);
  if (ret < 0)
    break;
  in_stream  = input_format_context->streams[packet.stream_index];
  if (packet.stream_index >= number_of_streams || streams_list[packet.stream_index] < 0) {
    av_packet_unref(&packet);
    continue;
  }
  packet.stream_index = streams_list[packet.stream_index];
  out_stream = output_format_context->streams[packet.stream_index];
  /* 赋值数据包 */
  packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
  packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
  packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
  packet.pos = -1;

  //https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
  ret = av_interleaved_write_frame(output_format_context, &packet);
  if (ret < 0) {
    fprintf(stderr, "Error muxing packet\n");
    break;
  }
  av_packet_unref(&packet);
}
```

最后我们要使用函数 [av_write_trailer](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13) 输出文件尾。

```c
av_write_trailer(output_format_context);
```

现在可以进行测试了，首先我们将文件从 MP4 转换成 MPEG-TS 格式。使用 libav 来代替命令行 `ffmpeg input.mp4 -c copy output.ts `的作用。

```bash
make run_remuxing_ts
```

它起作用了！！！不相信吗？我们可以使用 ffprobe 来检测一下：

```bash
ffprobe -i remuxed_small_bunny_1080p_60fps.ts

Input #0, mpegts, from 'remuxed_small_bunny_1080p_60fps.ts':
  Duration: 00:00:10.03, start: 0.000000, bitrate: 2751 kb/s
  Program 1
    Metadata:
      service_name    : Service01
      service_provider: FFmpeg
    Stream #0:0[0x100]: Video: h264 (High) ([27][0][0][0] / 0x001B), yuv420p(progressive), 1920x1080 [SAR 1:1 DAR 16:9], 60 fps, 60 tbr, 90k tbn, 120 tbc
    Stream #0:1[0x101]: Audio: ac3 ([129][0][0][0] / 0x0081), 48000 Hz, 5.1(side), fltp, 320 kb/s
```

下图中总结了我们所做的工作，我们可以回顾一下之前关于[libav如何工作](https://github.com/leandromoreira/ffmpeg-libav-tutorial#ffmpeg-libav-architecture)的介绍。但图中也表明我们跳过了编解码的部分。

![remuxing libav components](/img/remuxing_libav_components.png)

在结束本章之前，我想展示一下重新封装中的一个重要功能 — — **使用选项**。比如我们想要 [MPEG-DASH](https://developer.mozilla.org/en-US/docs/Web/Apps/Fundamentals/Audio_and_video_delivery/Setting_up_adaptive_streaming_media_sources#MPEG-DASH_Encoding) 格式的文件，需要使用 [fragmented mp4](https://stackoverflow.com/a/35180327)（有时称为fmp4）而不是 MPEG-TS 或者普通的 MPEG-4。

使用[命令行](https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE#Fragmenting)可以简单地实现该功能：

```
ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
```

使用 libav 进行实现也非常简单，只需要在写入输出头时（复制数据包之前），传递相应选项即可。

```c
AVDictionary* opts = NULL;
av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
ret = avformat_write_header(output_format_context, &opts);
```

现在可以生成 fragmented mp4 文件：

```bash
make run_remuxing_fragmented_mp4
```

可以使用非常优秀的 [gpac/mp4box.js](https://gpac.github.io/mp4box.js/)，或者在线工具 [http://mp4parser.com/](http://mp4parser.com/) 来对比差异。首先加载普通mp4：

![mp4 boxes](/img/boxes_normal_mp4.png)

如你所见，`mdat` atom/box 是**存放实际音视频帧数据**的地方。现在我们加载 fragmented mp4，看看它是如何组织 `mdat` 的。

![fragmented mp4 boxes](/img/boxes_fragmente_mp4.png)

## 章节 3 - 转码

> #### 展示代码并执行
>
> ```bash
> $ make run_transcoding
> ```
>
> 我们将跳过一些细节，但是请不用担心：[代码](https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/3_transcoding.c)维护在 github。

在这一章，我们将用 C 写一个精简的转码器，使用 **FFmpeg/libav库**，特别是[libavcodec](https://ffmpeg.org/libavcodec.html)、libavformat 和 libavutil，将 H264 编码的视频转换为 H265。

![media transcoding flow](/img/transcoding_flow.png)

> 简单回顾：[**AVFormatContext**](https://www.ffmpeg.org/doxygen/trunk/structAVFormatContext.html) 是多媒体文件格式的抽象（例如：MKV，MP4，Webm，TS）。 [**AVStream**](https://www.ffmpeg.org/doxygen/trunk/structAVStream.html) 代表给定格式的数据类型（例如：音频，视频，字幕，元数据）。 [**AVPacket**](https://www.ffmpeg.org/doxygen/trunk/structAVPacket.html) 是从 `AVStream` 获得的压缩数据的切片，可由 [**AVCodec**](https://www.ffmpeg.org/doxygen/trunk/structAVCodec.html)（例如av1，h264，vp9，hevc）解码，从而生成称为 [**AVFrame**](https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html) 的原始数据。

### 转封装

我们将从简单的转封装操作开始，然后在此代码基础上进行构建，第一步需要**加载输入文件**。

```c
// 为 AVFormatContext 分配内存
avfc = avformat_alloc_context();
// 打开一个输入流并读取头信息
avformat_open_input(avfc, in_filename, NULL, NULL);
// 读取文件数据包以获取流信息
avformat_find_stream_info(avfc, NULL);
```

现在需要设置解码器，`AVFormatContext` 将使我们能够访问所有 `AVStream` 组件，获取它们的 `AVCodec` 并创建特定的 `AVCodecContext`，最后我们可以打开给定的编解码器进行解码。

>  [**AVCodecContext**](https://www.ffmpeg.org/doxygen/trunk/structAVCodecContext.html) 保存相关媒体文件的数据包括：码率，帧率，采样率，通道、高度等等。

```c
for (int i = 0; i < avfc->nb_streams; i++)
{
  AVStream *avs = avfc->streams[i];
  AVCodec *avc = avcodec_find_decoder(avs->codecpar->codec_id);
  AVCodecContext *avcc = avcodec_alloc_context3(*avc);
  avcodec_parameters_to_context(*avcc, avs->codecpar);
  avcodec_open2(*avcc, *avc, NULL);
}
```

现在我们需要准备输出文件，首先为 `AVFormatContext` **分配内存**。我们为为输出的格式创建**每一个流**。为了正确打包这些流，我们从解码器中**复制编解码参数**。

通过设置 `AV_CODEC_FLAG_GLOBAL_HEADER` 来告诉编码器可以使用这个全局头信息，最终打开输出文件写入文件头。

```c
avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);

AVStream *avs = avformat_new_stream(encoder_avfc, NULL);
avcodec_parameters_copy(avs->codecpar, decoder_avs->codecpar);

if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
  encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avio_open(&encoder_avfc->pb, encoder->filename, AVIO_FLAG_WRITE);
avformat_write_header(encoder->avfc, &muxer_opts);

```

我们从解码器获得 `AVPacket`，调整时间戳后写到输出文件。尽管 `av_interleaved_write_frame` 从函数名上来看是 “写入帧信息”，但我们实际是在存储数据包。最后通过写入文件尾来结束转封装操作。

```c
AVFrame *input_frame = av_frame_alloc();
AVPacket *input_packet = av_packet_alloc();

while (av_read_frame(decoder_avfc, input_packet) >= 0)
{
  av_packet_rescale_ts(input_packet, decoder_video_avs->time_base, encoder_video_avs->time_base);
  av_interleaved_write_frame(*avfc, input_packet) < 0));
}

av_write_trailer(encoder_avfc);
```

### 转码

前面的章节展示了一个转封装的程序，现在我们将添加对文件做编码的功能，具体是将视频从 `h264` 编码转换为 `h265`。

在我们设置解码器之后及准备输出文件之前，需要设置编码器。

* 使用 [`avformat_new_stream`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827) 和编码器创建 `AVStream`
* 使用名为 `libx265` 的 `AVCodec`，利用 [`avcodec_find_encoder_by_name`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37) 获取
* 利用 [`avcodec_alloc_context3`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315) 及编解码器创建 `AVCodecContext`
* 为编解码设置基础属性
* 打开编解码器，使用 [`avcodec_open2`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) 和 [`avcodec_parameters_from_context`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe) 将参数从上下文复制到流中

```c
AVRational input_framerate = av_guess_frame_rate(decoder_avfc, decoder_video_avs, NULL);
AVStream *video_avs = avformat_new_stream(encoder_avfc, NULL);

char *codec_name = "libx265";
char *codec_priv_key = "x265-params";
// 我们将对 x265 使用内置的参数
// 禁用场景切换并且把 GOP 调整为 60 帧
char *codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

AVCodec *video_avc = avcodec_find_encoder_by_name(codec_name);
AVCodecContext *video_avcc = avcodec_alloc_context3(video_avc);
// 编码参数
av_opt_set(sc->video_avcc->priv_data, codec_priv_key, codec_priv_value, 0);
video_avcc->height = decoder_ctx->height;
video_avcc->width = decoder_ctx->width;
video_avcc->pix_fmt = video_avc->pix_fmts[0];
// 控制码率
video_avcc->bit_rate = 2 * 1000 * 1000;
video_avcc->rc_buffer_size = 4 * 1000 * 1000;
video_avcc->rc_max_rate = 2 * 1000 * 1000;
video_avcc->rc_min_rate = 2.5 * 1000 * 1000;
// 时间基数
video_avcc->time_base = av_inv_q(input_framerate);
video_avs->time_base = sc->video_avcc->time_base;

avcodec_open2(sc->video_avcc, sc->video_avc, NULL);
avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
```

为了视频流转码，我们需要拓展解码的步骤：

- 利用 [`avcodec_send_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3) 发送空的 `AVPacket` 给解码器
- 利用 [`avcodec_receive_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c) 接收未压缩的 `AVFrame`
- 开始转码原始数据
- 使用 [`avcodec_send_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169) 发送原始数据
- 基于编解码器和 `AVPacket`，利用 [`avcodec_receive_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decodinghtml#ga5b8eff59cf259747cf0b31563e38ded6) 接受编码数据
- 设置时间戳，调用 [`av_packet_rescale_ts`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e)
- 写入输出文件 [`av_interleaved_write_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1)

```c
AVFrame *input_frame = av_frame_alloc();
AVPacket *input_packet = av_packet_alloc();

while (av_read_frame(decoder_avfc, input_packet) >= 0)
{
  int response = avcodec_send_packet(decoder_video_avcc, input_packet);
  while (response >= 0) {
    response = avcodec_receive_frame(decoder_video_avcc, input_frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      return response;
    }
    if (response >= 0) {
      encode(encoder_avfc, decoder_video_avs, encoder_video_avs, decoder_video_avcc, input_packet->stream_index);
    }
    av_frame_unref(input_frame);
  }
  av_packet_unref(input_packet);
}
av_write_trailer(encoder_avfc);

// used function
int encode(AVFormatContext *avfc, AVStream *dec_video_avs, AVStream *enc_video_avs, AVCodecContext video_avcc int index) {
  AVPacket *output_packet = av_packet_alloc();
  int response = avcodec_send_frame(video_avcc, input_frame);

  while (response >= 0) {
    response = avcodec_receive_packet(video_avcc, output_packet);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      return -1;
    }

    output_packet->stream_index = index;
    output_packet->duration = enc_video_avs->time_base.den / enc_video_avs->time_base.num / dec_video_avs->avg_frame_rate.num * dec_video_avs->avg_frame_rate.den;

    av_packet_rescale_ts(output_packet, dec_video_avs->time_base, enc_video_avs->time_base);
    response = av_interleaved_write_frame(avfc, output_packet);
  }
  av_packet_unref(output_packet);
  av_packet_free(&output_packet);
  return 0;
}

```

我们将媒体流从 `h264` 编码转换为 `h265`，和预期的一样，`h265` 编码的文件相较于 h264 更小。本次[创建的程序](/3_transcoding.c)能够完成以下转换：

```c
  /*
   * H264 -> H265
   * Audio -> remuxed (untouched)
   * MP4 - MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx265";
  sp.codec_priv_key = "x265-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

  /*
   * H264 -> H264 (fixed gop)
   * Audio -> remuxed (untouched)
   * MP4 - MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";

  /*
   * H264 -> H264 (fixed gop)
   * Audio -> remuxed (untouched)
   * MP4 - fragmented MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
  sp.muxer_opt_key = "movflags";
  sp.muxer_opt_value = "frag_keyframe+empty_moov+delay_moov+default_base_moof";

  /*
   * H264 -> H264 (fixed gop)
   * Audio -> AAC
   * MP4 - MPEG-TS
   */
  StreamingParams sp = {0};
  sp.copy_audio = 0;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
  sp.audio_codec = "aac";
  sp.output_extension = ".ts";

  /* WIP :P  -> it's not playing on VLC, the final bit rate is huge
   * H264 -> VP9
   * Audio -> Vorbis
   * MP4 - WebM
   */
  //StreamingParams sp = {0};
  //sp.copy_audio = 0;
  //sp.copy_video = 0;
  //sp.video_codec = "libvpx-vp9";
  //sp.audio_codec = "libvorbis";
  //sp.output_extension = ".webm";
```

> 老实说，完成这个教程[比我想象中的难](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54)，必须深入理解 [FFmpeg 命令行源码](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54#issuecomment-570746749)并进行大量测试。而且我想我肯定遗漏了一些细节，因为我必须强制执行 `force-cfr` 才能使 h264 正常工作，并且现在仍然会出现一些 warning 信息，例如 `warning messages (forced frame type (5) at 80 was changed to frame type (3))`。
