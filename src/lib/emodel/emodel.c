#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eo.h>
#include <Emodel.h>
#include <Eina.h>

#define MY_CLASS_NAME "Emodel"

struct _Emodel_Data
{
};
typedef struct _Emodel_Data Emodel_Data;

static void
_emodel_property_set(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED, const char *property EINA_UNUSED, Eina_Value *value EINA_UNUSED){}

static void
_emodel_property_get(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED, const char *property EINA_UNUSED){}

static void
_emodel_children_slice_get(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED, Emodel_Cb children_slice_get_cb EINA_UNUSED, int start EINA_UNUSED, int count EINA_UNUSED, void *data EINA_UNUSED){}

static void
_emodel_child_del(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED, Emodel_Cb child_del_cb EINA_UNUSED, Eo *child EINA_UNUSED){}

static void
_emodel_properties_get(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED){}

static void
_emodel_load(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED){}

static void
_emodel_unload(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED){}

static void
_emodel_children_get(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED, Emodel_Cb children_get_cb EINA_UNUSED, void *data EINA_UNUSED){}

static void
_emodel_children_count_get(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED){}

static void
_emodel_child_select_set(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED, Eo *child EINA_UNUSED){}

static void
_emodel_child_select_get(Eo *obj EINA_UNUSED, Emodel_Data *pd EINA_UNUSED){}

#include "emodel.eo.c"