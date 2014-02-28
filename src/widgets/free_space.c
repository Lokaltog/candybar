#include "widgets.h"
#include "free_space.h"

static int
widget_free_space_send_update () {
    struct statvfs buf;
    lldiv_t final;
    double raw;

    if (statvfs(wkline_widget_free_space_dir, &buf) != -1) {
        raw = (double) buf.f_bavail * buf.f_bsize;
        final = lldiv(raw, 1073741824);
    }
     
    widget_data_t *widget_data = malloc(sizeof(widget_data_t) + 4096);
    widget_data->widget = "free_space";
    widget_data->data = final.quot;
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
