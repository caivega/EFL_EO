//Compile with:
// gcc -o emodel_test_file emodel_test_file.c `pkg-config --cflags --libs emodel`
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eo.h>
#include <Eio.h>
#include <Ecore.h>
#include <Emodel.h>
#include <emodel_eio.eo.h>
#include <stdio.h>

#include <check.h>

#define EMODEL_TEST_FILENAME_PATH "/tmp"

struct reqs_t {
   int filename;
   int size;
   int properties;
   int propset;
   int count;
   int children;
   int child_add;
   int child_del;
};

static struct reqs_t reqs;
static double _initial_time = 0;
Ecore_Timer         *timer1     = NULL;
Ecore_Event_Handler *handler   = NULL;

static Eina_Bool
__attribute__((unused))_try_quit(void *data EINA_UNUSED)
{
   printf("Timer1 expired after %0.3f seconds.\n", ecore_time_get() - _initial_time);
   printf("Try quit: filename=%d, size=%d, properties=%d, propset=%d, count=%d, children=%d, child_add=%d, child_del=%d\n",
          reqs.filename, reqs.size, reqs.properties, reqs.propset, reqs.count, reqs.children, reqs.child_add, reqs.child_del);

   fail_if((reqs.filename == -1) || (reqs.size == -1) || (reqs.properties == -1)
           || (reqs.propset == -1)  || (reqs.count == -1) || (reqs.children == -1)
           || reqs.child_add == -1 || reqs.child_del == -1);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
 exit_func(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *ev)
 {
    Ecore_Event_Signal_Exit *e;
 
    e = (Ecore_Event_Signal_Exit *)ev;
    if (e->interrupt)      fprintf(stdout, "Exit: interrupt\n");
    else if (e->quit)      fprintf(stdout, "Exit: quit\n");
    else if (e->terminate) fprintf(stdout, "Exit: terminate\n");
    ecore_main_loop_quit();
    return ECORE_CALLBACK_CANCEL;
 }

static Eina_Bool
_properties_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Eina_Value *properties = event_info;
   const char *prop;
   unsigned int i;

   reqs.properties = 1;
   for (i = 0; i < eina_value_array_count(properties); i++)
     {
        eina_value_array_get(properties, i, &prop);
        fprintf(stdout, "property %d: %s\n", i, prop);
     }
   return EINA_TRUE;
}

static Eina_Bool
_prop_change_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Emodel_Property_EVT *evt = event_info;
   fprintf(stdout, "property \"%s\" changed to \"%s\"\n", evt->prop, eina_value_to_string(evt->value));

   if (strncmp(evt->prop, "filename", strlen("filename")) == 0)
     {
        fprintf(stdout, "received Filename\n");
        reqs.filename = 1;
     }
   else if (strncmp(evt->prop, "size", strlen("size")) == 0)
     {
        fprintf(stdout, "received Size\n");
        reqs.size = 1;
     }
   else if (strncmp(evt->prop, "properties", strlen("properties")) == 0)
     {
        fprintf(stdout, "received Properties\n");
        reqs.properties = 1;
     }
   else if (strncmp(evt->prop, "propset", strlen("propset")) == 0)
     {
        fprintf(stdout, "received Propset\n");
        reqs.propset = 1;
     }

   return EINA_TRUE;
}

static void
_children_get_cb(void *data EINA_UNUSED, Eo *child, void *event_info)
{
   int *idx = (int*)event_info;
   fprintf(stdout, "Child received: child=%p, idx=%d\n", child, *idx);
   eo_do(child, eo_event_callback_add(EMODEL_EVENT_PROPERTY_CHANGE, _prop_change_cb, NULL));
   eo_do(child, emodel_property_get("filename"));
}

static Eina_Bool
_children_count_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   unsigned int *len = event_info;

   fprintf(stdout, "Children count len=%d\n", *len);
   reqs.children = *len;
   reqs.count = 1;
   return EINA_TRUE;
}

static void
_child_del_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, void *event_info)
{
   Eo *child = (Eo *)event_info;
   if(-1 == reqs.child_del) reqs.child_del = 1;
   fprintf(stdout, "Deleted child=%p\n", child);
}

static Eina_Bool
_null_cb(void *data, Eo *obj, const Eo_Event_Description *desc, void *event_info)
{
   Eo *child = (Eo *)event_info;
   const Eo_Callback_Array_Item *callback_array = event_info;

   if((callback_array->desc == EMODEL_EVENT_CHILD_ADD) || (callback_array->desc == EMODEL_EVENT_CHILD_DEL))
     fprintf(stdout, "Got event from child '%p'\n", child);

   (void) desc;
   (void) obj;
   (void) data;
   (void) event_info;
   return EO_CALLBACK_CONTINUE;
}

static void
_emodel_child_add_cb(void *data, Eo *obj, void *event_info)
{
   const char *name = (const char *)data;
   Eo *child = (Eo*)event_info;
   static int del = 0;

   fprintf(stdout, "Child add: parent=%p, child=%p path=%s\n", obj, child, name);

   if(-1 == reqs.child_add) reqs.child_add = 1;

   eo_do(child, eo_event_callback_add(EMODEL_EVENT_CHILD_ADD, _null_cb, NULL));
   eo_do(child, eo_event_callback_add(EMODEL_EVENT_CHILD_DEL, _null_cb, NULL));

   if(!del)
     {
        /**
         * This test means that you would not see one
         * of previously added directories because we'll
         * delete it as soon as add_cb is notified.
         */
        del = 1;
//        eo_do(obj, emodel_child_del(_child_del_cb, child));
     }
}

static Eina_Bool
_child_add_evt_cb(void *data EINA_UNUSED, Eo *obj, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Emodel_Children_EVT *evt = (Emodel_Children_EVT*)event_info;
   fprintf(stdout, "Child add event: parent=%p, child=%p index=%d dir=%s\n", obj, evt->child, evt->idx, (char*)evt->data);
   return EINA_TRUE;
}
static Eina_Bool
_child_del_evt_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Emodel_Children_EVT *evt = (Emodel_Children_EVT*)event_info;
   //child is NULL (already removed)
   fprintf(stdout, "Child del event: parent=%p, index=%d dir=%s\n", obj, evt->idx, (char*)evt->data);
   return EINA_TRUE;
}


START_TEST(emodel_test_test_file)
{
   Eo *filemodel;
   int i;
   static const char *dirs[] = {"emodel_test_dir_00", "emodel_test_dir_01", "emodel_test_dir_02", "emodel_test_dir_03", NULL};

   memset(&reqs, -1, sizeof(struct reqs_t));

   fail_if(!ecore_init(), "ERROR: Cannot init Ecore!\n");
   fail_if(!eio_init(), "ERROR: Cannot init EIO!\n");

   filemodel = eo_add_custom(EMODEL_EIO_CLASS, NULL, emodel_eio_constructor(EMODEL_TEST_FILENAME_PATH));
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_PROPERTY_CHANGE, _prop_change_cb, NULL));
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_PROPERTIES_CHANGE, _properties_cb, NULL));
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_CHILDREN_COUNT_GET, _children_count_cb, NULL));

   // Listener for child add
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_CHILD_ADD, _child_add_evt_cb, NULL));
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_CHILD_DEL, _child_del_evt_cb, NULL));

   eo_do(filemodel, emodel_property_get("filename"));
   eo_do(filemodel, emodel_property_get("size"));
   eo_do(filemodel, emodel_properties_get());
   eo_do(filemodel, emodel_children_get(_children_get_cb, NULL));
   eo_do(filemodel, emodel_children_count_get());
   eo_do(filemodel, emodel_children_slice_get(_children_get_cb, 0,15, NULL));
   eo_do(filemodel, emodel_children_slice_get(_children_get_cb, 20,5, NULL));

   // here we set the callback for child add
   for(i=0; dirs[i] != NULL; ++i)
     {
         eo_do(filemodel, emodel_eio_dir_add(_emodel_child_add_cb, dirs[i]));
     }

   /**
    * The following test works however
    * it is going to rename (move) the original directory to
    * new one so '/tmp' doesn't work , you'll need to use
    * '/tmp/some_other_dir' instead as source.
    */
//#define _RUN_LOCAL_TEST
#ifdef _RUN_LOCAL_TEST
   Eina_Value *nameset = eina_value_new(EINA_VALUE_TYPE_STRING);
   eina_value_set(nameset, "/tmp/emodel_test");
   eo_do(filemodel, emodel_property_set("filename", nameset));
   eo_do(filemodel, emodel_property_get("filename"));
#endif

   handler = ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_func, NULL);
   _initial_time = ecore_time_get();
   timer1 = ecore_timer_add(13, _try_quit, NULL);
   ecore_main_loop_begin();
   eo_unref(filemodel);
   ecore_shutdown();
   eio_shutdown();
}
END_TEST

void
emodel_test_file(TCase *tc)
{
   tcase_add_test(tc, emodel_test_test_file);
}

