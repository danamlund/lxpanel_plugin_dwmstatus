/**
 * dwmstatus plugin to lxpanel
 *
 * Copyright (C) 2014 by Dan Amlund Thomsen <dan@danamlund.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* version 1.1 */

#define STR_FORMAT "<span color=\"#FFFFFF\">%s</span>"

#include <stdio.h>
#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "lxpanel/plugin.h"

typedef struct {
  unsigned int timer;
  Display *dpy;
  GtkWidget *label;
  GtkComboBox *combo_box;
} DwmStatusPlugin; 

static char* error_string = "ERROR";


static int update_display(DwmStatusPlugin *sp) {
  char *window_name;
  if (NULL == sp->dpy) {
    window_name = error_string;
  }
  if (!XFetchName(sp->dpy, DefaultRootWindow(sp->dpy), &window_name)) {
    window_name = error_string;
  }

  if (NULL == window_name) {
    window_name = error_string;
  }

  char str[1024];

  sprintf(str, STR_FORMAT, window_name);

  gtk_label_set_markup(GTK_LABEL(sp->label), str);
  GtkRequisition size;
  gtk_widget_size_request(GTK_WIDGET(sp->label), &size); 
  gtk_widget_set_size_request(GTK_WIDGET(sp->label),
                              size.width, -1);
  if (window_name != error_string) {
    XFree(window_name);
  }
  return 1;
}


static int dwmstatus_constructor(Plugin * p, char ** fp) {
  DwmStatusPlugin *sp = g_new0(DwmStatusPlugin, 1);

  sp->dpy = XOpenDisplay(NULL);

  sp->timer = g_timeout_add_seconds(1, (GSourceFunc) update_display,
                                    (gpointer) sp);

  p->priv = sp;

  GtkWidget *label = gtk_label_new("..");
  sp->label = label;
  // Align x=right, y=middle
  gtk_misc_set_alignment(GTK_MISC(sp->label), 1.0, 0.5);
  p->pwid = gtk_event_box_new();
  gtk_container_set_border_width(GTK_CONTAINER(p->pwid), 3);
  gtk_container_add(GTK_CONTAINER(p->pwid), GTK_WIDGET(label));
  gtk_widget_set_has_window(p->pwid, FALSE);
  gtk_widget_show_all(p->pwid);
  update_display(sp);

  return 1;
}

static void dwmstatus_destructor(Plugin * p) {
  DwmStatusPlugin *sp = p->priv;
  g_source_remove(sp->timer);
  g_free(sp);
}

static void dwmstatus_configure(Plugin * p, GtkWindow * parent) {
}

static void dwmstatus_save_configuration(Plugin * p, FILE * fp) {
}

PluginClass dwmstatus_plugin_class = {
  PLUGINCLASS_VERSIONING,
  type : "dwmstatus",
  name : N_("dwmstatus displayer"),
  version: "1.0",
  description : N_("Shows the status bar like dwm"),

  one_per_system : FALSE,
  expand_available : FALSE,

  constructor : dwmstatus_constructor,
  destructor  : dwmstatus_destructor,
  config : dwmstatus_configure,
  save : dwmstatus_save_configuration
};
