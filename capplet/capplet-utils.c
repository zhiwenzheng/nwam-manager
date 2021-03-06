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

#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "capplet-utils.h"

void
capplet_list_foreach_merge_to_list_store(gpointer data, gpointer user_data)
{
    CAPPLET_LIST_STORE_ADD(user_data, data);
}

void
capplet_list_foreach_merge_to_tree_store(gpointer data, gpointer user_data)
{
    GtkTreeIter iter;
    CAPPLET_TREE_STORE_ADD(user_data, NULL, data, &iter);
}

static gboolean
capplet_model_foreach_find_by_name(GtkTreeModel *model,
    GtkTreePath *path,
    GtkTreeIter *iter,
    gpointer user_data)
{
    CappletForeachData *data = (CappletForeachData *)user_data;
    NwamuiObject   *object;
    NwamuiObject   *search_obj = NWAMUI_OBJECT(data->user_data);
    const gchar    *search_name = (const gchar*)data->user_data1;
    gboolean       *found_p = (gboolean*)data->ret_data;

    if ( search_obj != NULL ) {
        gtk_tree_model_get( GTK_TREE_MODEL(model), iter, 0, &object, -1);

        if ( object && object != search_obj ) { /* Don't check self or NULL object */
            const gchar   *name = nwamui_object_get_name(NWAMUI_OBJECT(object));

            if ( name != NULL && strcmp(name, search_name) == 0 ) {
                *found_p = TRUE;
            }
        }
    }

    return( *found_p ); /* Stop search if we found something */
}

static gboolean 
capplet_model_check_name_exists( GtkTreeModel *model, NwamuiObject* obj, const gchar* new_text )
{
    CappletForeachData  data;
    gboolean            exists = FALSE;
    NwamuiDaemon       *daemon;

    g_return_val_if_fail( model != NULL && obj != NULL, exists );

	data.user_data = (gpointer)obj;
	data.user_data1 = (gpointer)new_text;
	data.ret_data = (gpointer)&exists;

	gtk_tree_model_foreach(model, capplet_model_foreach_find_by_name,
                           (gpointer)&data);

    return( exists );
}

void
capplet_model_remove_object(GtkTreeModel *model, GObject *object)
{
	GtkTreeIter iter;

	if (capplet_model_find_object(model, object, &iter)) {
		if (GTK_IS_LIST_STORE(model))
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		else
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	}
}

void
capplet_update_model_from_daemon(GtkTreeModel *model, NwamuiDaemon *daemon, GType type)
{
	gtk_list_store_clear(GTK_LIST_STORE(model));

	if (type == NWAMUI_TYPE_NCP) {
        nwamui_daemon_foreach_ncp(daemon, capplet_list_foreach_merge_to_list_store, (gpointer)model);
	/* } else if (type == NWAMUI_TYPE_NCU) { */
	} else if (type == NWAMUI_TYPE_ENV) {
        nwamui_daemon_foreach_loc(daemon, capplet_list_foreach_merge_to_list_store, (gpointer)model);
	} else if (type == NWAMUI_TYPE_ENM) {
        nwamui_daemon_foreach_enm(daemon, capplet_list_foreach_merge_to_list_store, (gpointer)model);
	} else {
		g_error("unknow supported get nwamui obj list");
	}
}

void
nwamui_object_name_cell (GtkCellLayout *cell_layout,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           data)
{
	NwamuiObject *object = NULL;
	
	gtk_tree_model_get(model, iter, 0, &object, -1);
	
	if (object) {
		g_assert(NWAMUI_IS_OBJECT (object));
	
        g_object_set (G_OBJECT(renderer), "text", nwamui_object_get_name(object), NULL);

		g_object_unref(object);
	} else {
        g_object_set (G_OBJECT(renderer), "text", _("No name"), NULL);
	}
}

void
nwam_pref_iface_combo_changed_cb(GtkComboBox *combo, gpointer user_data)
{
	GtkTreeIter iter;
    GObject *obj;

    obj = capplet_combo_get_active_object(combo);
    g_assert(G_IS_OBJECT(obj));
    nwam_pref_refresh(NWAM_PREF_IFACE(user_data), obj, FALSE);
    g_object_unref(obj);
}

GtkWidget *
capplet_compose_combo(GtkComboBox *combo,
  GType tree_store_type,
  GType type,
  GtkCellLayoutDataFunc layout_func,
  GtkTreeViewRowSeparatorFunc separator_func,
  GCallback changed_func,
  gpointer user_data,
  GDestroyNotify destroy)
{
	GtkCellRenderer *renderer;
	GtkTreeModel      *model;
        
    g_return_val_if_fail(combo, NULL);

	if (type != G_TYPE_STRING) {
		g_return_val_if_fail(layout_func, NULL);

        if (tree_store_type == GTK_TYPE_LIST_STORE)
            model = GTK_TREE_MODEL(gtk_list_store_new(1, type));
        else if (tree_store_type == GTK_TYPE_TREE_STORE)
            model = GTK_TREE_MODEL(gtk_tree_store_new(1, type));
        else
            g_assert_not_reached();

		gtk_combo_box_set_model(GTK_COMBO_BOX(combo), model);
		g_object_unref(model);

		gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
		gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo),
		    renderer,
		    layout_func,
		    user_data,
		    destroy);
	}
	if (separator_func) {
		gtk_combo_box_set_row_separator_func(combo,
		    separator_func,
		    user_data,
		    destroy);
	}
	if (changed_func) {
		g_signal_connect(G_OBJECT(combo),
		    "changed",
		    changed_func,
		    user_data);
	}
    return GTK_WIDGET(combo);
}

GtkWidget *
capplet_compose_combo_with_model(GtkComboBox *combo,
  GtkTreeModel *model,
  GtkCellRenderer *renderer,
  GtkCellLayoutDataFunc layout_func,
  GtkTreeViewRowSeparatorFunc separator_func,
  GCallback changed_func,
  gpointer user_data,
  GDestroyNotify destroy)
{
    g_return_val_if_fail(combo, NULL);
    g_return_val_if_fail(layout_func, NULL);

    gtk_combo_box_set_model(GTK_COMBO_BOX(combo), model);

    gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);

    if (layout_func) {
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo),
          renderer,
          layout_func,
          user_data,
          destroy);
    }
	if (separator_func) {
		gtk_combo_box_set_row_separator_func(combo,
		    separator_func,
		    user_data,
		    destroy);
	}
	if (changed_func) {
		g_signal_connect(G_OBJECT(combo),
		    "changed",
		    changed_func,
		    user_data);
	}
    return GTK_WIDGET(combo);
}

void
capplet_remove_gtk_dialog_escape_binding(GtkDialogClass *dialog_class)
{
	GtkBindingSet *binding_set;

	binding_set = gtk_binding_set_by_class(dialog_class);
  
	gtk_binding_entry_remove(binding_set, GDK_Escape, 0);
}

gboolean
capplet_dialog_raise(NwamPrefIFace *iface)
{
    GtkWindow*  window;

    if ( ( window = nwam_pref_dialog_get_window(iface) ) != NULL ) {
        gtk_window_present(window);
        return ( TRUE );
    }

	return FALSE;
}

void
nwamui_object_name_cell_edited ( GtkCellRendererText *cell,
                     const gchar         *path_string,
                     const gchar         *new_text,
                     gpointer             data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(data);
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;

	if ( gtk_tree_model_get_iter (model, &iter, path)) {
		NwamuiObject*   object;
        gboolean        editable;

		gtk_tree_model_get(model, &iter, 0, &object, -1);

        editable = nwamui_object_can_rename( NWAMUI_OBJECT(object) );

        if ( !editable ) {
            GtkWindow  *top_level = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview)));
            gchar      *message;

            message = g_strdup(_("\n\nThe name of a committed object cannot be changed."));

            nwamui_util_show_message( top_level, GTK_MESSAGE_ERROR, _("Renaming Failed"), message, FALSE);

            g_free(message);
        }
        else if ( !capplet_model_check_name_exists( model, NWAMUI_OBJECT(object), new_text ) ) {
            nwamui_object_set_name(object, new_text);
        }
        else {
            GtkWindow  *top_level = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview)));
            gchar      *message;

            message = g_strdup_printf(_("The name '%s' is already in use"), new_text );

            nwamui_util_show_message( top_level, GTK_MESSAGE_ERROR, _("Name Already In Use"), message, FALSE);

            g_free(message);
        }

		g_object_unref(object);
	}
	gtk_tree_path_free (path);
}

void
nwamui_object_active_toggle_cell(GtkTreeViewColumn *col,
  GtkCellRenderer   *renderer,
  GtkTreeModel      *model,
  GtkTreeIter       *iter,
  gpointer           data)
{
    NwamuiObject*              object = NULL;
    
    gtk_tree_model_get(model, iter, 0, &object, -1);
    
    if ( object != NULL ) {
        /* We need show the active object instead of enabled object. */
        g_object_set(G_OBJECT(renderer),
          "active", nwamui_object_get_active(NWAMUI_OBJECT(object)),
          NULL); 
        g_object_unref(G_OBJECT(object));
    } else {
        g_object_set(G_OBJECT(renderer),
          "active", FALSE,
          NULL); 
    }
}

void
nwamui_object_active_mode_pixbuf_cell (GtkTreeViewColumn *col,
  GtkCellRenderer   *renderer,
  GtkTreeModel      *model,
  GtkTreeIter       *iter,
  gpointer           data)
{
    NwamuiObject*              object = NULL;
    gchar*                  object_markup;
    
    gtk_tree_model_get(model, iter, 0, &object, -1);
    
    if (object) {
        g_object_set(renderer,
          "icon-name", nwamui_util_get_active_mode_icon(object),
          NULL);

		g_object_unref(object);
    }

    return;
}

GtkTreeViewColumn *
capplet_column_new(GtkTreeView *treeview, ...)
{
	GtkTreeViewColumn *col = gtk_tree_view_column_new();
	va_list args;
	gchar *attribute;

	va_start(args, treeview);
	attribute = va_arg(args, gchar *);
	if (attribute)
		g_object_set_valist(G_OBJECT(col), attribute, args);
	va_end (args);

	gtk_tree_view_append_column (treeview, col);
	return col;
}

GtkCellRenderer *
capplet_column_append_cell(GtkTreeViewColumn *col,
    GtkCellRenderer *cell, gboolean expand,
    GtkTreeCellDataFunc func, gpointer user_data, GDestroyNotify destroy)
{
	gtk_tree_view_column_pack_start(col, cell, expand);
	gtk_tree_view_column_set_cell_data_func (col, cell, func, user_data, destroy);
	return cell;
}

GObject *
capplet_combo_get_active_object(GtkComboBox *combo)
{
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		GObject *object;
		gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter,
		    0, &object, -1);
		return object;
	}
	return NULL;
}

gboolean
capplet_combo_set_active_object(GtkComboBox *combo, GObject *object)
{
	GtkTreeIter iter;

	if (capplet_model_find_object(gtk_combo_box_get_model(combo), object, &iter)) {
		gtk_combo_box_set_active_iter(combo, &iter);
        return TRUE;
    }
    return FALSE;
}

void
capplet_tree_store_move_object(GtkTreeModel *model,
    GtkTreeIter *target,
    GtkTreeIter *source)
{
	NwamuiObject *object;
	gtk_tree_model_get(GTK_TREE_MODEL(model), source, 0, &object, -1);
	gtk_tree_store_set(GTK_TREE_STORE(model), target, 0, object, -1);
    if (object)
        g_object_unref(object);
}

/**
 * capplet_tree_model_foreach_nwamui_object_reload:
 *
 * All objects in tree model are revert.
 */
gboolean
capplet_tree_model_foreach_nwamui_object_reload(GtkTreeModel *model,
  GtkTreePath *path,
  GtkTreeIter *iter,
  gpointer user_data)
{
	NwamuiObject *object;

    gtk_tree_model_get( GTK_TREE_MODEL(model), iter, 0, &object, -1);
    if (object) {
        nwamui_object_reload(object);
        g_object_unref(object);
    }
    return FALSE;
}

/**
 * capplet_tree_model_foreach_nwamui_object_commit:
 *
 * Ensure all objects in tree model are updated before call this functions.
 */
gboolean
capplet_tree_model_foreach_nwamui_object_commit(GtkTreeModel *model,
  GtkTreePath *path,
  GtkTreeIter *iter,
  gpointer user_data)
{
    ForeachNwamuiObjectCommitData *data      = user_data;
	NwamuiObject                  *object    = NULL;

    g_assert(data);

    gtk_tree_model_get( GTK_TREE_MODEL(model), iter, 0, &object, -1);
    if (object) {
        if (nwamui_object_has_modifications(object)) {
            if (!nwamui_object_validate(NWAMUI_OBJECT(object), &data->prop_name) ||
              !nwamui_object_commit(object)) {
                /* Keep ref. */
                data->failone = object;
                data->iter = *iter;
                return TRUE;
            }
        }
        g_object_unref(object);
    }
    return FALSE;
}

/**
 * capplet_tree_store_merge_children:
 * @func is used to iterate the moved item, its return code is ignored.
 * Move source children to the target.
 */
void
capplet_tree_store_merge_children(GtkTreeStore *model,
    GtkTreeIter *target,
    GtkTreeIter *source,
    GtkTreeModelForeachFunc func,
    gpointer user_data)
{
	GtkTreeIter t_iter;
	GtkTreeIter s_iter;
	GtkTreeIter iter;

	if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(model), &t_iter, target)) {
		/* Think about the target position? */
	}

	/* If source doesn't have children, then we move it */
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(model), &s_iter, source)) {
		/* do { */
			/* Take care if there are customer monitoring
			 * row-add/delete signals */
			gtk_tree_store_insert_before(GTK_TREE_STORE(model), &iter, target, NULL);
			capplet_tree_store_move_object(GTK_TREE_MODEL(model),
			    &iter, &s_iter);

			if (func)
				func(GTK_TREE_MODEL(model), NULL, &iter, user_data);

		/* } while (gtk_tree_store_remove(GTK_TREE_STORE(model), &s_iter)); */
	} else {
		gtk_tree_store_insert_before(GTK_TREE_STORE(model), &iter, target, NULL);
		capplet_tree_store_move_object(GTK_TREE_MODEL(model), &iter, source);

		if (func)
			func(GTK_TREE_MODEL(model), NULL, &iter, user_data);

	}
	/* Remove the parent */
	gtk_tree_store_remove(GTK_TREE_STORE(model), source);
}

/**
 * capplet_tree_store_exclude_children:
 * @func is used to iterate the moved item, its return code is ignored.
 * Make the children become their parent's siblings.
 */
void
capplet_tree_store_exclude_children(GtkTreeStore *model,
    GtkTreeIter *source,
    GtkTreeModelForeachFunc func,
    gpointer user_data)
{
	GtkTreeIter target;
	GtkTreeIter t_iter;
	GtkTreeIter s_iter;
	GtkTreeIter iter;

	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(model), &s_iter, source)) {
		do {
			/* Take care if there are customer monitoring
			 * row-add/delete signals */
			gtk_tree_store_insert_before(GTK_TREE_STORE(model), &iter, NULL, source);
			capplet_tree_store_move_object(GTK_TREE_MODEL(model),
			    &iter, &s_iter);

			if (func)
				func(GTK_TREE_MODEL(model), NULL, &iter, user_data);

		} while (gtk_tree_store_remove(GTK_TREE_STORE(model), &s_iter));

		/* Remove the parent */
		gtk_tree_store_remove(GTK_TREE_STORE(model), source);
	} else {
		if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(model), &target, source)) {

			gtk_tree_store_insert_before(GTK_TREE_STORE(model), &iter, NULL, &target);
			capplet_tree_store_move_object(GTK_TREE_MODEL(model), &iter, source);

			if (func)
				func(GTK_TREE_MODEL(model), NULL, &iter, user_data);

			if (!gtk_tree_store_remove(GTK_TREE_STORE(model), source)) {
				/* Doesn't have sibling, delete parent */
				gtk_tree_store_remove(GTK_TREE_STORE(model), &target);
			}

		} else {
			/* Nothing to do is no parent? */
			return;
		}
	}
}

gboolean
capplet_tree_view_expand_row(GtkTreeView *treeview,
    GtkTreeIter *iter,
    gboolean open_all)
{
	GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(treeview), iter);
	gboolean ret = gtk_tree_view_expand_row(treeview, path, open_all);
	gtk_tree_path_free(path);
	return ret;
}

gboolean
capplet_tree_view_collapse_row(GtkTreeView *treeview,
    GtkTreeIter *iter)
{
	GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(treeview), iter);
	gboolean ret = gtk_tree_view_collapse_row(treeview, path);
	gtk_tree_path_free(path);
	return ret;
}

gboolean
capplet_tree_view_commit_object(NwamTreeView *self, NwamuiObject *object, gpointer user_data)
{
	return nwamui_object_commit(object);
}

static gboolean
capplet_model_foreach_add_to_list(GtkTreeModel *model,
    GtkTreePath *path,
    GtkTreeIter *iter,
    gpointer user_data)
{
	CappletForeachData *data = (CappletForeachData *)user_data;
	GList *list = (GList*)data->ret_data;
	GObject *object;

        gtk_tree_model_get( GTK_TREE_MODEL(model), iter, 0, &object, -1);
	list = g_list_append(list, object);
	data->ret_data = (gpointer)list;

	return FALSE;
}

/* Convert model to a list, each element is ref'ed. */
GList*
capplet_model_to_list(GtkTreeModel *model)
{
	CappletForeachData data;

	data.ret_data = NULL;

	gtk_tree_model_foreach(model,
	    capplet_model_foreach_add_to_list,
	    (gpointer)&data);

	return data.ret_data;
}

static gboolean
capplet_model_find_max_name_suffix(GtkTreeModel *model,
    GtkTreePath *path,
    GtkTreeIter *iter,
    gpointer user_data)
{
	CappletForeachData *data = (CappletForeachData *)user_data;
	const gchar *prefix = data->user_data;
	gint *max = data->ret_data;
	NwamuiObject *object;

    gtk_tree_model_get( GTK_TREE_MODEL(model), iter, 0, &object, -1);
	
	if (object) {
		const gchar *name = nwamui_object_get_name(object);
		gchar       *endptr;
		gint         num;
		if (g_str_has_prefix(name, prefix)) {
			gint prefix_len = strlen(prefix);
			if (*(name + prefix_len) != '\0') {
				num = strtoll(name + prefix_len + 1, &endptr, 10);
				if (num > 0 && *max < num)
					*max = num;
            } else if (*max < 0) {
                *max = 0;
            }
		}
		g_object_unref(object);
	}
	return FALSE;
}

gchar*
capplet_get_increasable_name(GtkTreeModel *model, const gchar *prefix, GObject *object)
{
	gchar *name;
	gint inc = -1;

	g_return_val_if_fail(object && G_IS_OBJECT(object), NULL);

	/* Initial flag */
	if (inc < 0) {
		inc = capplet_get_max_name_num(model, prefix, inc);
	}

	if (++inc > 0)
		name = g_strdup_printf("%s %d", prefix, inc);
	else
		name = g_strdup(prefix);

	return name;
}

gint
capplet_get_max_name_num(GtkTreeModel *model, const gchar *prefix, gint base)
{
	CappletForeachData data;
	gint max = base;

	data.user_data = (gpointer)prefix;
	data.ret_data = (gpointer)&max;

	gtk_tree_model_foreach(model,
	    capplet_model_find_max_name_suffix,
	    (gpointer)&data);

	return max;
}

/**
 * capplet_get_original_name:
 * @prefix: a string prefix for duplicating names, e.g. Copy of ...
 * @name: maybe has contained a prefix
 * Avoid Copy of Copy of ... 
 *
 * Return: the composed name including the @prefix but the last number
 * if it is there.
 */
gchar*
capplet_get_original_name(const gchar *prefix, const gchar *name)
{
    gchar *prefix_str = g_strconcat(prefix, " ", NULL);
    gchar *oname = NULL;

	if (g_str_has_prefix(name, prefix_str)) {
        gchar *p_num;
		gint num;
		gchar *endptr;

		oname = g_strdup(name);
        /* Assume a blank is ahead of a number, so delete the number. */
		p_num = g_strrstr(oname, " ");
		if ((size_t)(p_num - oname) > strlen(prefix_str)) {
			num = strtoll(p_num + 1, &endptr, 10);
			if (num > 0) {
				*p_num = '\0';
			}
		}
	} else {
        oname = g_strdup_printf(_("%s%s"), prefix_str, name);
    }
    g_free(prefix_str);
    return g_strstrip(oname);
}

extern void
capplet_util_object_notify_cb( GObject *gobject, GParamSpec *arg1, gpointer data)
{
    GtkTreeModel   *model = GTK_TREE_MODEL(data);
    GtkTreeIter     iter;
    gboolean        valid_iter = FALSE;

    nwamui_debug("%s's notify '%s' changed", g_type_name(G_TYPE_FROM_INSTANCE(gobject)), arg1->name);

    if (capplet_model_find_object(model, gobject, &iter)) {
        GtkTreePath *path;

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(model),
          &iter);
        gtk_tree_model_row_changed(GTK_TREE_MODEL(model),
          path,
          &iter);
        gtk_tree_path_free(path);
/*     } else { */
/*         g_signal_handlers_disconnect_by_func(gobject, */
/*           G_CALLBACK(capplet_util_object_notify_cb), data); */
    }
}

extern gint
nwam_ncu_compare_cb(GtkTreeModel *model,
  GtkTreeIter *a,
  GtkTreeIter *b,
  gpointer user_data)
{
    NwamuiObject *oa, *ob;
    int retval = 0;

    gtk_tree_model_get(model, a, 0, &oa, -1);
    gtk_tree_model_get(model, b, 0, &ob, -1);

    /* net_conf dialog uses nwamui_object as a fake node, so we have to
     * filter out non-NCU objects.
     */
    if (NWAMUI_IS_NCU(oa) && NWAMUI_IS_NCU(ob)) {
        retval = nwamui_object_sort(oa, ob, NWAMUI_OBJECT_SORT_BY_GROUP);
    }
    g_object_unref(oa);
    g_object_unref(ob);

    return retval;
}

