#include <libusb-1.0/libusb.h>
#include <libavformat/avio.h>

#include "aoakvm.h"
#include "usb.h"
#include "hid.h"
#include "video.h"
#include "aoakvm_log.h"
#include "window.h"

/*
	Accessory PID:      0x2D00 if phone is in AOA mode
	Accessory PID_ALT:  0x2D01 if phone is in AOA and ADB mode
	Accessory VID:      0x18D1 for all phones in AOA mode.
*/

#define ACCESSORY_PID       0x2D01
#define ACCESSORY_PID_ALT   0x2D00
#define ACCESSORY_VID       0x18D1

// Static Functions
static int usb_initAOA(libusb_device_handle *handle, struct aoakvmConfig_t *cfg);

static int init_HIDS(libusb_device_handle *handle);


// Local Variables

libusb_context *context;


static int init_HIDS(libusb_device_handle *handle)
{
  int r;
  // HID Inputs
  log_debug("Registering HID...");
  if ((r = register_hid(handle, REPORT_DESC_SIZE(REPORT_DESC_MOUSE), 0)))
  {
	log_error("Registering HID Mouse failed with %d", r);
  }
  if ((r = register_hid(handle, REPORT_DESC_SIZE(REPORT_DESC_KB), 1)))
  {
	log_error("Registering HID Keyoard failed with %d", r);
  }
  if ((r = register_hid(handle, REPORT_DESC_SIZE(REPORT_DESC_TOUCHPAD), 2)))
  {
	log_error("Registering HID Touchpad failed with %d", r);
  }

  struct libusb_device_descriptor desc;
  libusb_get_device_descriptor(libusb_get_device(handle), &desc);
  const int max_packet_size_0 = desc.bMaxPacketSize0;

  if ((r = send_hid_descriptor(handle, REPORT_DESC_MOUSE, REPORT_DESC_SIZE(REPORT_DESC_MOUSE), max_packet_size_0, 0)))
  {
	log_error("Sending HID Descriptor for Mouse failed.");
  }

  if ((r = send_hid_descriptor(handle, REPORT_DESC_KB, REPORT_DESC_SIZE(REPORT_DESC_KB), max_packet_size_0, 1)))
  {
	log_error("Sending HID Descriptor for Keyboard failed.");
  }

  if ((r = send_hid_descriptor(handle, REPORT_DESC_TOUCHPAD, REPORT_DESC_SIZE(REPORT_DESC_TOUCHPAD), max_packet_size_0, 2)))
  {
	log_error("Sending HID Descriptor for Touchpad failed.");
  }
  return 0;
}

libusb_device_handle *usb_getHandle(struct aoakvmConfig_t *cfg) {

	libusb_device **list = NULL;
	libusb_device_handle *handle = NULL;

  	if (context == NULL) {
		if (libusb_init(&context) < 0) {
	  	log_error("libusb init failed\n");
	  	libusb_exit(context);
	  	context = NULL;
	  	return NULL;
		}
 	 }

  	ssize_t count = libusb_get_device_list(context, &list);
  	if (count <= 0) {
		log_error("libusb get device list failed\n");
		libusb_free_device_list(list, count);
		libusb_exit(context);
		context = NULL;
		return NULL;
  	}

  	for (ssize_t idx = 0; idx < count; ++idx) {
		libusb_device *device = list[idx];
		struct libusb_device_descriptor desc;

		if (libusb_get_device_descriptor(device, &desc) < 0) {
			log_error("error get devicedescriptor\n");
		}

		if (desc.bDeviceClass == 0x00) {
			if (desc.idVendor == ACCESSORY_VID) {
				if (desc.idProduct == ACCESSORY_PID_ALT || desc.idProduct == ACCESSORY_PID) {
					handle = usb_get_aoa_handle();
					if (handle != NULL) {
					libusb_free_device_list(list, count);
					return handle;
					} else {
						libusb_close(handle);
						continue;
					}
			}
		}

	  	log_info("INIT AOA: %04x:%04x", desc.idVendor, desc.idProduct);

	  	if (libusb_open(device, &handle) < 0) {
			log_info("Error!");
			libusb_free_device_list(list, count);
			libusb_close(handle);
			libusb_exit(context);
			context = NULL;
			return NULL;
	  	}

	  	int ret = usb_initAOA(handle, cfg);
		  log_debug("usb_initAOA ret: %d", ret);
	  	if (ret == -2) {
			libusb_free_device_list(list, count);
			libusb_close(handle);
			handle = NULL;
			return NULL;
	  	} else if (ret == -1) {
			libusb_close(handle);
			handle = NULL;
			continue;
	  	} else {
			libusb_free_device_list(list, count);
			return usb_get_aoa_handle();
	  	}
	}
  }
  libusb_free_device_list(list, count);
  libusb_close(handle);
  SDL_Delay(100);
  return NULL;
}

libusb_device_handle *usb_get_aoa_handle() {
	libusb_device_handle *handle = NULL;
  	libusb_device **list = NULL;

	window_changeMsgscreenTo(screens, renderer, mainwindow, AOA_INITIALIZED);

  	for (int i = 0; i < 10; i++) {
		  log_debug("Test");
		size_t count = libusb_get_device_list(context, &list);
		for (size_t idx = 0; idx < count; ++idx) {
			libusb_device *device = list[idx];
	 		struct libusb_device_descriptor info = {0};

	  		if (libusb_get_device_descriptor(device, &info) < 0) {
				return NULL;
	  		}

	  		if (info.idVendor == ACCESSORY_VID) {
				log_debug("Android-Device: %04x:%04x:%04x\n", info.idVendor, info.idProduct, info.bDeviceClass);

				if (info.idProduct == ACCESSORY_PID_ALT || info.idProduct == ACCESSORY_PID) {
		  			log_info("Init: %04x:%04x", info.idVendor, info.idProduct);
		  			handle = libusb_open_device_with_vid_pid(context, info.idVendor, info.idProduct);

			  		if (libusb_open(device, &handle) < 0) {
						log_info("Error!");
						return NULL;
		  			}

		  			if (handle != NULL) {
						libusb_claim_interface(handle, 0);
						libusb_free_device_list(list, count);
						return handle;
		  			}
				} else {
		  			return NULL;
				}
	  		}
		}
		libusb_free_device_list(list, count);
		SDL_Delay(20);
  	}
	  return NULL;
}

void usb_setConnectionState(enum aoakvm_usb_status_e state) {
	usbCon->status = state;
    switch (state)
    {
    case CONNECTED:
        aoakvm_push_event(&DEVICE_CONNECTION_EVENT, NULL, NULL);
        break;
    case NOT_CONNECTED:
        aoakvm_push_event(&DEVICE_DISCONNECTION_EVENT, NULL, NULL);
        break;
    default:
        break;
    }
}

int usb_registerHIDS(libusb_device_handle *handle) {
	return init_HIDS(handle);
}

static int usb_initAOA(libusb_device_handle *handle, struct aoakvmConfig_t *cfg) {

	libusb_claim_interface(handle, 0);

  unsigned char ioBuffer[2];
  int devVersion;
  int ret = libusb_control_transfer(
	  handle,   //handle
	  0xC0,     //bmRequestType
	  51,       //bRequest
	  0,        //wValue
	  0,        //wIndex
	  ioBuffer, //data
	  2,        //wLength
	  0         //timeout
  );

  if (ret < 0) {
	log_debug("Device does not support AOA %s", cfg->serialNumber);
	return -1;
  }

  devVersion = (int)ioBuffer[1] << 8 | ioBuffer[0];
  log_debug("AOA Version: %04x\n", devVersion);

  /* manufacturer */
  ret = libusb_control_transfer(handle, 0x40, 52, 0, 0,
									 (unsigned char *)cfg->manufacturer, strlen(cfg->manufacturer), 0);
  if (ret < 0) {
	log_error(libusb_error_name(ret));
	return -2;
  }

  /* modelName */
  ret = libusb_control_transfer(handle, 0x40, 52, 0, 1,
									 (unsigned char *)cfg->modelName, strlen(cfg->modelName) + 1, 0);
  if (ret < 0) {
	log_error(libusb_error_name(ret));
	return -2;
  }

  /* description */
  ret = libusb_control_transfer(handle, 0x40, 52, 0, 2,
									 (unsigned char *)cfg->description, strlen(cfg->description) + 1, 0);
  if (ret < 0)
  {
	log_error(libusb_error_name(ret));
	return -2;
  }

  /* version */
  ret = libusb_control_transfer(handle, 0x40, 52, 0, 3,
									 (unsigned char *)cfg->version, strlen(cfg->version) + 1, 0);
  if (ret < 0)
  {
	log_error(libusb_error_name(ret));
	return -2;
  }

  /* uri */
  ret = libusb_control_transfer(handle, 0x40, 52, 0, 4,
									 (unsigned char *)cfg->uri, strlen(cfg->uri) + 1, 0);
  if (ret < 0)
  {
	log_error(libusb_error_name(ret));
	return -2;
  }

  /* serial number */
  ret = libusb_control_transfer(handle, 0x40, 52, 0, 5,
									 (unsigned char *)cfg->serialNumber, strlen(cfg->serialNumber) + 1, 0);
  if (ret < 0)
  {
	log_error(libusb_error_name(ret));
	return -2;
  }

  ret = libusb_control_transfer(handle, 0x40, 53, 0, 0, NULL, 0, 0);
  if (ret < 0)
  {
	log_error(libusb_error_name(ret));
	return -2;
  }

  	libusb_close(handle);
	handle = NULL;

  log_debug("Attempted to put device into accessory mode\n");
  return 0;
}

int usb_read_stream(void *data) {
  struct aoakvmAVCtx_t *render = data;

  AVFormatContext *fmt_ctx = render->fmt_ctx;
  AVCodecContext *codec_ctx = render->codec_ctx;

  AVFrame *frame = av_frame_alloc();
  AVPacket *pkt = av_packet_alloc();

  int ret = 0;
  if (!frame || !pkt) {
	log_error("failed to allocate Frame/Packet structure");
	exit(1);
  }

  while (ret >= 0) {
	ret = av_read_frame(fmt_ctx, pkt);
	if (ret == AVERROR_EOF)
	{
	  log_error("av_read_frame: `");
	  break;
	} else if (ret < 0) {
	  log_error("av_read_frame: %x", ret);
	  usb_setConnectionState(NOT_CONNECTED);
	  continue;
	}

	if (pkt->stream_index == 0) {
	  ret = avcodec_send_packet(codec_ctx, pkt);

	  int ignore_this_frame_flag = 0;
	  switch (ret) {
	  case AVERROR(EAGAIN):
		log_error("avcodec_send_packet \t AVERROR(EAGAIN)");
		exit(1);
		break;

	  case AVERROR_EOF:
		log_error("avcodec_send_packet \t AVERROR_EOF");
		exit(1);
		break;

	  case AVERROR(EINVAL):
		log_error("avcodec_send_packet \t AVERROR(EINVAL)");
		exit(1);
		break;

	  case AVERROR(ENOMEM):
		log_error("avcodec_send_packet \t VERROR(ENOMEM)");
		exit(1);
		break;

	  default:
		if (ret < 0) {
		  //log_warn("other legitimate decoder error!");
		  ignore_this_frame_flag = 1;
		} else {
		  ignore_this_frame_flag = 0;
		}
		break;
	  }

	  if (ignore_this_frame_flag != 1) {
		while (ret >= 0) {
		  ret = avcodec_receive_frame(codec_ctx, frame);

		  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			ret = 0;
			break;
		  } else if (ret < 0) {
			log_error("failed to decode frame");
			break;
		  }

		  ret = 1;
		  while (ret != 0) {
			if (usbCon->status == NOT_CONNECTED) {
			  log_debug("readPackagesFromStream connection loss");
			  return 0;
			}

			ret = fq_pushFrameIntoQueue(frame); // Push Frame into Queue on next position
		  }
		}
	  }
	  av_packet_unref(pkt);
	}
	ret = 0;
  }

  return -1;
}

void usb_writeToPhone(struct usbRequest_t req) {

	if (usbCon->status != CONNECTED) {
		return;
	}

    int ret;
    ret = libusb_control_transfer(usbCon->handle, req.requestType, req.request, req.value, req.index, req.buffer, req.length, req.timeout);
    if(ret < 0) {
        if (ret == LIBUSB_ERROR_NO_DEVICE) {
        usb_setConnectionState(NOT_CONNECTED);
        libusb_close(usbCon->handle);
        usbCon->handle = NULL;
        } else { // This is not allowed to happen
        // TODO: Error Handling
        log_error("libusb_control_transfer: Error while transferrig %s", libusb_error_name(ret));
        return;
        }
    }


	return;
}