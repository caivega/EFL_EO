#include <Eina.h>
#include "eolian_database.h"

EAPI Eina_Stringshare *
eolian_implement_full_name_get(const Eolian_Implement *impl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, NULL);
   return impl->full_name;
}

EAPI Eina_Bool
eolian_implement_information_get(const Eolian_Implement *impl, const Eolian_Class **class_out, const Eolian_Function **func_out, Eolian_Function_Type *type_out)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(impl, EINA_FALSE);
   const Eolian_Class *class;
   if (!database_class_name_validate(impl->full_name, &class) || !class) return EINA_FALSE;
   const char *class_name = class->full_name;
   if (class_out) *class_out = class;

   char *func_name = strdup(impl->full_name + strlen(class_name) + 1);
   char *colon = strchr(func_name, '.');
   Eolian_Function_Type type = EOLIAN_UNRESOLVED;
   if (colon)
     {
        *colon = '\0';
        if (!strcmp(colon+1, "set")) type = EOLIAN_PROP_SET;
        else if (!strcmp(colon+1, "get")) type = EOLIAN_PROP_GET;
     }

   const Eolian_Function *fid = eolian_class_function_get_by_name(class, func_name, type);
   if (func_out) *func_out = fid;
   if (type == EOLIAN_UNRESOLVED) type = eolian_function_type_get(fid);
   if (type_out) *type_out = type;
   free(func_name);
   return EINA_TRUE;
}
