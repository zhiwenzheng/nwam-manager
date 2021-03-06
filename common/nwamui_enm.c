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
 * File:   nwamui_enm.c
 *
 */

#include <glib-object.h>
#include <glib/gi18n.h>
#include <strings.h>
#include <stdlib.h>

#include <libnwam.h>
#include "libnwamui.h"

struct _NwamuiEnmPrivate {
    gchar*               name;

    nwam_enm_handle_t	nwam_enm;
    gboolean        	nwam_enm_modified;
};

#define NWAMUI_ENM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), NWAMUI_TYPE_ENM, NwamuiEnmPrivate))

enum {
    PROP_START_COMMAND = 1,
    PROP_STOP_COMMAND,
    PROP_SMF_FMRI,
};


static void nwamui_enm_set_property ( GObject         *object,
                                      guint            prop_id,
                                      const GValue    *value,
                                      GParamSpec      *pspec);

static void nwamui_enm_get_property ( GObject         *object,
                                      guint            prop_id,
                                      GValue          *value,
                                      GParamSpec      *pspec);

static void nwamui_enm_finalize (     NwamuiEnm *self);

static gboolean get_nwam_enm_boolean_prop( nwam_enm_handle_t enm, const char* prop_name );

static gchar* get_nwam_enm_string_prop( nwam_enm_handle_t enm, const char* prop_name );

static gchar*       get_nwam_enm_string_prop( nwam_enm_handle_t enm, const char* prop_name );
static gboolean     set_nwam_enm_string_prop( nwam_enm_handle_t enm, const char* prop_name, const char* str );

static gchar**      get_nwam_enm_string_array_prop( nwam_enm_handle_t enm, const char* prop_name );
static gboolean     set_nwam_enm_string_array_prop( nwam_enm_handle_t enm, const char* prop_name, char** strs, guint len);

static guint64      get_nwam_enm_uint64_prop( nwam_enm_handle_t enm, const char* prop_name );
static gboolean     set_nwam_enm_uint64_prop( nwam_enm_handle_t enm, const char* prop_name, guint64 value );

static gint         nwamui_object_real_open(NwamuiObject *object, const gchar *name, gint flag);
static nwam_state_t nwamui_object_real_get_nwam_state(NwamuiObject *object, nwam_aux_state_t* aux_state_p, const gchar**aux_state_string_p);
static gboolean     nwamui_object_real_destroy( NwamuiObject* object );
static gboolean     nwamui_object_real_can_rename (NwamuiObject *object);
static gboolean     nwamui_object_real_set_name ( NwamuiObject *object, const gchar* name );
static const gchar* nwamui_object_real_get_name ( NwamuiObject *object );
static void         nwamui_object_real_set_active ( NwamuiObject *object, gboolean active );
static gboolean     nwamui_object_real_get_active ( NwamuiObject *object );
static void         nwamui_object_real_set_enabled ( NwamuiObject *object, gboolean enabled );
static gboolean     nwamui_object_real_get_enabled ( NwamuiObject *object );
static void         nwamui_object_real_set_activation_mode ( NwamuiObject *object, gint activation_mode );
static gint         nwamui_object_real_get_activation_mode ( NwamuiObject *object );
static GList*       nwamui_object_real_get_conditions( NwamuiObject* object );
static void         nwamui_object_real_set_conditions( NwamuiObject* object, const GList* conditions );
static gboolean     nwamui_object_real_validate(NwamuiObject *object, gchar **prop_name_ret);
static gboolean     nwamui_object_real_commit( NwamuiObject* object );
static void         nwamui_object_real_reload(NwamuiObject* object);
static NwamuiObject* nwamui_object_real_clone(NwamuiObject *object, const gchar *name, NwamuiObject *parent);
static gboolean     nwamui_object_real_has_modifications(NwamuiObject* object);

/* Callbacks */

G_DEFINE_TYPE (NwamuiEnm, nwamui_enm, NWAMUI_TYPE_OBJECT)

static void
nwamui_enm_class_init (NwamuiEnmClass *klass)
{
    /* Pointer to GObject Part of Class */
    GObjectClass *gobject_class = (GObjectClass*) klass;
    NwamuiObjectClass *nwamuiobject_class = NWAMUI_OBJECT_CLASS(klass);
        
    /* Override Some Function Pointers */
    gobject_class->set_property = nwamui_enm_set_property;
    gobject_class->get_property = nwamui_enm_get_property;
    gobject_class->finalize = (void (*)(GObject*)) nwamui_enm_finalize;

    nwamuiobject_class->open = nwamui_object_real_open;
    nwamuiobject_class->get_name = nwamui_object_real_get_name;
    nwamuiobject_class->can_rename = nwamui_object_real_can_rename;
    nwamuiobject_class->set_name = nwamui_object_real_set_name;
    nwamuiobject_class->get_conditions = nwamui_object_real_get_conditions;
    nwamuiobject_class->set_conditions = nwamui_object_real_set_conditions;
    nwamuiobject_class->get_activation_mode = nwamui_object_real_get_activation_mode;
    nwamuiobject_class->set_activation_mode = nwamui_object_real_set_activation_mode;
    nwamuiobject_class->get_active = nwamui_object_real_get_active;
    nwamuiobject_class->set_active = nwamui_object_real_set_active;
    nwamuiobject_class->get_enabled = nwamui_object_real_get_enabled;
    nwamuiobject_class->set_enabled = nwamui_object_real_set_enabled;
    nwamuiobject_class->get_nwam_state = nwamui_object_real_get_nwam_state;
    nwamuiobject_class->validate = nwamui_object_real_validate;
    nwamuiobject_class->commit = nwamui_object_real_commit;
    nwamuiobject_class->reload = nwamui_object_real_reload;
    nwamuiobject_class->destroy = nwamui_object_real_destroy;
    nwamuiobject_class->clone = nwamui_object_real_clone;
    nwamuiobject_class->has_modifications = nwamui_object_real_has_modifications;

	g_type_class_add_private(klass, sizeof(NwamuiEnmPrivate));

    /* Create some properties */
    g_object_class_install_property (gobject_class,
      PROP_START_COMMAND,
      g_param_spec_string ("start_command",
        _("start_command"),
        _("start_command"),
        "",
        G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
      PROP_STOP_COMMAND,
      g_param_spec_string ("stop_command",
        _("stop_command"),
        _("stop_command"),
        "",
        G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
      PROP_SMF_FMRI,
      g_param_spec_string ("smf_fmri",
        _("smf_fmri"),
        _("smf_fmri"),
        "",
        G_PARAM_READWRITE));

}


typedef struct _ncu_info    ncu_info_t;

static void
nwamui_enm_init ( NwamuiEnm *self)
{  
    NwamuiEnmPrivate *prv      = NWAMUI_ENM_GET_PRIVATE(self);

    self->prv = prv;
}

static void
nwamui_enm_set_property (   GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec)
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    gchar*      tmpstr = NULL;
    gint        tmpint = 0;
    nwam_error_t nerr;
    int num;
    nwam_value_t nwamdata;

    switch (prop_id) {
        case PROP_START_COMMAND: {
                const gchar *start_command = g_value_get_string( value );
                nwamui_enm_set_start_command ( self, start_command );
            }
            break;

        case PROP_STOP_COMMAND: {
                const gchar *stop_command = g_value_get_string( value );
                nwamui_enm_set_stop_command ( self, stop_command );
            }
            break;

        case PROP_SMF_FMRI: {
                const gchar *smf_fmri = g_value_get_string( value );
                nwamui_enm_set_smf_fmri ( self, smf_fmri );
            }
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
    /* Note, contruct properties will set this flag initially, so make sure
     * after xxx_new function unset this flag. The same to other objects.
     */
    self->prv->nwam_enm_modified = TRUE;
}

static void
nwamui_enm_get_property (   GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec)
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    nwam_error_t nerr;
    int num;
    nwam_value_t nwamdata;

    switch (prop_id) {
        case PROP_START_COMMAND: {
                gchar* start_command = NULL;
                if (self->prv->nwam_enm != NULL) {
                    start_command = get_nwam_enm_string_prop( self->prv->nwam_enm, NWAM_ENM_PROP_START );
                }
                else {
                    g_warning("Unexpected null enm handle");
                }
                g_value_set_string( value, start_command );
        }
        break;

        case PROP_STOP_COMMAND: {
                gchar* stop_command = NULL;
                if (self->prv->nwam_enm != NULL) {
                    stop_command = get_nwam_enm_string_prop( self->prv->nwam_enm, NWAM_ENM_PROP_STOP );
                }
                else {
                    g_warning("Unexpected null enm handle");
                }
                g_value_set_string( value, stop_command );
        }
        break;

        case PROP_SMF_FMRI: {
                gchar* smf_fmri = NULL;
                if (self->prv->nwam_enm != NULL) {
                    smf_fmri = get_nwam_enm_string_prop( self->prv->nwam_enm, NWAM_ENM_PROP_FMRI );
                }
                else {
                    g_warning("Unexpected null enm handle");
                }
                g_value_set_string( value, smf_fmri );
        }
        break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gboolean
get_nwam_enm_boolean_prop( nwam_enm_handle_t enm, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    boolean_t           value = FALSE;

    g_return_val_if_fail( prop_name != NULL, value );

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_BOOLEAN ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return value;
    }

    if ( (nerr = nwam_enm_get_prop_value (enm, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    if ( (nerr = nwam_value_get_boolean(nwam_data, &value )) != NWAM_SUCCESS ) {
        g_debug("Unable to get boolean value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    nwam_value_free(nwam_data);

    return( (gboolean)value );
}

static gboolean
set_nwam_enm_boolean_prop( nwam_enm_handle_t enm, const char* prop_name, gboolean bool_value )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( enm == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_BOOLEAN ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_value_create_boolean ((boolean_t)bool_value, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a boolean value" );
        return retval;
    }

    if ( (nerr = nwam_enm_set_prop_value (enm, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}


static gchar*
get_nwam_enm_string_prop( nwam_enm_handle_t enm, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gchar*              retval = NULL;
    char*               value = NULL;

    g_return_val_if_fail( prop_name != NULL, retval );

    if ( enm == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_enm_get_prop_value (enm, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( (nerr = nwam_value_get_string(nwam_data, &value )) != NWAM_SUCCESS ) {
        g_debug("Unable to get string value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( value != NULL ) {
        retval  = g_strdup ( value );
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static gboolean
set_nwam_enm_string_prop( nwam_enm_handle_t enm, const char* prop_name, const gchar* str )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( enm == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_value_create_string( (char*)str, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a string value for string %s", str?str:"NULL");
        return retval;
    }

    if ( (nerr = nwam_enm_set_prop_value (enm, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static gchar**
get_nwam_enm_string_array_prop( nwam_enm_handle_t enm, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gchar**             retval = NULL;
    char**              value = NULL;
    uint_t              num = 0;

    g_return_val_if_fail( prop_name != NULL, retval );

    if ( enm == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_enm_get_prop_value (enm, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( (nerr = nwam_value_get_string_array(nwam_data, &value, &num )) != NWAM_SUCCESS ) {
        g_debug("Unable to get string value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return retval;
    }

    if ( value != NULL && num > 0 ) {
        int i;

        /* Create a NULL terminated list of stirngs, allocate 1 extra place
         * for NULL termination. */
        retval = (gchar**)g_malloc0( sizeof(gchar*) * (num+1) );

        for (i = 0; i < num; i++ ) {
            retval[i]  = g_strdup ( value[i] );
        }
        retval[num]=NULL;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static gboolean
set_nwam_enm_string_array_prop( nwam_enm_handle_t enm, const char* prop_name, char** strs, guint len )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;

    g_return_val_if_fail( prop_name != NULL, retval );

    if ( enm == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_STRING ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( strs == NULL ) {
        nwam_error_t nerr;

        /* Delete property, don't set to empty list */
        if ( (nerr = nwam_enm_delete_prop( enm, prop_name ))  != NWAM_SUCCESS ) {
            retval = FALSE;
        }
        else {
            retval = TRUE;
        }
        return retval;
    }


    if ( len == 0 ) { /* Assume a strv, i.e. NULL terminated list, otherwise strs would be NULL */
        int i;

        for( i = 0; strs != NULL && strs[i] != NULL; i++ ) {
            /* Do Nothing, just count. */
        }
        len = i;
    }

    if ( (nerr = nwam_value_create_string_array (strs, len, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a value for string array 0x%08X", strs);
        return retval;
    }

    if ( (nerr = nwam_enm_set_prop_value (enm, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

static guint64
get_nwam_enm_uint64_prop( nwam_enm_handle_t enm, const char* prop_name )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    uint64_t            value = 0;

    g_return_val_if_fail( prop_name != NULL, value );

    if ( enm == NULL ) {
        return( value );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_UINT64 ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return value;
    }

    if ( (nerr = nwam_enm_get_prop_value (enm, prop_name, &nwam_data)) != NWAM_SUCCESS ) {
        g_debug("No value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    if ( (nerr = nwam_value_get_uint64(nwam_data, &value )) != NWAM_SUCCESS ) {
        g_debug("Unable to get uint64 value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
        return value;
    }

    nwam_value_free(nwam_data);

    return( (guint64)value );
}

static gboolean
set_nwam_enm_uint64_prop( nwam_enm_handle_t enm, const char* prop_name, guint64 value )
{
    nwam_error_t        nerr;
    nwam_value_type_t   nwam_type;
    nwam_value_t        nwam_data;
    gboolean            retval = FALSE;
    g_return_val_if_fail( prop_name != NULL, retval );

    if ( enm == NULL ) {
        return( retval );
    }

    if ( (nerr = nwam_enm_get_prop_type( prop_name, &nwam_type ) ) != NWAM_SUCCESS 
         || nwam_type != NWAM_VALUE_TYPE_UINT64 ) {
        g_warning("Unexpected type for enm property %s - got %d\n", prop_name, nwam_type );
        return retval;
    }

    if ( (nerr = nwam_value_create_uint64 (value, &nwam_data )) != NWAM_SUCCESS ) {
        g_debug("Error creating a uint64 value" );
        return retval;
    }

    if ( (nerr = nwam_enm_set_prop_value (enm, prop_name, nwam_data)) != NWAM_SUCCESS ) {
        g_debug("Unable to set value for enm property %s, error = %s", prop_name, nwam_strerror( nerr ) );
    }
    else {
        retval = TRUE;
    }

    nwam_value_free(nwam_data);
    
    return( retval );
}

/**
 * nwamui_enm_new:
 * @returns: a new #NwamuiEnm.
 *
 **/
extern  NwamuiObject*          
nwamui_enm_new (const gchar *name)
{
    NwamuiObject *self = NWAMUI_OBJECT(g_object_new (NWAMUI_TYPE_ENM, NULL));

    nwamui_object_set_name(NWAMUI_OBJECT(self), name);

    if (nwamui_object_real_open(self, name, NWAMUI_OBJECT_OPEN) != 0) {
        nwamui_object_real_open(self, name, NWAMUI_OBJECT_CREATE);
    }
    return NWAMUI_OBJECT( self );
}

/**
 * nwamui_enm_new_with_handle:
 * @returns: a new #NwamuiEnm.
 *
 **/
extern  NwamuiObject*          
nwamui_enm_new_with_handle (nwam_enm_handle_t enm)
{
    nwam_error_t  nerr;
    char         *name   = NULL;
    NwamuiObject *object = NULL;
    
    if ((nerr = nwam_enm_get_name(enm, &name)) != NWAM_SUCCESS) {
        g_warning("Failed to get name for enm, error: %s", nwam_strerror(nerr));
        return NULL;
    }

    object = g_object_new(NWAMUI_TYPE_ENM, NULL);
    g_assert(NWAMUI_IS_ENM(object));

    /* Will update handle. */
    nwamui_object_set_name(object, name);

    nwamui_object_real_open(object, name, NWAMUI_OBJECT_OPEN);

    nwamui_object_real_reload(object);

    free (name);

    return object;
}

/**
 * nwamui_enm_can_rename:
 * @self: a #NwamuiEnm.
 * @returns: TRUE if the name.can be changed.
 *
 **/
static gboolean
nwamui_object_real_can_rename (NwamuiObject *object)
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    NwamuiEnmPrivate *prv = NWAMUI_ENM(object)->prv;

    g_return_val_if_fail (NWAMUI_IS_ENM (object), FALSE);
    if ( prv->nwam_enm != NULL ) {
        if (nwam_enm_can_set_name( prv->nwam_enm )) {
            return( TRUE );
        }
    }
    return FALSE;
}

/** 
 * nwamui_enm_set_name:
 * @nwamui_enm: a #NwamuiEnm.
 * @name: Value to set name to.
 * 
 **/ 
static gboolean
nwamui_object_real_set_name(NwamuiObject *object, const gchar*  name)
{
    NwamuiEnmPrivate *prv  = NWAMUI_ENM_GET_PRIVATE(object);
    nwam_error_t      nerr;

    g_return_val_if_fail(NWAMUI_IS_ENM(object), FALSE);
    g_return_val_if_fail(name, FALSE);

    /* Initially set name or rename. */
    if (prv->name) {
        /* we may rename here */
        if (prv->nwam_enm != NULL) {
            nerr = nwam_enm_set_name (prv->nwam_enm, name);
            if (nerr != NWAM_SUCCESS) {
                g_debug ("nwam_enm_set_name %s error: %s", name, nwam_strerror (nerr));
                return FALSE;
            }
            prv->nwam_enm_modified = TRUE;
            g_debug ("nwam_enm_set_name %s", name);
        } else {
            g_warning("Unexpected null enm handle");
        }
        g_free(prv->name);
    }

    prv->name = g_strdup(name);
    return TRUE;
}

static gint
nwamui_object_real_open(NwamuiObject *object, const gchar *name, gint flag)
{
    NwamuiEnmPrivate *prv = NWAMUI_ENM_GET_PRIVATE(object);
    nwam_error_t      nerr;

    g_assert(name);

    if (flag == NWAMUI_OBJECT_CREATE) {

        nerr = nwam_enm_create(name, NULL, &prv->nwam_enm);
        if (nerr == NWAM_SUCCESS) {
            prv->nwam_enm_modified = TRUE;
        } else {
            g_warning("nwamui_enm_create error creating nwam_enm_handle %s", name);
            prv->nwam_enm = NULL;
        }
    } else if (flag == NWAMUI_OBJECT_OPEN) {
        nwam_enm_handle_t  handle;

        nerr = nwam_enm_read(name, 0, &handle);
        if (nerr == NWAM_SUCCESS) {
            if (prv->nwam_enm) {
                nwam_enm_free(prv->nwam_enm);
            }
            prv->nwam_enm = handle;
        } else if (nerr == NWAM_ENTITY_NOT_FOUND) {
            /* Most likely only exists in memory right now, so we should use
             * handle passed in as parameter. In clone mode, the new handle
             * gets from nwam_enm_copy can't be read again.
             */
            g_debug("Failed to read enm information for %s error: %s", name, nwam_strerror(nerr));
        } else {
            g_warning("Failed to read enm information for %s error: %s", name, nwam_strerror(nerr));
            prv->nwam_enm = NULL;
        }
    } else {
        g_assert_not_reached();
    }
    return nerr;
}

/**
 * nwamui_enm_get_name:
 * @nwamui_enm: a #NwamuiEnm.
 * @returns: the name.
 *
 **/
static const gchar*
nwamui_object_real_get_name (NwamuiObject *object)
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    nwam_error_t    nerr;

    g_return_val_if_fail (NWAMUI_IS_ENM (self), NULL);

    if (self->prv->name == NULL) {
        if (self->prv->nwam_enm != NULL) {
            char *name;
            if ( (nerr = nwam_enm_get_name (self->prv->nwam_enm, &name)) != NWAM_SUCCESS ) {
                g_debug ("Failed to get name for enm, error: %s", nwam_strerror (nerr));
            }

            self->prv->name = g_strdup(name);
            free (name);
        }
        else {
            g_warning("Unexpected null enm handle");
        }
    }
    return( self->prv->name );
}

/** 
 * nwamui_enm_set_active:
 * @nwamui_enm: a #NwamuiEnm.
 * @active: Value to set active to.
 * 
 **/ 
static void
nwamui_object_real_set_active(NwamuiObject *object, gboolean active)
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    nwam_error_t    nerr;

    if (self->prv->nwam_enm != NULL) {
        if (active) {
            if ((nerr = nwam_enm_enable (self->prv->nwam_enm)) != NWAM_SUCCESS) {
                g_debug ("nwam_enm_start error: %s", nwam_strerror (nerr));
            }
        } else {
            if ((nerr = nwam_enm_disable (self->prv->nwam_enm)) != NWAM_SUCCESS) {
                g_debug ("nwam_enm_stop error: %s", nwam_strerror (nerr));
            }
        }
    } else {
        g_warning("Unexpected null enm handle");
    }
}

/**
 * nwamui_enm_get_active:
 * @nwamui_enm: a #NwamuiEnm.
 * @returns: the active.
 *
 **/
static gboolean
nwamui_object_real_get_active (NwamuiObject *object)
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    gboolean active = FALSE;
    if ( self->prv->nwam_enm ) {
        nwam_state_t        state = NWAM_STATE_OFFLINE;
        nwam_aux_state_t    aux_state = NWAM_AUX_STATE_UNINITIALIZED;

        /* Use cached state in nwamui_object... */
        state = nwamui_object_get_nwam_state( NWAMUI_OBJECT(self), &aux_state, NULL);

        if ( state == NWAM_STATE_ONLINE && aux_state == NWAM_AUX_STATE_ACTIVE ) {
            active = TRUE;
        }
    }

    return( active );
}

/** 
 * nwamui_enm_set_enabled:
 * @nwamui_enm: a #NwamuiEnm.
 * @enabled: Value to set enabled to.
 * 
 **/ 
static void
nwamui_object_real_set_enabled (   NwamuiObject *object, gboolean        enabled )
{
    NwamuiEnmPrivate        *prv  = NWAMUI_ENM_GET_PRIVATE(object);
    nwam_error_t    nerr;

    if (prv->nwam_enm != NULL) {
        if ( !set_nwam_enm_boolean_prop( prv->nwam_enm, NWAM_ENM_PROP_ENABLED, enabled ) ) {
            g_debug("Error setting ENM boolean prop ENABLED");
        }
        prv->nwam_enm_modified = TRUE;
        g_object_notify(G_OBJECT(object), "enabled" );
    } else {
        g_warning("Unexpected null enm handle");
    }
}

/**
 * nwamui_enm_get_enabled:
 * @nwamui_enm: a #NwamuiEnm.
 * @returns: the enabled.
 *
 **/
static gboolean
nwamui_object_real_get_enabled (NwamuiObject *object)
{
    NwamuiEnmPrivate        *prv  = NWAMUI_ENM_GET_PRIVATE(object);
    gboolean  enabled = FALSE; 

    g_return_val_if_fail (NWAMUI_IS_ENM (object), enabled);

    if (prv->nwam_enm != NULL) {
        enabled = get_nwam_enm_boolean_prop( prv->nwam_enm, NWAM_ENM_PROP_ENABLED ); 
    } else {
        g_warning("Unexpected null enm handle");
    }

    return( enabled );
}

/** 
 * nwamui_enm_set_start_command:
 * @nwamui_enm: a #NwamuiEnm.
 * @start_command: Value to set start_command to.
 * 
 **/ 
extern gboolean
nwamui_enm_set_start_command (   NwamuiEnm *self,
                              const gchar*  start_command )
{
    nwam_error_t    nerr;
    g_return_val_if_fail (NWAMUI_IS_ENM (self), FALSE);

    if (self->prv->nwam_enm != NULL) {
        gboolean delete_prop = TRUE;

        if ( start_command != NULL && strlen( start_command ) > 0 ) {
            if ( !set_nwam_enm_string_prop( self->prv->nwam_enm, NWAM_ENM_PROP_START, start_command ) ) {
                return( FALSE );
            }
        }
        else {
            nwam_error_t nerr;

            /* Delete property, don't set to empty string */
            if ( (nerr = nwam_enm_delete_prop( self->prv->nwam_enm, NWAM_ENM_PROP_START ))  != NWAM_SUCCESS ) {
                return( FALSE );
            }
        }
        self->prv->nwam_enm_modified = TRUE;
        g_object_notify(G_OBJECT (self), "start_command");
    }
    else {
        g_warning("Unexpected null enm handle");
        return( FALSE );
    }

    return( TRUE );
}

/**
 * nwamui_enm_get_start_command:
 * @nwamui_enm: a #NwamuiEnm.
 * @returns: the start_command.
 *
 **/
extern gchar*
nwamui_enm_get_start_command (NwamuiEnm *self)
{
    gchar*  start_command = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENM (self), start_command);

    g_object_get (G_OBJECT (self),
                  "start_command", &start_command,
                  NULL);

    return( start_command );
}

extern gboolean
nwamui_enm_set_stop_command (   NwamuiEnm *self,
                              const gchar*  stop_command )
{
    nwam_error_t    nerr;
    g_return_val_if_fail (NWAMUI_IS_ENM (self), FALSE);

    if (self->prv->nwam_enm != NULL) {
        gboolean delete_prop = TRUE;

        if ( stop_command != NULL && strlen( stop_command ) > 0 ) {
            if ( !set_nwam_enm_string_prop( self->prv->nwam_enm, NWAM_ENM_PROP_STOP, stop_command ) ) {
                return( FALSE );
            }
        } else {
            nwam_error_t nerr;

            /* Delete property, don't set to empty string */
            if ( (nerr = nwam_enm_delete_prop( self->prv->nwam_enm, NWAM_ENM_PROP_STOP ))  != NWAM_SUCCESS ) {
            }
        }
        self->prv->nwam_enm_modified = TRUE;
        g_object_notify(G_OBJECT (self), "stop_command");
    } else {
        g_warning("Unexpected null enm handle");
        return( FALSE );
    }

    return( TRUE );
}

/**
 * nwamui_enm_get_stop_command:
 * @nwamui_enm: a #NwamuiEnm.
 * @returns: the stop_command.
 *
 **/
extern gchar*
nwamui_enm_get_stop_command (NwamuiEnm *self)
{
    gchar*  stop_command = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENM (self), stop_command);

    g_object_get (G_OBJECT (self),
                  "stop_command", &stop_command,
                  NULL);

    return( stop_command );
}

/** 
 * nwamui_enm_set_smf_fmri:
 * @nwamui_enm: a #NwamuiEnm.
 * @smf_fmri: Value to set smf_fmri to.
 * 
 **/ 
extern gboolean
nwamui_enm_set_smf_fmri (   NwamuiEnm *self,
                              const gchar*  smf_fmri )
{
    nwam_error_t    nerr;
    g_return_val_if_fail (NWAMUI_IS_ENM (self), FALSE);

    if (self->prv->nwam_enm != NULL) {
        gboolean delete_prop = TRUE;

        if ( smf_fmri != NULL && strlen( smf_fmri ) > 0 ) {
            if ( !set_nwam_enm_string_prop( self->prv->nwam_enm, NWAM_ENM_PROP_FMRI, smf_fmri ) ) {
                return( FALSE );
            }
        }
        else {
            nwam_error_t nerr;

            /* Delete property, don't set to empty string */
            if ( (nerr = nwam_enm_delete_prop( self->prv->nwam_enm, NWAM_ENM_PROP_FMRI ))  != NWAM_SUCCESS ) {
                return( FALSE );
            }
        }
        self->prv->nwam_enm_modified = TRUE;
        g_object_notify(G_OBJECT (self), "smf_fmri");
    }
    else {
        g_warning("Unexpected null enm handle");
        return( FALSE );
    }

    return( TRUE );
}

/**
 * nwamui_enm_get_smf_fmri:
 * @nwamui_enm: a #NwamuiEnm.
 * @returns: the smf_fmri.
 *
 **/
extern gchar*
nwamui_enm_get_smf_fmri (NwamuiEnm *self)
{
    gchar*  smf_fmri = NULL; 

    g_return_val_if_fail (NWAMUI_IS_ENM (self), smf_fmri);

    g_object_get (G_OBJECT (self),
                  "smf_fmri", &smf_fmri,
                  NULL);

    return( smf_fmri );
}

static void
nwamui_object_real_set_activation_mode ( NwamuiObject *object, gint activation_mode )
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    g_return_if_fail (NWAMUI_IS_ENM (self));

    if (self->prv->nwam_enm != NULL) {

        set_nwam_enm_uint64_prop( self->prv->nwam_enm, NWAM_ENM_PROP_ACTIVATION_MODE, (guint64)nwamui_from_ui_activation_mode(activation_mode) );
        self->prv->nwam_enm_modified = TRUE;
    } else {
        g_warning("Unexpected null enm handle");
    }
}

static gint
nwamui_object_real_get_activation_mode ( NwamuiObject *object )
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    nwamui_cond_activation_mode_t activation_mode = NWAMUI_COND_ACTIVATION_MODE_MANUAL;
    uint64_t                      nwamvalue       = 0;

    g_return_val_if_fail (NWAMUI_IS_ENM (self), activation_mode );

    if (self->prv->nwam_enm != NULL) {
        nwamvalue = get_nwam_enm_uint64_prop( self->prv->nwam_enm, NWAM_ENM_PROP_ACTIVATION_MODE );
    }
    else {
        g_warning("Unexpected null enm handle");
    }
    activation_mode = nwamui_to_ui_activation_mode(nwamvalue);

    return( activation_mode );
}


static GList*
nwamui_object_real_get_conditions( NwamuiObject *object )
{
    NwamuiEnmPrivate *prv = NWAMUI_ENM_GET_PRIVATE(object);
    GList *conditions = NULL;

    g_return_val_if_fail(NWAMUI_IS_ENM(object), conditions );

    if (prv->nwam_enm != NULL) {
        gchar** condition_strs = get_nwam_enm_string_array_prop(prv->nwam_enm, NWAM_ENM_PROP_CONDITIONS);
        conditions = nwamui_util_map_condition_strings_to_object_list( condition_strs );
        g_strfreev( condition_strs );
    } else {
        g_warning("Unexpected null enm handle");
    }

    return conditions;
}


static void
nwamui_object_real_set_conditions( NwamuiObject *object, const GList* conditions )
{
    NwamuiEnmPrivate *prv = NWAMUI_ENM_GET_PRIVATE(object);

    g_return_if_fail(NWAMUI_IS_ENM(object));

    if (prv->nwam_enm != NULL) {
        char  **condition_strs = NULL;
        guint   len = 0;

        condition_strs = nwamui_util_map_object_list_to_condition_strings((GList*)conditions, &len);
        if ( condition_strs == NULL ) {
            /* We need to set activation mode to MANUAL or it will
             * not allow deletion of conditions
             */
            nwamui_object_real_set_enabled(object, FALSE );
            nwamui_object_real_set_activation_mode(object, NWAMUI_COND_ACTIVATION_MODE_MANUAL);
        } else {
            set_nwam_enm_string_array_prop(prv->nwam_enm, NWAM_ENM_PROP_CONDITIONS, condition_strs, len);
            prv->nwam_enm_modified = TRUE;
            g_debug("%s set conditions", prv->name);
            free(condition_strs);
        }
    }
    else {
        g_warning("Unexpected null enm handle");
    }
}

/**
 * nwamui_enm_reload:   re-load stored configuration
 **/
static void
nwamui_object_real_reload(NwamuiObject* object)
{
    NwamuiEnmPrivate  *prv  = NWAMUI_ENM_GET_PRIVATE(object);
    nwam_error_t       nerr;

    g_return_if_fail(NWAMUI_IS_ENM(object));

    nwamui_object_real_open(object, prv->name, NWAMUI_OBJECT_OPEN);

    /* nwamui_object_set_handle will cause re-read from configuration */
    g_object_freeze_notify(G_OBJECT(object));

    /* Tell GUI to refresh */
    g_object_notify(G_OBJECT(object), "activation-mode");

    g_object_thaw_notify(G_OBJECT(object));
}

/**
 * nwamui_enm_destroy:   destroy in-memory configuration, to persistant storage
 * @returns: TRUE if succeeded, FALSE if failed
 **/
static gboolean
nwamui_object_real_destroy( NwamuiObject *object )
{
    NwamuiEnm *self = NWAMUI_ENM(object);
    nwam_error_t    nerr;

    g_return_val_if_fail( NWAMUI_IS_ENM(self), FALSE );

    if ( self->prv->nwam_enm != NULL ) {

        if ( (nerr = nwam_enm_destroy( self->prv->nwam_enm, 0 ) ) != NWAM_SUCCESS ) {
            g_warning("Failed when destroying ENM for %s", self->prv->name);
            return( FALSE );
        }
        self->prv->nwam_enm = NULL;
    }

    return( TRUE );
}

static NwamuiObject*
nwamui_object_real_clone(NwamuiObject *object, const gchar *name, NwamuiObject *parent)
{
    NwamuiEnm         *self    = NWAMUI_ENM(object);
    NwamuiObject      *new_enm = NULL;;
    nwam_enm_handle_t  new_enm_h;
    nwam_error_t       nerr;
    NwamuiEnmPrivate *new_prv;

    g_return_val_if_fail(NWAMUI_IS_ENM(object), NULL);
    g_return_val_if_fail(name != NULL, NULL);

    nerr = nwam_enm_copy(self->prv->nwam_enm, name, &new_enm_h);

    if (nerr != NWAM_SUCCESS) { 
        nwamui_warning("Failed to clone new ENM %s from existing ENM %s: %s",
          name, self->prv->name, nwam_strerror(nerr) );
        return new_enm;
    }

    /* new_enm = nwamui_enm_new_with_handle(new_enm_h); */
    new_enm = NWAMUI_OBJECT(g_object_new(NWAMUI_TYPE_ENM,
        "name", name,
        NULL));
    new_prv = NWAMUI_ENM_GET_PRIVATE(new_enm);
    new_prv->nwam_enm = new_enm_h;
    new_prv->nwam_enm_modified = TRUE;

    return NWAMUI_OBJECT(new_enm);
}

static gboolean
nwamui_object_real_has_modifications(NwamuiObject* object)
{
    NwamuiEnmPrivate  *prv  = NWAMUI_ENM_GET_PRIVATE(object);

    return prv->nwam_enm_modified;
}

/**
 * nwamui_enm_validate:   validate in-memory configuration
 * @prop_name_ret:  If non-NULL, the name of the property that failed will be
 *                  returned, should be freed by caller.
 * @returns: TRUE if valid, FALSE if failed
 **/
static gboolean
nwamui_object_real_validate(NwamuiObject *object, gchar **prop_name_ret)
{
    NwamuiEnmPrivate  *prv  = NWAMUI_ENM_GET_PRIVATE(object);
    nwam_error_t    nerr;
    const char*     prop_name = NULL;

    g_return_val_if_fail(NWAMUI_IS_ENM(object), FALSE );

    if ( prv->nwam_enm_modified && prv->nwam_enm != NULL ) {
        if ( (nerr = nwam_enm_validate( prv->nwam_enm, &prop_name ) ) != NWAM_SUCCESS ) {
            g_debug("Failed when validating ENM for %s : invalid value for %s", 
                    prv->name, prop_name);
            if ( prop_name_ret != NULL ) {
                *prop_name_ret = g_strdup( prop_name );
            }
            return( FALSE );
        }
    }

    return( TRUE );
}

/**
 * nwamui_enm_commit:   commit in-memory configuration, to persistant storage
 * @returns: TRUE if succeeded, FALSE if failed
 **/
static gboolean
nwamui_object_real_commit( NwamuiObject *object )
{
    NwamuiEnmPrivate  *prv  = NWAMUI_ENM_GET_PRIVATE(object);
    NwamuiEnm *self = NWAMUI_ENM(object);
    nwam_error_t    nerr;

    g_return_val_if_fail( NWAMUI_IS_ENM(self), FALSE );

    if (prv->nwam_enm) {
        if (prv->nwam_enm_modified) {
            if ((nerr = nwam_enm_commit(prv->nwam_enm, 0)) != NWAM_SUCCESS) {
                g_warning("Failed when committing ENM for %s, %s", prv->name, nwam_strerror( nerr ));
                return FALSE;
            }
            prv->nwam_enm_modified = FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static void
nwamui_enm_finalize (NwamuiEnm *self)
{
    if (self->prv->nwam_enm != NULL) {
        nwam_enm_free (self->prv->nwam_enm);
    }

    if (self->prv->name != NULL ) {
        g_free( self->prv->name );
    }

    self->prv = NULL;

	G_OBJECT_CLASS(nwamui_enm_parent_class)->finalize(G_OBJECT(self));
}

static nwam_state_t
nwamui_object_real_get_nwam_state(NwamuiObject *object, nwam_aux_state_t* aux_state_p, const gchar**aux_state_string_p)
{
    nwam_state_t    rstate = NWAM_STATE_UNINITIALIZED;

    if ( aux_state_p ) {
        *aux_state_p = NWAM_AUX_STATE_UNINITIALIZED;
    }

    if ( aux_state_string_p ) {
        *aux_state_string_p = (const gchar*)nwam_aux_state_to_string( NWAM_AUX_STATE_UNINITIALIZED );
    }

    if ( NWAMUI_IS_ENM( object ) ) {
        NwamuiEnm   *enm = NWAMUI_ENM(object);
        nwam_state_t        state;
        nwam_aux_state_t    aux_state;


        /* First look at the LINK state, then the IP */
        if ( enm->prv->nwam_enm &&
             nwam_enm_get_state( enm->prv->nwam_enm, &state, &aux_state ) == NWAM_SUCCESS ) {

            rstate = state;
            if ( aux_state_p ) {
                *aux_state_p = aux_state;
            }
            if ( aux_state_string_p ) {
                *aux_state_string_p = (const gchar*)nwam_aux_state_to_string( aux_state );
            }
        }
    }

    return(rstate);
}

/* Callbacks */


