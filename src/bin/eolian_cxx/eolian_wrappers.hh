#ifndef EOLIAN_CXX_EOLIAN_WRAPPERS_HH
#define EOLIAN_CXX_EOLIAN_WRAPPERS_HH

#include <cassert>

#include <Eolian.h>

#include "eo_types.hh"
#include "safe_strings.hh"
#include "type_lookup.hh"

namespace eolian_cxx
{

struct property_t { static constexpr ::Eolian_Function_Type value = ::EOLIAN_PROPERTY; };
property_t const property = {};

struct setter_t { static constexpr ::Eolian_Function_Type value = ::EOLIAN_PROP_SET; };
setter_t const setter = {};

struct getter_t { static constexpr ::Eolian_Function_Type value = ::EOLIAN_PROP_GET; };
getter_t const getter = {};

struct method_t { static constexpr ::Eolian_Function_Type value = ::EOLIAN_METHOD; };
method_t const method = {};

struct ctor_t { static constexpr ::Eolian_Function_Type value = ::EOLIAN_CTOR; };
ctor_t const ctor = {};

inline Eolian_Class
class_from_file(std::string const& file)
{
   return ::eolian_class_find_by_file(file.c_str());
}

inline std::string
class_file(Eolian_Class const& klass)
{
   return safe_str(::eolian_class_file_get(klass));
}

inline std::string
class_base_file(Eolian_Class const& klass)
{
   return path_base(safe_str(::eolian_class_file_get(klass)));
}

inline std::string
class_name(Eolian_Class const& klass)
{
   return safe_str(::eolian_class_name_get(klass));
}

inline std::string
class_full_name(Eolian_Class const& klass)
{
   return safe_str(::eolian_class_full_name_get(klass));
}

inline Eolian_Class
class_from_name(std::string const& classname)
{
   return ::eolian_class_find_by_name(classname.c_str());
}

inline std::string
class_eo_name(Eolian_Class const& klass)
{
   std::string suffix;
   switch (eolian_class_type_get(klass))
     {
        case EOLIAN_CLASS_REGULAR:
        case EOLIAN_CLASS_ABSTRACT:
           suffix = "CLASS";
           break;
        case EOLIAN_CLASS_MIXIN:
           suffix = "MIXIN";
           break;
        case EOLIAN_CLASS_INTERFACE:
           suffix = "INTERFACE";
           break;
        default:
           break;
     }
   std::string s = class_full_name(klass) + "_" + suffix;
   std::transform(s.begin(), s.end(), s.begin(),
                  [](int c)
                  {
                     return c == '.' ? '_' : c ;
                  });
   return safe_upper(s);
}

inline std::string
class_format_cxx(std::string const& fullname)
{
   std::string s = fullname;
   auto found = s.find(".");
   while (found != std::string::npos)
     {
        s.replace(found, 1, "::");
        found = s.find(".");
     }
   return s;
}

inline std::string
class_prefix(Eolian_Class const& klass)
{
   std::string prefix = safe_lower(::eolian_class_eo_prefix_get(klass));
   if (prefix.empty())
     prefix = safe_lower(find_replace(class_full_name(klass), ".", "_"));
   assert(!prefix.empty());
   return prefix;
}

inline efl::eolian::eo_class::eo_class_type
class_type(Eolian_Class const& klass)
{
   assert(klass != NULL);
   efl::eolian::eo_class::eo_class_type type;
   Eolian_Class_Type cls_type = ::eolian_class_type_get(klass);

   if (cls_type == EOLIAN_CLASS_REGULAR)
     type = efl::eolian::eo_class::regular_;
   else if (cls_type == EOLIAN_CLASS_ABSTRACT)
     type = efl::eolian::eo_class::regular_noninst_;
   else if (cls_type == EOLIAN_CLASS_MIXIN)
     type = efl::eolian::eo_class::mixin_;
   else if (cls_type == EOLIAN_CLASS_INTERFACE)
     type = efl::eolian::eo_class::interface_;
   else assert(false);

   return type;
}

inline std::string
class_namespace_full(Eolian_Class const& klass)
{
   std::string s;
   const Eina_List* list =
     ::eolian_class_namespaces_list_get(klass), *itr;
   void* name;
   EINA_LIST_FOREACH(list, itr, name)
     {
        s += static_cast<const char*>(name);
        s += "::";
     }
   if (s.size() >= 2)
     s = s.substr(0, s.size()-2);
   return safe_lower(s);
}

inline efl::eina::range_ptr_list<const Eolian_Class>
class_list_all()
{
   return ::eolian_all_classes_list_get();
}

inline std::string
function_name(Eolian_Function const& func)
{
   return safe_str(::eolian_function_name_get(func));
}

inline std::string
function_impl(Eolian_Function const& func, std::string const& prefix)
{
   const char *s = ::eolian_function_full_c_name_get(func, prefix.c_str());
   std::string ret(s);
   ::eina_stringshare_del(s);
   return ret;
}

inline Eolian_Function_Type
function_type(Eolian_Function const& func)
{
   return ::eolian_function_type_get(func);
}

inline efl::eolian::eolian_type_instance
function_return_type(Eolian_Function const& func, Eolian_Function_Type func_type = method_t::value)
{
   return type_lookup
     (::eolian_function_return_type_get(func, func_type));
}

inline efl::eolian::eolian_type_instance
function_return_type(Eolian_Function const& func, setter_t func_type)
{
   return function_return_type(func, func_type.value);
}

inline efl::eolian::eolian_type_instance
function_return_type(Eolian_Function const& func, getter_t func_type)
{
   return function_return_type(func, func_type.value);
}

inline efl::eolian::eolian_type_instance
function_return_type(Eolian_Function const& func, method_t func_type)
{
   return function_return_type(func, func_type.value);
}

inline efl::eolian::eolian_type_instance
function_return_type(Eolian_Function const& func, ctor_t func_type)
{
   return function_return_type(func, func_type.value);
}

inline bool
property_is_getter(Eolian_Function_Type func_type)
{
   return func_type == property_t::value || func_type == getter_t::value;
}

inline bool
property_is_getter(Eolian_Function const& func)
{
   return property_is_getter(function_type(func));
}

inline bool
property_is_setter(Eolian_Function_Type func_type)
{
   return func_type == property_t::value || func_type == setter_t::value;
}

inline bool
property_is_setter(Eolian_Function const& func)
{
   return property_is_setter(function_type(func));
}

inline std::string
parameter_name(Eolian_Function_Parameter const& parameter)
{
   return safe_strshare(::eolian_parameter_name_get(parameter)) + "_";
}

inline bool
parameter_is_out(Eolian_Function_Parameter const& parameter)
{
   Eolian_Parameter_Dir direction;
   ::eolian_parameter_information_get(parameter, &direction, NULL, NULL, NULL);
   return direction == EOLIAN_OUT_PARAM || direction == EOLIAN_INOUT_PARAM;
}

inline bool
parameter_is_const(Eolian_Function_Parameter const& parameter,
                   Eolian_Function_Type func_type)
{
   return ::eolian_parameter_const_attribute_get
     (parameter, property_is_getter(func_type));
}

inline bool
parameter_is_const(Eolian_Function_Parameter const& parameter,
                   getter_t func_type)
{
   return ::eolian_parameter_const_attribute_get
     (parameter, property_is_getter(func_type.value));
}

inline bool
parameter_is_const(Eolian_Function_Parameter const& parameter,
                   setter_t func_type)
{
   return ::eolian_parameter_const_attribute_get
     (parameter, property_is_getter(func_type.value));
}

inline bool
parameter_is_const(Eolian_Function_Parameter const& parameter,
                   Eolian_Function const& func)
{
   assert(function_type(func) != EOLIAN_PROPERTY);
   return ::eolian_parameter_const_attribute_get
     (parameter, property_is_getter(func));
}

inline efl::eolian::eolian_type_instance
parameter_type(Eolian_Function_Parameter const& parameter,
               Eolian_Function_Type func_type = method_t::value)
{
   efl::eolian::eolian_type_instance type
     (type_lookup(::eolian_parameter_type_get(parameter)));
   assert(!type.empty());
   // XXX implement complex types.
   if (parameter_is_out(parameter))
     type = { type_to_native(type) + "*" };
   if (parameter_is_const(parameter, func_type))
     type.insert(0, "const ");
   return type;
}

inline efl::eolian::eolian_type_instance
parameter_type(Eolian_Function_Parameter const& parameter, getter_t func_type)
{
   return parameter_type(parameter, func_type.value);
}

inline efl::eolian::eolian_type_instance
parameter_type(Eolian_Function_Parameter const& parameter, setter_t func_type)
{
   return parameter_type(parameter, func_type.value);
}

inline efl::eolian::eo_event
event_create(Eolian_Class const& klass, const Eolian_Event event_)
{
   efl::eolian::eo_event event;
   const char *name, *type, *comment;
   if(::eolian_class_event_information_get(event_, &name, &type, &comment))
     {
        std::string name_ = safe_str(name);
        std::transform(name_.begin(), name_.end(), name_.begin(),
                       [](int c) { return c != ',' ? c : '_'; });
        event.name = normalize_spaces(name_);
        event.eo_name = safe_upper(class_name(klass) + "_EVENT_" + event.name);
        event.comment = safe_str(comment);
     }
   return event;
}

inline efl::eolian::events_container_type
event_list(Eolian_Class const& klass)
{
   efl::eolian::events_container_type events;
   const Eina_List* list = eolian_class_events_list_get(klass);
   unsigned int length = eina_list_count(list);
   for (unsigned int i = 0; i < length; ++i)
     {
        Eolian_Event e = static_cast<Eolian_Event>(eina_list_nth(list, i));
        events.push_back(event_create(klass, e));
     }
   return events;
}

}

#endif // EOLIAN_CXX_EOLIAN_WRAPPERS_HH
