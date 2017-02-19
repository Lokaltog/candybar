#include "widgets.h"
#include "free_space.h"

static int
widget_free_space_send_update () {

    json_t *json_data_object = json_object();
    json_t *json_list = json_array();
    char *json_payload;

    int i;
    for (i = 0; i < sizeof(wkline_widget_free_space_mount_points); i++) {
        struct statvfs buf;
        if (statvfs(wkline_widget_free_space_mount_points[i], &buf) != -1) {
            json_t *json_list_entry = json_object();
            json_object_set_new(json_data_object, "mount_point", json_string(wkline_widget_free_space_mount_points[i]));
            json_object_set_new(json_data_object, "space_free", json_real((double) buf.f_bavail * buf.f_bsize));
            json_object_set_new(json_data_object, "space_total", json_real((double) buf.f_blocks * buf.f_frsize));
            json_array_append(json_list, json_list_entry);
        }
    }

    json_object_set_new(json_data_object, "drives", json_list);
    json_payload = json_dumps(json_data_object, 0);
     
    widget_data_t *widget_data = malloc(sizeof(widget_data_t) + 4096);
    widget_data->widget = "free_space";
    widget_data->data = json_payload;
    g_idle_add((GSourceFunc)update_widget, widget_data);

    return 0;
}

void
*widget_free_space () {
    for (;;) {
        widget_free_space_send_update();

        sleep(600);
    }

    return 0;
}
