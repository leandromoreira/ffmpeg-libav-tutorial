[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)
# 介绍

我正在找一个教程/书，教会我如何开始使用[FFmpeg](https://www.ffmpeg.org/)作为一个库（又名 libav），然后我发现[“如何写一个视频播放器不到1K行“](http://dranger.com/ffmpeg/)教程。
不幸的是它被弃用，所以我决定写这个。

在这里的大部分代码将在`c`中，**但不用担心**，您可以轻松理解并将其应用于您的首选语言。FFmpeg libav 对许多语言具有很多绑定，如：[python](https://mikeboers.github.io/PyAV/)、[go](https://github.com/imkira/go-libav)，甚至你的语言没有它，你仍然可以通过`ffi`（这里有一个[Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua)的案例）来支持它。

我们将从一个关于什么是视频，音频，编解码器和容器的快速课程开始，然后我们将进入关于如何使用`ffmpeg`命令行的速成课程，最后我们将编写代码，随意直接跳过 [学习FFmpeg libav的艰辛之路](https://github.com/leandromoreira/ffmpeg-libav-tutorial#learn-ffmpeg-libav-the-hard-way)。

有人曾经说过，互联网视频流是传统电视的未来，无论如何FFmpeg是值得研究的东西。

__目录__

* [介绍](#介绍)
  * [视频 - 你看到了什么！](#视频 - 你看到了什么！)
  * [音频 - 你听到什么！](#audio---what-you-listen)
  * [编解码器-数据收缩](#codec---shrinking-data)
  * [集装箱 - 一个音频和视频的舒适的地方](#container---a-comfy-place-for-audio-and-video)
* [FFmpeg - 命令行](#ffmpeg---command-line)
  * [FFmpeg 命令行工具 101](#ffmpeg-command-line-tool-101)
* [常见的视频操作](#common-video-operations)
  * [转码](#transcoding)
  * [Transmuxing](#transmuxing)
  * [Transrating](#transrating)
  * [Transsizing](#transsizing)
  * [奖金回合：自适应流媒体](#bonus-round-adaptive-streaming)
  * [去超越](#going-beyond)
* [学习 FFmpeg libav 的艰辛之路](#learn-ffmpeg-libav-the-hard-way)
  * [第0章 - 臭名昭着的“你好”世界](#chapter-0---the-infamous-hello-world)
    * [FFmpeg libav 架构](#ffmpeg-libav-architecture)
    * [第0章 - 代码演练](#chapter-0---code-walkthrough)

#  介绍
##  视频 - 你看到了什么！

如果你有一系列的图像序列，并以给定的频率改变它们，那么假设[每秒钟有24个图像](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)，那么就会产生一个[运动错觉](https://en.wikipedia.org/wiki/Persistence_of_vision)，总之这是视频背后的一个非常基本的想法，一系列运行的图像/帧以给定的速度。

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)

##  音频 - 你听到什么！

虽然一个无声的视频可以表达相当多的感觉，但添加声音会给体验带来更多的乐趣。

声音是一种通过空气（或气体、液体或固体）传播的压力波的振动。  

> 在数字音频系统中，麦克风将声音转换成模拟电信号，然后是模拟数字转换器(ADC) — [通常使用脉冲编码调制(PCM)](https://en.wikipedia.org/wiki/Pulse-code_modulation) 将模拟信号转换成数字信号。

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")

> https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg

##  编解码器-数据收缩
> 编解码器是一种电子电路或软件，可以**压缩或解压数字音频/视频**。它将原始(未压缩的)数字音频/视频转换成压缩格式，反之亦然。
> https://en.wikipedia.org/wiki/Video_codec

但如果我们选择在一个文件中打包数百万的图像，并称之为电影，我们可能会以巨大的文件结束。让我们算一算:

假设我们正在创造一个视频的分辨率1080 x 1920 x宽度（高度），我们将花3个字节每像素（最小点屏幕）编码的颜色（或[24位颜色](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29),给我们16777215不同的颜色），这段视频 `每秒24帧` ，时长 `30分钟` 。

```bash
toppf = 1080 * 1920 // 每帧的像素总数
cpp = 3 // 每像素成本
tis = 30 * 60 // 时间以秒为单位
fps = 24 // 每秒帧数

所需的存储 = tis * fps * toppf * cpp
```
这个视频需要我们存储大约 `250.28GB` 或 `1.11Gbps` 的带宽，这就是为什么我们需要使用一个编解码器。

##  集装箱 - 一个音频和视频的舒适的地方
> 容器或包装器格式是元文件格式，其规范描述了数据和元数据的不同元素如何共存于计算机文件中。
> https://en.wikipedia.org/wiki/Digital_container_format

一个**包含所有流的文件**（主要是音频和视频），它还提供同步和通用元数据，例如标题、分辨率等。通常，我们可以通过查看其扩展名来推断文件的格式，例如 ` video.webm` 可能是一个使用容器的视频 [webm](https://www.webmproject.org/)。

![container](/img/container.png)

##  FFmpeg - 命令行
> 一个完整的，跨平台的解决方案来记录，转换和流式音频和视频。

要使用多媒体，我们可以使用名为 [FFmpeg](https://www.ffmpeg.org/) 的 `AMAZING` 工具/库，您可能直接或间接知道/使用它（你使用 [Chrome?](https://www.chromium.org/developers/design-documents/video)）。

它有一个名为 `ffmpeg` 的命令行程序，它是一个非常简单但功能强大的二进制文件。 例如，只需输入follow命令，就可以从 `mp4` 转换到容器 `avi`。

```bash
$ ffmpeg -i input.mp4 output.avi
```
我们只是做了一个从一个容器转换到另一个容器的技术上，在这里 `FFmpeg` 也可以做一个转码，但我们稍后再谈。

##  FFmpeg命令行工具 101
`FFmpeg` 确实有一个[文档](https://www.ffmpeg.org/ffmpeg.html)，说明它是如何工作的。 为了简短起见，`FFmpeg` 命令行程序需要以下参数格式来执行其操作 `ffmpeg {1} {2} -i {3} {4} {5}`，其中：

1. 全局选项
2. 输入文件选项
3. 输入网址
4. 输出文件选项
5. 输出的url

第 2、3、4 和 5 部分可以根据需要设置。 理解这个参数格式在行动中更容易：

``` bash
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # global options
-c:a libfdk_aac -c:v libx264 \ # input options
-i bunny_1080p_60fps.mp4 \ # input url
-c:v libvpx-vp9 -c:a libvorbis \ # output options
bunny_1080p_60fps_vp9.webm # output url
```
这个命令包含一个包含两个流的输入文件 `mp4` ，一个音频编码的 `aac`  编解码器和一个使用 `h264`  编解码器编码的视频，并将其转换为 `webm`  改变它的音频和视频编解码器。

我们可以简化上面的命令，但是请注意 `FFmpeg` 将采用或猜测您的默认值，例如当你只是键入 `ffmpeg -i input.avi output.mp4` 它使用什么音频/视频编解码器来产生 `output.mp4` ？

`Werner Robitza` 写了一个必须读/执行 [关于使用FFmpeg进行编码和编辑的教程](http://slhck.info/ffmpeg-encoding-course/#/)。

#  常见的视频操作

在处理音频/视频时，我们通常使用媒体执行一系列任务。

## 转码

![transcoding](/img/transcoding.png)

**什么？** 将一个流（音频或视频）从一个编解码器转换为另一个编解码器的行为。  

**为什么？** 有时候一些设备（电视机，智能手机，控制台等）不支持X，但Y和更新的编解码器提供更好的压缩率。  

**怎么样？** 将 `H264` （AVC） 视频转换成 `H265` （HEVC）。  

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```
##  Transmuxing

![transmuxing](/img/transmuxing.png)

**什么？** 从一种格式（容器）转换到另一种格式的行为。  

**为什么？** 有时候一些设备（电视机，智能手机，控制台等）不支持X，但是有时新的容器提供了现代化的必需功能。  

**怎么样？** 将 `mp4` 转换成 `webm` 。

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # just saying to ffmpeg to skip encoding
bunny_1080p_60fps.webm
```

##  Transrating

![transrating](/img/transrating.png)

**什么？**  变比特率的行为，或者产生其他引用。  

**为什么？** 人们会尝试使用功能不太强大的智能手机或使用光纤互联网连接在 `4K` 电视上以`2G`（边缘）连接观看视频。因此，您应该提供不同比特率的相同视频的再现。  

**怎么样？** 产生 `3856K` 到 `2000K` 之间的比特率的再现 。  

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

通常我们会用 `transsizing` 进行翻译。 `Werner Robitza` 写了另外一篇 [关于FFmpeg速率控制的文章](http://slhck.info/posts/)。

## Transsizing

![transsizing](/img/transsizing.png)

**什么？** 从一个决议转换到另一个决议的行为，正如转换之前所说的那样，通常用于翻译。  

**为什么？** 原因与翻译相同。  

**怎么样？** 将`1080p` 转换为 `480p` 分辨率。

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

##  奖金回合：自适应流媒体

![adaptive streaming](/img/adaptive-streaming.png)

**什么？** 产生许多分辨率（比特率）的行为，并将媒体分成块并通过http服务。  

**为什么？** 为了提供可在低端智能手机或4K电视机上观看的灵活媒体，还可以轻松扩展和部署，但会增加延迟。  

**怎么样？** 使用 `DASH` 创建自适应 `WebM`。

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
PS：我从指令中偷取了这个例子，[使用DASH播放Adaptive WebM](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)。  

## 超越

FFmpeg有很多[FFmpeg的许多其他用法](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly)，我和iMovie一起使用它来为YouTube制作/编辑一些视频，你当然可以专业地使用它。

#  学习FFmpeg libav的艰辛之路

> 难道你不知道有时会发出声音和视觉吗？
> **David Robert Jones**

由于 `FFmpeg` 作为命令行工具非常有用，可以在媒体文件上执行大量任务，因此我们如何在程序中使用（嵌入）它？
事实证明，`FFmpeg` 本身是 [由几个库组成](https://www.ffmpeg.org/doxygen/trunk/index.html) ，可以被用来整合到我们自己的程序中。
通常当你安装FFmpeg时，它会自动安装所有这些库，我将把这些库的集合称为 **FFmpeg libav** 。
> 这个称号是对Zed Shaw系列的致敬 [学习X的艰难之路](https://learncodethehardway.org/)，特别是他的书“学习 c 艰苦的路”。

##  第0章 - 臭名昭着的“你好”世界

这个 `hello world` 实际上不会在终端显示 `“hello world”` 信息，而是打印出关于视频的信息，如格式（容器）、持续时间、分辨率、音频通道等等。 **将解码一些帧并保存为图像文件**。

###  FFmpeg libav 架构

但是在我们开始编写代码之前，我们先来了解一下 **FFmpeg libav 架构** 是如何工作的，以及它的组件如何与其他组件通信 例如，下面是一个解码视频的过程图。

![ffmpeg libav architecture - decoding process](/img/decoding.png)

您首先需要将媒体文件加载到名为[AVFormatContext](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html)（视频容器也称为格式）的组件中，但实际上并未完全加载整个文件，它通常只读取标题。

一旦我们加载了**我们的容器的最小头部**，我们就可以访问它的流，把它们想象成一个基本的音频和视频数据。 每个流将在一个名为 [AVStream](https://ffmpeg.org/doxygen/trunk/structAVStream.html) 的组件中可用。
> 流是连续数据流的一个奇特名称。

假设我们的视频有两个流：用一个编码的音频  [AAC CODEC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding) 和一个编码的视频  [H264 (AVC) CODEC](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC)。从每个流中，我们可以提取称为数据包的 **数据块（片）**，这些数据将被加载到名为的组件中 [AVPacket](https://ffmpeg.org/doxygen/trunk/structAVPacket.html)。

**数据包内的数据仍然是编码**（压缩），为了解码数据包，我们需要将它们传递给特定的数据包  [AVCodec](https://ffmpeg.org/doxygen/trunk/structAVCodec.html)。

`AVCodec` 将它们解码成 [AVFrame](https://ffmpeg.org/doxygen/trunk/structAVFrame.html) ，最后这个组件给了我们**未压缩的帧**。 注意，同样的术语/过程被音频和视频流所使用。


##  第0章 代码演练
> TLDR; 让我看看代码和执行。
> ```bash
> $ make download
> $ make cut_smaller_version
> $ make hello_world
> ```

但让我们来讨论一下代码，但请不要担心：[源代码已经同步在github](/0_hello_world.c)。
我们需要做的第一件事是注册所有的编解码器，格式和协议。
要做到这一点，我们只需要调用该函数 [`av_register_all`](http://ffmpeg.org/doxygen/trunk/group__lavf__core.html#ga917265caec45ef5a0646356ed1a507e3):
```c
av_register_all();
```

现在我们要分配内存给组件[`AVFormatContext`](http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html)将保存有关格式（容器）
```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```
现在我们打开文件并读取它的头文件并填写 `AVFormatContext` 关于格式的最少信息（注意通常编解码器没有打开）。
用来做这个的功能是 [`avformat_open_input`](http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49)。它期望一个`AVFormatContext`, 一个 `filename` 和两个可选参数：[`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html)（如果你传递的值`NULL`，FFmpeg会猜测格式）和 [`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html)（这是分流器的选项）。

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```
我们可以打印格式名称和媒体时长：

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```
要访问`流`，我们需要从媒体读取数据。通过函数 [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb)可以获取

```c
avformat_find_stream_info(pFormatContext,  NULL);
```
现在我们将遍历所有的流。

```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
  //
}
```
对于每一个流，我们要保持 [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html)，其中描述了流“i”使用的编解码器的属性。

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```
通过编解码器属性，我们可以查找正确的`编解码器`,函数 [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca)并找到注册的解码器的编解码器ID并返回一个[`AVCodec`](http://ffmpeg.org/doxygen/trunk/structAVCodec.html)，知道如何使用CO ** de和** DEC **的组件。

```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```
现在我们可以打印有关编解码器的信息。

```c
// specific for video and audio
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
  printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
  printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
}
// general
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pCodecParameters->bit_rate);
```
通过编解码器，我们可以为它分配内存 [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html)，这将为我们的解码/编码过程保留上下文，但是随后我们需要使用编解码器参数填充此编解码器上下文；我们这样做[`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16)。
一旦我们填充了编解码器上下文，我们需要打开编解码器。 我们称之为函数[`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d)，然后我们可以使用它。

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```
现在我们要从流中读取数据包并将它们解码为帧，但首先我们需要为这两个组件分配内存，[`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) and [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html)

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```
让我们用这个函数从流中提供我们的数据包[`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61)当有数据包时。

```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //...
}
```
让我们**发送原始数据包**（压缩帧）通过编解码器上下文，使用函数[`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)。

```c
avcodec_receive_frame(pCodecContext, pFrame);
```
我们可以打印帧号，[PTS](https://en.wikipedia.org/wiki/Presentation_timestamp), DTS, [frame type](https://en.wikipedia.org/wiki/Video_compression_picture_types) 和 etc。

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
最后，我们可以将解码的帧保存到一个[simple gray image](https://en.wikipedia.org/wiki/Netpbm_format#PGM_example)。这个过程非常简单，我们将使用`pFrame->data`那里的指数与之相关[planes Y, Cb and Cr](https://en.wikipedia.org/wiki/YCbCr)，我们刚刚采摘`0`（Y）保存我们的灰色图像。

```c
save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
```

瞧！ 现在我们有一个2MB的灰度图像：
![saved frame](/img/generated_frame.png)


## 第1章  同步音频和视频
> **Be the player** : 一位年轻的JS开发者正在写一个新的MSE视频播放器。

在我们搬到之前[code a transcoding example](#chapter-2---transcoding)，让我们来谈谈**timing（定时）**,或者视频播放器如何知道播放帧的正确时间。

在最后一个例子中，我们保存了一些可以在这里看到的框架：

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

当我们设计一个视频播放器时，我们需要**以给定的速度播放每一帧**，否则很难愉快地看视频，因为它播放的速度太快或太慢。

因此我们需要引入一些逻辑来平滑地播放每一帧。对于这个问题，每个框架都有一个**演示文稿时间戳** (PTS)这是一个越来越多的因素，**时基**这是一个有理数（分母是知道的**时间**）
**可以被帧速率（fps）**整除。
当我们看一些例子时，我们更容易理解，让我们模拟一些场景。
对于 `fps=60/1` 和 `timebase=1/60000`每个PTS将增加`timescale / fps = 1000`，因此每个帧的** PTS实时**可以是（假设它从0开始）：

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

对于几乎相同的情况，但时间等于`1/60`。

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
* `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

对于 `fps=25/1` 和 `timebase=1/75` 每个PTS将增加 `timescale / fps = 3` 并且 临时的时间可能是:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
* `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
* `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`
  现在与`pts_time`，我们可以找到一种方法来渲染与音频`pts_time`或系统时钟同步。 `FFmpeg libav`通过`API`提供这些信息：

- fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
- tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
- tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

出于好奇，我们保存的帧以DTS的顺序发送（帧：1,6,4,2,3,5），但是以PTS命令（帧：1,2,3,4,5）播放。 另外，请注意与P帧或I帧相比，B帧的便宜程度。

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


##  第2章  转码