/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ccsp_message_bus.h>
#include <ccsp_base_api.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include "ccsp_memory.h"
#include <ccsp_custom.h>
#include <dslh_definitions_database.h>
#include <sys/ucontext.h>
#include "ansc_platform.h"
#include <ccsp_psm_helper.h>

// TELEMETRY 2.0 //RDKB-25996
#include <telemetry_busmessage_sender.h>

#ifdef UNIT_TEST_DOCKER_SUPPORT
#define STATIC
#else
#define STATIC static
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

#ifndef SAFEC_DUMMY_API
#include "safe_str_lib.h"
#endif

#define RDK_SAFECLIB_ERR(func)  printf("safeclib error at %s %s:%d %s", __FILE__, __FUNCTION__, __LINE__, func)

#ifdef SAFEC_DUMMY_API

typedef int errno_t;
#define EOK 0

#define strcpy_s(dst,max,src) EOK; \
 strcpy(dst,src);
#define strncpy_s(dst,max,src,len)  EOK; \
 strncpy(dst,src,len);
#define strcat_s(dst,max,src) EOK; \
 strcat(dst,src);

static inline int strcmp_s(const char *dst, int dmax, const char *src, int *r) {
    UNREFERENCED_PARAMETER(dmax);
    *r = strcmp(dst, src);
    return EOK;
}

#endif

#define CCSP_MESSAGE_BUS_TEST

#ifdef CCSP_MESSAGE_BUS_TEST

static void *bus_handle = NULL;

static char dst_pathname_cr[64] =  {0};
STATIC char subsystem_prefix[32] = "";

#define MAX_PARAM 20000
struct param_rtt
{
  char *param_name;
  int rtt;
};

typedef struct
{
    char *description;
    unsigned long retValue;
}
RETURN_VALUE_TO_STRING;

STATIC struct param_rtt *rtt_result = NULL;
static int rtt_ct = 0;
static int runSteps = __LINE__;
static BOOL bVerbose = TRUE;

STATIC int param_rtt_cmp (const void *c1, const void *c2)
{
    return ((struct param_rtt *)c1)->rtt < ((struct param_rtt *)c2)->rtt;
}

static void free_rtt_result (void)
{
    rtt_ct = 0;
    AnscFreeMemory (rtt_result);
    rtt_result = NULL;
}

#define color_parametername    color_green
#define color_parametervalue   color_yellow
#define color_error            color_red
#define color_succeed          color_green
#define color_end              color_none

#define color_gray   "\033[1;30m"
#define color_green  "\033[0;32m"
#define color_red    "\033[0;31m"
#define color_yellow "\033[0;33m"
#define color_none   "\033[0m"

#define BUSCLIENT_MAX_COUNT_SUPPORTED 20
#define MAX_CMD_ARG_NUM               20*3+2
#define MAX_COMP_NAME_OR_ID_LEN       64

#define NUM_CCSP_TYPES (sizeof(ccsp_type_table)/sizeof(ccsp_type_table[0]))

typedef struct ccsp_pair {
  char              *name;
  enum dataType_e   type;
} CCSP_PAIR;

static CCSP_PAIR ccsp_type_table[] = {
  { "string",   ccsp_string },
  { "int",      ccsp_int },
  { "uint",     ccsp_unsignedInt },
  { "bool",     ccsp_boolean },
  { "dateTime", ccsp_dateTime },
  { "base64",   ccsp_base64 },
  { "long",     ccsp_long },
  { "ulong",    ccsp_unsignedLong },
  { "float",    ccsp_float },
  { "double",   ccsp_double },
  { "byte",     ccsp_byte },
  { "none",     ccsp_none}
};

typedef struct 
{
    char * pathname;
    char * val1;  // type or notify
    char * val2;  // value or accesslist
}TRIPLE_VALUE, *PTRIPLE_VALUE;

typedef struct
{
    char * command;
    char * val1; // commit or nextlevel.
    TRIPLE_VALUE  result[BUSCLIENT_MAX_COUNT_SUPPORTED+1];
}CMD_CONTENT, *PCMD_CONTENT;

static char * help_description[] =
{
    color_parametername"setvalues"color_parametervalue" pathname type value [pathname type value] ... [commit]",
    color_parametername"setcommit"color_parametervalue,
    color_parametername"getvalues"color_parametervalue" pathname [pathname] ...",
    color_parametername"retv"color_parametervalue" pathname [pathname]",
    color_parametername"sgetvalues"color_parametervalue" pathname [pathname] ...",
    color_parametername"setattributes"color_parametervalue" pathname notify accesslist [pathname notify accesslist ] ...",
    color_parametername"getattributes"color_parametervalue" pathname [pathname] ...",
    color_parametername"addtable"color_parametervalue" pathname",
    color_parametername"deltable"color_parametervalue" pathname",
    color_parametername"getnames"color_parametervalue" pathname [nextlevel]",
    color_parametername"setsub"color_parametervalue" set subsystem_prefix when call CcspBaseIf_discComponentSupportingNamespace",
    color_parametername"psmget"color_parametervalue" pathname",
    color_parametername"psmset"color_parametervalue" pathname type value",
    color_parametername"psmdel"color_parametervalue" pathname",
    color_parametername"help"color_parametervalue,
    color_parametername"exit"color_parametervalue,
    "-------------------------------------",
    "retv      : This cmd is used to return the value of parameter only.",
    "sgetvalues: This cmd is used to calculate GPV time.",
    "pathname  : It's a full name or partial name.",
    "type      : It is one of string/int/uint/bool/datetime/base64/float/double/byte.",
    "value     : It is a string for all types, even for int or enumeration. ",
    "            If the string need to contain blank space, pls put value like \"aa bb\".",
    "commit    : It is true or false. It's true by default.",
    "notify    : It is one of unchange/off/passive/active.",
    "accesslist: It can be only one of unchange/acs/xmpp/cli/webgui/anybody.",
    "nextlevel : It is true or false. It's true by default."color_end,
    NULL
};


#if 0
static const char* msg_path = "/com/cisco/spvtg/ccsp/EVENT" ;
static const char* msg_interface = "com.cisco.spvtg.ccsp.EVENT" ;
static const char* msg_method = "__send" ;
//static const char* app_msg_method = "__app_request" ;
static const char* Introspect_msg = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                                    "<node name=\"/com/cisco/ccsp/dbus\">\n"
                                    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                                    "    <method name=\"Introspect\">\n"
                                    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
                                    "    </method>\n"
                                    "  </interface>\n"
                                    "  <interface name=\"ccsp.msg\">\n"
                                    "    <method name=\"__send\">\n"
                                    "      <arg type=\"s\" name=\"from\" direction=\"in\" />\n"
                                    "      <arg type=\"s\" name=\"request\" direction=\"in\" />\n"
                                    "      <arg type=\"s\" name=\"response\" direction=\"out\" />\n"
                                    "    </method>\n"
                                    "    <method name=\"__app_request\">\n"
                                    "      <arg type=\"s\" name=\"from\" direction=\"in\" />\n"
                                    "      <arg type=\"s\" name=\"request\" direction=\"in\" />\n"
                                    "      <arg type=\"s\" name=\"argu\" direction=\"in\" />\n"
                                    "      <arg type=\"s\" name=\"response\" direction=\"out\" />\n"
                                    "    </method>\n"
                                    "  </interface>\n"
                                    "</node>\n"
                                    ;
#endif

static RETURN_VALUE_TO_STRING retValueToString[] =
{
    {"CCSP_SUCCESS",                               100},
    {"CCSP_ERR_MEMORY_ALLOC_FAIL",                 101},
    {"CCSP_FAILURE",                               102},
    {"CCSP_ERR_NOT_CONNECT",                       190},   //can't connect to daemon
    {"CCSP_ERR_TIMEOUT",                           191},
    {"CCSP_ERR_NOT_EXIST",                         192},   //remote not exist 
    {"CCSP_ERR_NOT_SUPPORT",                       193},   //remote can't support this api
    
     /* Bin add more TR Fault Error Code */
    {"CCSP_ERR_CWMP_BEGINNING",                     9000},
    {"CCSP_ERR_METHOD_NOT_SUPPORTED",               9000},
    {"CCSP_ERR_REQUEST_REJECTED",                   9001},
    {"CCSP_ERR_INTERNAL_ERROR",                     9002},
    {"CCSP_ERR_INVALID_ARGUMENTS",                  9003},
    {"CCSP_ERR_RESOURCE_EXCEEDED",                  9004},
    {"CCSP_ERR_INVALID_PARAMETER_NAME",             9005},
    {"CCSP_ERR_INVALID_PARAMETER_TYPE",             9006},
    {"CCSP_ERR_INVALID_PARAMETER_VALUE",            9007},
    {"CCSP_ERR_NOT_WRITABLE",                       9008},
    {"CCSP_ERR_SETATTRIBUTE_REJECTED",              9009},
    {"CCSP_ERR_FILE_TRANSFER_FAILURE",              9010},
    {"CCSP_ERR_UPLOAD_FAILURE",                     9011},
    {"CCSP_ERR_FILE_TRANSFER_AUTH_FAILURE",         9012},
    {"CCSP_ERR_UNSUPPORTED_PROTOCOL",               9013},
    {"CCSP_ERR_UNABLE_TO_JOIN_MULTICAST",           9014},
    {"CCSP_ERR_UNABLE_TO_CONTACT_FILE_SERVER",      9015},
    {"CCSP_ERR_UNABLE_TO_ACCESS_FILE",              9016},
    {"CCSP_ERR_UNABLE_TO_COMPLETE_DOWNLOAD",        9017},
    {"CCSP_ERR_FILE_CORRUPTED_OR_UNUSABLE",         9018},
    {"CCSP_ERR_FILE_AUTH_FAILURE",                  9019},
    {"CCSP_ERR_UNABLE_TO_COMPLETE_ONTIME",          9020},
    {"CCSP_ERR_CANCELATION_NOT_PERMITTED",          9021},
    {"CCSP_ERR_INVALID_UUID_FORMAT",                9022},
    {"CCSP_ERR_UNKNOWN_EE",                         9023},
    {"CCSP_ERR_DISABLED_EE",                        9024},
    {"CCSP_ERR_DU_EE_MISMATCH",                     9025},
    {"CCSP_ERR_DUPLICATE_DU",                       9026},
    {"CCSP_ERR_SYSTEM_RES_EXCEEDED",                9027},
    {"CCSP_ERR_UNKNOWN_DU",                         9028},
    {"CCSP_ERR_INVALID_DU_STATE",                   9029},
    {"CCSP_ERR_DOWNGRADE_NOT_PERMITTED",            9030},
    {"CCSP_ERR_VERSION_NOT_SPECIFIED",              9031},
    {"CCSP_ERR_VERSION_EXISTS",                     9032},     
     /* the maxi CWMP error code */
    {"CCSP_ERR_CWMP_ENDING",                        9032},

    {"CCSP_CR_ERR_NAMESPACE_OVERLAP",              201},
    {"CCSP_CR_ERR_UNKNOWN_COMPONENT",              202},
    {"CCSP_CR_ERR_NAMESPACE_MISMATCH",             203},
    {"CCSP_CR_ERR_UNSUPPORTED_NAMESPACE",          204},
    {"CCSP_CR_ERR_DP_COMPONENT_VERSION_MISMATCH",  205},
    {"CCSP_CR_ERR_INVALID_PARAM",                  206},
    {"CCSP_CR_ERR_UNSUPPORTED_DATATYPE",           207},
    {"CCSP_CR_ERR_SESSION_IN_PROGRESS",            208},
    {NULL, 0}
};

static int ccsp_type_from_name(char *name, enum dataType_e *type_ptr)
{
  errno_t rc = -1;
  int ind = -1;
  unsigned int i = 0;
  if(name == NULL)
     return 0;
  for (i = 0 ; i < NUM_CCSP_TYPES ; ++i)
  {
      rc = strcmp_s(name, strlen(name), ccsp_type_table[i].name, &ind);
      if((rc == EOK) && (!ind))
      {
          *type_ptr = ccsp_type_table[i].type;
          return 1;
      }
  }
  return 0;
}
#ifndef INCLUDE_BREAKPAD
STATIC void ccsp_exception_handler(int sig, siginfo_t *info, void *context)
{
    UNREFERENCED_PARAMETER(context);
    int fd1;
    pid_t pid = getpid();
    char mapsFile[32]     = {0};
    char cmdFile[32]      = {0};
    char cmdName[32]      = {0}; 
    time_t rawtime;
    struct tm * timeinfo;

    sprintf( mapsFile, "/proc/%d/maps",    pid );
    sprintf( cmdFile,  "/proc/%d/cmdline", pid );

    /* Get current time */
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );   
    
    /* Get command name */
    fd1 = open( cmdFile, O_RDONLY );
    if( fd1 > 0 )
    {
        /*Coverity Fix CID:72093 CHECKED_RETURN */
        if( read( fd1, cmdName, sizeof(cmdName)-1 ) <= 0)
            fprintf( stderr, "Error in read function:%s\n",__FUNCTION__ );
      
       /* Coverity Fix CID:68933 RESOURCE_LEAK */
        close(fd1);

    }
      

    /* dump general information */
    fprintf( stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    fprintf( stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!! Exception Caught !!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    fprintf( stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );

    fprintf( stderr, "\n\nSignal info:\n"
                        "\tTime: %s "
                        "\tProcess name: <%s>\n"
                        "\tPID: %d\n"
                        "\tFault Address: 0x%08lx\n"
                        "\tSignal: %d \n"
                        "\tSignal Code: %d\n"
                        "\tLast errno: %d:%s \n"
                        "\tLast error (by signal): %d\n", 
                        asctime (timeinfo),
                        cmdName, 
                        pid,
                        (unsigned long int)info->si_addr,
                        sig,
                        info->si_code,
                        errno, strerror(errno),
                        info->si_errno );

    fprintf( stderr, "\nThe latest Line number is:%d.\n", runSteps);
    
    /* Output maps information in order to locate crash pointer */
    fd1 = open( mapsFile, O_RDONLY );
    if( fd1 > 0 )
    {
        unsigned char    buf[ 512 ] = {0};
        unsigned int     readBytes = 0;

        /* Read the maps file */
        fprintf(stderr, "\n/proc/%d/maps:\n", pid);
        while( ( readBytes = read( fd1, buf, sizeof(buf) ) ) > 0 )
        {
            /* Coverity Issue Fix - CID:67949 : Non Constant PrintFormat */
            fprintf( stderr,"%s", buf );
            memset(buf, 0, sizeof(buf));
        }

        close(fd1);
    }

    fprintf( stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    fprintf( stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!! Dump Ending!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    fprintf( stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

#ifdef UNIT_TEST_DOCKER_SUPPORT
    return;
#else
    _exit(1);
#endif
}

static void enable_ccsp_exception_handlers (void)
{
    struct sigaction sigact;
    
    memset( &sigact, 0, sizeof( struct sigaction ) );
    sigact.sa_sigaction = ccsp_exception_handler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;
    
    //sigaction(SIGINT,  &sigact, 0L); 
    sigaction(SIGSEGV, &sigact, 0L); 
    sigaction(SIGILL,  &sigact, 0L); 
    sigaction(SIGBUS,  &sigact, 0L); 
    sigaction(SIGQUIT, &sigact, 0L);

    return;
}
#endif

#if 0
static DBusHandlerResult
path_message_func (DBusConnection  *conn,
                   DBusMessage     *message,
                   void            *user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    const char *interface = dbus_message_get_interface(message);
    const char *method   = dbus_message_get_member(message);
    DBusMessage *reply;
//        char tmp[4098];
    char *resp = "888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888";
    char *from = 0;
    char *req = 0;
    char * err_msg  = DBUS_ERROR_NOT_SUPPORTED;
    errno_t rc1 = -1, rc2 = -1;
    int ind1 = -1, ind2 = -1;

    reply = dbus_message_new_method_return (message);
    if (reply == NULL)
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    rc1 = strcmp_s("org.freedesktop.DBus.Introspectable", strlen("org.freedesktop.DBus.Introspectable"), interface, &ind1);
    rc2 = strcmp_s("Introspect", strlen("Introspect"), method, &ind2);
    if((rc1 != EOK) || (rc2 != EOK))
    {
        RDK_SAFECLIB_ERR("strcmp_s");
    }
    if(((rc1 == EOK) && !(ind1)) && ((rc2 == EOK) && !(ind2)))
    {
        printf("Received method Introspect...\n");

        if ( !dbus_message_append_args (reply, DBUS_TYPE_STRING, &Introspect_msg, DBUS_TYPE_INVALID))
            printf ("No memory\n");

        if (!dbus_connection_send (conn, reply, NULL))
            printf ("No memory\n");

        dbus_message_unref (reply);
        return DBUS_HANDLER_RESULT_HANDLED;

    }

//    printf("dbus_message_is_method_call %d\n", UserGetTickInMilliSeconds2());
    rc1 = strcmp_s(msg_interface, strlen(msg_interface), interface, &ind1);
    rc2 = strcmp_s(msg_method, strlen(msg_method), method, &ind2);
    if((rc1 != EOK) || ( rc2 != EOK))
    {
        RDK_SAFECLIB_ERR("strcmp_s");
    }
    if(((rc1 == EOK) && !(ind1)) && ((rc2 == EOK) && !(ind2)))
    {
        printf("Received method %s...\n", method);

        if(dbus_message_get_args (message,
                                  NULL,
                                  DBUS_TYPE_STRING, &from,
                                  DBUS_TYPE_STRING, &req,
                                  DBUS_TYPE_INVALID))
        {
            dbus_message_append_args (reply, DBUS_TYPE_STRING, &resp, DBUS_TYPE_INVALID);

            if (!dbus_connection_send (conn, reply, NULL))
                printf ("No memory\n");

            dbus_message_unref (reply);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    dbus_message_set_error_name (reply, err_msg) ;
    dbus_connection_send (conn, reply, NULL);
    dbus_message_unref (reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}
#endif

/*
This is an example with 1 in and 1 out parameter, if you need more parameter, change the message pack/unpack here and  path_message_func.
example for pack a struct:
    dbus_message_iter_init_append (message, &iter);

    DBusMessageIter container_iter;
//    unsigned char byte = 5;
//    ret = dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE, &byte);
    unsigned char byte = 5;
    int threadnr = 1;
    if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &threadnr))
        printf ("error\n");

    char sig[5];
    sig[0] = DBUS_TYPE_STRING;
    sig[1] = 0;

    ret = dbus_message_iter_open_container (&iter,
                                            DBUS_TYPE_ARRAY,
                                            sig,
                                            &container_iter);

    tmp = "168";
    ret = dbus_message_iter_append_basic (&container_iter, DBUS_TYPE_STRING, &tmp);
    tmp = "169";
    ret = dbus_message_iter_append_basic (&container_iter, DBUS_TYPE_STRING, &tmp);
    ret = dbus_message_iter_close_container (&iter,
            &container_iter);

    ret = dbus_message_iter_open_container (&iter,
                                            DBUS_TYPE_STRUCT,
                                            NULL,
                                            &container_iter);
    tmp = "170";
    ret = dbus_message_iter_append_basic (&container_iter, DBUS_TYPE_STRING, &tmp);
    ret = dbus_message_iter_append_basic (&container_iter, DBUS_TYPE_BYTE, &byte);
    ret = dbus_message_iter_close_container (&iter,
            &container_iter);

unpack:
      DBusMessageIter iter;
              int val;
                 dbus_message_iter_init (message, &iter);
                dbus_message_iter_get_basic (&iter, &val);
                printf("%d\n", val);

            dbus_message_iter_next (&iter)    ;
                DBusMessageIter subiter;

                 char * str;
                dbus_message_iter_recurse (&iter, &subiter);
                dbus_message_iter_get_basic (&subiter, &str);
                printf("%s\n", str);
             dbus_message_iter_next     (&subiter);
                dbus_message_iter_get_basic (&subiter, &str);
                printf("%s\n", str);

            dbus_message_iter_next (&iter)    ;
                dbus_message_iter_recurse (&iter, &subiter);
                dbus_message_iter_get_basic (&subiter, &str);
                printf("%s\n", str);

*/
#if 0
static int CCSP_Message_Bus_Send
(
    void* bus_handle,
    char* component_id,
    const char* path,
    const char* interface,
    const char* method,
    char* request,
    char** response,
    int timeout_seconds
)
{
    DBusMessage *message;
    DBusMessage *reply;
    int ret = CCSP_Message_Bus_ERROR;
    char * res = 0;

    message = dbus_message_new_method_call (component_id,
                                            path,
                                            interface,
                                            method);
    if (!message )
    {
        printf ("No memory9a\n");
        return CCSP_Message_Bus_OOM;
    }

    if ( !dbus_message_append_args (message, DBUS_TYPE_STRING, &component_id, DBUS_TYPE_STRING, &request, DBUS_TYPE_INVALID))
    {
        printf ("No memory9\n");
        ret = CCSP_Message_Bus_OOM;
        goto EXIT;
    }

    ret = CCSP_Message_Bus_Send_Msg(bus_handle, message, timeout_seconds, &reply);
    if(reply )
    {
        if(dbus_message_get_args (reply,
                                  NULL,
                                  DBUS_TYPE_STRING, &res,
                                  DBUS_TYPE_INVALID))
        {
            size_t len = strlen(res);
            *response = AnscAllocateMemory(len + 1);
            if(*response)
            {
                memcpy (*response, res, len + 1);
                ret = CCSP_Message_Bus_OK;
            }

        }

        dbus_message_unref (reply);
    }
EXIT:
    dbus_message_unref (message);

    return ret;
}
#endif

#if 0
static int CCSP_Message_Bus_Send_Event
(
    void* bus_handle,
    const char* path,
    const char* interface,
    const char* event_name,
    char* arg
)
{
    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    DBusMessage *message;
    int i;
    DBusConnection *conn;

    /*to support daemon redundency*/
    pthread_mutex_lock(&bus_info->info_mutex);
    for(i = 0; i < CCSP_MESSAGE_BUS_MAX_CONNECTION; i++)
    {
        if(bus_info->connection[i].connected && bus_info->connection[i].conn )
        {
            conn = bus_info->connection[i].conn;
            dbus_connection_ref (conn);
            break;
        }

    }
    pthread_mutex_unlock(&bus_info->info_mutex);

    if(i ==  CCSP_MESSAGE_BUS_MAX_CONNECTION)
        return CCSP_MESSAGE_BUS_CANNOT_CONNECT;


    message = dbus_message_new_signal (path, interface, event_name );

    if(!message)
        return CCSP_Message_Bus_OOM;

    if(arg)
    {
        if(!dbus_message_append_args (message, DBUS_TYPE_STRING, &arg, DBUS_TYPE_INVALID))
        {
            printf ("No memory\n");
            dbus_message_unref (message);
            return CCSP_Message_Bus_OOM;
        }
    }

     /*Coverity Fix CID: 80290 CHECKED_RETURN */
    if(dbus_connection_send (conn, message, NULL)!= TRUE)
       printf("dbus_connection_send is failed\n");

    dbus_message_unref (message);
    return CCSP_Message_Bus_OK;
}
#endif

#if 0
typedef struct _test_data
{
    void * bus_handle;
    char **argv;
    int argc;
} test_data;

static void *test_Send_Thread (void *dataptr)
{
    test_data *pdata =(test_data*)dataptr;
    void * bus_handle = pdata->bus_handle;
    char **argv = pdata->argv;
    int argc = pdata->argc;
    int count = 1;
    int errcount = 0;
    int ret,mod = 0;


    while(1)
    {
        char *resp = NULL;
        if(argc >= 4)
        {

///*
            ret = CCSP_Message_Bus_Send(bus_handle,argv[2],msg_path, msg_interface, msg_method ,"88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888", &resp ,5);
           /* Coverity Issue Fix - CID:57209 : Divide By Zero */
	   mod = atoi(argv[3]);
           if (mod == 0) 
	   {	
        	printf("count cannot be zero\n");
                /*Coverity Fix CID:121819 RESOURCE_LEAK */
                AnscFreeMemory(resp);
        	return (void *)(-1);
    	   } 		

	    if( ( count % mod ) == 0)
            {
                printf("count 2 %d  errcount %d\n", count, errcount);
                //                sleep(1);
//                CCSP_Msg_SleepInMilliSeconds(300);
            }

            if(ret == CCSP_Message_Bus_OK)
            {
                if(strcmp(resp,"888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888"))
                    errcount++;
                AnscFreeMemory(resp);
            }
            else
            {
//               printf("ret fail \n");
                errcount++;
                CCSP_Msg_SleepInMilliSeconds(5000);
            }
//*/


            count ++;
            if(count > (atoi(argv[4])/2))
                break;
        }
        else
            CCSP_Msg_SleepInMilliSeconds(20);
    }
    printf("exit thread \n");
    return 0;
}
#endif

//unused function
#if 0
static DBusHandlerResult
evt_callback (DBusConnection  *conn,
              DBusMessage     *message,
              void            *user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    UNREFERENCED_PARAMETER(conn);
    const char *interface = dbus_message_get_interface(message);
    const char *method   = dbus_message_get_member(message);
    printf("evt_callback %s %s \n", interface , method);
    return DBUS_HANDLER_RESULT_HANDLED;

}
#endif

/* This function transfers bus return values to string*/
STATIC char *ccspReturnValueToString(unsigned long ret)
{
    int i = 0;

    while( retValueToString[i].description )
    {
        if ( retValueToString[i].retValue == ret )
            return retValueToString[i].description;

        i++;
    }

    return "UNKNOWN MESSAGE TYPE";

}

STATIC int apply_cmd(PCMD_CONTENT pInputCmd )
{
//    void *bus_handle2;
    int ret ;
    unsigned char bTmp = 1;
    parameterInfoStruct_t **parameter = NULL;
    parameterValStruct_t **parameterVal = NULL;
    char * pFaultParameter = NULL;
    int size = 0;
    int size2 = 0;
    int i = 0;
    int index  = 0;
    char * pTmp = NULL;
    char * parameterNames[BUSCLIENT_MAX_COUNT_SUPPORTED] = {NULL}; /*RDKB-7436, CID-33429, init before use */
    parameterValStruct_t val[BUSCLIENT_MAX_COUNT_SUPPORTED] = {{0}};
    parameterAttributeStruct_t valAttr[BUSCLIENT_MAX_COUNT_SUPPORTED] = {{0}};
    parameterAttributeStruct_t ** parameterAttr = NULL;
    componentStruct_t ** ppComponents = NULL;
	int ct ;
    /* Coverity Issue Fix - CID:110565,110596  : UnInitialised Variable*/
    unsigned int psmType = 0;
    char *psmValue = NULL;
    char *psmName  = NULL ;
    struct timeval start, end;
    long mtime, mtime_onetime, seconds, useconds, total_mtime;    

    char *                          dst_componentid         =  NULL;
    char *                          dst_pathname            =  NULL;

    static char                     PsmComponentId[MAX_COMP_NAME_OR_ID_LEN]      = {0};
    static char                     PsmComponentPath[MAX_COMP_NAME_OR_ID_LEN]    = {0};
    errno_t rc  = -1;
    int     ind = -1;
    bool skip_Component_discovery = false;

  //CCSP_Msg_SleepInMilliSeconds(500);

    runSteps = __LINE__;

    if ( bVerbose )
    {
        printf(color_parametername"subsystem_prefix %s\n", subsystem_prefix);
    }

    if ((strncmp( pInputCmd->command, "setvalues", 4 ) == 0 ) || (strncmp( pInputCmd->command, "getvalues", 4 ) == 0 ) || (strncmp( pInputCmd->command, "getnames", 4 ) == 0))
    {
        skip_Component_discovery = true;
    }
    /* We need look for destination from CR*/
    if ( pInputCmd[0].result[0].pathname )
    {

        runSteps = __LINE__;

      if (strncmp(pInputCmd->command, "psm", 3) == 0 || skip_Component_discovery)
        {
            size2 = 1;
        }
        else
        {
            gettimeofday(&start, NULL);

            ret = CcspBaseIf_discComponentSupportingNamespace 
                (
                 bus_handle,
                 dst_pathname_cr,
                 pInputCmd[0].result[0].pathname,
                 subsystem_prefix,
                 &ppComponents,
                 &size2
                );

            runSteps = __LINE__;

            if ( ret == CCSP_SUCCESS )
            {
                /*
                   printf("componentName:%s dbusPath:%s %s %s %d\n", ppComponents[0]->componentName, ppComponents[0]->dbusPath, ppComponents[0]->remoteCR_dbus_path,
                   ppComponents[0]->remoteCR_name, ppComponents[0]->type );
                   */
                if ( size2 == 0 )
                {
                    if ( bVerbose )
                    {
                        printf(color_error"Can't find destination component.\n"color_end);
                    }
                    return 1;
                }
            }
            else
            {
               if((ret == CCSP_MESSAGE_BUS_NOT_EXIST)||(ret == CCSP_CR_ERR_UNSUPPORTED_NAMESPACE))
               {
                  if ( bVerbose )
                  {
                     printf(color_error"Can't find destination component.\n"color_end);
                  }
               }
               else
               {
                  if ( bVerbose )
                  {
                     printf(color_error"Ccsp msg bus internal error %d \n"color_end,ret);
                  }
	          if(ret == CCSP_ERR_NOT_CONNECT)
	          {
	             t2_event_d("SYS_ERROR_CCSPBus_error190", 1);	
	          }
	          else if(ret == CCSP_ERR_TIMEOUT)
	          {
	             t2_event_d("SYS_ERROR_CCSPBus_error191", 1);
	          }
	       }
               return 1;
            }     

            gettimeofday(&end, NULL);
            seconds  = end.tv_sec  - start.tv_sec;
            useconds = end.tv_usec - start.tv_usec;
            mtime_onetime = ((seconds) * 1000000 + useconds);		
         
            if ( strncmp( pInputCmd->command, "sgetvalues"     , 4 ) == 0 )
                printf(color_succeed"\nTotal time to look up destination component from CR: %ld microseconds.\n\n", mtime_onetime);

            runSteps = __LINE__;
            
        }
    }
    

    /* we need handle all component */
    for ( index = 0; index < size2; index++ )
    {
        if (strncmp(pInputCmd->command, "psm", 3) == 0)
        {
            /*
             *  subsystem prefix is needed for platforms other than "simu"
             */
            if ( _ansc_strlen(PsmComponentId) == 0 )
            {
                if ( _ansc_strlen(subsystem_prefix) != 0 )
                {
                    rc = strcpy_s(PsmComponentId, MAX_COMP_NAME_OR_ID_LEN, subsystem_prefix);
                    if(rc != EOK)
                    {
                        RDK_SAFECLIB_ERR("strcpy_s");
                    }
                    rc = strcat_s(PsmComponentId, MAX_COMP_NAME_OR_ID_LEN, "com.cisco.spvtg.ccsp.psm");
                    if(rc != EOK)
                    {
                        RDK_SAFECLIB_ERR("strcat_s");
                    }
                    rc = strcpy_s(PsmComponentPath, MAX_COMP_NAME_OR_ID_LEN, subsystem_prefix);
                    if(rc != EOK)
                    {
                        RDK_SAFECLIB_ERR("strcpy_s");
                    }
                    /* Remove the "." */
                    PsmComponentPath[_ansc_strlen(PsmComponentPath) - 1] = '\0';
                    rc = strcat_s(PsmComponentPath, MAX_COMP_NAME_OR_ID_LEN, "com/cisco/spvtg/ccsp/psm");
                    if(rc != EOK)
                    {
                        RDK_SAFECLIB_ERR("strcat_s");
                    }
                }
                else
                {
                    rc = strcpy_s(PsmComponentId, MAX_COMP_NAME_OR_ID_LEN, "com.cisco.spvtg.ccsp.psm");
                    if(rc != EOK)
                    {
                        RDK_SAFECLIB_ERR("strcpy_s");
                    }
                    rc = strcpy_s(PsmComponentPath, MAX_COMP_NAME_OR_ID_LEN, "com/cisco/spvtg/ccsp/psm");
                    if(rc != EOK)
                    {
                        RDK_SAFECLIB_ERR("strcpy_s");
                    }
                }

                dst_componentid = PsmComponentId;
                dst_pathname    = PsmComponentPath;
            }
        }
        else if (ppComponents)
        {
            dst_componentid = ppComponents[index]->componentName;
            dst_pathname    = ppComponents[index]->dbusPath;
        }

        if ( bVerbose )
        {
//          printf(color_parametername"%s from/to component(%s):\n"color_parametervalue, pInputCmd->command, dst_componentid);
            if (!skip_Component_discovery)
                printf(color_parametername"%s from/to component(%s): %s\n", pInputCmd->command, dst_componentid, pInputCmd->result[0].pathname);
        }

        runSteps = __LINE__;
	/* Coverity Issue Fix - CID:110537 : Forward NULL*/
	if((!skip_Component_discovery) && (dst_componentid == NULL))
	{
		printf("%s-%d:Coverity Error occured as Forward NULL in dst_componentid\n",__FUNCTION__,__LINE__);
		return -1;
	}
        if ( strncmp( pInputCmd->command, "addtable"     , 4 ) == 0 )
        {
            ret = CcspBaseIf_AddTblRow(
                bus_handle,
                dst_componentid,
                dst_pathname,
                0,
                pInputCmd->result[0].pathname,
                &size
            );

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
                printf(color_succeed"%s%d. is added.\n", pInputCmd->result[0].pathname, size);
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

        }
        else if ( strncmp( pInputCmd->command, "deltable"     , 4 ) == 0 )
        {
            ret = CcspBaseIf_DeleteTblRow(
                bus_handle,
                dst_componentid,
                dst_pathname,
                0,
                pInputCmd->result[0].pathname
            );

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }
        }
        else if ( strncmp( pInputCmd->command, "psmdel"     , 4 ) == 0 )
        {
            ret = PSM_Del_Record(bus_handle, subsystem_prefix, pInputCmd->result[0].pathname);

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }
        }
        else if ( strncmp( pInputCmd->command, "setvalues"     , 4 ) == 0 )
        {

            runSteps = __LINE__;
        
            i = 0;
            while ( pInputCmd->result[i].pathname && (i < (BUSCLIENT_MAX_COUNT_SUPPORTED-1)) ) /*RDKB-7436, CID-33004, Limiting Out of bound access*/
            {           
                val[i].parameterName  = AnscCloneString(pInputCmd->result[i].pathname);

                runSteps = __LINE__;
                rc = strcmp_s("void", strlen("void"), pInputCmd->result[i].val2, &ind);
                if(rc != EOK)
                {
                    RDK_SAFECLIB_ERR("strcmp_s");
                }

                if ((rc == EOK) && (!ind))
                    val[i].parameterValue = NULL;
                else
                    val[i].parameterValue = AnscCloneString(pInputCmd->result[i].val2);

                if (! ccsp_type_from_name(pInputCmd->result[i].val1, &val[i].type))
                {
                    printf("unrecognized type name: %s", pInputCmd->result[i].val1);
                    return 0;
                }

                if(val[i].type == ccsp_boolean)
                {
                    /* support true/false or 1/0 here.*/
                    if ( !(rc = strcmp_s("1", strlen("1"), val[i].parameterValue, &ind)) )
                    {
                         if(!(ind))
                         {
                              val[i].parameterValue = AnscCloneString("true");
                         }
                    }
                    else if ( !(rc = strcmp_s("0", strlen("0"), val[i].parameterValue, &ind)) )
                    {
                         if(!ind)
                         {
                              val[i].parameterValue = AnscCloneString("false");
                         }
                    }
                    else if(rc != EOK)
                    {
                         RDK_SAFECLIB_ERR("strcmp_s");
                    }
                }

                runSteps = __LINE__;

                //printf("parameterName:%s, parameterValue:%s, type:%d, i:%d.\n", val[i].parameterName, val[i].parameterValue, val[i].type, i);
                i++;

            }

            runSteps = __LINE__;

            bTmp = 1;
            if ( pInputCmd->val1 )
            {
                if ( !strncmp(pInputCmd->val1, "false", 5 ) )
                {
                    bTmp = 0;
                }
            }

            runSteps = __LINE__;

            //printf("bTmp:%d.\n",bTmp);

            ret = CcspBaseIf_setParameterValues(
                bus_handle,
                dst_componentid,
                dst_pathname,
                0,
                DSLH_MPA_ACCESS_CONTROL_CLIENTTOOL,
                val,
                i,
                bTmp,
                &pFaultParameter
            );

            runSteps = __LINE__;

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else if((ret == CCSP_MESSAGE_BUS_CANNOT_CONNECT)||(ret == CCSP_CR_ERR_UNSUPPORTED_NAMESPACE))
            {
                printf(color_error"Can't find destination component.\n"color_end);
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

            runSteps = __LINE__;

            if ( pFaultParameter != NULL )
                AnscFreeMemory(pFaultParameter);

            runSteps = __LINE__;

        }
        else if ( strncmp( pInputCmd->command, "psmset"     , 4 ) == 0 )
        {
            val[0].parameterName  = AnscCloneString(pInputCmd->result[0].pathname);

            rc = strcmp_s("void", strlen("void"), pInputCmd->result[0].val2, &ind);
            if(rc != EOK)
            {
                 RDK_SAFECLIB_ERR("strcmp_s");
            }
            if ((rc == EOK) && (!ind))
                val[0].parameterValue = NULL;
            else
                val[0].parameterValue = AnscCloneString(pInputCmd->result[0].val2);

            if (! ccsp_type_from_name(pInputCmd->result[0].val1, &val[0].type))
            {
                printf("unrecognized type name: %s", pInputCmd->result[0].val1);
                return 0;
            }

            if(val[0].type == ccsp_boolean)
            {
                  /* support true/false or 1/0 here.*/
                  if ( !(rc = strcmp_s("1", strlen("1"), val[0].parameterValue, &ind)) )
                  {
                       if(!(ind))
                       {
                            val[i].parameterValue = AnscCloneString("true");
                       }
                  }
                  else if ( !(rc = strcmp_s("0", strlen("0"), val[0].parameterValue, &ind)) )
                  {
                       if(!ind)
                       {
                            val[i].parameterValue = AnscCloneString("false");
                       }
                  }
                  else if(rc != EOK)
                  {
                       RDK_SAFECLIB_ERR("strcmp_s");
                  }
            }

            ret = PSM_Set_Record_Value2(
                    bus_handle,
                    subsystem_prefix,
                    val[0].parameterName,
                    val[0].type,
                    val[0].parameterValue);

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }
        }
        else if ( strncmp( pInputCmd->command, "setcommit"     , 4 ) == 0 )
        {
            runSteps = __LINE__;
        
            ret = CcspBaseIf_setCommit(
                bus_handle,
                dst_componentid,
                dst_pathname,
                0,
                DSLH_MPA_ACCESS_CONTROL_CLIENTTOOL,
                1
            );

            runSteps = __LINE__;

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }
            
            runSteps = __LINE__;

        }
        else if ( strncmp( pInputCmd->command, "setattributes"     , 4 ) == 0 )
        {
            size = 0;
            while ( pInputCmd->result[size].pathname )
            {
                valAttr[size].parameterName  = AnscCloneString(pInputCmd->result[size].pathname);
                if ( pInputCmd->result[size].val1 )
                {
                    valAttr[size].notificationChanged = 1;
                    if ( !strncmp(pInputCmd->result[size].val1, "passive", 7 ) )
                        valAttr[size].notification = 1;
                    else if ( !strncmp(pInputCmd->result[size].val1, "active", 6 ) )
                        valAttr[size].notification = 2;
                    else if ( !strncmp(pInputCmd->result[size].val1, "off", 3 ) )
                        valAttr[size].notification = 0;
                    else
                        valAttr[size].notification = 0;
                }
                else
                {
                    valAttr[size].notificationChanged = 0;
                    valAttr[size].notification = 0;
                }

                if ( pInputCmd->result[size].val2 )
                {
                    valAttr[size].accessControlChanged = 1;
                    
                    //acs/xmpp/cli/webgui/anybody
                    if ( !strncmp(pInputCmd->result[size].val2, "acs", 3 ) )
                        valAttr[size].accessControlBitmask = 0x0;
                    else if ( !strncmp(pInputCmd->result[size].val2, "xmpp", 4 ) )
                        valAttr[size].accessControlBitmask = 0x1;
                    else if ( !strncmp(pInputCmd->result[size].val2, "cli", 3 ) )
                        valAttr[size].accessControlBitmask = 0x2;
                    else if ( !strncmp(pInputCmd->result[size].val2, "webgui", 6 ) )
                        valAttr[size].accessControlBitmask = 0x4;
                    else if ( !strncmp(pInputCmd->result[size].val2, "anybody", 7 ) )
                        valAttr[size].accessControlBitmask = 0xFFFFFFFF;
                }
                else
                {
                    valAttr[size].accessControlChanged = 0;
                    valAttr[size].accessControlBitmask = 0;
                }

                valAttr[size].access = 1;

                /*
                printf("name:%s, notifycation:%d, changed:%d, accessmap:0x%x, changed:%d.\n",valAttr[size].parameterName,
                    valAttr[size].notification, valAttr[size].notificationChanged,
                    valAttr[size].accessControlBitmask, valAttr[size].accessControlChanged  );
                */
                size++;
            };

            ret = CcspBaseIf_setParameterAttributes(
                bus_handle,
                dst_componentid,
                dst_pathname,
                0,
                valAttr,
                size
            );
            
            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

        }
        else if ( strncmp( pInputCmd->command, "getnames"     , 4 ) == 0 )
        {
            runSteps = __LINE__;
        
            bTmp =1 ;
            if ( pInputCmd->val1 )
            {
                if ( !strncmp(pInputCmd->val1, "false", 5 ) )
                {
                    bTmp = 0;
                }
            }

            runSteps = __LINE__;
            
            ret = CcspBaseIf_getParameterNames(
                bus_handle,
                dst_componentid,
                dst_pathname,
                pInputCmd->result[0].pathname,
                bTmp,
                &size ,
                &parameter
            );

            runSteps = __LINE__;

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else if((ret == CCSP_MESSAGE_BUS_CANNOT_CONNECT)||(ret == CCSP_CR_ERR_UNSUPPORTED_NAMESPACE))
            {
                printf(color_error"Can't find destination component.\n"color_end);
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

            runSteps = __LINE__;

            //printf("!!! ret = %d size = %d !!!\n", ret, size);
            if(ret == CCSP_SUCCESS  && size >= 1)
            {
                for ( i = 0; i < size; i++ )
                {
                     printf(color_parametername"Parameter %4d name: %s "color_parametervalue"\n\twritable:%s. \n", i+1, parameter[i]->parameterName, parameter[i]->writable?"Writable":"ReadOnly");
                     AnscFreeMemory(parameter[i]->parameterName);
                     AnscFreeMemory(parameter[i]);
                }
                AnscFreeMemory(parameter);
                parameter = NULL;
            }
            
            runSteps = __LINE__;

        }
        else if ( strncmp( pInputCmd->command, "getvalues"     , 4 ) == 0 )
        {

            runSteps = __LINE__;

            i = 0;
            while ( pInputCmd->result[i].pathname )
            {
                parameterNames[i] = pInputCmd->result[i].pathname;
                i++;
            }

            runSteps = __LINE__;

            ret = CcspBaseIf_getParameterValues(
                bus_handle,
                dst_componentid,
                dst_pathname,
                parameterNames,
                i,
                &size ,
                &parameterVal
            );

            runSteps = __LINE__;

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else if((ret == CCSP_MESSAGE_BUS_CANNOT_CONNECT)||(ret == CCSP_CR_ERR_UNSUPPORTED_NAMESPACE))
            {
                printf(color_error"Can't find destination component.\n"color_end);
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

            //printf("%d, size:%d.\n", __LINE__,size);
            if(ret == CCSP_SUCCESS  && size >= 1)
            {
                for ( i = 0; i < size; i++ )
                {
                    printf
                        (
                            color_parametername"Parameter %4d name: %s\n"
                            color_parametervalue"               type: %10s,    value: %s \n",
                            i+1,
                            parameterVal[i]->parameterName,
                            (parameterVal[i]->type == ccsp_string)
                                ? "string"
                                : (parameterVal[i]->type == ccsp_int)
                                    ? "int"
                                    : (parameterVal[i]->type == ccsp_unsignedInt)
                                        ? "uint"
                                        : (parameterVal[i]->type == ccsp_boolean)
                                            ? "bool"
                                            : (parameterVal[i]->type == ccsp_dateTime)
                                                ? "dateTime"
                                                : (parameterVal[i]->type == ccsp_base64)
                                                    ? "base64"
                                                    : (parameterVal[i]->type == ccsp_long)
                                                        ? "long"
                                                        : (parameterVal[i]->type == ccsp_unsignedLong)
                                                            ? "ulong"
                                                            : (parameterVal[i]->type == ccsp_float)
                                                                ? "float"
                                                                : (parameterVal[i]->type == ccsp_double)
                                                                    ? "double"
                                                                    : (parameterVal[i]->type == ccsp_byte)
                                                                        ? "byte"
                                                                        : (parameterVal[i]->type == ccsp_none)
                                                                            ? "none"
                                                                            : "unknown",
                            parameterVal[i]->parameterValue
                        );
                }

                runSteps = __LINE__;
                
                free_parameterValStruct_t (bus_handle, size, parameterVal);

                runSteps = __LINE__;

                parameterVal = NULL;
            }

        }
        else if (strcmp(pInputCmd->command, "retv") == 0)
        {
            runSteps = __LINE__;

            int paramCount = 1;
            parameterNames[0] = pInputCmd->result[0].pathname;

            ret = CcspBaseIf_getParameterValues(
                bus_handle,
                dst_componentid,
                dst_pathname,
                parameterNames,
                paramCount,
                &size,
                &parameterVal
            );

            runSteps = __LINE__;

            printf("%s\n", ((ret == CCSP_SUCCESS) && (size > 0)) ? parameterVal[0]->parameterValue : "");

            free_parameterValStruct_t (bus_handle, size, parameterVal);

            parameterVal = NULL;
        }
        else if ( strncmp( pInputCmd->command, "sgetvalues"     , 4 ) == 0 )
        {
            i = 0;
    		
            while ( pInputCmd->result[i].pathname )
            {
                parameterNames[i] = pInputCmd->result[i].pathname;
                i++;
            }
            gettimeofday(&start, NULL);
            ret = CcspBaseIf_getParameterValues(
                bus_handle,
                dst_componentid,
                dst_pathname,
                parameterNames,
                i,
                &size ,
                &parameterVal
            );

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

            gettimeofday(&end, NULL);
			seconds  = end.tv_sec  - start.tv_sec;
		    useconds = end.tv_usec - start.tv_usec;
		
		    mtime_onetime = ((seconds) * 1000000 + useconds);		
		    
            //printf("%d, size:%d.\n", __LINE__,size);
            if(ret == CCSP_SUCCESS  && size >= 1)
            {

            /*
                for ( i = 0; i < size; i++ )
                {
                    printf
                        (
                            color_parametername"Parameter %4d name: %s""\n"
                            color_parametervalue"               type: %10s,    value: %s \n",
                            i+1,
                            parameterVal[i]->parameterName,
                            (parameterVal[i]->type == ccsp_string)
                                ? "string"
                                : (parameterVal[i]->type == ccsp_int)
                                    ? "int"
                                    : (parameterVal[i]->type == ccsp_unsignedInt)
                                        ? "uint"
                                        : (parameterVal[i]->type == ccsp_boolean)
                                            ? "bool"
                                            : (parameterVal[i]->type == ccsp_dateTime)
                                                ? "dateTime"
                                                : (parameterVal[i]->type == ccsp_base64)
                                                    ? "base64"
                                                    : (parameterVal[i]->type == ccsp_long)
                                                        ? "long"
                                                        : (parameterVal[i]->type == ccsp_unsignedLong)
                                                            ? "ulong"
                                                            : (parameterVal[i]->type == ccsp_float)
                                                                ? "float"
                                                                : (parameterVal[i]->type == ccsp_double)
                                                                    ? "double"
                                                                    : (parameterVal[i]->type == ccsp_byte)
                                                                        ? "byte"
                                                                        : (parameterVal[i]->type == ccsp_none)
                                                                            ? "none"
                                                                            : "unknown",
                            parameterVal[i]->parameterValue
                        );
                }
                */

                rtt_result = (struct param_rtt * )AnscAllocateMemory((sizeof(struct param_rtt)*(size+1)));
                
                total_mtime = 0;
                for ( i = 0; i < size; i++ )
                {
                    if(parameterVal[i]->parameterName[0])
                    {
                        char * mparameterNames[2];
                        int msize;
                        parameterValStruct_t **mparameterval;
                
                        mparameterNames[0] = parameterVal[i]->parameterName;
                        gettimeofday(&start, NULL);
                        ret = CcspBaseIf_getParameterValues(
                            bus_handle,
                            dst_componentid,
                            dst_pathname,
                            mparameterNames,
                            1,
                            &msize ,
                            &mparameterval
                        );
                        
                        if ( ret == CCSP_SUCCESS )                  //CID:72042 
                        {
                            printf(color_succeed"Execution succeed.\n");    
                        }
                        else
                        {
                            printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
                        } 
 
                        gettimeofday(&end, NULL);
                        seconds  = end.tv_sec  - start.tv_sec;
                        useconds = end.tv_usec - start.tv_usec;
                    
                        mtime = ((seconds) * 1000000 + useconds);
                        total_mtime += mtime;
                        
                        rtt_result[rtt_ct].param_name = parameterVal[i]->parameterName;
                        rtt_result[rtt_ct].rtt = mtime;
                        rtt_ct++;
                        
                        free_parameterValStruct_t (bus_handle,msize,mparameterval);
                        mparameterval = NULL;
                    }                    
                }

                /*sort and print out top 40*/
                qsort (rtt_result, rtt_ct, sizeof (struct param_rtt), param_rtt_cmp);

                printf(color_succeed"\n\nTotal time to get all in one call: %ld microseconds.", mtime_onetime);
                printf(color_succeed"\nTotal %d parameters, Average time to get one by one: %ld microseconds.", rtt_ct, total_mtime/rtt_ct);
                printf(color_succeed"\nTop %d time-consumed parameters:\n", (rtt_ct>40)?40:rtt_ct);

                for(ct = 0; ct < 40 && ct < rtt_ct; ct++)
                    printf(color_succeed"%6d microseconds -- %s \n", rtt_result[ct].rtt, rtt_result[ct].param_name);

                free_rtt_result();

                /* free memory now */
                free_parameterValStruct_t(bus_handle, size, parameterVal);

                parameterVal = NULL;
            }

        }
        else if ( strncmp( pInputCmd->command, "getattributes"     , 4 ) == 0 )
        {
            i = 0;
            while ( pInputCmd->result[i].pathname )
            {
                parameterNames[i] = pInputCmd->result[i].pathname;
                i++;
            }

            ret = CcspBaseIf_getParameterAttributes(
                bus_handle,
                dst_componentid,
                dst_pathname,
                parameterNames,
                i,
                &size ,
                &parameterAttr
            );

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

            //printf("%d, i:%d.\n", __LINE__,i);
            if(ret == CCSP_SUCCESS  && size >= 1)
            {
                for ( i = 0; i < size; i++ )
                {
                    if ( parameterAttr[i]->accessControlBitmask == 0x0 )
                        pTmp = "acs";
                    else if ( parameterAttr[i]->accessControlBitmask == 0x1 )
                        pTmp = "xmpp";
                    else if ( parameterAttr[i]->accessControlBitmask == 0x2 )
                        pTmp = "cli";
                    else if ( parameterAttr[i]->accessControlBitmask == 0x4 )
                        pTmp = "webgui";
                    else if ( parameterAttr[i]->accessControlBitmask == 0xFFFFFFFF )
                        pTmp = "anybody";

                    printf(color_parametername"Parameter %4d name: %s"color_parametervalue"\n\tnotification:%s, accessControlChanged:%s.\n", i+1, 
                            parameterAttr[i]->parameterName, 
                            parameterAttr[i]->notification?"on":"off",
                            pTmp
                          );
                    AnscFreeMemory(parameterAttr[i]->parameterName);
                    AnscFreeMemory(parameterAttr[i]);
                }
                AnscFreeMemory(parameterAttr);
                parameterAttr = NULL;
            }
        }
        else if ( strncmp( pInputCmd->command, "psmget"     , 4 ) == 0 )
        {
            psmName = pInputCmd->result[i].pathname;

            ret = PSM_Get_Record_Value2(
                    bus_handle, subsystem_prefix,
                    pInputCmd->result[i].pathname,
                    &psmType, &psmValue);

            if ( ret == CCSP_SUCCESS )
            {
                printf(color_succeed"Execution succeed.\n");
            }
            else
            {
                printf(color_error"Execution fail(error code:%s(%d)).\n", ccspReturnValueToString(ret),ret);
            }

            if(ret == CCSP_SUCCESS)
            {
                printf
                    (
                        color_parametername"Parameter %4d name: %s"color_none"\n"
                        color_parametervalue"               type: %10s,    value: %s \n",
                        i+1,
                        psmName,
                        (psmType == ccsp_string)
                            ? "string"
                            : (psmType == ccsp_int)
                                ? "int"
                                : (psmType == ccsp_unsignedInt)
                                    ? "uint"
                                    : (psmType == ccsp_boolean)
                                        ? "bool"
                                        : (psmType == ccsp_dateTime)
                                            ? "dateTime"
                                            : (psmType == ccsp_base64)
                                                ? "base64"
                                                : (psmType == ccsp_long)
                                                    ? "long"
                                                    : (psmType == ccsp_unsignedLong)
                                                        ? "ulong"
                                                        : (psmType == ccsp_float)
                                                            ? "float"
                                                            : (psmType == ccsp_double)
                                                                ? "double"
                                                                : (psmType == ccsp_byte)
                                                                    ? "byte"
                                                                    : (psmType == ccsp_none)
                                                                        ? "none"
                                                                        : "unknown",
                        psmValue
                    );
                AnscFreeMemory(psmValue);
            }

        }

        if ( bVerbose )
        {
            printf("\n"color_end);
        }
    }
    
    while( size2 && ppComponents)
    {
        if (ppComponents[size2-1]->remoteCR_dbus_path)
          AnscFreeMemory(ppComponents[size2-1]->remoteCR_dbus_path);
        
        if (ppComponents[size2-1]->remoteCR_name)
          AnscFreeMemory(ppComponents[size2-1]->remoteCR_name);
    
        if ( ppComponents[size2-1]->componentName )
          AnscFreeMemory( ppComponents[size2-1]->componentName );
    
        if ( ppComponents[size2-1]->dbusPath )
          AnscFreeMemory( ppComponents[size2-1]->dbusPath );
    
        AnscFreeMemory(ppComponents[size2-1]);
    
        size2--;
    }

    if (ppComponents)
    {
        AnscFreeMemory(ppComponents);
    }
    return 1;
}

static void print_help (void)
{
    int i = 0;
    printf(color_parametername"Commands supported:"color_end"\n");

    while(help_description[i] != NULL )
    {
        printf("\t%s\n",help_description[i]);
        i++;
    }
    return;
}

/* return >0, it's right input
    or else it's bad format  */
STATIC int analyse_cmd (char **args, PCMD_CONTENT pInputCmd)
{
    char * pCmd = NULL;
    char * pPathname = NULL;
    char * pType = NULL;
    char * pValue = NULL;
    char * pNotify = NULL;
    char * pAccesslist = NULL;
    char * pNextlevel = NULL;
    int    index   = 0;
    errno_t rc = -1;
    int ind = -1;
    int i = 0;
    int validDataType = 0;
    char datatype[][11] = {"string","int","uint","dateTime","base64","float","double","bool","byte","long","ulong"};

	//zqiu: fix crash when *args is NULL
	if ( *args == NULL )
		goto EXIT1;
    if ( strncmp( *args, "setsub", 6 ) == 0 )
    {
         if(*(args+1) != NULL)
         rc = strncpy_s(subsystem_prefix, sizeof(subsystem_prefix), *(args+1), strlen(*(args+1)));
         if(rc != EOK)
         {
             RDK_SAFECLIB_ERR("strncpy_s");
         }
         printf("subsystem_prefix %s\n", subsystem_prefix);
         return -3;
    }

    /* setvalues pathname type value commit */
    /*-----------------this is command ---------------------------*/
    pCmd = *args;
    pInputCmd->command  = pCmd;

    //now we get command, we will analyse according its name
    if ( pCmd == NULL )
    {
        goto EXIT1;
    }
    else if ( strncmp( pCmd, "setvalues", 4 ) == 0 
            || strncmp( pCmd, "psmset", 4) == 0)
    {
        if (*(args+1) == NULL)
            goto EXIT1;

        runSteps = __LINE__;

        //    "setvalues pathname type value [pathname type value] ... [commit]"
        while ( (*++args != NULL) && (index < BUSCLIENT_MAX_COUNT_SUPPORTED))
        {
            pPathname = *args++;
            if (*args == NULL)
            {
                // it's supposed that pPathname is "commit"
                if ( pPathname && 
                     (index > 0 )  &&
                     ( ( !strncmp(pPathname, "true", 4) ) ||
                       ( !strncmp(pPathname, "false", 5 )   )  
                     ) 
                   )
                {
                    pInputCmd->val1 = pPathname;
                    goto EXIT;
                }

                goto EXIT1;
            }

            runSteps = __LINE__;

            if ( pPathname ) 
            {
                pInputCmd->result[index].pathname = pPathname;
            }

            pType = *args++;
            if(!pType)/*RDKB-7436, CID-33340, null check before use*/
            {
                goto EXIT1;
            }
            for(i=0; i<11; i++)
            {
                 rc =strcmp_s(datatype[i], strlen(datatype[i]), pType, &ind);
                 if(rc != EOK)
                 {
                      RDK_SAFECLIB_ERR("strcmp_s");
                 }

                 if((rc == EOK) && (!ind))
                 {
                    validDataType=1;
                    break;
                 }
              }
              if(!validDataType)  /* not matching to any data type */
                goto EXIT1;

            runSteps = __LINE__;

            if ( pType ) 
            {
                pInputCmd->result[index].val1 = pType;
            }

            if ( *args == NULL ) { 
                goto EXIT1;
            }

            runSteps = __LINE__;
            pValue = *args;
                
            if ( pValue ) 
            {
                pInputCmd->result[index++].val2 = pValue;
            }
            else
            {
                goto EXIT1;
            }
        }
    }
    else if ( strncmp( pCmd, "getvalues", 4 ) == 0 
            || strncmp( pCmd, "sgetvalues", 4 ) == 0
            || strcmp( pCmd, "retv") == 0
            || strncmp( pCmd, "psmget", 4 ) == 0)
    {
        if ( *(args+1) == NULL )
            goto EXIT1;

        runSteps = __LINE__;

        if (strcmp(pCmd, "retv") == 0)
        {
            bVerbose = FALSE;
        }

        while ((*++args != NULL) && (index < BUSCLIENT_MAX_COUNT_SUPPORTED))
        {
            pInputCmd->result[index++].pathname = *args;
        }

        runSteps = __LINE__;
        
        if ( index == 0 )
        {
            goto EXIT1;
        }
        else
        {
            goto EXIT;
        }
    }
    else if ( ( !strncmp( pCmd, "addtable", 4 ) ) ||
              ( !strncmp( pCmd, "deltable", 4 ) ) ||
              ( !strncmp( pCmd, "psmdel", 4 ) ) )
    {
        if ( *(args+1) == NULL)
            goto EXIT1;

        pInputCmd->result[index++].pathname = *++args;
        
        goto EXIT;       //CID:53643  Removed Deadcode

    }
    else if ( strncmp( pCmd, "getnames", 4 ) == 0 )
    {
        if (*(args+1) == NULL)
            goto EXIT1;

        pInputCmd->result[index++].pathname = *++args;
        
        if(*++args != NULL)
        {
            pNextlevel = *args;
            if ( pNextlevel )
            {
                if ( strncmp(pNextlevel, "true", 4) &&
                     strncmp(pNextlevel, "false", 5)  )
                {
                    goto EXIT1;
                }

                pInputCmd->val1 = pNextlevel;
            }
        }

        goto EXIT;            //CID:53643 Removed dead code
        
    }
    else if ( strncmp( pCmd, "getattributes", 4 ) == 0 )          
    {
        if (*(args+1) == NULL)
            goto EXIT1;

        while ((*++args != NULL) && (index < BUSCLIENT_MAX_COUNT_SUPPORTED))
        {
            pInputCmd->result[index++].pathname = *args;
        }
        
        if ( index == 0 )
        {
            goto EXIT1;
        }
        else
        {
            goto EXIT;
        }

    }
    else if ( strncmp( pCmd, "setattributes", 4 ) == 0 )
    {
        if (*(args+1) == NULL)
            goto EXIT1;


        //    "setattributes pathname notify accesslist [ pathname notify accesslist ] ..."
        index = 0;
        while ((*++args != NULL) && (index < BUSCLIENT_MAX_COUNT_SUPPORTED))
        {
            pPathname = *args++;
            if (*args == NULL)
            {
                goto EXIT1;
            }

            pInputCmd->result[index].pathname = pPathname;

            pNotify = *args++;

            if ( pNotify ) 
            {
                if ( !strncmp( pNotify, "unchange", 8) )
                {
                    pInputCmd->result[index].val1 = NULL;
                }
                else if ( strncmp( pNotify, "active", 6) &&
                          strncmp( pNotify, "passive", 7) &&
                          strncmp( pNotify, "off", 3)
                        )
                {
                    goto EXIT1;
                }
                else
                {
                    pInputCmd->result[index].val1 = pNotify;
                }
            }

            if (*args == NULL)
            {
                goto EXIT1;
            }

            pAccesslist = *args;
            if ( pAccesslist ) 
            {
                ////acs/xmpp/cli/webgui/anybody
                if ( !strncmp( pAccesslist, "unchange", 8) )                    
                    pInputCmd->result[index++].val2 = NULL;
                else if (  strncmp( pAccesslist, "acs", 3) &&
                           strncmp( pAccesslist, "xmpp", 4)&&
                           strncmp( pAccesslist, "cli", 3) &&
                           strncmp( pAccesslist, "webgui", 6) &&
                           strncmp( pAccesslist, "anybody", 7)
                        )
                {
                    goto EXIT1;
                }
                else
                {
                    pInputCmd->result[index++].val2 = pAccesslist;
                }

            }
            else
            {
                goto EXIT1;
            }
        }
    }
    else if ( strncmp( pCmd, "setcommit" , 4) == 0 )
    {
        /* we need set commit to all componets which have data models*/
        pInputCmd->result[index].pathname = DM_ROOTNAME;

        goto EXIT;
    }
    else if (( strncmp( pCmd, "exit" , 4) == 0 ) ||
             ( strncmp( pCmd, "quit" , 4) == 0 ) )
    {
        return -2;
    }
    else if ( ( strncmp( pCmd, "?", 1) == 0 ) ||
              ( strncmp( pCmd, "help", 4) == 0 ))
    {
        return -1;
    }
    else
    {
        goto EXIT1;
    }

EXIT:
    return 1;

EXIT1:
    return 0;
}

STATIC int analyse_interactive_cmd (char *inputLine, char **args)
{
    int index = 0;
    int quote_flag = 0;
    int len = 0;
    int arg_num = 0;

    if(args == NULL)
        return 0;

    len = strlen(inputLine);

    if ((len > 0) && (inputLine[len - 1] == '\n'))
    {
        inputLine[len - 1] = 0;
        len--;
    }

    while((index < len) && (arg_num < MAX_CMD_ARG_NUM))
    {
        if(!quote_flag)
        {
            while((index < len) && 
                    ((inputLine[index] == ' ') || (inputLine[index] == '\t') || (inputLine[index] == '\0')))
                inputLine[index++] = '\0';
                
            if(index >= len)
                break;

            if(inputLine[index] == '\"'){ /* " start*/
                *args++ = &inputLine[index+1];
                arg_num++;
                quote_flag = 1;
            }
            else
            {
                if((index == 0) || (inputLine[index-1] == '\0')){
                    *args++ = &inputLine[index];
                    arg_num++;
                }
            }
        }
        else
        {
            if(inputLine[index] == '\"'){ /* " end*/
                inputLine[index] = '\0';
                quote_flag = 0;
            }
        }
        index++; /*next char*/
    }
    return 1;
}

#ifndef INCLUDE_BREAKPAD
STATIC void signal_interrupt (int i)
{
    UNREFERENCED_PARAMETER(i);
    return;
}
#endif

#define  PRINT_HELP(pName)                                                                                              \
            printf(                                                                                                     \
                color_parametername"Usage:"color_parametervalue"\n"                                                                        \
                "\t#%s <eRT|eMG|eEP|simu> [config]\n"                                                                            \
                "\t#%s <eRT|eMG|eEP|simu> [config] command p1 p2 p3 ....\n\n"                                                    \
                color_parametername"Description:"color_parametervalue"\n"                                                                  \
                "\tThe first usage provides a interactive interface to run commands.\n"                                 \
                "\tThe second usage provides one direct one-line method to run commands.\n"                             \
                "\tConfig should point to a file including one kinde of daemon address for example:\n"                  \
                "\t\t\ttcp:host=10.74.52.92,port=54321\n"                                                               \
                "\tIf have not config, it supposes that a file(msg_daemon.cfg) exists here("CCSP_MSG_BUS_CFG")\n\n",    \
                pName, pName);                                                                                          \
                                                                                                                        \
            print_help();



#ifdef UNIT_TEST_DOCKER_SUPPORT
int dmcli_main(int argc, char *argv[])
#else
int main (int argc, char *argv[])
#endif
{
//    void *bus_handle2;
    int             ret = 0;
    char *          pCfg = CCSP_MSG_BUS_CFG;
    int             i = 0;
    char            inputLine[2048];
    CMD_CONTENT     inputCmd;
    FILE           *fp               = NULL;
    int             nextIndex        = 0;
    int             cmdFormat        = 0;
    BOOL            bFirstError      = true;
    int             idx              = 0;
    BOOL            bInteractive     = FALSE;
    char            **args           = NULL;
    errno_t         rc               = -1;
    int             ind              = -1;
    int             validArg         = 0;
    char            param[][5]       = {"eRT","eMG","eEP","simu"};

    /*
       Set stdout to unbuffered mode as this app needs to generate terminal
       output which is not always line based (e.g. the "dmcli>" prompt) and
       therefore does not always work as expected if stdout is left in the
       default line buffered mode.
    */
    setbuf(stdout, NULL);

    if ( argc < 2 )
    {
        PRINT_HELP(argv[0]);
        
        return 0;
    }
    else
    {
        if ( !strcmp( argv[1], "help") ||
             !strcmp( argv[1], "?") ||
             !strcmp( argv[1], "-h") ||
             !strcmp( argv[1], "-help") ||
             !strcmp( argv[1], "--help")
            )
        {
            PRINT_HELP(argv[0]);
            
            return 0;
        }
    }

    inputLine[0] = 0;

    // handle parameters
    //if ( argc <= 4 )
        //signal(SIGINT, signal_interrupt);

    runSteps = __LINE__;
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
    enable_ccsp_exception_handlers();
#endif

    idx = 1;

    runSteps = __LINE__;

    if ( TRUE )    
    {
        int                         iLen    = 0;

        for (i=0; i<4; i++)
        {
           rc = strcmp_s(param[i],strlen(param[i]), argv[idx],&ind);
           if(rc != EOK)
           {
               RDK_SAFECLIB_ERR("strcmp_s");
           }

           if((rc == EOK) && (!ind))
           {
               validArg = 1;
               break;
           }
        }
        if(!validArg)  /* not matching to Valid Arguments */
        {
            PRINT_HELP(argv[0]);
            
            return 0;
        }
        rc = strcmp_s("simu",  strlen("simu"),  argv[idx], &ind );
        if(rc != EOK)
        {
            RDK_SAFECLIB_ERR("strcmp_s");
        }
        if((rc == EOK) && (ind != 0))
        {
            rc = strncpy_s(dst_pathname_cr,  sizeof(dst_pathname_cr), argv[idx], strlen(argv[idx]));
            if(rc != EOK)
            {
                 RDK_SAFECLIB_ERR("strncpy_s");
            }

            iLen = strlen(dst_pathname_cr);

            if ( dst_pathname_cr[iLen - 1] != '.' )
            {
                dst_pathname_cr[iLen]       = '.';
                dst_pathname_cr[iLen + 1]   = 0;
            }
            
            rc = strncpy_s(subsystem_prefix,  sizeof(subsystem_prefix), dst_pathname_cr, strlen(dst_pathname_cr));
            if(rc != EOK)
            {
                RDK_SAFECLIB_ERR("strncpy_s");
            }
        }
        rc = strcat_s(dst_pathname_cr,  sizeof(dst_pathname_cr), CCSP_DBUS_INTERFACE_CR);
        if(rc != EOK)
        {
            RDK_SAFECLIB_ERR("strcat_s");
        }
    }

    idx += 1;

    runSteps = __LINE__;

    if ( argc >= 3 )
    {
       if ( (fp = fopen(argv[idx], "r")) )
       {
            pCfg = argv[idx];
            nextIndex = idx + 1;
           
            fclose(fp);

            runSteps = __LINE__;

            if ( nextIndex == argc)
            {           
                bInteractive = TRUE;
            }
       }
       else
       {
           nextIndex = idx;
       }
    }
    else
    {
        printf(color_succeed"CR component name is: %s\n", dst_pathname_cr);
        bInteractive = TRUE;
    }

    runSteps = __LINE__;

    /* If parameters like this, it will be internal cmd format
                cmd
                cmd cfg

        */
    if ( bInteractive )
    {
        cmdFormat = 1;
        printf(color_succeed"dmcli>");

        bFirstError = false;
        
        runSteps = __LINE__;

        if (fgets(inputLine, sizeof(inputLine), stdin) == NULL)
        {
            printf("%s,%d: error in reading inputLine\n", __FUNCTION__,__LINE__);
            return -1;
        }
    }
    else
    {
        cmdFormat = 2;

        runSteps = __LINE__;        
    }
    // we begin the initiation of dbus
    ret = CCSP_Message_Bus_Init("ccsp.busclient", pCfg, &bus_handle, (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
    if ( ret == -1 )
    {
        printf(color_end);
        return -1;
    }

    runSteps = __LINE__;

    while( 1 )
    {
        if ( strlen(inputLine) > 1022 )
        {
            printf(color_error"too long input. Just support 1022 characters.\n");
        }
        else
        {
            runSteps = __LINE__;

            if(cmdFormat == 1)
            {
                args = (char **)calloc(MAX_CMD_ARG_NUM, sizeof(char *));
                /* Coverity Issue Fix - CID:53037 : Forward NULL*/
                if( args == NULL)
                {
                    printf("%s-%d:Coverity Error occured as Forward NULL in args\n",__FUNCTION__,__LINE__);
                    return -1;
                }
                analyse_interactive_cmd(inputLine, args);
            }
            else
                args = &argv[nextIndex];
		
            memset(&inputCmd, 0, sizeof(inputCmd));

            ret = analyse_cmd(args, &inputCmd);

            runSteps = __LINE__;

            if ( ret > 0 )
            {
                /*
                printf("%s\n", inputCmd.command);
                if ( inputCmd.val1 ) printf("\t%s\n", inputCmd.val1);
                i = 0;
                while( inputCmd.result[i].pathname )
                {
                    printf("\t%s %s %s\n", inputCmd.result[i].pathname, inputCmd.result[i].val1, inputCmd.result[i].val2);
                    i++;
                }
                          */
                if ( bVerbose )
                {
                    printf(color_succeed"CR component name is: %s\n", dst_pathname_cr);
                }
                apply_cmd(&inputCmd);
            }
            else if ( ret == -1 )
            {
                print_help();
            }
            else if ( ret == -2 )
            {
                printf(color_end);
                /*Coverity Fix CID:67422 RESOURCE_LEAK */
                free(args);
                args = NULL;
                return 0;
            }
            else if ( ret == -3 )
            {
            }
            else
            {
                if (inputCmd.command != NULL )
                    printf(color_error"Syntax error. see help.\n"); 

                /* When the first input is error, exit immediately */
                if ( bFirstError  )
                {
                    printf(color_end);
                    return 1;
                }
            }
        }

        bFirstError = false;
        
        if ( cmdFormat == 1 )
        {
            free(args);
            args = NULL;
        }

        if ( cmdFormat == 2 )
            break;

        printf(color_succeed"dmcli>");

        if (fgets(inputLine, sizeof(inputLine), stdin) == NULL)
        {
            printf("%s,%d: error in reading inputLine\n", __FUNCTION__,__LINE__);
            return -1;
        }
    }

    runSteps = __LINE__;
#ifndef INCLUDE_BREAKPAD 
    signal(SIGSEGV, signal_interrupt);
#endif
    // exit     
    CCSP_Message_Bus_Exit(bus_handle);
    //printf("count %d %d  \n", count, errcount );

    runSteps = __LINE__;
    
    if ( bVerbose )
    {
        printf(color_end);
    }
    return 1;

}
#endif
