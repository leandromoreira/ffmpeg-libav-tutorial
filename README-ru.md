[🇨🇳](/README-cn.md "Simplified Chinese")
[🇰🇷](/README-ko.md "Korean")
[🇪🇸](/README-es.md "Spanish")
[🇻🇳](/README-vn.md "Vietnamese")
[🇧🇷](/README-pt.md "Portuguese")

[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

Я искал туториал/книгу, которая научит использовать [FFmpeg](https://www.ffmpeg.org/) с помощью libav, и наткнулся на руководство ["How to write a video player in less than 1k lines"](http://dranger.com/ffmpeg/).
К сожалению, оно устарело, поэтому я решил написать этот гайд.

Большая часть кода здесь на C, **но не пугайтесь**: понять и перенести идеи на любимый язык несложно.
Для FFmpeg/libav есть биндинги ко многим языкам — например, [python](https://pyav.org/), [go](https://github.com/imkira/go-libav); даже если для вашего языка биндингов нет, можно подключиться через `ffi` (вот пример с [Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua)).

Мы начнём с краткого ликбеза: что такое видео, аудио, кодек и контейнер, затем вкратце пройдемся по командной строке `FFmpeg`, и, наконец, перейдём к коду. Если вам это неинтересно, смело пролистывайте прямо к разделу [Изучаем FFmpeg и libav трудным путем](#Изучаем FFmpeg и libav трудным путем) (да, ссылка с картинкой посреди фразы — это отсылка к мемам, не баг).

Некоторые люди раньше говорили, что видеостриминг в Интернете — будущее традиционного ТВ. Как бы то ни было, FFmpeg — вещь, которую стоит изучить.

**Оглавление**

* [Введение](#intro)

  * [видео — то, что ты видишь!](#видео--то-что-ты-видишь)
  * [аудио — то, что ты слышишь!](#аудио--то-что-ты-слышишь)
  * [кодек — сжатие данных](#кодек--сжатие-данных)
  * [контейнер — дом для аудио и видео](#контейнер--дом-для-аудио-и-видео)
* [FFmpeg — командная строка](#ffmpeg--командная-строка)

  * [FFmpeg CLI 101](#ffmpeg-cli-101)
* [Типовые операции с видео](#типовые-операции-с-видео)

  * [Transcoding (перекодирование)](#transcoding-перекодирование)
  * [Transmuxing (перемультиплексирование)](#transmuxing)
  * [Transrating (изменение битрейта)](#transrating-изменение-битрейта)
  * [Transsizing (изменение разрешения)](#transsizing)
  * [Бонус: адаптивный стриминг](#bonus-round-adaptive-streaming)
  * [Дальше — больше](#going-beyond)
* [Learning FFmpeg libav the hard way](#learn-ffmpeg-libav-the-hard-way)

  * [Глава 0 — печально известный hello world](#глава-0--печально-известный-hello-world)

    * [Архитектура FFmpeg libav](#архитектура-ffmpeg-libav)
  * [Глава 1 — тайминги/синхронизация](#глава-1--таймингисинхронизация)
  * [Глава 2 — remuxing](#глава-2--remuxing)
  * [Глава 3 — транскодирование](#глава-3--транскодирование)

# Введение

## видео — то, что ты видишь

Если последовательно показывать серию изображений с заданной частотой (скажем, [24 кадра в секунду](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)), возникнет [иллюзия движения](https://en.wikipedia.org/wiki/Persistence_of_vision).
В итоге получаем базовую идею, стояющую за видео: **ряд картинок/кадров, идущих с заданной частотой**.

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)

## аудио — то, что ты слышишь!

Хотя видео без звука может выражать самые разные чувства, добавление звука делает просмотр более приятным.

Звук — это вибрация, распространяющаяся как волна давления через воздух или любую другую среду — газ, жидкость, твёрдое тело.

> В цифровой аудиосистеме микрофон преобразует звук в аналоговый электрический сигнал, затем АЦП — обычно с использованием [PCM](https://en.wikipedia.org/wiki/Pulse-code_modulation) — превращает аналог в цифровой.

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")

> [Источник](https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg)

## кодек — сжатие данных

> CODEC — это электронная схема или ПО, которое **сжимает/распаковывает цифровое аудио/видео.** Оно переводит сырые (несжатые) данные в сжатый формат и обратно.
> [https://en.wikipedia.org/wiki/Video_codec](https://en.wikipedia.org/wiki/Video_codec)

Если же просто сложить миллионы картинок в один файл и назвать это фильмом, размер получится чудовищным. Посчитаем:

Пусть есть видео с разрешением `1080 x 1920` (высота × ширина), на каждый пиксель тратим `3 байта` (цвет в [24 битах](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29), т.е. 16 777 216 цветов), частота `24 кадра/с`, длительность `30 минут`.

```c
toppf = 1080 * 1920 // total_of_pixels_per_frame — всего пикселей на кадр
cpp = 3 // cost_per_pixel — байт на пиксель
tis = 30 * 60 // time_in_seconds — время в секундах
fps = 24 // frames_per_second — кадры в секунду

required_storage = tis * fps * toppf * cpp
```

Такое видео займёт примерно `250.28GB` или потребует `1.19 Gbps` пропускной способности! Поэтому нам и нужен [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work).

## контейнер — дом для аудио и видео

> Контейнер (wrapper format) — это метафайл-формат, спецификация которого описывает, как в одном компьютерном файле сосуществуют разные элементы данных и метаданных.
> [https://en.wikipedia.org/wiki/Digital_container_format](https://en.wikipedia.org/wiki/Digital_container_format)

**Один файл, содержащий все потоки** (обычно аудио и видео) и обеспечивающий **синхронизацию и общие метаданные** — название, разрешение и т.п.

Часто формат файла можно понять по расширению: например, `video.webm` — это, скорее всего, видео в контейнере [`webm`](https://www.webmproject.org/).

![container](/img/container.png)

# FFmpeg — командная строка

> Полноценное кроссплатформенное решение для записи, конвертации и стриминга аудио и видео.

Для работы с мультимедиа есть великолепный инструмент/библиотека [FFmpeg](https://www.ffmpeg.org/). Скорее всего, ты уже знаком с ним напрямую или косвенно (пользуешься [Chrome?](https://www.chromium.org/developers/design-documents/video)).

Есть CLI-программа `ffmpeg` — простая, но мощная.
Например, чтобы преобразовать контейнер `mp4` в `avi`, достаточно:

```bash
$ ffmpeg -i input.mp4 output.avi
```

Мы только что сделали **ремультиплексирование** (remuxing) — конвертировали один контейнер в другой.
Технически FFmpeg мог и перекодировать потоки, но об этом позже.

## FFmpeg CLI 101

У FFmpeg есть [документация](https://www.ffmpeg.org/ffmpeg.html), отлично объясняющая, как он работает.

```bash
# документацию можно смотреть и из командной строки

ffmpeg -h full | grep -A 10 -B 10 avoid_negative_ts
```

Если кратко, `ffmpeg` получает аргументы в формате `ffmpeg {1} {2} -i {3} {4} {5}`, где:

1. глобальные опции
2. опции входного файла
3. входной URL/путь
4. опции выходного файла
5. выходной URL/путь

Блоки 2, 3, 4 и 5 можно повторять сколько угодно.
Порядок аргументов проще понять на примере:

```bash
# ВНИМАНИЕ: файл около 300 МБ
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # глобальные опции
-c:a libfdk_aac \ # опции входа
-i bunny_1080p_60fps.mp4 \ # входной url
-c:v libvpx-vp9 -c:a libvorbis \ # опции выхода
bunny_1080p_60fps_vp9.webm # выходной url
```

Эта команда берёт на входе файл mp4 с двумя потоками (аудио закодированное кодеком aac и видео закодированное с помощью кодека h264) и конвертирует в webm, изменяя изначальные аудио, и видео кодеки.

Команду можно упростить, но тогда FFmpeg подставит/угадает значения по умолчанию.
Например, если написать `ffmpeg -i input.avi output.mp4`, то какой аудио/видео кодек будет выбран для `output.mp4`?

У Вернера Робитцы есть обязательный к прочтению/выполнению [курс по кодированию и редактированию в FFmpeg](http://slhck.info/ffmpeg-encoding-course/#/).

# Типовые операции с видео

Работая с аудио/видео, мы обычно решаем ряд типовых задач.

## Transcoding (перекодирование)

![transcoding](/img/transcoding.png)

**Что это?** преобразование одного из потоков (аудио или видео) из одного кодека в другой.

**Зачем?** иногда некоторые устройства (TV, смартфон, консоль и т.д.) не поддерживает кодек X, но поддерживает кодек Y; также новые кодеки нередко предоставляют лучшую степень сжатия.

**Как?** конвертируем видео `H264` (AVC) в `H265` (HEVC).

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## Transmuxing (перемультиплексирование)

![transmuxing](/img/transmuxing.png)

**Что это?** конвертация из одного формата (контейнера) в другой.

**Зачем?** некоторые устройства не поддерживают контейнер X, но поддерживают Y; новые контейнеры иногда дают современные фичи.

**Как?** конвертируем `mp4` в `ts`.

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # просим ffmpeg пропустить перекодирование
bunny_1080p_60fps.ts
```

## Transrating (изменение битрейта)

![transrating](/img/transrating.png)

**Что это?** изменение битрейта или создание альтернативных версий видео (renditions).

**Зачем?** кто-то будет смотреть через `2G` на слабом смартфоне, а кто-то — через `оптоволокно` на 4K-TV; сттоит предложить несколько версий одного видео с разным битрейтом под возможности зрителя.

**Как?** делаем версию с битрейтом между 964K и 3856K.

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

Обычно transrating используют вместе с transsizing. У Вернера Робитцы есть ещё одна отличная [серия постов о контроле битрейта в FFmpeg](http://slhck.info/posts/).

## Transsizing (изменение разрешения)

![transsizing](/img/transsizing.png)

**Что это?** изменение разрешения. Как уже сказано, transsizing часто идёт в паре с transrating.

**Зачем?** причины те же, что и для transrating.

**Как?** конвертируем `1080p` в `480p`.

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## Bonus Round: Adaptive Streaming

![adaptive streaming](/img/adaptive-streaming.png)

**Что это?** производим несколько разрешений (битрейтов), режем медиа на фрагменты и отдаём по HTTP.

**Зачем?** гибкость — чтобы обеспечить гибкий медиаконтент, который можно смотреть как на маломощном смартфоне, так и на 4K-телевизоре, его также легко масштабировать и развертывать, но это может добавить задержку.

**Как?** создаём адаптивное WebM через DASH.

```bash
# видеопотоки
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 160x90 -b:v 250k -keyint_min 150 -g 150 -an -f webm -dash 1 video_160x90_250k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 320x180 -b:v 500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_320x180_500k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 750k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_750k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 1000k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_1000k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 1280x720 -b:v 1500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_1280x720_1500k.webm

# аудиопотоки
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:a libvorbis -b:a 128k -vn -f webm -dash 1 audio_128k.webm

# DASH-манифест
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

PS: Я позаимствовал пример из [Instructions to playback Adaptive WebM using DASH](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)

## Дальше — больше

[Существует множество других способов использования FFmpeg.](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly).
Я использую его вместе с *iMovie* для создания/монтажа роликов для YouTube — и вы можете применять его профессионально.

# Learn FFmpeg libav the Hard Way

> Don't you wonder sometimes 'bout sound and vision?
> **David Robert Jones**

Раз уж [FFmpeg](#ffmpeg---command-line) настолько полезен в CLI для базовых операций с медиафайлами, как использовать его в своих программах?

FFmpeg — это [набор библиотек](https://www.ffmpeg.org/doxygen/trunk/index.html), которые можно интегрировать в свои приложения.
Обычно при установке FFmpeg ставятся и эти библиотеки. Я буду называть набор этих библиотек **FFmpeg libav**.

> Это название — дань уважения серии Zed Shaw [Learn X the Hard Way](https://learncodethehardway.org/), в частности его книге Learn C the Hard Way.

## Глава 0 — печально известный hello world

Этот hello world не выведет `"hello world"` в терминал :tongue:
Вместо этого мы **распечатаем информацию о видео** — формат (контейнер), длительность, разрешение, аудио каналы; и напоследок **декодируем несколько кадров и сохраним их как изображения**.

### Архитектура FFmpeg libav

Прежде чем писать код, разберёмся, **как устроена FFmpeg libav** и как разные компоненты взаимодействуют между собой.

Схема процесса декодирования видео:

![ffmpeg libav architecture - decoding process](/img/decoding.png)

Сначала нужно загрузить медиафайл в компонент [`AVFormatContext`](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) (контейнер также называют форматом).
На самом деле полностью файл не читается: зачастую парсится только заголовок.

После чтения минимального **заголовка контейнера** можно получить доступ к его потокам (думайте о них как о примитивных аудио и видеоданных).
Каждый поток доступен через компонент, называемый [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html).

> Stream — красивое слово для «непрерывного потока данных».

Допустим, у нашего видео есть два потока: аудио в [AAC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding) Кодеке и видео в [H264 (AVC)](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC). Из каждого потока мы извлекаем **кусочки данных**, называемые пакетами которые будут загружены в компонент называемый [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html).

**Данные в пакетах всё ещё закодированы** (сжаты), и чтобы их декодировать, передаём их соответствующему [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html).

`AVCodec` декодирует их в [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html), и, наконец, мы получаем **несжатый кадр**. Обрати внимание: терминология/процесс одинаковы и для аудио, и для видео.

### Требования

Поскольку некоторые сталкивались с [проблемами при сборке/запуске примеров](https://github.com/leandromoreira/ffmpeg-libav-tutorial/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+compiling), **мы будем использовать [`Docker`](https://docs.docker.com/install/) как окружение разработки/запуска**. Также используем ролик big buck bunny; если его нет локально, выполните `make fetch_small_bunny_video`.

### Глава 0 - пройдемся по коду

> #### TLDR; покажи [код](/0_hello_world.c) и как его запускать.
>
> ```bash
> $ make run_hello
> ```

Опустим некоторые детали, но не переживайте: [исходники на GitHub](/0_hello_world.c).

Выделим память под [`AVFormatContext`](http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html), который будет хранить данные о формате (контейнере).

```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```

Теперь откроем файл, прочитаем заголовок и заполним `AVFormatContext` минимальной информацией о формате (заметь, кодеки обычно не открываются).
Используем [`avformat_open_input`](http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49). На вход — `AVFormatContext`, `filename` и два необязательных аргумента: [`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html) (если `NULL`, FFmpeg угадает формат) и [`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html) (опции демультиплексора).

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```

Можно вывести название формата и длительность:

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```

Чтобы получить `streams`, нужно прочитать данные из медиа. Функция [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb) делает это.
Теперь `pFormatContext->nb_streams` — число потоков, а `pFormatContext->streams[i]` — сам `i`-й поток (`AVStream`).

```c
avformat_find_stream_info(pFormatContext,  NULL);
```

Пройдёмся по всем потокам:

```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
  //
}
```

Для каждого потока сохраним [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html) — свойства кодека, которым закодирован поток `i`.

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```

Зная свойства кодека, ищем подходящий декодер через [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca) — получаем зарегистрированный декодер по `codec_id`, т.е. [`AVCodec`](http://ffmpeg.org/doxygen/trunk/structAVCodec.html) — компонент, который умеет **enCO**de/**DEC**ode поток.

```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```

Теперь можно вывести информацию о кодеках.

```c
// видео и аудио
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
  printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
  printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->ch_layout.nb_channels, pLocalCodecParameters->sample_rate);
}
// общее
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
```

С кодеком можно выделить память под [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html) — контекст для процессов кодирования/декодирования, затем заполнить его параметрами кодека через [`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16).

После заполнения контекста — открыть кодек функцией [`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d).

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```

Теперь мы будем читать пакеты из потока и декодировать их в кадры, но сперва выделим память для [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) и [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html).

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```

Считываем пакеты из потоков через [`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61), пока они есть.

```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //...
}
```

**Отправляем сжатый пакет** (compressed frame) в декодер через контекст кодека — [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3).

```c
avcodec_send_packet(pCodecContext, pPacket);
```

И **получаем несжатый кадр** из декодера через тот же контекст — [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c).

```c
avcodec_receive_frame(pCodecContext, pFrame);
```

Можно вывести номер кадра, [PTS](https://en.wikipedia.org/wiki/Presentation_timestamp), DTS, [тип кадра](https://en.wikipedia.org/wiki/Video_compression_picture_types) и т.д.

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

И наконец можно сохранить декодированный кадр как [простое «серое» изображение](https://en.wikipedia.org/wiki/Netpbm_format#PGM_example). Всё просто: берём `pFrame->data`, где индексы соответствуют [плоскостям Y, Cb и Cr](https://en.wikipedia.org/wiki/YCbCr), и берём `0` (Y), чтобы сохранить градации серого.

```c
save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // пишем минимальный заголовок для формата pgm
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // пишем построчно
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
```

Voilà! У нас есть оттенки серого ~2 МБ:

![saved frame](/img/generated_frame.png)

## Глава 1 — тайминги/синхронизация

> **Будь плеером** — молодой JS-разработчик пишет новый MSE-видеоплеер.

Прежде чем [перейти к примеру транскодирования](#chapter-2---transcoding), поговорим про **тайминг** — как плеер понимает, когда показывать кадр.

В прошлом примере мы сохранили несколько кадров:

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

При разработке видеоплеера нужно **проигрывать каждый кадр в нужный момент** — иначе будет либо слишком быстро, либо слишком медленно.

Для плавного воспроизведения у каждого кадра есть **PTS (presentation timestamp)** — возрастающее число в **timebase** (рациональное число, где знаменатель — **timescale**), кратное **fps**.

Проще на примерах.

Для `fps=60/1` и `timebase=1/60000` PTS увеличивается на `timescale / fps = 1000`, значит **реальное время PTS** каждого кадра (если старт с 0):

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

Почти тот же сценарий с `timebase=1/60`:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = 0.016`
* `frame=2, PTS = 2, PTS_TIME = 0.033`
* `frame=3, PTS = 3, PTS_TIME = 0.050`

Для `fps=25/1` и `timebase=1/75` PTS растёт на `3`, времена:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = 0.04`
* `frame=2, PTS = 6, PTS_TIME = 0.08`
* `frame=3, PTS = 9, PTS_TIME = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = 162.56`

Зная `pts_time`, можно рендерить, синхронизируя с аудио `pts_time` или системными часами. FFmpeg libav отдаёт эти параметры через API:

* fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
* tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
* tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

К слову: кадры, которые мы сохранили, приходили в порядке DTS (1,6,4,2,3,5), но воспроизводились по PTS (1,2,3,4,5). И обрати внимание, насколько «дешевле» B-кадры по сравнению с P и I.

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

## Глава 2 — remuxing

Remuxing — смена формата (контейнера). Например, можно без боли поменять [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4_Part_14) на [MPEG-TS](https://en.wikipedia.org/wiki/MPEG_transport_stream) с FFmpeg:

```bash
ffmpeg input.mp4 -c copy output.ts
```

FFmpeg демультиплексирует mp4, но **не** декодирует/кодирует (`-c copy`), а в конце мультиплексирует в `mpegts`. Если не указать формат через `-f`, FFmpeg попытается угадать по расширению.

Общая архитектура использования FFmpeg/libav такова:

* **[protocol layer](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** — принимает `input` (например, `file`, но может быть `rtmp`/`HTTP`)
* **[format layer](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** — `demuxes` (демультиплексирует) содержимое, отдаёт метаданные и потоки
* **[codec layer](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** — `decodes` (декодирует) сжатые данные потоков <sup>*необязательно*</sup>
* **[pixel layer](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)** — применяет `filters` к сырым кадрам (например, масштабирование) <sup>*необязательно*</sup>
* потом обратный путь:
* **[codec layer](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** — `encodes`/`re-encodes`/`transcodes` сырые кадры <sup>*необязательно*</sup>
* **[format layer](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** — `muxes`/`remuxes` сжатые данные
* **[protocol layer](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** — выдаёт результат на `output` (файл или сеть)

![ffmpeg libav workflow](/img/ffmpeg_libav_workflow.jpeg)

> График вдохновлён работами [Leixiaohua](http://leixiaohua1020.github.io/#ffmpeg-development-examples) и [Slhck](https://slhck.info/ffmpeg-encoding-course/#/9).

Теперь напишем пример на libav, эквивалентный `ffmpeg input.mp4 -c copy output.ts`.

Мы читаем из входа (`input_format_context`) и пишем в выход (`output_format_context`).

```c
AVFormatContext *input_format_context = NULL;
AVFormatContext *output_format_context = NULL;
```

Сначала — обычные шаги: выделить память и открыть вход. В этом кейсе — открыть входной файл и выделить память под выходной.

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

Ремуксить будем только видео, аудио и субтитры, поэтому держим индексы нужных потоков в массиве.

```
number_of_streams = input_format_context->nb_streams;
streams_list = av_mallocz_array(number_of_streams, sizeof(*streams_list));
```

После выделения памяти проходим по всем потокам; для каждого создаём выходной поток в `output_format_context` через [avformat_new_stream](https://ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827). Потоки не типа видео/аудио/субтитры помечаем, чтобы их потом пропустить.

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

Теперь создаём выходной файл.

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

После этого копируем потоки пакет за пакетом из входа в выход. В цикле, пока есть пакеты (`av_read_frame`): пересчитываем PTS/DTS и пишем (`av_interleaved_write_frame`) в выходной контекст.

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
  /* копируем пакет */
  packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
  packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
  packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
  packet.pos = -1;

  // https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
  ret = av_interleaved_write_frame(output_format_context, &packet);
  if (ret < 0) {
    fprintf(stderr, "Error muxing packet\n");
    break;
  }
  av_packet_unref(&packet);
}
```

Чтобы завершить, записываем «хвост» потока в файл функцией [av_write_trailer](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13).

```c
av_write_trailer(output_format_context);
```

Теперь протестируем — первый тест: смена контейнера из MP4 в MPEG-TS. По сути, повторяем `ffmpeg input.mp4 -c copy output.ts` на libav.

```bash
make run_remuxing_ts
```

Работает!!! Не верите? И правильно — проверим с помощью `ffprobe`:

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

Итог на картинке: возвращаемся к [идее архитектуры libav](https://github.com/leandromoreira/ffmpeg-libav-tutorial#ffmpeg-libav-architecture), но показываем, что кодек-слой мы пропустили.

![remuxing libav components](/img/remuxing_libav_components.png)

Перед закрытием главы — важный момент: **можно передавать опции мультиплексору**. Допустим, хотим выдавать [MPEG-DASH](https://developer.mozilla.org/en-US/docs/Web/Apps/Fundamentals/Audio_and_video_delivery/Setting_up_adaptive_streaming_media_sources#MPEG-DASH_Encoding); для этого нужен [fragmented mp4](https://stackoverflow.com/a/35180327) (`fmp4`) вместо MPEG-TS или обычного MP4.

Через CLI это просто:

```
ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
```

Почти столь же просто — на libav: передаём опции при записи заголовка (до копирования пакетов).

```c
AVDictionary* opts = NULL;
av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
ret = avformat_write_header(output_format_context, &opts);
```

Теперь можно сгенерировать fragmented mp4:

```bash
make run_remuxing_fragmented_mp4
```

Чтобы убедиться, что я не вру, можно воспользоваться шикарными тулзами [gpac/mp4box.js](http://download.tsi.telecom-paristech.fr/gpac/mp4box.js/filereader.html) или [http://mp4parser.com/](http://mp4parser.com/). Сначала загрузите «обычный» mp4.

![mp4 boxes](/img/boxes_normal_mp4.png)

Видим один `mdat` атом/бокс — **здесь лежат видео/аудио кадры**. Теперь загрузите fragmented mp4 и увидите, что `mdat` разбит на части.

![fragmented mp4 boxes](/img/boxes_fragmente_mp4.png)

## Глава 3 — транскодирование

> #### TLDR; покажи [код](/3_transcoding.c) и как его запускать.
>
> ```bash
> $ make run_transcoding
> ```
>
> Детали частично опустим — [исходники на GitHub](/3_transcoding.c).

В этой главе создадим минималистичный транскодер на C, который конвертирует видео из H264 в H265, используя **FFmpeg/libav**: [libavcodec](https://ffmpeg.org/libavcodec.html), libavformat и libavutil.

![media transcoding flow](/img/transcoding_flow.png)

> *Короткий повтор:* [**AVFormatContext**](https://www.ffmpeg.org/doxygen/trunk/structAVFormatContext.html) — абстракция формата (контейнера: MKV, MP4, WebM, TS). [**AVStream**](https://www.ffmpeg.org/doxygen/trunk/structAVStream.html) — тип данных в формате (audio, video, subtitle, metadata). [**AVPacket**](https://www.ffmpeg.org/doxygen/trunk/structAVPacket.html) — фрагмент сжатых данных из `AVStream`, который декодируется [**AVCodec**](https://www.ffmpeg.org/doxygen/trunk/structAVCodec.html) (av1, h264, vp9, hevc) в сырые [**AVFrame**](https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html).

### Transmuxing

Начнём с простого ремукса, затем просто дополним его. Первый шаг — **загрузка входного файла**.

```c
// Выделяем AVFormatContext
avfc = avformat_alloc_context();
// Открываем вход и читаем заголовок.
avformat_open_input(avfc, in_filename, NULL, NULL);
// Читаем пакеты, чтобы получить информацию о потоках.
avformat_find_stream_info(avfc, NULL);
```

Далее готовим декодер: `AVFormatContext` даёт доступ ко всем `AVStream`. Для каждого находим `AVCodec`, создаём `AVCodecContext` и открываем кодек — после этого можно декодировать.

> [**AVCodecContext**](https://www.ffmpeg.org/doxygen/trunk/structAVCodecContext.html) хранит конфигурацию медиа: битрейт, fps, sample rate, channels, высоту/ширину и многое другое.

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

Нужно подготовить и выходной медиафайл для ремакса: **выделяем память** под `AVFormatContext` для выхода. Создаём **каждый поток** в выходном формате. Чтобы корректно упаковалось, **копируем параметры кодека** из декодера.

Ставим флаг `AV_CODEC_FLAG_GLOBAL_HEADER`, говоря энкодеру использовать глобальные заголовки, затем открываем **файл на запись** и сохраняем заголовки.

```c
avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);

AVStream *avs = avformat_new_stream(encoder_avfc, NULL);
avcodec_parameters_copy(avs->codecpar, decoder_avs->codecpar);

if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
  encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avio_open(&encoder_avfc->pb, encoder->filename, AVIO_FLAG_WRITE);
avformat_write_header(encoder->avfc, &muxer_opts);
```

Мы берём `AVPacket` из декодера, корректируем таймстампы и записываем его в выходной файл. Несмотря на название `av_interleaved_write_frame`, записывается пакет. Завершаем ремакс записью трейлера.

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

### Transcoding

В предыдущем разделе был простой ремуксер. Теперь добавим кодирование — научим программу транскодировать видео `h264` → `h265`.

После подготовки декодера, но до настройки выходного медиафайла — настраиваем энкодер.

* Создаём видео-`AVStream` в энкодере, [`avformat_new_stream`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827)
* Берём `AVCodec` по имени `libx265`, [`avcodec_find_encoder_by_name`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37)
* Создаём `AVCodecContext` для этого кодека, [`avcodec_alloc_context3`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315)
* Настраиваем базовые параметры сессии
* Открываем кодек и копируем параметры из контекста в поток: [`avcodec_open2`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d), [`avcodec_parameters_from_context`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe)

```c
AVRational input_framerate = av_guess_frame_rate(decoder_avfc, decoder_video_avs, NULL);
AVStream *video_avs = avformat_new_stream(encoder_avfc, NULL);

char *codec_name = "libx265";
char *codec_priv_key = "x265-params";
// используем внутренние опции x265:
// отключаем детекцию смены сцены и фиксируем
// GOP на 60 кадров.
char *codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

AVCodec *video_avc = avcodec_find_encoder_by_name(codec_name);
AVCodecContext *video_avcc = avcodec_alloc_context3(video_avc);
// параметры энкодера
av_opt_set(sc->video_avcc->priv_data, codec_priv_key, codec_priv_value, 0);
video_avcc->height = decoder_ctx->height;
video_avcc->width  = decoder_ctx->width;
video_avcc->pix_fmt = video_avc->pix_fmts[0];
// контроль битрейта
video_avcc->bit_rate       = 2 * 1000 * 1000;
video_avcc->rc_buffer_size = 4 * 1000 * 1000;
video_avcc->rc_max_rate    = 2 * 1000 * 1000;
video_avcc->rc_min_rate    = 2.5 * 1000 * 1000;
// time base
video_avcc->time_base = av_inv_q(input_framerate);
video_avs->time_base  = sc->video_avcc->time_base;

avcodec_open2(sc->video_avcc, sc->video_avc, NULL);
avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
```

Расширим цикл декодирования для видеотранскодирования:

* Отправляем входной `AVPacket` в декодер — [`avcodec_send_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3)
* Получаем сырой `AVFrame` — [`avcodec_receive_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)
* Начинаем кодировать этот сырой кадр,
* Отправляем кадр — [`avcodec_send_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169)
* Получаем сжатый `AVPacket` — [`avcodec_receive_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga5b8eff59cf259747cf0b31563e38ded6)
* Выставляем таймстампы — [`av_packet_rescale_ts`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e)
* Пишем в выходной файл — [`av_interleaved_write_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1)

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

// используемая функция
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

Мы конвертировали видеопоток `h264` → `h265`. Как и ожидалось, `h265`-версия меньше `h264`. При этом [написанная программа](/3_transcoding.c) умеет проводить следующие операции:

```c

  /*
   * H264 -> H265
   * Audio -> remuxed (без изменений)
   * MP4 - MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx265";
  sp.codec_priv_key = "x265-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

  /*
   * H264 -> H264 (фиксированный GOP)
   * Audio -> remuxed (без изменений)
   * MP4 - MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";

  /*
   * H264 -> H264 (фиксированный GOP)
   * Audio -> remuxed (без изменений)
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
   * H264 -> H264 (фиксированный GOP)
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

  /* В работе :P  -> не играет во VLC, итоговый битрейт огромен
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

> Честно говоря, это оказалось [сложнее, чем я думал](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54): пришлось копать [исходники FFmpeg CLI](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54#issuecomment-570746749) и много тестировать. Похоже, я ещё что-то упускаю: пришлось включить `force-cfr`, чтобы `h264` заработал, и я всё ещё вижу предупреждения вроде `warning messages (forced frame type (5) at 80 was changed to frame type (3))`.
