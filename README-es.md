[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)




Estaba buscando un tutorial/libro que pudiera enseñarme como usar [FFmpeg](https://www.ffmpeg.org/) como una librería (alias libav) y encontré el tutorial de ["How to write a video player in less than 1k lines"](http://dranger.com/ffmpeg/). Desafortunadamente estaba obsoleto, así que decidí escribir el siguiente tutorial.



La mayoría del código aquí estará en C, **pero no te preocupes**: tu podrás entenderlo fácilmente y aplicarlo a tu lenguaje preferido. FFmpeg libav tiene montones de bindings para muchos lenguajes como [python](https://pyav.org/), [go](https://github.com/imkira/go-libav) e incluso si tu lenguaje no lo tiene, aún es posible darle soporte mediante `ffi` (aquí hay un ejemplo en [Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua)).

Empezaremos con una lección rápida de lo que es video, audio, códec y contenedor,  entonces iremos a un curso rápido en como usar el comando `FFmpeg` y finalmente, escribiremos algo de código, siéntete libre de saltar directamente[ ](http://newmediarockstars.com/wp-content/uploads/2015/11/nintendo-direct-iwata.jpg "Secret Leandro´s Easter Egg")a la sección [Aprender FFmpeg libav de la manera difícil.](#learn-ffmpeg-libav-the-hard-way) 

Algunas personas solían decir que la transmisión de video por internet era el futuro de la televisión tradicional, en cualquier caso, FFmpeg es algo que vale la pena estudiar.

__Tabla de Contenido__

* [Intro](#intro)
  * [video - ¡lo que ves!](#video---¡lo-que-ves!)
  * [audio - ¡lo que escuchas!](#audio---¡lo-que-escuchas!)
  * [códec - comprimiendo datos](#códec---comprimiendo-datos)
  * [Contenedor - un lugar cómodo para audio y video](#Contenedor---un-lugar-cómodo-para-audio-y-video)
* [FFmpeg - línea de comandos](#FFmpeg---línea-de-comandos)
  * [FFmpeg herramienta de línea de comandos 101](#FFmpeg-herramienta-de-línea-de-comandos-101)
* [Operaciones de video comunes](#Operaciones-de-video-comunes)
  * [Transcoding](#transcoding)
  * [Transmuxing](#transmuxing)
  * [Transrating](#transrating)
  * [Transsizing](#transsizing)
  * [Round Bonus:  Transmisión adaptativa](#Round-Bonus-Transmisión-adaptativa)
  * [Ve más allá](#Ve-más-allá)
* [Aprende FFmpeg libav de la manera difícil](#Aprende-FFmpeg-libav-de-la-manera-difícil)
  * [Capítulo 0 - El infame hola mundo](#Capítulo-0---El-infame-hola-mundo)
    * [Arquitectura de FFmpeg libav](#Arquitectura-de-FFmpeg-libav)
  * [Capítulo 1 - timing](#Capítulo-1---sincronizando-audio-y-video)
  * [Capítulo 2 - remuxing](#Capítulo-2---remuxing)
  * [Capítulo 3 - transcoding](#Capítulo-3---transcoding)

# Intro

## Video - ¡lo que ves!

Si tu tienes una secuencia de imágenes en serie y las cambias a cierta frecuencia (digamos [24 imagenes por segundo](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)), crearías una [ilusion de movimiento](https://en.wikipedia.org/wiki/Persistence_of_vision).
En resumen, esta es una muy básica idea detrás de un video: **una serie de imágenes / cuadros, corriendo a una velocidad dada**.

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>


Ilustración Zeitgenössische (1886)

## Audio - ¡lo que escuchas!

Aunque un video mudo puede expresar una variedad de sentimientos, el agregarle sonido lo vuelve una experiencia mas placentera.

El sonido es la vibración que se propaga como una onda de presión, a través del aire o de cualquier otro medio de transmisión, como un gas, líquido o sólido.

> En un sistema de audio digital, el micrófono convierte sonido a una señal eléctrica analógica, después un convertidor analógico-a-digital  (ADC) — típicamente se usa [pulse-code modulation (PCM)](https://en.wikipedia.org/wiki/Pulse-code_modulation)  - que convierte la señal analógica en una señal digital.

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analogo a digital")
>[Fuente](https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg)

## Códec - comprimiendo datos

> CODEC es un circuito electrónico o software que **comprime o descomprime audio/video digital.** 

Convierte audio/video digital en bruto (raw) a un formato comprimido o vice versa.

> https://en.wikipedia.org/wiki/Video_codec

Pero si deseamos empaquetar millones de imágenes dentro de un solo archivo y generamos una película, entonces terminaríamos con un archivo enorme. Veamos las matemáticas:

Supongamos que creamos el video con una resolución de `1080 x 1920` (altura x anchura) y que utilizaremos `3 bytes` por píxel (la unidad mínima en una pantalla) para codificar el color (o [un color de 24 bit](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29), que nos da 16,777,216 diferentes colores) y este video se reproduce a `24 cuadros por segundo` entonces serán  `30 minutos` de duración.

```c
toppf = 1080 * 1920 //total_de_pixeles_por_cuadro
cpp = 3 //costo_por_pixel
tis = 30 * 60 //tiempo_en_segundos
fps = 24 //cuadros_por_segundo

almacenamiento_requerido = tis * fps * toppf * cpp
```

¡Este video requeriría aproximadamente `250.28GB` de almacenamiento o `1.19 Gbps` de banda ancha! Es por esto que necesitamos hacer uso de un [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work).

## Contenedor - un lugar cómodo para audio y video

> Un contenedor o formato de envoltura es un formato de meta-archivos cuyas especificaciones describen que diferentes elementos de datos y metadatos coexisten en un mismo archivo de computadora.
>
>  https://en.wikipedia.org/wiki/Digital_container_format

Es un **sólo archivo que contiene todos los streams (en su mayoría de audio y video) y también provee una sincronización y metadatos generales**, como un titulo, resolución, etc.

Usualmente, podemos inferir el formato de un archivo al ver su extensión: por ejemplo un `video.webm` es probablemente un video usando el contenedor [`webm`](https://www.webmproject.org/).

![container](/img/container.png)

# FFmpeg - línea de comandos

> Una completa solución multi-plataforma para grabar, convertir y transmitir audio y video.

Para trabajar con multimedia podemos hacer uso de esta MARAVILLOSA herramienta/librería llamada [FFmpeg](https://www.ffmpeg.org/). Existen posibilidades de que ya la conoces/usas, directa o indirectamente (¿usas [Chrome](https://www.chromium.org/developers/design-documents/video)?).

Éste tiene una programa para línea de comandos llamado `ffmpeg`,un binario muy simple y poderoso. Por ejemplo, puedes convertir desde un contenedor `mp4`a uno `avi` solo escribiendo el siguiente comando:

```bash
$ ffmpeg -i input.mp4 output.avi
```
Acabamos de hacer **remuxing** (remultiplexación) aquí, el cual consiste convertir de un contenedor a otro. Técnicamente, FFmpeg puede también hacer un transcoding, pero hablaremos de eso después. 

## FFmpeg herramienta de línea de comandos 101

FFmpeg posee [documentación](https://www.ffmpeg.org/ffmpeg.html) que hace un gran trabajo explicando como funciona.

Para ser breves, el comando de línea para FFmpeg espera el siguiente formato de argumentos para realizar sus acciones `ffmpeg {1} {2} -i {3} {4} {5}`, donde:

1. Opciones globales
2. Opciones de archivo de entrada
3. URL de entrada
4. Opciones de archivo de salida
5. URL de salida

Las partes 2,3,4 y 5 pueden ser tantas como sean necesarias.

Es mas fácil entender este formato de argumentos en acción:

``` bash
# ADVERTENCIA: este archivo pesa alrededor de 300MB
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # opciones globales
-c:a libfdk_aac \ # opciones de entrada
-i bunny_1080p_60fps.mp4 \ # url de entrada
-c:v libvpx-vp9 -c:a libvorbis \ # opciones de salida
bunny_1080p_60fps_vp9.webm # url de salida
```

Este comando toma el archivo de entrada `mp4` que contiene 2 streams (un audio codificado con el CODEC `aac` y el video codificado usando el CODEC `h264`) y va a convertirlo a `webm`, cambiando también los CODECs de audio y video.

Podríamos simplificar el comando de arriba pero tenemos que saber que FFmpeg adoptará o supondrá los valores predeterminados por ti.

Por ejemplo, cuando tu introduces `ffmpeg -i input.avi output.mp4` ¿qué CODEC para audio/video va a usar para producir `output.mp4`?

Werner Robitza escribió un [tutorial acerca de codificacion y edicion con FFmpeg](http://slhck.info/ffmpeg-encoding-course/#/) que se tiene que leer/realizar para una mejor comprensión.

# Operaciones de video comunes

Cuando trabajamos con audio/video nosotros usualmente hacemos una serie de tareas con archivos multimedia.

## Transcoding

![transcoding](/img/transcoding.png)

**¿Qué?** el acto de convertir uno de los flujos de transmisión (audio o video) de un CODEC a otro.

**¿Por qué?** en ocasiones algunos dispositivos (TVs, smartphones, consolas, etc.) no soportan X pero si Y y nuevos CODECs proveen mejor tasa de compresión. 

**¿Cómo?** convirtiendo un video `H264` (AVC) a un `H265` (HEVC).

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## Transmuxing 

![transmuxing](/img/transmuxing.png)

**¿Qué?** el acto de convertir un formato (contenedor) a otro.

**¿Por qué?** en ocasiones algunos dispositivos (TVs, smartphones, consolas, etc.) no soportan X pero si Y y a veces nuevos contenedores proveen características modernas que son requeridas.

**¿Cómo?** convirtiendo de `mp4` a `webm`.

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # con esto se dice a ffmpeg que se salte la codificación
bunny_1080p_60fps.webm
```

## Transrating

![transrating](/img/transrating.png)

**¿Qué?** el acto de cambiar la tasa de bits, o produciendo otras presentaciones.

**¿Por qué?** las personas intentaran ver tu video usando una conexión `2G` (edge) en un smartphone de baja gama o una conexión por `fibra` a Internet en los televisores a 4K, por lo tanto tu deberías ofrecer mas de una presentación para el mismo video a diferente tasa de bits.

**¿Cómo?** produciendo una presentación con una tasa de bits entre 3856K y 2000K.

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

Usualmente vamos a estar usando transrating con transsizing. Werner Robitza escribió otra [serie de posts acerca del control de tasa para FFmpeg](http://slhck.info/posts/) que debes leer/realizar.

# Transsizing

![transsizing](/img/transsizing.png)

**¿Qué?** el acto de convertir desde una resolución a otro. Como antes se dijo, transsizing es usualmente usado con transrating.

**¿Por qué?** las razones serian las mismas que las de transrating.

**¿Cómo?** convirtiendo de una resolución de `1080p` a `480p`.

```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## Round Bonus: Transmisión adaptativa

![adaptive streaming](/img/adaptive-streaming.png)

**¿Qué?** el acto de producir varias resoluciones (tasas de bits) y dividir el contenido en porciones y después servirlos mediante http.

**¿Por qué?** para proveer un contenido flexible que puede ser observado en un smartphone de baja gama o en una televisión en 4K, también es fácil de escalar y desplegar pero puede agregar latencia.

**¿Cómo?** creando un WebM adaptativo usando DASH.
```bash
# emisiones de video
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 160x90 -b:v 250k -keyint_min 150 -g 150 -an -f webm -dash 1 video_160x90_250k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 320x180 -b:v 500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_320x180_500k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 750k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_750k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 1000k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_1000k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 1280x720 -b:v 1500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_1280x720_1500k.webm

# emisiones de audio
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:a libvorbis -b:a 128k -vn -f webm -dash 1 audio_128k.webm

# el manifiesto DASH
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

PD: Tomé este ejemplo desde las [Instrucciones de la reproducción de WebM adaptativo usando DASH](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)

## Ve más allá

Hay [muchos y bastantes mas usos para FFmpeg](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly).
Yo lo uso en conjunto con *iMovie* para producir/editar algunos videos de Youtube y tu ciertamente puedes usarle de manera profesional.

# Aprende FFmpeg libav de la manera difícil

> ¿A veces no te preguntas acerca de el sonido y la visión?
> **David Robert Jones**

Sabiendo que [FFmpeg]() es tan útil como una herramienta de línea de comandos para realizar tareas esenciales en archivos multimedia, pero ¿cómo se pueden usar en nuestros programas?

FFmpeg está [compuesto de multiples librerías](https://www.ffmpeg.org/doxygen/trunk/index.html) que pueden ser integradas en nuestros propios programas.

Usualmente, cuando instalas FFmpeg, se instalan automáticamente todas esas librerías. De aquí en adelante, me voy a referir a estas set de librerías como **FFmpeg libav**.

> Este título es un homenaje a las series de Zed Shaw [Aprende X de la manera difícil](https://learncodethehardway.org/), particularmente a su libro Aprende C de la manera difícil.

## Capítulo 0 - El infame hola mundo

Éste hola mundo, de hecho, no enseñara el mensaje de `"hola mundo"` en la terminal :tongue: En su lugar, vamos a **imprimir la información acerca del video**. cosas como su formato (contenedor), duración, resolución, canales de audio y, al final, vamos a **decodificar algunos cuadros y a guardarlos como archivos de imagen**.

### Arquitectura de FFmpeg libav

Pero antes de que podamos empezar a codificar, vamos a aprender como la **Arquitectura de FFmpeg libav** funciona y como sus componentes se comunican con otros.

Aquí hay un diagrama del proceso de decodificación de video:

![ffmpeg libav architecture - decoding process](/img/decoding.png)

Primero, vas a necesitar cargar tu archivo multimedia dentro de un componente llamado [`AVFormatContext`](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html)(el contenedor de video es también conocido como formato). 

De hecho, no se carga todo el archivo: usualmente solo lee el encabezado (header) del mismo.

Una vez cargamos el **encabezado de nuestro contenedor** en su forma mínima, nosotros podemos acceder a sus streams (piensa de ellos como datos rudimentarios de audio y video).

Cada stream estará disponible en un componente llamado [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html).

> Stream es un nombre elegante para un flujo continuo de datos.

Supongamos que nuestro video tiene dos streams: un audio codificado con [AAC CODEC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding) y un video codificado con [H264 (AVC) CODEC](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC). Por cada stream, nosotros podemos extraer **piezas de datos** llamados paquetes, los que serán cargados en componentes llamados [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html).

Los **datos dentro de los paquetes siguen codificados** (comprimidos) y para decodificar los paquetes, necesitamos pasarlos a un [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html) específico.

El `AVCodec` va a decodificarlos dentro de un [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html) y finalmente, este componente nos da **el cuadro (frame) descomprimido**. Hay que poner atención en que se usa la misma terminología o mismo proceso es usado de igual manera por un stream de audio y video.

### Requerimientos

Debido a que algunas personas estuvieron [enfrentandose a varios problemas durante la compilacion o ejecucion de los ejemplos](https://github.com/leandromoreira/ffmpeg-libav-tutorial/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+compiling) **vamos a usar [`Docker`](https://docs.docker.com/install/) como nuestro entorno de desarrollo/ejecución, también haremos uso del video: "The Big Buck Bunny", que en caso de no contar con él de manera local, solo ejecuta el comando `make fetch_small_bunny_video`.

 ### Capítulo 0 - el código, paso a paso

> #### TLDR; enséname el [codigo](/0_hello_world.c) y ejecuta.
> ```bash
> $ make run_hello
> ```

Vamos a saltarnos unos detalles, pero no te preocupes: el [código fuente esta disponible en GitHub](/0_hello_world.c).

Vamos a acomodar (allocate) la memoria para el componente [`AVFormatContext`](http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html), el cual va a contener la información acerca del formato (contenedor).

```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```

Ahora vamos a abrir el archivo y leer su encabezado para llenar el `AVFormatContext` con la información mínima acerca del formato (note que usualmente los códecs no son abiertos).

La función usada para hacer esto es [`avformat_open_input`](http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49). Éste espera un `AVFormatContext`, un archivo (`filename`) y dos argumentos opcionales: el [`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html) (si tu colocas un `NULL`, FFmpeg va a suponer el formato por ti) y el [`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html) (el cual son las opciones para el desmultiplexador).

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```

Podemos imprimir el nombre del formato y la duración media:

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```

Para acceder a los `streams`, necesitamos leer los datos del archivo. La función [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb) hace eso.

Ahora, el `pFormatContext->nb_streams` contendrá el numero de streams y el `pFormatContext->streams[i]` nos dará el  stream `i`([`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html)).

```c
avformat_find_stream_info(pFormatContext,  NULL);
```
Ahora, navegaremos por todos los streams.
```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
  //
}
```
Por cada stream, vamos a mantener los [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html), los cuales describen las propiedades de un códec usado por el stream `i`.

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```
Ya con las propiedades del códec, podremos ver el CODEC apropiado solicitándolo a la función [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca)  y encontrar el decodificador para un códec id y regresar un [`AVCodec`](http://ffmpeg.org/doxygen/trunk/structAVCodec.html), el componente que conoce como **CO**dificar y **DEC**odificar el stream.

```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```

Ahora, vamos a imprimir la información acerca de los códecs.

```c
// especifico para video y audio
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
  printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
  printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
}
// general
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
```

Con el códec, podemos acomodar memoria para el [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html), el cual va a contener el contexto para nuestro proceso de decodificación/codificación, pero antes debemos llenar el contexto del códec con los parámetros CODEC; esto lo hacemos con [`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16).

Una vez llenado el contexto del códec, necesitamos abrirlo. Entonces tenemos que llamar a la función [`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) y después de ello,  lo podremos usar.

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```

Ahora, vamos a leer los paquetes desde el stream y decodificarlos dentro de cuadros, vamos a necesitar acomodar la memoria para ambos componentes, el [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) y [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html).

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```

Hay que sustraer nuestros paquetes desde los streams con la función [`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61) mientras contenga paquetes.

```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //...
}
```

Ahora, hay que **mandar los paquetes de datos en bruto** (cuadro comprimido) al decodificador, mediante el contexto del códec, usando la función  [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3).

```c
avcodec_send_packet(pCodecContext, pPacket);
```

Y vamos a **recibir el cuadro de datos en bruto** (cuadro descomprimido) desde el decodificador, mediante el mismo contexto del códec, usando la función [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c).

```c
avcodec_receive_frame(pCodecContext, pFrame);
```

Podemos imprimir el numero de cuadro, el [PTS](https://en.wikipedia.org/wiki/Presentation_timestamp), DTS, [frame type](https://en.wikipedia.org/wiki/Video_compression_picture_types), etc.

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

Finalmente, podemos guardar nuestro cuadro decodificado dentro de una [imagen gris simple](https://en.wikipedia.org/wiki/Netpbm_format#PGM_example). El proceso es muy sencillo, nosotros usaremos el `pFrame->data,`, donde el índice esta relacionado con los [planos Y, Cb y Cr](https://en.wikipedia.org/wiki/YCbCr), nosotros solo seleccionamos  `0` (Y) para guardar nuestra imagen gris.

```c
save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // escribiendo el encabezado mínimo para un formato de un archivo pgm
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // escribiendo linea por linea
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
```

¡Y voilà! Ahora nosotros tenemos una imagen gris a escala de 2MB:

![saved frame](/img/generated_frame.png)

## Capítulo 1 - sincronizando audio y video

> **Sé el jugador** - un joven desarrollador de JS escribiendo un nuevo reproductor de video MSE.

Antes de que nos movamos a [codificar un ejemplo de transcoding](#capitulo2-transcoding) ahora vamos a hablar acerca de la **sincronización (timing)**, o como el reproductor de video lo conoce, el tiempo correcto para reproducir un cuadro.

En el ultimo ejemplo, hemos guardado algunos cuadros que pueden verse aquí:

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

Cuando nosotros estamos diseñando un reproductor de video, nosotros necesitamos **reproducir cada cuadro a su debido tiempo**, de otra forma sería difícil ver un video de manera agradable, porque se estaría reproduciendo demasiado rápido o lento.

Por lo tanto, necesitamos introducir algo de lógica para reproducir sin complicaciones cada cuadro. Para ello, cada cuadro tiene un **Timestamp de presentación** (PTS) el cual tiene un numero creciente factorizado en un **timebase (tiempo base)**, que es un numero racional (donde el denominador es conocido como **timescale**) divisible por el **frame rate (fps)**.

Es fácil entender cuando vemos algunos ejemplos, vamos a simular varios escenarios.

Para un `fps=60/1` y `timebase=1/60000` cada PTS se incrementará `timescale / fps = 1000`, por lo tanto el **PTS en tiempo real** por cada cuadro podría ser (suponiendo que empieza en 0):

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

Para casi el mismo escenario pero con un timebase igual a `1/60`.

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
* `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

Para un `fps=25/1` y `timebase=1/75` cada PTS se incrementará `timescale / fps = 3` y el tiempo PTS podría ser:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
* `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
* `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`

Ahora con el `pts_time` podemos encontrar una forma de renderizarlo, esto es sincronizándolo con el audio `pts_time` o con el reloj del sistema. El FFmpeg libav provee esa información a través de su API:

- fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
- tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
- tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

Por pura curiosidad, observa que los cuadros fueron guardados en el orden DTS (cuadros: 1, 6, 4, 2, 3, 5) pero reproducidos en un orden PTS (cuadros: 1, 2, 3, 4, 5). Además, nota que poco costo tienen los cuadros-B en comparación con los cuadros-P o cuadros-I.

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

## Capítulo 2 - remuxing

Remuxing (remultiplexar) es el acto de cambiar de un formato (contenedor) a otro, por ejemplo, nosotros podemos cambiar un video [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4_Part_14) a uno  [MPEG-TS](https://en.wikipedia.org/wiki/MPEG_transport_stream) sin muchos problemas usando FFmpeg:

```bash
ffmpeg input.mp4 -c copy output.ts
```

Esto va a desmultiplexar (demux) el mp4 pero no lo va a decodificar o codificar (`-c copy`) y al final, esto lo multiplexa (mux) dentro de un archivo `mpegts`. Si tu no provees el formato `-f`, entonces FFmpeg va a tener que determinarlo en base de la extensión del archivo.

El uso general de FFmpeg o libav sigue un patrón/arquitectura o flujo de trabajo:

* **[protocol layer](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - este acepta una entrada (`input`) (un archivo o `file`, o por ejemplo la entrada también podría ser  `rtmp` o `HTTP`)
* **[format layer](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - este desmultiplexa (`demuxes`) su contenido, revelando, en mayor parte, los metadatos y sus streams
* **[codec layer](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - esto decodifica (`decodes`) sus datos de stream comprimidos<sup>*opcional*</sup>
* **[pixel layer](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)** - aquí también se pueden aplicar filtros (`filters`) a los cuadros en bruto (como resizing)<sup>*optional*</sup>
* y después lo hace en el sentido contrario.
* **[codec layer](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - esto codifica (`encodes`) (o re-encodifica (`re-encodes`) o incluso transcodifican o `transcodes`) los cuadros en bruto<sup>*opcional*</sup>
* **[format layer](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - esto multiplexa (`muxes`) (o remultiplexa  (`remuxes`) los streams en bruto (los datos comprimidos)
* **[protocol layer](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - y finalmente los datos multiplexados son enviados a una salida o `output` (otro archivo o quizás, un servidor remoto en la red)

![ffmpeg libav workflow](/img/ffmpeg_libav_workflow.jpeg)

> Esta imagen está fuertemente inspirada por los trabajos de [Leixiaohua](http://leixiaohua1020.github.io/#ffmpeg-development-examples) y [Slhck](https://slhck.info/ffmpeg-encoding-course/#/9).

Ahora vamos a codificar un ejemplo usando libav para proveer el mismo efecto que en `ffmpeg input.mp4 -c copy output.ts`.

```c
AVFormatContext *input_format_context = NULL;
AVFormatContext *output_format_context = NULL;
```

Como en los ejemplos anteriores, empezaremos por acomodar la memoria y abrir el formato de la entrada. Para este caso en específico, vamos a abrir un archivo de entrada y acomodar memora para un archivo de salida.

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

Vamos a remultiplexar solamente los tipos de streams de video, audio y subtítulos, así que vamos a obtener que streams vamos a estar usando dentro de un arreglo de índices.

 ```c
number_of_streams = input_format_context->nb_streams;
streams_list = av_mallocz_array(number_of_streams, sizeof(*streams_list));
 ```

Después de haber acomodado la memoria requerida, vamos a navegar por todos los streams, por cada uno necesitaremos crear un nuevo stream dentro de nuestro contexto de formato de salida, usando la función [avformat_new_stream](https://ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827). Nota como estamos marcando todos los streams que no son video, audio o subtitulo, así que podemos saltarlos para luego.

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

Ahora, podemos crear un archivo de salida.

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

Después, podemos copiar los streams, paquete por paquete, desde nuestros streams de entrada a los de salida. Continuaremos navegando por los paquetes, mientras estos sigan llegando (`av_read_frame`), por cada paquete vamos a necesitar recalcular el PTS y el DTS, para finalmente escribirlo (`av_interleaved_write_frame`) a nuestro contexto de formato de salida.

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
  /* copiar paquete */
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

Para finalizar, necesitamos escribir el stream trailer a un archivo multimedia de salida con la función [av_write_trailer](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13).

```c
av_write_trailer(output_format_context);
```

Ahora, estamos listos para probarlo y la primera prueba va a ser una conversión de formato (contenedor de video) de un video MP4 a un video MPEG-TS. Estamos básicamente realizando la línea de comando `ffmpeg input.mp4 -c copy output.ts` con libav.

```bash
make run_remuxing_ts
```

¡Funciona! !¿No me crees?! no deberías, podemos checarlo con `ffprobe`:

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

Para resumir todo lo que hicimos esto en una imagen, podemos revisitar nuestra [idea inicial acerca de cómo libav funciona](https://github.com/leandromoreira/ffmpeg-libav-tutorial#ffmpeg-libav-architecture) pero observa que nos saltamos la parte del códec.

![remuxing libav components](/img/remuxing_libav_components.png)

Antes de terminar este capítulo, me gustaría enseñarte una parte importante del proceso de remultiplexación, **tu puedes pasar esas opciones al multiplexor**. Digamos que se desea entregar un formato [MPEG-DASH](https://developer.mozilla.org/en-US/docs/Web/Apps/Fundamentals/Audio_and_video_delivery/Setting_up_adaptive_streaming_media_sources#MPEG-DASH_Encoding), para eso, necesitamos usar [mp4 fragmentado](https://stackoverflow.com/a/35180327) (a veces es referido como `fmp4`) en lugar de MPEG-TS o MPEG-4 plano.

Con la [línea de comando, podemos hacer eso fácilmente](https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE#Fragmenting).

```
ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
```

Casi igual de fácil como en la línea de comando, para su versión en libav, solamente debemos pasar las opciones y después escribir el encabezado de salida, justo antes de copiar los paquetes.

 ```c
AVDictionary* opts = NULL;
av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
ret = avformat_write_header(output_format_context, &opts);
 ```

Ahora podemos generar este archivo mp4 fragmentado:

```bash
make run_remuxing_fragmented_mp4
```

Para asegurarte que no te estoy mintiendo. Puedes usar esta maravillosa página/herramienta [gpac/mp4box.js](http://download.tsi.telecom-paristech.fr/gpac/mp4box.js/filereader.html) o el sitio [http://mp4parser.com/](http://mp4parser.com/) para ver las diferencias, primero carga el mp4 "común".

![mp4 boxes](/img/boxes_normal_mp4.png)

Como podrás ver, este tiene un solo atom (o caja) `mdat`, **este es el espacio donde se encuentran los cuadros de video y audio**. Ahora carga el mp4 fragmentado y ve lo que despliega de las cajas `mdat`.

![fragmented mp4 boxes](/img/boxes_fragmente_mp4.png)

## Capítulo 3 - transcoding

> #### TLDR; enséñame el [código](/3_transcoding.c) y ejecuta.
> ```bash
> $ make run_transcoding
> ```
> Vamos a saltarnos unos detalles, pero no te preocupes: el [el codigo fuente está disponible en GitHub](/3_transcoding.c).

En este capitulo, vamos a crear un transcoder minimalista, escrito en C, que pueda convertir videos codificados en H264 a H265 usando la librería **FFmpeg/libav**, específicamente [libavcodec](https://ffmpeg.org/libavcodec.html), libavformat, y libavutil.

![media transcoding flow](/img/transcoding_flow.png)

> _Solo una recapitulación rápida:_ El [**AVFormatContext**](https://www.ffmpeg.org/doxygen/trunk/structAVFormatContext.html) es la abstracción del formato para un archivo multimedia, alias contenedor (ej. MKV, MP4, Webm, TS). El [**AVStream**](https://www.ffmpeg.org/doxygen/trunk/structAVStream.html) representa cada tipo de datos para un formato dado (ej: audio, video, subtitulo, metadatos). El [**AVPacket**](https://www.ffmpeg.org/doxygen/trunk/structAVPacket.html) es una porción de datos comprimidos, los cuales son adquiridos desde `AVStream` y que pueden ser decodificados por un [**AVCodec**](https://www.ffmpeg.org/doxygen/trunk/structAVCodec.html) (ej: av1, h264, vp9, hevc) generando datos en bruto, llamados [**AVFrame**](https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html).

### Transmuxing 

Vamos a empezar con una simple operación de transmultiplexación (transmuxing) y después podemos construir sobre este código, el primer paso es **cargar el archivo de entrada**.

```c
// Acomoda un AVFormatContext
avfc = avformat_alloc_context();
// Abre un stream de entrada y lee el encabezado.
avformat_open_input(avfc, in_filename, NULL, NULL);
// Lee los paquetes del archivo para obtener la informacion de streams.
avformat_find_stream_info(avfc, NULL);
```

Ahora vamos a poner en pie el decodificador, el `AVFormatContext` nos va a dar acceso a todos los componentes `AVStream` y por cada uno de ellos, podremos obtener su `AVCodec` y crear su `AVCodecContext` en particular y finalmente podremos abrir el códec dado, así entonces podremos proceder con el proceso de decodificación.

>  El [**AVCodecContext**](https://www.ffmpeg.org/doxygen/trunk/structAVCodecContext.html) contiene datos acerca de la configuración del archivo como la tasa de bits (bit rate), tasa de cuadros (frame rate), tasa de muestreo (sample rate), canales (channels), altura (height), así como muchos otros.

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

Necesitamos preparar el archivo multimedia para transmultiplexación también, primero debemos **acomodar memoria** para la salida `AVFormatContext`. Creamos **cada uno de los streams** en el formato de salida. Para poder empaquetar propiamente el stream, **copiamos los parámetros del códec** desde el decodificador.

**Establecemos la bandera** `AV_CODEC_FLAG_GLOBAL_HEADER` el cual le dice al encodificador que puede usar los encabezados globales y finalmente abrimos el **archivo de salida para vaciar los datos** y mantener los encabezados.

```c
avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);

AVStream *avs = avformat_new_stream(encoder_avfc, NULL);
avcodec_parameters_copy(avs->codecpar, decoder_avs->codecpar);

if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
  encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avio_open(&encoder_avfc->pb, encoder->filename, AVIO_FLAG_WRITE);
avformat_write_header(encoder->avfc, &muxer_opts);

```

Nosotros conseguiremos los `AVPacket` desde el decodificador, ajustando los timestamps, y así poder escribir apropiadamente el paquete en el archivo de salida. Aunque la función `av_interleaved_write_frame` dice "write frame" (escribir cuadro), estamos guardando el paquete. Terminaremos el proceso de transmultiplexación escribiendo el stream del trailer, que se encuentra dentro del archivo.

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

La sección previa mostró un programa transmultiplexador, ahora vamos a agregar la capacidad para codificar los archivos, específicamente, vamos a habilitarlo para transcodificar videos desde `h264` a `h265`.

Después de que preparamos el decodificador, pero antes de acomodar el archivo de salida multimedia, vamos a configurar el encodificador.

* Crea el video `AVStream` en el encodificador, [`avformat_new_stream`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827)
* Usa el `AVCodec` llamado `libx265`, [`avcodec_find_encoder_by_name`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37)
* Crear el `AVCodecContext` basado en el códec creado, [`avcodec_alloc_context3`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315)
* Configurar los atributos básicos para la sesión de transcodificación, y
* Abre el códec y copia los parámetros del contexto al stream. [`avcodec_open2`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) y [`avcodec_parameters_from_context`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe) 

```c
AVRational input_framerate = av_guess_frame_rate(decoder_avfc, decoder_video_avs, NULL);
AVStream *video_avs = avformat_new_stream(encoder_avfc, NULL);

char *codec_name = "libx265";
char *codec_priv_key = "x265-params";
// vamos a usar las opciones internas para x265
// esto deshabilita la deteccion de cambio de escena y despues fija
// GOP en 60 cuadros.
char *codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

AVCodec *video_avc = avcodec_find_encoder_by_name(codec_name);
AVCodecContext *video_avcc = avcodec_alloc_context3(video_avc);
// parametros de codec para el encoder 
av_opt_set(sc->video_avcc->priv_data, codec_priv_key, codec_priv_value, 0);
video_avcc->height = decoder_ctx->height;
video_avcc->width = decoder_ctx->width;
video_avcc->pix_fmt = video_avc->pix_fmts[0];
// control de tasa
video_avcc->bit_rate = 2 * 1000 * 1000;
video_avcc->rc_buffer_size = 4 * 1000 * 1000;
video_avcc->rc_max_rate = 2 * 1000 * 1000;
video_avcc->rc_min_rate = 2.5 * 1000 * 1000;
// tiempo base
video_avcc->time_base = av_inv_q(input_framerate);
video_avs->time_base = sc->video_avcc->time_base;

avcodec_open2(sc->video_avcc, sc->video_avc, NULL);
avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
```

Necesitamos expandir nuestro ciclo decodificador para la transcodificación del stream de video:

* Envía el `AVPacket` vacío al decodificador, [`avcodec_send_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3)
* Recibir el `AVFrame` descomprimido, [`avcodec_receive_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)
* Empezar a transcodificar este cuadro en bruto,
* Enviar este cuadro en bruto, [`avcodec_send_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169)
* Recibe el contenido comprimido, basado en nuestro códec, `AVPacket`, [`avcodec_receive_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga5b8eff59cf259747cf0b31563e38ded6)
* Establece el timestamp, y[`av_packet_rescale_ts`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e)
* Escríbelo a un archivo de salida. [`av_interleaved_write_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1)

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

// funcion usada
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

Vamos a convertir el stream desde `h264` a `h265`, como se espera de la versión `h265`, el archivo es más pequeño que el `h264` sin embargo el [programa creado](/3_transcoding.c) es capaz de:

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

> Ahora, para ser honesto, esto fue [más difícil de lo que pensé](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54), voy a tener que y ya me he metido dentro de [el código fuente de la linea de comandos FFmpeg](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54#issuecomment-570746749) y probarlo bastante, y también pienso que estoy olvidando algo, ya que cuando tengo que forzar `force-cfr` para que el `h264` funcione, me sigue arrojando algunos mensajes como `warning messages (forced frame type (5) at 80 was changed to frame type (3))`.