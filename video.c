#include "aoakvm.h"
#include "video.h"
#include "usb.h"

// Defines
#define MIDDLE_BUFFER_SIZE 1024
#define AVIO_BUFFER_SIZE 4 * 1024
#define IN 0x81 //0x85
#define LENGTH_FRAME_QUEUE 30

// Struct Definition

/* used by usb_reader and read_packet */
struct usb_source_context
{
  libusb_device_handle *device;
  uint8_t *ptr; // points to datastart
  int size;     // how much data should be copied
};

/*
    struct videoFrameQueue_t

    Fields:
        int nextRead;
        int nextWrite;
        SDL_mutex *frameQueueMutex;
        AVFrame frame[LENGTH_FRAME_QUEUE];
*/
struct FrameQueue {
  int nextRead;
  int nextWrite;
  SDL_mutex *frameQueueMutex;
  AVFrame frame[LENGTH_FRAME_QUEUE];
};

// Static Functions
static void fq_incrementReadIndex();
static void fq_incrementWriteIndex();

static int fq_getFrameFromQueue(AVFrame *frame);

static int create_texture(SDL_Renderer **renderer, SDL_Texture **texture, AVCodecContext *codec_ctx);

static int read_packet(void *opaque, uint8_t *buf, int buf_size);

// Local Variables
unsigned char middle_buffer[MIDDLE_BUFFER_SIZE];

SDL_Texture *texture;

struct FrameQueue frameQueue = {
    .nextRead = 0,
    .nextWrite = 0,
};

int stream_width = 0;
int stream_height = 0;
int screen_width = 0;
int screen_height = 0;


static int create_texture(SDL_Renderer **renderer, SDL_Texture **texture, AVCodecContext *codec_ctx) {
#define DIFF_TO_EDGE 100

	int w = stream_width = codec_ctx->width;
	int h = stream_height = codec_ctx->height;
	log_trace("Stream Resolution: \t %d x %d", w, h);

	*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, w, h);
	log_debug("Texture Created");
	SDL_Rect rect;
	SDL_GetDisplayUsableBounds(0, &rect);

	if (w > h) { // landscapemode
		screen_width = (rect.w - DIFF_TO_EDGE);
		screen_height = h * (rect.w - DIFF_TO_EDGE) / w;
	} else {  // portraitmode
		screen_width = w * (rect.h - DIFF_TO_EDGE) / h;
		screen_height = (rect.h - DIFF_TO_EDGE);
	}
  	SDL_PumpEvents();
  	SDL_SetWindowSize(mainwindow, screen_width, screen_height);
  return 0;
}

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
  struct usb_source_context *ctx = (struct usb_source_context *)opaque;
  int response = 0;
  int transferred = 0;

  if (buf_size == 0) {
    return 0;
  }

  if (ctx->size > 0) {

    if (ctx->size > buf_size) {
      memcpy(buf, ctx->ptr, buf_size);
      ctx->ptr += buf_size;
      ctx->size -= buf_size;
      return buf_size;
    }

    memcpy(buf, ctx->ptr, ctx->size);
    int size = ctx->size;
    ctx->size = 0; // reset
    return size;
  }

  while (transferred == 0) {
    response = libusb_bulk_transfer(ctx->device, IN, middle_buffer, MIDDLE_BUFFER_SIZE, &transferred, 0);

    if (response < 0 && response != LIBUSB_ERROR_IO) {
      log_debug("libusb_bulk_transfer failed: %s \t %d\n", libusb_error_name(response), transferred);
      if (response == LIBUSB_ERROR_NO_DEVICE) {
        //Send SDL_Event connection lost;
        return response;
      }
    }
  }

  if (transferred > buf_size) {
    memcpy(buf, middle_buffer, buf_size);
    ctx->ptr = middle_buffer + buf_size;
    ctx->size = transferred - buf_size;
    return buf_size;
  }

  memcpy(buf, middle_buffer, transferred);

  return transferred;
}

AVIOContext *video_setupAVContext(libusb_device_handle *handle) {
  struct usb_source_context *ctx;
  uint8_t *avio_buffer = NULL;

  ctx = malloc(sizeof(struct usb_source_context));
  ctx->device = handle;

  ctx->ptr = middle_buffer;
  ctx->size = 0;

  avio_buffer = av_malloc(AVIO_BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
  if (!avio_buffer) {
    log_error("failed to allocate memory for avio_buffer");
    return NULL;
  }

  return avio_alloc_context(avio_buffer, AVIO_BUFFER_SIZE, 0, ctx, &read_packet, NULL, NULL);
}

int video_initRenderer(struct aoakvmAVCtx_t *data, SDL_Renderer **renderer){
	create_texture(renderer, &texture, data->codec_ctx);
  return 0;
}

int video_rendering(SDL_Renderer *renderer) {
    int ret = 0;

    // Get a Frame from the Queue
    AVFrame frame;
    ret = fq_getFrameFromQueue(&frame);
    if (ret >= 0) {
      ret = SDL_UpdateYUVTexture(texture, NULL,
                                 frame.data[0], frame.linesize[0],
                                 frame.data[1], frame.linesize[1],
                                 frame.data[2], frame.linesize[2]);

      if (ret < 0) {
        log_error("Update YUV Texture failed: %s", SDL_GetError());
        return 0;
      }
    } else {
      return 0;
    }

    ret = SDL_RenderClear(renderer);
    if (ret < 0){
      log_error("SDL_RenderClear failed.");
      return ret;
    }

    ret = SDL_RenderCopy(renderer, texture, NULL, NULL);
    if (ret < 0) {
      log_error("SDL_RenderCopy failed.");
      return ret;
    }

    SDL_RenderPresent(renderer);
    return 0;
}

int video_openStream(AVIOContext *source, AVFormatContext **format, AVCodecContext **codec) {
    // Set logging of ffmpeg
#ifdef DEBUG
    av_log_set_level(AV_LOG_VERBOSE);
#else
    av_log_set_level(AV_LOG_FATAL);
#endif

    /* alloc and open AVFormatContext from reader */
    if (!(*format = avformat_alloc_context())) {
      return AVERROR(ENOMEM);
    }

    (*format)->pb = source;
    (*format)->video_codec_id = AV_CODEC_ID_H264;
    (*format)->audio_codec_id = AV_CODEC_ID_NONE;
    (*format)->probesize = 1024 * 1024;
    (*format)->format_probesize = 1024 * 1024;
    (*format)->max_analyze_duration = 1024 * 1024;
    (*format)->flags |= AVFMT_FLAG_NOBUFFER;
    (*format)->debug = FF_DEBUG_THREADS | FF_DEBUG_BUFFERS;

    log_info("Sie können an ihrem Gerät nun die Übertragung starten!");
    log_info("");
    log_info("Sollte der der Stream nicht Starten. Stoppen und starten sie die Übertragung neu.");

    if (avformat_open_input(format, NULL, NULL, NULL) < 0) {
      log_error("Could not open input stream.");
      return -1;
    }

    if (avformat_find_stream_info(*format, NULL) < 0) {
      log_error("Could not find stream information");
      return -1;
    }

    /* allocate codec */
    AVCodecParameters *codec_params = (*format)->streams[0]->codecpar;
    AVCodec *cd = avcodec_find_decoder(codec_params->codec_id);
    if (cd == NULL || codec_params == NULL)
    {
      log_error("Cannot find codec");
      return -1;
    }

    if (codec_params->codec_type != AVMEDIA_TYPE_VIDEO)
    {
      log_error("First stream isn't video");
      return -1;
    }

    if ((codec_params->width == 0) | (codec_params->height == 0))
    {
      log_error("Resolution is wrong. (== 0x0)");
      return -1;
    }

    *codec = avcodec_alloc_context3(cd);
    if (*codec == NULL)
    {
      log_error("failed to allocate codec context");
      return -1;
    }

    if (avcodec_parameters_to_context(*codec, codec_params) < 0)
    {
      log_error("failed to configure codec context");
      return -1;
    }

    (*codec)->flags2 |= AV_CODEC_FLAG2_FAST;

    if (avcodec_open2(*codec, cd, NULL) < 0)
    {
      log_error("could not open codec");
      return -1;
    }

  return 0;
}

static void fq_incrementReadIndex() {
	if (frameQueue.nextRead == LENGTH_FRAME_QUEUE - 1)
	{
		frameQueue.nextRead = 0;
		return;
  	}
	frameQueue.nextRead++;
    return;
}

static void fq_incrementWriteIndex() {
	if ((frameQueue.nextWrite == LENGTH_FRAME_QUEUE - 1 && frameQueue.nextRead == 0) || ((frameQueue.nextWrite + 1) == (frameQueue.nextRead)))
	{
		fq_incrementReadIndex();
		fq_incrementWriteIndex();
    	return;
	} else if (frameQueue.nextWrite == LENGTH_FRAME_QUEUE - 1) {
		frameQueue.nextWrite = 0;
		return;
  	}
    frameQueue.nextWrite++;
    return;
}

static int fq_getFrameFromQueue(AVFrame *frame) {
	if (frameQueue.nextRead == frameQueue.nextWrite) {
		SDL_Delay(1);
    	return -1;
    }

	SDL_LockMutex(frameQueue.frameQueueMutex);
	int ret = 0;
	*frame = frameQueue.frame[frameQueue.nextRead];
	fq_incrementReadIndex();
	SDL_UnlockMutex(frameQueue.frameQueueMutex);
	return ret;
}

int fq_pushFrameIntoQueue(AVFrame *frame) {
	SDL_LockMutex(frameQueue.frameQueueMutex);
	//log_debug("Write at %d", renderQueue.nextWrite);
	frameQueue.frame[frameQueue.nextWrite] = *frame;
	fq_incrementWriteIndex();
	SDL_UnlockMutex(frameQueue.frameQueueMutex);
	return 0;
}