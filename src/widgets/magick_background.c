#include "widgets.h"
#include "magick_background.h"

static
xcb_screen_t*
screen_of_display (xcb_connection_t *c, int screen) {
	xcb_screen_iterator_t iter;

	iter = xcb_setup_roots_iterator(xcb_get_setup(c));
	for (; iter.rem; --screen, xcb_screen_next(&iter)) {
		if (screen == 0) {
			return iter.data;
		}
	}

	return NULL;
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "image", config.image);
	widget_init_config_string(widget->config, "css_gradient_overlay", config.css_gradient_overlay);
	widget_init_config_integer(widget->config, "blur_radius", config.blur_radius);
	widget_init_config_integer(widget->config, "brightness", config.brightness);
	widget_init_config_integer(widget->config, "saturation", config.saturation);

	InitializeMagick(NULL);

	xcb_connection_t *conn = NULL;
	xcb_get_property_reply_t *pixmap_r = NULL;
	xcb_intern_atom_reply_t *atom_r = NULL;
	xcb_generic_error_t *err = NULL;
	xcb_get_image_reply_t *im_r = NULL;

	size_t img_len;
	void *img_data;
	char *img_base64 = NULL;
	Image *img = NULL;
	ImageInfo *img_info = CloneImageInfo(0);
	ExceptionInfo exception;
	GetExceptionInfo(&exception);

	if (!strlen(config.image)) {
		int screen_nbr = 0;
		conn = xcb_connect(NULL, NULL);
		if (xcb_connection_has_error(conn)) {
			LOG_ERR("X connection invalid");
			goto cleanup;
		}
		xcb_intern_atom_cookie_t atom_c;
		xcb_get_property_cookie_t pixmap_c;
		xcb_get_image_cookie_t im_c;

		xcb_screen_t *screen = screen_of_display(conn, screen_nbr);
		xcb_drawable_t root_pixmap = XCB_NONE;
		const char *atom = "_XROOTPMAP_ID";
		atom_c = xcb_intern_atom_unchecked(conn, false, strlen(atom), atom);
		atom_r = xcb_intern_atom_reply(conn, atom_c, NULL);
		if (!atom_r) {
			LOG_ERR("could not get %s atom", atom);
			goto cleanup;
		}

		pixmap_c = xcb_get_property_unchecked(conn, false, screen->root, atom_r->atom, XCB_ATOM_PIXMAP, 0, 1);
		if ((pixmap_r = xcb_get_property_reply(conn, pixmap_c, NULL))) {
			if (!pixmap_r->value_len) {
				LOG_ERR("could not get background pixmap");
				goto cleanup;
			}
			root_pixmap = *(xcb_drawable_t*)xcb_get_property_value(pixmap_r);
		}

		im_c = xcb_get_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, root_pixmap,
		                     0, 0, widget->wkline->width, widget->wkline->height, 0xffffffff);
		im_r = xcb_get_image_reply(conn, im_c, &err);
		if (err != NULL) {
			LOG_ERR("could not get background image");
			goto cleanup;
		}

		uint8_t *data = xcb_get_image_data(im_r);

		img = ConstituteImage(widget->wkline->width, widget->wkline->height, "BGRA", CharPixel, data, &exception);
		strncpy(img->magick, "png", MaxTextExtent - 1);
		if (exception.severity != UndefinedException) {
			LOG_ERR("could not read background from root window: %s", exception.reason);
			goto cleanup;
		}
	}
	else {
		strncpy(img_info->filename, config.image, MaxTextExtent - 1);
		RectangleInfo geom = { widget->wkline->width, widget->wkline->height, 0, 0 };
		img = ReadImage(img_info, &exception);
		strncpy(img->magick, "png", MaxTextExtent - 1);
		if (exception.severity != UndefinedException) {
			LOG_ERR("could not read image '%s': %s", config.image, exception.reason);
			goto cleanup;
		}
		img = CropImage(img, &geom, &exception);
	}

	if (config.blur_radius != 0) {
		img = GaussianBlurImage(img, config.blur_radius, config.blur_radius, &exception);
	}
	if ((config.brightness != 100) || (config.saturation != 100)) {
		char modulate_str[20];
		snprintf(modulate_str, 19, "%i,%i,%i", config.brightness, config.saturation, 100);
		ModulateImage(img, (const char*)&modulate_str);
	}

	img_data = ImageToBlob(img_info, img, &img_len, &exception);
	if (exception.severity != UndefinedException) {
		LOG_ERR("could not write image blob: %s", exception.reason);
		goto cleanup;
	}

	img_base64 = g_base64_encode(img_data, img_len);

	widget_data_callback(widget,
	                     widget_data_arg_string(img_base64),
	                     widget_data_arg_string(config.css_gradient_overlay));

cleanup:
	g_free(img_base64);
	DestroyImageInfo(img_info);
	DestroyMagick();
	if (conn != NULL) {
		xcb_disconnect(conn);
	}
	if (err != NULL) {
		free(err);
	}
	if (atom_r != NULL) {
		free(atom_r);
	}
	if (pixmap_r != NULL) {
		free(pixmap_r);
	}
	if (im_r != NULL) {
		free(im_r);
	}
	if (img_data != NULL) {
		free(img_data);
	}
	if (img != NULL) {
		free(img);
	}

	return 0;
}
