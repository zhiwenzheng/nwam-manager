/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set expandtab ts=4 shiftwidth=4: */
/* 
 * Copyright 2007-2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * CDDL HEADER START
 * 
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 * 
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * 
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 * 
 * CDDL HEADER END
 * 
 */

#define DEF_CUSTOM_TREEVIEW_TOOLTIP 0

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <strings.h>
#include <stdlib.h>

#include "libnwamui.h"
#include "status_icon_tooltip.h"
#include "nwam-tooltip-widget.h"

#include "nwam-menuitem.h"
#include "nwam-wifi-item.h"
#include "nwam-enm-item.h"
#include "nwam-env-item.h"
#include "nwam-ncu-item.h"

#define TOOLTIP_WIDGET_DATA "sitw_label"

typedef struct _NwamTooltipWidgetPrivate	NwamTooltipWidgetPrivate;
#define NWAM_TOOLTIP_WIDGET_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NWAM_TYPE_TOOLTIP_WIDGET, NwamTooltipWidgetPrivate))

struct _NwamTooltipWidgetPrivate {
#if DEF_CUSTOM_TREEVIEW_TOOLTIP
    GtkTreeView *tooltip_treeview;
#else
    GtkWidget *env_widget;
    GtkWidget *ncp_widget;
    GtkWidget *vpn_widget;
    GtkWidget *ncu_vbox;

    /* Other */
    GList *w_list;
#endif
};


static void nwam_tooltip_widget_finalize (NwamTooltipWidget *self);

static gint tooltip_ncu_compare(NwamuiObject *a, NwamuiObject *b); /* unused */

#if DEF_CUSTOM_TREEVIEW_TOOLTIP
static void nwam_compose_tree_view(NwamTooltipWidget *self);
#endif

/* call back */
static void tooltip_treeview_cell (GtkTreeViewColumn *col,
  GtkCellRenderer   *renderer,
  GtkTreeModel      *model,
  GtkTreeIter       *iter,
  gpointer           data);

G_DEFINE_TYPE(NwamTooltipWidget, nwam_tooltip_widget, GTK_TYPE_VBOX)

static void
nwam_tooltip_widget_class_init (NwamTooltipWidgetClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass*) klass;
        
    gobject_class->finalize = (void (*)(GObject*)) nwam_tooltip_widget_finalize;

    g_type_class_add_private (klass, sizeof (NwamTooltipWidgetPrivate));
}
        
static void
nwam_tooltip_widget_init (NwamTooltipWidget *self)
{
	NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);
#if DEF_CUSTOM_TREEVIEW_TOOLTIP
    GtkTreeModel *model = GTK_TREE_MODEL(gtk_list_store_new(1, NWAMUI_TYPE_OBJECT));
    GtkTreeIter iter;

    prv->tooltip_treeview = gtk_tree_view_new_with_model(model);
    /* Tooltip: env */
    gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);
    /* Tooltip: profile */
    gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);
    g_object_unref(model);
    nwam_compose_tree_view(self);

    gtk_box_pack_start(GTK_BOX(self), GTK_WIDGET(prv->tooltip_treeview),
      TRUE, TRUE, 1);
/*         GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL); */
/*         gtk_window_set_decorated(GTK_WINDOW(win), TRUE); */
/*         gtk_window_set_screen(GTK_WINDOW(win), gtk_status_icon_get_screen(status_icon)); */
/*         gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_MOUSE); */
/*         gtk_widget_set_parent_window(GTK_WIDGET(prv->tooltip_treeview), */
/*           gtk_widget_get_tooltip_window(GTK_WIDGET(user_data))); */
/*         GtkWidget *p = gtk_widget_get_parent(GTK_WIDGET(prv->tooltip_treeview)); */
/*         gtk_widget_set_parent_window(GTK_WIDGET(prv->tooltip_treeview), */
/*           gtk_widget_get_root_window(p)); */
/*         gtk_widget_show(GTK_WIDGET(prv->tooltip_treeview)); */

#else

    prv->env_widget = GTK_WIDGET(g_object_ref(nwam_object_tooltip_widget_new(NULL)));
    gtk_widget_show(prv->env_widget);
    gtk_box_pack_start(GTK_BOX(self), prv->env_widget, TRUE, TRUE, 1);

    prv->ncp_widget = GTK_WIDGET(g_object_ref(nwam_object_tooltip_widget_new(NULL)));
    gtk_widget_show(prv->ncp_widget);
    gtk_box_pack_start(GTK_BOX(self), prv->ncp_widget, TRUE, TRUE, 1);

    prv->vpn_widget = GTK_WIDGET(g_object_ref(nwam_menu_item_new()));
    gtk_widget_show(prv->vpn_widget);
    gtk_box_pack_start(GTK_BOX(self), prv->vpn_widget, TRUE, TRUE, 1);

    prv->ncu_vbox = gtk_vbox_new(TRUE, 0);
    gtk_widget_show(prv->ncu_vbox);
    gtk_box_pack_start(GTK_BOX(self), prv->ncu_vbox, TRUE, TRUE, 1);
#endif
}

static void
nwam_tooltip_widget_finalize (NwamTooltipWidget *self)
{
	NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);

	G_OBJECT_CLASS(nwam_tooltip_widget_parent_class)->finalize(G_OBJECT(self));
}

#if DEF_CUSTOM_TREEVIEW_TOOLTIP
static void
tooltip_treeview_cell (GtkTreeViewColumn *col,
  GtkCellRenderer   *renderer,
  GtkTreeModel      *model,
  GtkTreeIter       *iter,
  gpointer           data)
{
    gint          cell_num = (gint)data; /* Number of cell in column */
    NwamuiObject *object;
    GType         type;
    GtkWidget    *item;
	GString      *gstr;
    gchar        *name;

	gtk_tree_model_get(model, iter, 0, &object, -1);

    if (!object)
        return;

    gstr = g_string_new("");
    type = G_OBJECT_TYPE(object);

    name = nwamui_object_get_name(object);

	if (type == NWAMUI_TYPE_NCU) {
        if (cell_num == 0) {
        } else {
            gchar *state = nwamui_ncu_get_connection_state_detail_string(NWAMUI_NCU(object), FALSE);
            switch (nwamui_ncu_get_ncu_type(NWAMUI_NCU(object))) {
            case NWAMUI_NCU_TYPE_WIRELESS:
                g_string_append_printf(gstr, _("<b>Wireless (%s):</b> %s"),
                  name, state);
                break;
            case NWAMUI_NCU_TYPE_WIRED:
            case NWAMUI_NCU_TYPE_TUNNEL:
                g_string_append_printf(gstr, _("<b>Wired (%s):</b> %s"),
                  name, state);
                break;
            default:
                g_assert_not_reached ();
            }
            g_free(state);
            g_object_set(renderer, "markup", gstr->str, NULL);
        }
	} else if (type == NWAMUI_TYPE_ENV) {
        if (cell_num == 0) {
        } else {
            g_string_append_printf(gstr, _("<b>Location:</b> %s"), name);
            g_object_set(renderer, "markup", gstr->str, NULL);
        }
	} else if (type == NWAMUI_TYPE_NCP) {
        if (cell_num == 0) {
        } else {
            g_string_append_printf(gstr, _("<b>Network Profile:</b> %s"), name);
            g_object_set(renderer, "markup", gstr->str, NULL);
        }
/* 	} else if (type == NWAMUI_TYPE_WIFI_NET) { */
/* 	} else if (type == NWAMUI_TYPE_ENM) { */
/* 	} else { */
	}
    g_string_free(gstr, TRUE);
    g_object_unref(object);
}

static void
nwam_compose_tree_view(NwamTooltipWidget *self)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	GtkTreeView *view = prv->tooltip_treeview;

	g_object_set (view,
      "headers-clickable", FALSE,
      "headers-visible", FALSE,
      "reorderable", FALSE,
      "enable-search", FALSE,
      NULL);

    {
        col = gtk_tree_view_column_new();
        gtk_tree_view_append_column (view, col);

        g_object_set(col,
          "title", _("Connection Icon"),
          "resizable", TRUE,
          "clickable", TRUE,
          "sort-indicator", TRUE,
          "reorderable", TRUE,
          NULL);

        /* Add 1st "type" icon cell */
        cell = gtk_cell_renderer_pixbuf_new();
        gtk_tree_view_column_pack_start(col, cell, FALSE);
        gtk_tree_view_column_set_cell_data_func (col,
          cell,
          tooltip_treeview_cell,
          (gpointer) 0,
          NULL);
        /* Add 2nd object name and state */
        cell = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_end(col, cell, TRUE);
        gtk_tree_view_column_set_cell_data_func (col,
          cell,
          tooltip_treeview_cell,
          (gpointer) 1, /* Important to know this is wireless cell in 1st col */
          NULL);
    } /* column icon */
}
#endif

void
nwam_tooltip_widget_update_env(NwamTooltipWidget *self, NwamuiObject *object)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);

    g_assert(NWAMUI_IS_ENV(object));

#if DEF_CUSTOM_TREEVIEW_TOOLTIP
    GtkTreeModel *model = gtk_tree_view_get_model(prv->tooltip_treeview);
    GtkTreeIter iter;

    gtk_tree_model_iter_nth_child(model, &iter, NULL, 0);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, object, -1);
#else
    g_object_set(prv->env_widget, "proxy-object", object, NULL);
#endif
}

void
nwam_tooltip_widget_update_enm(NwamTooltipWidget *self, const gchar *vpn_info)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);
    gchar *text;

    text = g_strdup_printf(_("<b>Network Modifiers:</b> %s"), vpn_info);
    menu_item_set_markup(GTK_MENU_ITEM(prv->vpn_widget), text);
    g_free(text);
}

static void
foreach_ncu_update_tooltip(gpointer obj, gpointer self)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);

    g_return_if_fail(NWAMUI_IS_OBJECT(obj));

    if (prv->w_list) {
        g_object_set(prv->w_list->data, "proxy-object", obj, NULL);
        prv->w_list = g_list_delete_link(prv->w_list, prv->w_list);
    } else {
        nwam_tooltip_widget_add_ncu(NWAM_TOOLTIP_WIDGET(self), obj);
    }
}

void
nwam_tooltip_widget_update_ncp(NwamTooltipWidget *self, NwamuiObject *ncp)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);

    g_return_if_fail(NWAMUI_IS_NCP(ncp));

    g_object_set(prv->ncp_widget, "proxy-object", ncp, NULL);

    /* Get list of children, remove non-NCU children and then process.  */
    g_assert(prv->w_list == NULL);
    prv->w_list = gtk_container_get_children(GTK_CONTAINER(prv->ncu_vbox));

    /* For each entry in the ncu_list, re-use any existing w_list nodes by
     * changing their proxy-objects.
     */
    nwamui_ncp_foreach_ncu(NWAMUI_NCP(ncp), (GFunc)foreach_ncu_update_tooltip, (gpointer)self);

    /* Any remaining w_list entries, then remove them since they are unused.
     */
    for (; prv->w_list; ) {
        gtk_container_remove(GTK_CONTAINER(prv->ncu_vbox), GTK_WIDGET(prv->w_list->data));
        prv->w_list = g_list_delete_link(prv->w_list, prv->w_list);
    }
}

void
nwam_tooltip_widget_add_ncu(NwamTooltipWidget *self, NwamuiObject *object)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);

    g_assert(NWAMUI_IS_OBJECT(object));

#if DEF_CUSTOM_TREEVIEW_TOOLTIP
    GtkTreeModel *model = gtk_tree_view_get_model(prv->tooltip_treeview);
    GtkTreeIter iter;

    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, object, -1);
#else
    GtkWidget *label = nwam_object_tooltip_widget_new(NWAMUI_OBJECT(object));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(prv->ncu_vbox), label, TRUE, TRUE, 1);
#endif
}

void
nwam_tooltip_widget_remove_ncu(NwamTooltipWidget *self, NwamuiObject *object)
{
    NwamTooltipWidgetPrivate *prv = NWAM_TOOLTIP_WIDGET_GET_PRIVATE(self);

    g_assert(NWAMUI_IS_OBJECT(object));

#if DEF_CUSTOM_TREEVIEW_TOOLTIP
    GtkTreeModel *model = gtk_tree_view_get_model(prv->tooltip_treeview);
    GtkTreeIter iter;
    NwamuiObject *ncu;

    if (gtk_tree_model_iter_nth_child(model, &iter, NULL, 2)) {
        do {
            gtk_tree_model_get(model, 0, &ncu, -1);
            if (ncu == (gpointer)object) {
                gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
                g_object_unref(ncu);
                break;
            }
            g_object_unref(ncu);
        } while (gtk_tree_model_iter_next(model, &iter));
    }
#else
    GtkWidget *label;
    GList *w_list;

    w_list = gtk_container_get_children(GTK_CONTAINER(prv->ncu_vbox));

    for (; w_list;) {
        NwamuiObject *old = (NwamuiObject*)nwam_obj_proxy_get_proxy(NWAM_OBJ_PROXY_IFACE(w_list->data));
        
        if (old == (gpointer)object) {
            gtk_container_remove(GTK_CONTAINER(prv->ncu_vbox), GTK_WIDGET(w_list->data));
            break;
        }
        w_list = g_list_delete_link(w_list, w_list);
    }
    g_list_free(w_list);
#endif
}

GtkWidget*
nwam_tooltip_widget_new(void)
{
    return GTK_WIDGET(g_object_new(NWAM_TYPE_TOOLTIP_WIDGET,
        "homogeneous", FALSE,
        NULL));
}

static gint
tooltip_ncu_compare(NwamuiObject *a, NwamuiObject *b)
{
    if (NWAMUI_IS_NCU(a) && NWAMUI_IS_NCU(b)) {
        return nwamui_object_sort(a, b, NWAMUI_OBJECT_SORT_BY_GROUP);
    }
    return 0;
}
