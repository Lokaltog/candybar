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

static unsigned char*
rgb_to_bmp (uint8_t *rgb, int w, int h) {
	/* blank bmp headers for 24-bit bitmaps */
	unsigned char file[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 40 + 14, 0, 0, 0 };
	unsigned char info[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x13, 0x0B, 0, 0, 0x13, 0x0B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	int size_pad = (4 - w % 4) % 4;
	int size_data = w * h * 3 + h * size_pad;
	int size_all = size_data + sizeof(file) + sizeof(info);

	unsigned char *ret = malloc(size_all);
	unsigned int offset = 0;

	file[2] = (unsigned char)(size_all);
	file[3] = (unsigned char)(size_all >> 8);
	file[4] = (unsigned char)(size_all >> 16);
	file[5] = (unsigned char)(size_all >> 24);

	info[4] = (unsigned char)(w);
	info[5] = (unsigned char)(w >> 8);
	info[6] = (unsigned char)(w >> 16);
	info[7] = (unsigned char)(w >> 24);

	info[8] = (unsigned char)(h);
	info[9] = (unsigned char)(h >> 8);
	info[10] = (unsigned char)(h >> 16);
	info[11] = (unsigned char)(h >> 24);

	info[24] = (unsigned char)(size_data);
	info[25] = (unsigned char)(size_data >> 8);
	info[26] = (unsigned char)(size_data >> 16);
	info[27] = (unsigned char)(size_data >> 24);

	memcpy(&ret[offset], file, LENGTH(file));
	offset += LENGTH(file);
	memcpy(&ret[offset], info, LENGTH(info));
	offset += LENGTH(info);

	int pos;
	int line_len = (3 * (w + 1) / 4) * 4;
	unsigned char *line = malloc(line_len);

	for (int y = h - 1; y >= 0; y--) {
		for (int x = 0; x < w; x++) {
			pos = 4 * (w * y + x);

			/* FIXME should check for endianness, the resulting bg
			   image may look a bit wonky on big-endian systems */
			line[3 * x] = rgb[pos];
			line[3 * x + 1] = rgb[pos + 1];
			line[3 * x + 2] = rgb[pos + 2];
		}

		memcpy(&ret[offset], line, line_len);
		offset += line_len;
	}

	free(line);

	return ret;
}

void*
widget_main (struct widget *widget) {
	struct widget_config config = widget_config_defaults;
	widget_init_config_string(widget->config, "image", config.image);
	widget_init_config_string(widget->config, "css_gradient_overlay", config.css_gradient_overlay);
	widget_init_config_integer(widget->config, "blur_radius", config.blur_radius);
	widget_init_config_integer(widget->config, "brightness", config.brightness);
	widget_init_config_integer(widget->config, "saturation", config.saturation);

	MagickWand *m_wand = NULL;
	MagickPassFail status = MagickPass;
	int width, height;
	xcb_connection_t *conn = NULL;
	xcb_get_property_reply_t *pixmap_r = NULL;
	xcb_intern_atom_reply_t *atom_r = NULL;
	xcb_generic_error_t *err = NULL;
	xcb_get_image_reply_t *im_r = NULL;
	char *img_base64 = NULL;

	InitializeMagick(NULL);
	m_wand = NewMagickWand();

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

		size_t data_len = xcb_get_image_data_length(im_r);
		uint8_t *data = xcb_get_image_data(im_r);
		unsigned char *blob = rgb_to_bmp(data, widget->wkline->width, widget->wkline->height);

		status = MagickReadImageBlob(m_wand, blob, data_len);

		free(blob);
		if (status != MagickPass) {
			LOG_ERR("could not read background from root window");

			return 0;
		}
	}
	else {
		status = MagickReadImage(m_wand, config.image);
		if (status != MagickPass) {
			LOG_ERR("could not read image %s", config.image);

			return 0;
		}
	}

	width = MagickGetImageWidth(m_wand);
	height = widget->wkline->height;

	/* modify image */
	MagickCropImage(m_wand, width, height, 0, 0);

	if (config.blur_radius != 0) {
		MagickBlurImage(m_wand, config.blur_radius, config.blur_radius);
	}
	if ((config.brightness != 100) || (config.saturation != 100)) {
		MagickModulateImage(m_wand, config.brightness, config.saturation, 100);
	}

	/* get image jpg data and encode it to base64 */
	MagickSetCompressionQuality(m_wand, 95);
	MagickSetFormat(m_wand, "jpg");

	size_t img_len;
	unsigned char *img_data = MagickWriteImageBlob(m_wand, &img_len);
	img_base64 = g_base64_encode(img_data, img_len);

	widget_data_callback(widget,
	                     widget_data_arg_string(img_base64),
	                     widget_data_arg_string(config.css_gradient_overlay));

cleanup:
	g_free(img_base64);
	DestroyMagickWand(m_wand);
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

	return 0;
}
