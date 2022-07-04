[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

[FFmpeg](https://www.ffmpeg.org/)을 라이브러리처럼(a.k.a. libav) 사용하려면 어떻게 시작해야할지 알려줄만할 튜토리얼/책을 찾아봤었습니다. 그리고는 ["How to write a video player in less than 1k lines"](http://dranger.com/ffmpeg/) 라는 튜토리얼을 찾았죠.
하지만 안타깝게도 그건 더이상 관리가 안되고 있어서 이 글을 쓰기로 결정했습니다.

여기서 사용된 대부분의 코드는 C로 되어있습니다. **하지만 걱정하지 마세요**: 당신도 쉽게 이해할 것이고 선호하는 언어에도 적용하실 수 있을겁니다.
FFmpeg libav는 [python](https://pyav.org/), [go](https://github.com/imkira/go-libav)와 같은 다양한 언어로 된 많은 bindings을 제공합니다. 만약 사용하려는 언어에 그것이 없다면 `ffi`를 통해서도 지원할 수 있습니다. ([Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua) 예시)

우리는 비디오와 오디오, 코덱, 컨테이너가 무엇인지에 대해 빠르게 학습한 후에 `FFmpeg` 명령을 어떻게 사용하는지 대해서 파헤쳐보고 마지막으로 코드도 작성해볼 것입니다, [삽질하면서 FFmpeg libav 배우기](#삽질하면서-FFmpeg-libav-배우기) 섹션으로 바로 넘어가셔도 좋습니다.

혹자는 인터넷 비디오 스트리밍이 전통적인 TV의 미래라고 이야기하기도 합니다. 어떻게 되든 FFmpeg은 공부해둘만한 가치가 있는 것입니다.

__목차__

* [소개](#intro)
  * [비디오 - 당신이 무엇을 보는지!](#비디오---당신이-무엇을-보는지!)
  * [오디오 - 당신이 무엇을 듣는지!](#오디오---당신이-무엇을-듣는지!)
  * [코덱 - 데이터를 줄이기](#코덱---데이터를-줄이기)
  * [컨테이너 - 오디오와 비디오의 안식처](#[컨테이너---오디오와-비디오의-안식처)
* [FFmpeg - 명령줄 도구](#FFmpeg---명령줄-도구)
  * [FFmpeg 명령줄 도구 101](#FFmpeg-명령줄-도구-101)
* [공통 비디오 연산](#공통-비디오-연산)
  * [트랜스코딩 (Transcoding)](#트랜스코딩-(Transcoding))
  * [트랜스먹싱 (Transmuxing)](#트랜스먹싱-(Transmuxing))
  * [트랜스레이팅 (Transrating)](#트랜스레이팅-(Transrating))
  * [트랜스사이징 (Transsizing)](#트랜스사이징-(Transsizing))
  * [보너스: 적응형 스트리밍 (Adaptive Streaming)](#보너스:-적응형-스트리밍-(Adaptive-Streaming))
  * [더 들어가기](#더-들어가기)
* [삽질하면서 FFmpeg libav 배우기](#삽질하면서-FFmpeg-libav-배우기)
  * [챕터 0 - 악명 높은 hello world](#챕터-0---악명-높은-hello-world)
    * [FFmpeg libav 아키텍처](#FFmpeg-libav-아키텍처)
  * [챕터 1 - 타이밍 (timing)](#챕터-1---오디오와-비디오-동기화)
  * [챕터 2 - 리먹싱 (remuxing)](#챕터-2---리먹싱-(remuxing))
  * [챕터 3 - 트랜스코딩 (transcoding)](#챕터-3---트랜스코딩-(transcoding))

# 소개

## 비디오 - 당신이 무엇을 보는지!

만약 당신이 여러 연속된 이미지들을 가지고 있고 이것들을 주어진 주파수에 맞게 변화시킨다면 (이를테면 [초당 24장의 이미지](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)), [움직임의 잔상](https://en.wikipedia.org/wiki/Persistence_of_vision)을 만들게 될 것입니다.
요약하면 이게 비디오라는 것의 가장 기본적인 아이디어입니다: **정해진 속도에 맞게 돌아가는 연속된 사진들 / 프레임들**.

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)

## 오디오 - 당신이 무엇을 듣는지!

음소거된 비디오만으로도 다양한 감정들을 표현할 수는 있지만 여기에 소리를 더해준다면 훨씬 더 즐거운 경험을 가져다 줄 것입니다.  

소리는 공기 혹은 가스, 액체, 고체와 같은 다른 매체들을 통해 압력의 파동 형태로 전파되는 진동입니다.

> 디지털 오디오 시스템에서는 마이크가 소리를 아날로그 전기 신호로 전환하고, 아날로그-디지털 변환기 (ADC) - 보통 [펄스-부호 변조 (PCM)](https://en.wikipedia.org/wiki/Pulse-code_modulation)를 이용하여 - 아날로그 신호를 디지탈 신호로 변환합니다.

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")
>[원문](https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg)

## 코덱 - 데이터를 줄이기

> CODEC은 **디지털 오디오/비디오를 압축하거나 압축해제하는** 전자회로나 소프트웨어입니다. 이것은 raw (압축이안된) 디지털 오디오/비디오를 압축된 형태로 혹은 그 반대로 변환합니다.
> https://en.wikipedia.org/wiki/Video_codec

만약 우리가 수많은 이미지들을 차곡차곡 채워서 영화라고 부르는 하나의 파일로 만든다면, 결과적으로 엄청나게 큰 하나의 파일을 접하게 될 것 입니다. 한번 계산해봅시다: 

한번 가정해봅시다. 해상도가 `1080 x 1920` (높이 x 너비)인 비디오를 하나 만들건데 색을 인코딩하는데 픽셀당 `3 bytes` (화면의 최소 화소)를 쓸 것입니다. (혹은 [24비트 컬러](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29), 16,777,216개의 다른 색상을 제공) 그리고 이 비디오는 `초당 24프레임`으로 재생되고 `30분` 정도 길이입니다. 

```c
toppf = 1080 * 1920 //total_of_pixels_per_frame
cpp = 3 //cost_per_pixel
tis = 30 * 60 //time_in_seconds
fps = 24 //frames_per_second

required_storage = tis * fps * toppf * cpp
```

이 비디오는 거의 `250.28GB`의 저장 용량이 필요하며 `1.19Gbps`의 대역폭이 요구됩니다! 이것이 바로 우리가 [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work)을 사용해야하는 이유입니다.

## 컨테이너 - 오디오와 비디오의 안식처

> 컨테이너 혹은 래퍼(wrapper) 포맷은 데이터와 메타데이터의 다양한 요소들이 어떻게 하나의 컴퓨터 파일에 구성되어있는지를 기술하는 스펙을 담은 메타파일 포맷입니다.
> https://en.wikipedia.org/wiki/Digital_container_format

**하나의 파일이 모든 스트림을 담고 있고** (주로 오디오와 비디오) 이것은 또 동기화와 제목, 해상도 등과 같은 일반적인 메타데이터도 제공합니다.

보통 우리는 파일의 확장자를 보고 포맷을 유추할 수 있습니다: 예를들면 `video.webm`은 아마도 [`webm`](https://www.webmproject.org/)를 컨테이너로 사용하는 비디오겠죠.

![container](/img/container.png)

# FFmpeg - 명령줄 도구

> 오디오와 비디오를 녹화하고 변환하고 스트리밍할 수 있는 완전한 크로스-플랫폼 솔루션.

멀티미디어 작업을 한다면 우리는 [FFmpeg](https://www.ffmpeg.org/)이라고 하는 정말 쩌는 툴/라이브러리를 사용할 수 있습니다. 아마도 여러분도 이것을 직간접적으로 알고있거나/사용했던 기회가 있었을 것입니다. ([Chrome](https://www.chromium.org/developers/design-documents/video) 사용시죠?). 

이것은 `ffmpeg`이라고하는 아주 단순하지만 파워풀한 바이너리 형태의 명려줄 프로그램도 제공합니다.
예를들어, 아래 명령을 치는 것만으로도 컨테이너를 `mp4`에서 `avi`로 변환할 수 있습니다:

```bash
$ ffmpeg -i input.mp4 output.avi
```

우리는 방금 어떤 컨테이너에서 다른 컨테이너로 변환하는 과정인 **remuxing**을 해보았습니다.
기술적으로 FFmpeg은 트랜스코딩(transcoding)도 할 수 있습니다만 이것들에 대해서는 뒤에서 다시 이야기하겠습니다.

## FFmpeg 명령줄 도구 101

FFmpeg이 어떻게 동작하는지를 아주 잘 설명하고 있는 [문서](https://www.ffmpeg.org/ffmpeg.html)가 있습니다. 

간단히 정리하면, FFmpeg 명령줄 프로그램은 실행하기 위해 다음과 같은 형식의 인자를 갖춰야합니다 `ffmpeg {1} {2} -i {3} {4} {5}`, 여기서:

1. 전역 옵션
2. 입력 파일 옵션
3. 입력 url
4. 출력 파일 옵션
5. 출력 url

2, 3, 4, 5 부분은 필요한만큼 많아질 수 있습니다.
실제로 수행해보면 이 인자 형식을 더 쉽게 이해할 수 있습니다:

``` bash
# WARNING: this file is around 300MB
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # 전역 옵션
-c:a libfdk_aac \ # 입력 파일 옵션
-i bunny_1080p_60fps.mp4 \ # 입력 url
-c:v libvpx-vp9 -c:a libvorbis \ # 출력 파일 옵션
bunny_1080p_60fps_vp9.webm # 출력 url
```
이 명령은 두개의 스트림(`aac` 코덱으로 인코딩된 오디오와 `h264` 코덱으로 인코딩된 비디오)을 포함하는 `mp4`를 입력 파일로 받고 이를 `webm`으로 변환합니다, 물론 그 안의 오디오와 비디오 코덱들도 변환하고 있죠.

위의 명령을 더 단순화할 수도 있는데 그러면 FFmpeg이 기본값들을 사용하거나 추측하게될 것입니다.
예를들어 `ffmpeg -i input.avi output.mp4` 이렇게만 친다면 어떤 오디오/비디오 코덱이 `output.mp4`를 만들기 위해 사용될까요?

Werner Robitza가 작성한 꼭 읽고/실행해볼만한 [FFmpeg으로 인코딩하고 편집하는 것에 대한 튜토리얼](http://slhck.info/ffmpeg-encoding-course/#/)이 있습니다.

# 공통 비디오 연산

오디오/비디오 작업 중 보통 미디어에 대해 일련의 작업을 수행하게 됩니다.

## 트랜스코딩 (Transcoding)

![transcoding](/img/transcoding.png)

**무엇인가?** 스트림 (오디오 또는 비디오) 중에 하나를 기존 코덱에서 다른 코덱으로 변환하는 작업.

**왜?** 가끔 어떤 장치들은 (텔레비전, 스마트폰, 콘솔 등) X는 지원하지 않지만 Y를 지원합니다. 그리고 더 새로운 코덱들은 더 나은 압축률을 제공하기도 합니다.

**어떻게?** `H264` (AVC) 비디오를 `H265` (HEVC)로 변환하기.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## 트랜스먹싱 (Transmuxing)

![transmuxing](/img/transmuxing.png)

**무엇인가?** 하나의 포맷을 (컨테이너) 다른 포맷으로 변환하는 작업.

**왜?** 가끔 어떤 장치들은 (텔레비전, 스마트폰, 콘솔 등) X는 지원하지 않지만 Y를 지원합니다. 그리고 때때로 더 새로운 컨테이터들은 최신으로 요구되는 피처들을 제공합니다.

**어떻게?** `mp4`에서 `webm`으로 변환하기.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # just saying to ffmpeg to skip encoding
bunny_1080p_60fps.webm
```

## 트랜스레이팅 (Transrating)

![transrating](/img/transrating.png)

**무엇인가?** 비트레이트를 변환하거나 다른 변환본(renditions)을 만드는 작업.

**왜?** 사람들은 `2G` (edge)가 연결된 저사양의 스마트폰에서든 `광통신` 인터넷이 연결된 4K 텔레비전에든 당신의 비디오 볼 것이다. 그래서 같은 비디오라도 여러 비트레이트를 가진 하나 이상의 변환본을 제공해야합니다.

**어떻게?** 3856K와 2000K 사이의 비트레이트를 가진 변환본을 생성하기.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

보통 트랜스레이팅(transrating)은 트랜스사이징(transsizing)과 함께 사용합니다. Werner Robitza가 작성한 또 다른 필독/실행물 [FFmpeg rate 제어에 대한 연재 포스팅](http://slhck.info/posts/)가 있습니다.

## 트랜스사이징 (Transsizing)

![transsizing](/img/transsizing.png)

**무엇인가?** 하나의 해상도에서 다른 것으로 변환하는 작업. 이전에 언급한 것처럼 트랜스사이징(transsizing)은 주로 트랜스레이팅(transrating)과 함께 사용됩니다.

**왜?** 트랜스레이팅(transrating)에서의 이유와 동일함.

**어떻게?** `1080p`의 해상도를 `480p`로 변환하기.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## 보너스: 적응형 스트리밍 (Adaptive Streaming)

![adaptive streaming](/img/adaptive-streaming.png)

**무엇인가?** 다양한 (비트레이트의) 해상도를 생성하고 미디어들을 여러 청크로 나눠서 http를 통해 서비스하는 작업.

**왜?** 저사양 스마트폰 혹은 4K TV에서 시청할 수 있는 유연한 미디어를 제공하기 위해. 또한 이렇게 하면 확장이나 배포하기가 쉽습니다. 다만 지연시간이 생길 수 있습니다.

**어떻게?** DASH를 이용하여 적응형 WebM을 생성하기.
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

PS: 저는 이 예제를 [DASH를 이용한 Adaptive WebM 재생에 대한 지침](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)에서 가져왔습니다.

## 더 들어가기

[FFmpeg에 대한 아주 수많은 다른 사용방법들이](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly) 있습니다.
저는 이걸 YouTube 용 동영상들을 만들고/편집하는데 *iMovie*와 함께 사용합니다. 물론 여러분도 프로페셔널처럼 사용하실 수 있습니다.

# 삽질하면서 FFmpeg libav 배우기

> 가끔 '소리나는 것과 보이는 것이' 궁금하지 않으세요?
> **David Robert Jones**

[FFmpeg](#ffmpeg---command-line)는 미디어 파일들에 대한 필수 작업들을 수행하는 명령줄 도구로써 매우 유용합니다. 어떻게 우리의 프로그램에 이용할 수 있을까요?

FFmpeg는 우리의 프로그램에 통합될 수 있는 [여러 라이브러리들로 구성](https://www.ffmpeg.org/doxygen/trunk/index.html)되어있습니다.
보통, FFmpeg을 설치할때 이 모든 라이브러리들도 자동으로 설치됩니다. 이 라이브러리 모음들을 **FFmpeg libav**라고 해보죠.

> 이 제목은 Zed Shaw의 [Learn X the Hard Way](https://learncodethehardway.org/) 시리즈, 특히 그의 책 Learn C the Hard Way에 대한 오마주입니다.

## 챕터 0 - 악명 높은 hello world
이 hello world는 실제로 `"hello world"` 메시지를 터미널에 보여주진 않습니다. :tongue:
대신 우리는 **비디오의 정보를 출력**할 것입니다. 비디오의 포맷 (컨테이너), 길이, 해상도, 오디오 채널들 같은 것들을 말이죠. 그리고 마지막으로 **몇몇 프레임들을 디코딩하고 이미지 파일로 저장**해보겠습니다.

### FFmpeg libav 아키텍처

하지만 코딩을 시작하기 전에, **FFmpeg libav 아키텍처**가 어떻게 동작하는지 이것들의 컴포넌트들이 서로 어떻게 통신하는지를 배워봅시다. 

여기 비디오가 디코딩되는 프로세스를 담은 다이어그램이 하나 있습니다.

![ffmpeg libav architecture - decoding process](/img/decoding.png)

우선 여러분의 미디어 파일을 [`AVFormatContext`](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) (비디오 컨테이너는 포맷이라고도 합니다)라고 불리는 컴포넌트로 불러올 필요가 있습니다.
이건 사실 파일 전체를 불러오는건 아닙니다: 종종 헤더만을 읽죠.

일단 최소한의 **컨테이너 헤더**를 불러왔다면, 우리는 이것의 스트림 (기본적이고 필수적인 오디오와 비디오 데이터라고 간주하시면 됩니다)에 접근할 수 있습니다.
각 스트림은 [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html)라고 하는 컴포넌트로 접근 가능합니다.

> 스트림은 데이터의 연속적인 흐름을 의미하는 fancy한 이름입니다.

비디오가 두개의 스트림을 가지고 있다고 해봅시다: 오디오는 [AAC CODEC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding)로 인코딩되어있고 비디오는 [H264 (AVC) CODEC](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC)로 인코딩되어있습니다. 각 스트림으로부터 [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) 컴포넌트로 로드될 패킷이라 칭하는 **데이터의 조각들**을 추출할 수 있습니다.

**패킷안의 데이터는 여전히 인코딩되어 있습니다** (압축된상태). 이 패킷을 디코딩하기 위해서 우리는 이것들을 특정한 [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html)에 넘겨야합니다.

`AVCodec`은 그것들을 [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html)으로 디코딩하며 최종적으로 우리에게 **압축 해제된 프레임**을 넘겨줍니다. 오디오 및 비디오 스트림에서 동일한 용어/프로세스가 사용된다는 점을 유의하십시오.

### 요구 사항

간혹 예제를 컴파일하고 실행하는데 이슈들을 겪는 분들이 계셔서 **우리의 개발/실행 환경으로 [`Docker`](https://docs.docker.com/install/)를 사용할 것입니다,** 우리는 또한 big buck bunny 비디오를 사용할 것인데 따로 로컬에 가지고 있지 않다면 `make fetch_small_bunny_video` 명령만 실행해주시면 됩니다.

### 챕터 0 - 몸풀기 코드

> #### TLDR; [코드](/0_hello_world.c)랑 실행하는거나 보여주세요.
> ```bash
> $ make run_hello
> ```
> 좀 상세한 부분은 넘어가겠습니다. 그러나 걱정하진 마세요: [소스 코드는 github에 있습니다](/0_hello_world.c). 

포맷 (컨테이너)에 관한 정보를 담고 있는 [`AVFormatContext`](http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) 컴포넌트에게 메모리를 할당합니다.

```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```

이제 우리는 파일을 열고 헤더를 읽어서 `AVFormatContext`에 포맷에 관한 기본적인 정보를 채워줄 것입니다 (보통 코덱은 열리지 않음).
이를 위해 사용할 함수는 [`avformat_open_input`](http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49)입니다. 이 함수는 `AVFormatContext`와 `filename` 두개의 옵셔널 인자를 받습니다: [`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html) (`NULL`을 넘기면 FFmpeg이 포맷을 추측)과 [`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html) (demuxer에 대한 옵션)

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```

포맷 이름과 미디어 길이를 출력할 수 있습니다:

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```

`streams`에 접근하기 위해서는, 미디어로부터 데이터를 읽어야합니다. [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb) 함수가 그 일을 하죠.
`pFormatContext->nb_streams`가 스트림의 개수를 가지고 있고 `pFormatContext->streams[i]`는 `i`번째 스트림 ([`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html))을 반환합니다.

```c
avformat_find_stream_info(pFormatContext,  NULL);
```

이제 모든 스트림에 대해 루프를 돌아보겠습니다.

```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
  //
}
```

각 스트림에 대해서, `i`번째 스트림에 사용된 코덱 속성들을 담고있는 [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html)를 가져오겠습니다.

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```

이 코덱 속성을 이용하여 [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca) 함수를 통해 적절한 코덱을 찾을 수 있습니다. 코덱 id에 맞는 등록된 디코더를 찾고 스트림을 어떻게 en**CO**de와 **DEC**ode할지를 알고 있는 [`AVCodec`](http://ffmpeg.org/doxygen/trunk/structAVCodec.html) 컴포넌트를 반환받을 수 있습니다.

```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```

이제 코덱에 관한 정보를 출력할 수 있습니다.

```c
// specific for video and audio
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
  printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
  printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
}
// general
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
```

이 코덱을 기반으로 디코딩/인코딩 프로세스에 대한 컨텍스트를 담고있는 [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html)의 메모리를 할당할 수 있습니다. 그 다음 코덱 파라미터로 코덱 컨텍스트를 채워줍니다; [`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16)로 가능합니다.

일단 코덱 컨텍스트를 채웠다면 이제 코덱을 열 수 있습니다. [`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d)로 가능합니다. 

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```

이제 스트림으로부터 패킷을 읽고 디코딩하여 프레임으로 만들어볼 예정입니다. 그러나 우선, [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html)와 [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html) 두 컴포넌트에 대해 메모리 할당이 필요합니다.

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```

패킷이 존재하는 동안 루프를 돌면서 [`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61) 함수를 이용해 스트림으로부터 패킷을 받아오겠습니다. 

```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //...
}
```

코덱 컨텍스트를 [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3) 함수를 통해 디코더에 **raw 데이터 패킷 (압축된 프레임)을 보내**봅시다.

```c
avcodec_send_packet(pCodecContext, pPacket);
```

그리고 마찬가지로 코덱 컨텍스트를 [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c) 함수를 통해 디코더로부터 **raw 데이터 프레임 (압축 해제된 프레임)를 받아**봅시다.

```c
avcodec_receive_frame(pCodecContext, pFrame);
```

프레임 번호, [PTS](https://en.wikipedia.org/wiki/Presentation_timestamp), DTS, [프레임 타입](https://en.wikipedia.org/wiki/Video_compression_picture_types) 등을 출력해볼 수 있습니다.

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

마지막으로 디코딩된 프레임을 [심플 흑백 이미지](https://en.wikipedia.org/wiki/Netpbm_format#PGM_example)로 저장해볼 수 있습니다. 이 과정은 매우 단순합니다, 인덱스가 [planes Y, Cb, Cr](https://en.wikipedia.org/wiki/YCbCr)를 참조하고 있는 `pFrame->data`를 사용할 것입니다. 우리는 흑백 이미지를 저장하기 위해 `0` (Y) 인덱스를 선택했습니다.

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

voilà! 이제 우리는 2MB짜리 흑백 이미지를 얻어냈습니다:

![saved frame](/img/generated_frame.png)

## 챕터 1 - 오디오와 비디오 동기화

> **플레이어가 되세요** - 신규 MSE 비디오 플레이어를 작성 중인 젊은 JS 개발자

[트랜스코딩 예제 코드](#챕터-3---트랜스코딩-(transcoding))로 넘어가기 전에 **타이밍** 혹은 어떻게 비디오 플레이어가 하나의 프레임을 제시간에 재생해야하는지에 대해서 이야기해봅시다.

지난 예제에서, 우리는 이렇게 보이는 프레임들을 저장했습니다.

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

비디오 플레이어를 디자인 할때 **각 프레임을 주어진 속도에 재생**해야합니다, 그렇지 않으면 너무 빠르거나 너무 느리게 재생되기 때문에 비디오를 제대로 즐기기 어려울 것입니다.

그래서 뭔가 프레임을 원활하게 재생할 수 있는 로직을 소개할 필요가 있습니다. 이 이슈를 위해, 각 프레임은 **프리젠테이션 타임스탬프** (PTS)를 갖게 되는데 이것은 **프레임속도(fps)** 로 나누어지는 **타임베이스(timebase)** 라고 하는 유리수(분모가 **타임스케일(timescale)** 로 알려진)로 구성된(factored) 증가하는 숫자입니다.

예제를 좀 본다면 이해가 더 쉬울 것입니다, 몇개의 시나리오를 시뮬레이션해죠.

`fps=60/1` 이고 `timebase=1/60000` 라면 각 PTS는 `timescale / fps = 1000`를 증가할 것 입니다. 그래서 각 프레임의 **PTS 실제 시간**은 이렇게 됩니다 (0부터 시작한다고 하면):

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

동일한 시나리오지만 타임베이스가 `1/60`이라면.

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
* `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

`fps=25/1`와 `timebase=1/75`에 대해서는 각 PTS는 `timescale / fps = 3`만큼 증가할 것이고 PTS 시간은 이렇게 될 것 입니다:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
* `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
* `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`

이제 이 `pts_time`으로 오디오의 `pts_time` 혹은 시스템 시간과 동기화해서 재생할 방법을 찾을 수 있습니다. FFmpeg libav는 그 정보들을 아래 API를 통해 제공합니다.

- fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
- tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
- tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

호기심에 보자면, 우리가 저장했던 프레임들을 DTS 순으로 (frames: 1,6,4,2,3,5) 보내졌지만 재생은 PTS 순 (frames: 1,2,3,4,5)로 되었습니다. 또한, B-프레임이 P 혹은 I-프레임 대비 얼마나 저렴한지도 알 수 있죠.

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

## 챕터 2 - 리먹싱 (remuxing)

Remuxing은 하나의 포맷 (컨테이너)에서 다른 것으로 변경하는 작업입니다. 다음 예제처럼 FFmpeg을 쓰면 별로 어렵지 않게 [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4_Part_14) 비디오를 [MPEG-TS](https://en.wikipedia.org/wiki/MPEG_transport_stream)로 변경할 수 있습니다:

```bash
ffmpeg input.mp4 -c copy output.ts
```

이것은 mp4를 demux하지만 디코딩이나 인코딩은 하지 않습니다. (`-c copy`) 최종적으로 `mpegts` 파일로 mux할 것입니다. 만약 포맷을 의미하는 `-f`를 제공하지 않으면 ffmpeg은 파일 확장자로 포맷을 추측할 것입니다.

FFmpeg 혹은 libav의 일반적인 사용법은 아래 패턴/아키텍처 또는 워크플로우를 따릅니다: 
* **[프로토콜 레이어](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - `input`을 받음 (예를들면 `file`이지만 `rtmp` 또는 `HTTP` 입력도 가능).
* **[포맷 레이어](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - 컨텐츠를 `demuxes`, 대부분 메타데이터와 스트림을 열어봄
* **[코덱 레이어](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - 압축된 스트림 데이터를 `decodes` <sup>*optional*</sup>
* **[픽셀 레이어](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)** - raw 프레임에 대해 (리사이징 같은) `filters`를 적용할 수도 있음 <sup>*optional*</sup>
* and then it does the reverse path
* **[코덱 레이어](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - raw 프레임을 `encodes` (또는 `re-encodes` 혹은 `transcodes` 까지도) <sup>*optional*</sup>
* **[포맷 레이어](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - raw 스트림 (압축된 데이터)를 `muxes` (또는 `remuxes`)
* **[프로토콜 레이어](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - 그리고 마지막으로 muxed된 데이터를 `output`으로 전송 (또다른 파일 혹은 네트워크 원격 서버일 수도 있음)

![ffmpeg libav workflow](/img/ffmpeg_libav_workflow.jpeg)
> 이 그래프는 [Leixiaohua's](http://leixiaohua1020.github.io/#ffmpeg-development-examples)와 [Slhck's](https://slhck.info/ffmpeg-encoding-course/#/9)의 작업으로부터 큰 영감을 받은 것입니다.

자 이제 `ffmpeg input.mp4 -c copy output.ts`와 동일한 효과를 제공할 수 있도록 libav 를 이용한 예제를 하나 구현해봅시다.

입력 (`input_format_context`)으로부터 읽은 것을 다른 출력 (`output_format_context`)으로 변환해보겠습니다.

```c
AVFormatContext *input_format_context = NULL;
AVFormatContext *output_format_context = NULL;
```

일반적으로 메모리 할당을 시작하고 입력 포맷을 엽니다. 이번같은 특정한 경우에는, 입력 파일을 열고나서 출력 파일을 위한 메모리를 할당하겠습니다. 

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

비디오, 오디오, 자막 타입의 스트림만 remux할 것이며 사용하게될 스트림을 인덱스 배열에 들고 있겠습니다. 

```c
number_of_streams = input_format_context->nb_streams;
streams_list = av_mallocz_array(number_of_streams, sizeof(*streams_list));
```

필요한만큼의 메모리를 할당한 후, 모든 스트림에 대해 각각 루프를 돌면서 [avformat_new_stream](https://ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827) 함수를 통해 출력 포맷 컨텍스트에다가 새로운 출력 스트림을 생성해야합니다. 비디오, 오디오, 자막이 아닌 모든 스트림들에 대해서는 마킹을 해서 나중에 스킵할 수 있게 하겠습니다.

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

이제 출력 파일을 생성할 수 있습니다.

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

그런 후에, 입력 스트림에서 패킷을 하나씩 출력 스트림으로 복사하겠습니다. 패킷이 존재하는 동안 (`av_read_frame`), 각 패킷에 대해 PTS와 DTS를 다시 계산하고 마지막으로 포맷 컨텍스트에 (`av_interleaved_write_frame`) 씁니다.

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
  /* copy packet */
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

마무리를 위해 [av_write_trailer](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13) 함수를 통해 스트림 트레일러(trailer)를 출력 미디어 파일에 씁니다.

```c
av_write_trailer(output_format_context);
```

이제 테스트할 준비가 되었습니다. 첫번째 테스트는 MP4에서 MPEG-TS 비디오 파일로의 포맷 (비디오 컨테이너) 변환입니다. 우리는 기본적으로 `ffmpeg input.mp4 -c copy output.ts` 명령줄을 libav를 이용해 만든 것입니다.

```bash
make run_remuxing_ts
```

동작합니다!!! 절 믿지 않았나요?! 그러시면 안되죠, `ffprobe`로 한번 확인해보겠습니다:

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

우리가 했던 것을 그래프로 정리하기 위해, 초반 [libav의 동작 방식에 대한 아이디어](https://github.com/leandromoreira/ffmpeg-libav-tutorial#ffmpeg-li기bav-architecture)를 다시 한번 살펴보면 코덱 부분만 건너뛴걸 볼 수 있습니다.

![remuxing libav components](/img/remuxing_libav_components.png)

이 챕터를 끝내기 전에 리먹싱(remuxing) 프로세스의 중요한 부분을 보여드리고자 합니다, **muxer에 옵션을 줄 수 있다**는 것인데요. 만약에 전송을 [MPEG-DASH](https://developer.mozilla.org/en-US/docs/Web/Apps/Fundamentals/Audio_and_video_delivery/Setting_up_adaptive_streaming_media_sources#MPEG-DASH_Encoding) 포맷으로 하고 싶다면 MPEG-TS나 기본 MPEG-4 대신 (`fmp4`라고 부르는) [fragmented mp4](https://stackoverflow.com/a/35180327)를 사용해야합니다.

[명령줄로는 이렇게 쉽게 할 수 있습니다](https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE#Fragmenting).

```
ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
```

libav 버전도 거의 명령줄 만큼이나 쉽습니다. 패킷을 복사하기 바로 전, 출력 헤더를 쓸때 해당 옵션을 넘겨주기만 하면 됩니다. 

```c
AVDictionary* opts = NULL;
av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
ret = avformat_write_header(output_format_context, &opts);
```

이제 fragmented mp4 파일을 생성할 수 있습니다:

```bash
make run_remuxing_fragmented_mp4
```

제가 여러분께 거짓말하고 있지 않다는걸 보여드리죠. 결과물의 차이를 확인하기 위해 [gpac/mp4box.js](http://download.tsi.telecom-paristech.fr/gpac/mp4box.js/filereader.html) 혹은 [http://mp4parser.com/](http://mp4parser.com/) 같은 아주 훌륭한 사이트/툴을 이용할 수 있습니다. 일단 "common" mp4 파일을 로드해보세요. 

![mp4 boxes](/img/boxes_normal_mp4.png)

보시다시피 단 하나의 `mdat` 박스(atom)가 있습니다, **여기에 비디오와 오디오 프레임이 담겨있습니다**. 이번엔 fragmented mp4를 로드해서 `mdat` 박스가 어떻게 흩어져있는지 보시겠습니다.

![fragmented mp4 boxes](/img/boxes_fragmente_mp4.png)

## 챕터 3 - 트랜스코딩 (transcoding)

> #### TLDR; [코드](/3_transcoding.c)랑 실행하는거나 보여주세요.
> ```bash
> $ make run_transcoding
> ```
> 좀 상세한 부분은 넘어가겠습니다, 그러나 걱정하진 마세요: [소스 코드는 github에 있습니다](/3_transcoding.c). 

이번 챕터에서는 아주 간단한 트랜스코더를 만들어보겠습니다, C로 작성할 것이고, 이것으로 H264로 인코딩된 비디오를 H265로 변환할 수 있을겁니다. **FFmpeg/libav** 라이브러리, 특히 [libavcodec](https://ffmpeg.org/libavcodec.html), libavformat, libavutil를 이용하겠습니다. 

![media transcoding flow](/img/transcoding_flow.png)

> _빠르게 복습해보면:_ [**AVFormatContext**](https://www.ffmpeg.org/doxygen/trunk/structAVFormatContext.html)는 컨테이너 (ex: MKV, MP4, Webm, TS) 같은 미디어 파일 포맷에 대한 추상화 구조체입니다. [**AVStream**](https://www.ffmpeg.org/doxygen/trunk/structAVStream.html)는 주어진 포맷 (ex: 오디오, 비디오, 자막, 메타데이터)에 대한 각 데이터 유형을 나타냅니다. [**AVPacket**](https://www.ffmpeg.org/doxygen/trunk/structAVPacket.html)은 `AVStream`으로부터 얻어진 압축된 데이터의 조각입니다. 그리고 이것은 [**AVCodec**](https://www.ffmpeg.org/doxygen/trunk/structAVCodec.html) (ex: av1, h264, vp9, hevc)에 의해 디코딩되어 [**AVFrame**](https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html)라고 불리는 raw 데이터로 만들어집니다.

### 트랜스먹싱 (Transmuxing)

간단한 트랜스먹싱 작업을 시작해봅시다. 그리고나서 이 코드 기반으로 빌드할 수 있을겁니다. 첫번째 단계는 **입력 파일 로드하기**입니다.

```c
// Allocate an AVFormatContext
avfc = avformat_alloc_context();
// Open an input stream and read the header.
avformat_open_input(avfc, in_filename, NULL, NULL);
// Read packets of a media file to get stream information.
avformat_find_stream_info(avfc, NULL);
```

이제 디코더를 설정할 것인데, `AVFormatContext`가 모든 `AVStream` 컴포넌트에 접근할 수 있게 해줄 것입니다. 그리고 각각의 스트림에 대해서, `AVCodec`을 가져와서 특정 `AVCodecContext`를 생성합니다. 그리고 마지막으로 주어진 코덱을 열게되고 디코딩 프로세스를 수행할 수 있습니다.

> [**AVCodecContext**](https://www.ffmpeg.org/doxygen/trunk/structAVCodecContext.html)는 비트레이트, 프레임 속도, 샘플레이트, 채널, 높이 등과 같은 미디어 설정에 대한 데이터를 가지고 있습니다.

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

마찬가지로 트랜스먹싱에서도 출력 미디어 파일을 준비해둬야합니다, 우선 출력 `AVFormatContext`에 대해 **메모리를 할당**합니다. 이 출력 포맷에 **각 스트림**을 생성합니다. 스트림을 제대로 적재시키기 위해 디코더로부터 **코덱 파라미터를 복사**합니다.

인코더가 글로벌 헤더를 사용할 수 있도록 지정하는 `AV_CODEC_FLAG_GLOBAL_HEADER` **플래그를 설정**합니다. 그리고 출력으로 **쓰기 위한 파일**을 열고 헤더를 저장합니다.

```c
avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);

AVStream *avs = avformat_new_stream(encoder_avfc, NULL);
avcodec_parameters_copy(avs->codecpar, decoder_avs->codecpar);

if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
  encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avio_open(&encoder_avfc->pb, encoder->filename, AVIO_FLAG_WRITE);
avformat_write_header(encoder->avfc, &muxer_opts);

```

디코더로부터 `AVPacket`을 얻어서, 타임스탬프를 조정하고, 패킷을 출력 파일에 제대로 씁니다. `av_interleaved_write_frame` 이 함수 이름이 "write frame"라고 되어있긴 하지만 이것은 패킷을 저장합니다 . 이제 파일에 스트림 트레일러를 쓰면서 트랜스먹싱 프로세스를 마무리합니다. 

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

### 트랜스코딩 (Transcoding)

이전 섹션에서 간단한 트랜스먹서 프로그램을 봤는데요, 이번엔 여기에 인코딩을 기능을 추가해보겠습니다. 특히, `h264`에서 `h265`로 비디오를 트랜스코딩할 수 있게 하겠습니다.

디코더를 준비한 후, 그리고 출력 미디어 파일을 다루기 전에 인코더를 설정할 것입니다.

* 인코더에 비디오 `AVStream`를 생성합니다, [`avformat_new_stream`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827)
* `libx265`라고 하는 `AVCodec`을 사용합니다, [`avcodec_find_encoder_by_name`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37)
* 생성한 코덱을 기반으로 `AVCodecContext`를 생성합니다,[`avcodec_alloc_context3`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315)
* 트랜스코딩 세션에 대해 기본적인 속성들을 설정합니다, 그리고
* 코덱을 열고 컨텍스트에서 스트림으로 파라미터를 복사합니다. [`avcodec_open2`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d), [`avcodec_parameters_from_context`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe)

```c
AVRational input_framerate = av_guess_frame_rate(decoder_avfc, decoder_video_avs, NULL);
AVStream *video_avs = avformat_new_stream(encoder_avfc, NULL);

char *codec_name = "libx265";
char *codec_priv_key = "x265-params";
// we're going to use internal options for the x265
// it disables the scene change detection and fix then
// GOP on 60 frames.
char *codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

AVCodec *video_avc = avcodec_find_encoder_by_name(codec_name);
AVCodecContext *video_avcc = avcodec_alloc_context3(video_avc);
// encoder codec params
av_opt_set(sc->video_avcc->priv_data, codec_priv_key, codec_priv_value, 0);
video_avcc->height = decoder_ctx->height;
video_avcc->width = decoder_ctx->width;
video_avcc->pix_fmt = video_avc->pix_fmts[0];
// control rate
video_avcc->bit_rate = 2 * 1000 * 1000;
video_avcc->rc_buffer_size = 4 * 1000 * 1000;
video_avcc->rc_max_rate = 2 * 1000 * 1000;
video_avcc->rc_min_rate = 2.5 * 1000 * 1000;
// time base
video_avcc->time_base = av_inv_q(input_framerate);
video_avs->time_base = sc->video_avcc->time_base;

avcodec_open2(sc->video_avcc, sc->video_avc, NULL);
avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
```

비디오 스트림의 트랜스코딩을 위해 디코딩 루프를 확장해야합니다.

* 디코더에 빈 `AVPacket`를 전송합니다, [`avcodec_send_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3)
* 압축이 해제된 `AVFrame`를 받아옵니다, [`avcodec_receive_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)
* 이 raw 프레임의 트랜스코딩을 시작합니다,
* raw 프레임을 (인코더에) 보내고, [`avcodec_send_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169)
* 코덱에 맞게 압축된 `AVPacket`을 받아옵니다, [`avcodec_receive_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga5b8eff59cf259747cf0b31563e38ded6)
* 타임스탬프를 설정하고, [`av_packet_rescale_ts`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e)
* 패킷을 출력 파일에 씁니다. [`av_interleaved_write_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1)

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

아시다시피 `h265` 버전의 미디어 파일이 `h264`보다 사이즈가 작기 때문에 미디어 스트림을 `h264`에서 `h265`로 변환했습니다. 하지만 [작성한 프로그램](/3_transcoding.c)은 다음의 작업들도 수행할 수 있습니다:

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

> 이제서야 솔직히 말하자면, [제가 생각했던 것보다 더 삽질](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/53)했는데요. [FFmpeg 명령줄 소스코드](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54#issuecomment-570746749)를 파봐야했고 테스트도 엄청 돌려봤습니다. 그리고 제가 뭔가 놓치는게 있는 것 같은데요, 왜냐하면 `force-cfr`을 강제로 넣어줘야지만 `h264`가 작용하고 `warning messages (forced frame type (5) at 80 was changed to frame type (3))` 같은 경고 메시지도 여전히 나고 있기 때문이죠.
