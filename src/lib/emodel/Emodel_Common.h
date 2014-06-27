#ifndef _EMODEL_COMMON_H
#define _EMODEL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Emodel_Cb)(const void *data, Eo *child, void *event_info, int error);

struct _Emodel_Property_EVT
{
   Eina_Value *value; /**< the property value */
   const char *prop; /**< the property name */ 
};

typedef struct _Emodel_Property_EVT Emodel_Property_EVT;

struct _Emodel_Children_EVT
{
   Eo *child; /**< child, for child_add */
   int idx; /**< child index */
};

typedef struct _Emodel_Children_EVT Emodel_Children_EVT;


#ifdef __cplusplus
}
#endif
#endif
