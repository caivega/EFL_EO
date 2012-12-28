/**
 * @page edbus_main EDBus
 *
 * @section edbus_intro_sec Introduction
 *
 * EDBus is a wrapper around the
 * <a href="http://www.freedesktop.org/wiki/Software/dbus">dbus</a>
 * library, which is a message bus system. It also implements a set of
 * specifications using dbus as interprocess communication.
 *
 * @section edbus_modules_sec Modules
 *
 * <ul>
 * <li> @ref EDBus_Core
 * <li> @ref EDBus_Conneciton
 * <li> @ref EDBus_Object_Mapper
 * <li> @ref EDBus_Proxy
 * <li> @ref EDBus_Message
 *      <ul>
 *              <li>@ref EDBus_Message_Iter
 *              <li>@ref EDBus_Message_Helpers
 *              <li>@ref Eina_Value
 *      </ul>
 * <li> @ref EDBus_Signal_Handler
 * <li> @ref EDBus_Pending
 * <li> @ref EDBus_Service
 * <li> @ref EDBus_Basic
 * </ul>
 *
 * @section edbus_examples_sec Examples
 *
 * Here some EDBus examples:
 *
 * @li @ref banshee.c
 * @li @ref client.c
 * @li @ref complex-types.c
 * @li @ref complex-types-client-eina-value.c
 * @li @ref complex-types-server.c
 * @li @ref connman-list-services.c
 * @li @ref ofono-dial.c
 * @li @ref server.c
 * @li @ref simple-signal-emit.c
 *
 * @example banshee.c
 * Access Banshee music player and send commands to it.
 *
 * @example client.c
 * Client to test various call message types against a provided server
 * (@ref server.c)
 *
 * @example complex-types.c
 * Client to test complex types (arrays, structs, dicts) against a
 * provided server (@ref complex-types-server.c)
 *
 * @example complex-types-client-eina-value.c
 * Client to test complex types (arrays, structs, dicts) against a
 * provided server (@ref complex-types-server.c) returning them as
 * @ref Eina_Value.
 *
 * @example complex-types-server.c
 * Server to test complex types (arrays, structs, dicts).
 *
 * @example connman-list-services.c
 * Client to list networks/services from connman.
 *
 * @example ofono-dial.c
 * Client to ask oFono to dial.
 *
 * @example server.c
 * Server to reply to @ref client.c requests.
 *
 * @example simple-signal-emit.c
 * Server that registers a service interface and emits simple signals.
 */
#ifndef EDBUS_H
#define EDBUS_H

#include <Eina.h>
#include <stdarg.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_EDBUS_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_EDBUS_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup EDBus_Core Core
 * @ingroup EDBus
 *
 * @{
 */
#define EDBUS_VERSION_MAJOR 1
#define EDBUS_VERSION_MINOR 6

#define EDBUS_FDO_BUS "org.freedesktop.DBus"
#define EDBUS_FDO_PATH "/org/freedesktop/DBus"
#define EDBUS_FDO_INTERFACE EDBUS_FDO_BUS
#define EDBUS_FDO_INTERFACE_PROPERTIES "org.freedesktop.DBus.Properties"
#define EDBUS_FDO_INTERFACE_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#define EDBUS_FDO_INTERFACE_INTROSPECTABLE "org.freedesktop.DBus.Introspectable"
#define EDBUS_FDO_INTEFACE_PEER "org.freedesktop.DBus.Peer"

typedef struct _EDBus_Version
{
   int major;
   int minor;
   int micro;
   int revision;
} EDBus_Version;

EAPI extern const EDBus_Version * edbus_version;

/**
 * @brief Initialize edbus.
 *
 * @return 1 or greater on success, 0 otherwise
 */
EAPI int edbus_init(void);
/**
 * @brief Shutdown edbus.
 *
 * @return 0 if e_dbus shuts down, greater than 0 otherwise.
 */
EAPI int edbus_shutdown(void);

typedef void                       (*EDBus_Free_Cb)(void *data, const void *deadptr);

/**
 * @typedef EDBus_Connection
 *
 * Represents a connection of one the type of connection with the DBus daemon.
 */
typedef struct _EDBus_Connection     EDBus_Connection;
/**
 * @typedef EDBus_Object
 *
 * Represents an object path already attached with bus name or unique id.
 */
typedef struct _EDBus_Object         EDBus_Object;
/**
 * @typedef EDBus_Proxy
 *
 * Represents an interface of an object path.
 */
typedef struct _EDBus_Proxy          EDBus_Proxy;
/**
 * @typedef EDBus_Message
 *
 * Represents the way data is sent and received in DBus.
 */
typedef struct _EDBus_Message        EDBus_Message;
/**
 * @typedef EDBus_Message_Iter
 *
 * Represents an iterator over a complex message type (array, dict, struct,
 * or variant). Its life is bound to the message that contains
 * it. The same applies to the returned data.
 */
typedef struct _EDBus_Message_Iter EDBus_Message_Iter;
/**
 * @typedef EDBus_Pending
 *
 * Represents a message that has been sent but has not yet reached its
 * destination.
 */
typedef struct _EDBus_Pending        EDBus_Pending;

/**
 * @typedef EDBus_Signal_Handler
 *
 * Represents a listener that will listen for signals emitted by other
 * applications.
 */
typedef struct _EDBus_Signal_Handler EDBus_Signal_Handler;

typedef void (*EDBus_Message_Cb)(void *data, const EDBus_Message *msg, EDBus_Pending *pending);
typedef void (*EDBus_Signal_Cb)(void *data, const EDBus_Message *msg);
/**
 * @}
 */

#include "edbus_connection.h"
#include "edbus_message.h"
#include "edbus_signal_handler.h"
#include "edbus_pending.h"
#include "edbus_object.h"
#include "edbus_proxy.h"
#include "edbus_freedesktop.h"
#include "edbus_service.h"

#ifdef __cplusplus
}
#endif

#endif