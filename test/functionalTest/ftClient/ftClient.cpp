/*
*
* Copyright 2024 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin, David Campo, Luis Arturo Frigolet
*/
#include <unistd.h>                                         // sleep
#include <strings.h>                                        // bzero
#include <stdlib.h>                                         // exit, malloc, calloc, free

#include <string>                                           // std::string

#include "fastdds/dds/log/FileConsumer.hpp"                 // DDS logging

extern "C"
{
#include "ktrace/kTrace.h"                                  // trace messages - ktrace library
#include "ktrace/ktGlobals.h"                               // globals for KT library
#include "kargs/kargs.h"                                    // argument parsing - kargs library
#include "kalloc/kaInit.h"                                  // kaInit
#include "kalloc/kaBufferInit.h"                            // kaBufferInit
#include "kalloc/kaBufferReset.h"                           // kaBufferReset
#include "kalloc/kaAlloc.h"                                 // kaAlloc
#include "kjson/kjBufferCreate.h"                           // kjBufferCreate
#include "kjson/KjNode.h"                                   // KjNode
#include "kjson/kjBuilder.h"                                // kjObject, kjArray, kjString, kjChildAdd, ...
#include "kjson/kjFree.h"                                   // kjFree
#include "kjson/kjRender.h"                                 // kjRender
#include "kjson/kjRenderSize.h"                             // kjRenderSize
#include "kjson/kjParse.h"                                  // kjParse
#include "kjson/kjLookup.h"                                 // kjLookup
}

#include "types/Verb.h"                                     // HTTP Verbs
#include "mhd/mhdStart.h"                                   // mhdStart - initialize MHD and start receiving REST requests
#include "common/traceLevels.h"                             // Trace levels for ktrace
#include "dds/ddsPublish.h"                                 // ddsPublish
#include "dds/ddsSubscribe.h"                               // ddsSubscribe

#include "ftClient/getDump.h"                               // getDump
#include "ftClient/deleteDump.h"                            // deleteDump
#include "ftClient/die.h"                                   // die



using eprosima::fastdds::dds;



// -----------------------------------------------------------------------------
//
// FTCLIENT_VERSION -
//
#define FTCLIENT_VERSION "0.0.1"



// -----------------------------------------------------------------------------
//
// FtService -
//
typedef KjNode* (*FtTreat)(int* statusCodeP);
typedef struct FtService
{
  Verb             verb;
  const char*      url;
  FtTreat          treatP;
} FtService;



// -----------------------------------------------------------------------------
//
// CLI Param variables
//
char*          traceLevels;
char*          logDir        = NULL;
char*          logLevel;
KBool          logToScreen;
KBool          fixme;
unsigned short ldPort;
char*          httpsKey;
char*          httpsCertificate;
unsigned int   mhdPoolSize;
unsigned int   mhdMemoryLimit;
unsigned int   mhdTimeout;
unsigned int   mhdMaxConnections;
bool           distributed;



// -----------------------------------------------------------------------------
//
// kargs - vector of CLI parameters
//
KArg kargs[] =
{
  //
  // Potential builtins
  //
  { "--trace",            "-t",    KaString,  &traceLevels,       KaOpt, 0,         KA_NL,    KA_NL,    "trace levels (csv of levels/ranges)"               },
  { "--logDir",           "-ld",   KaString,  &logDir,            KaOpt, _i "/tmp", KA_NL,    KA_NL,    "log file directory"                                },
  { "--logLevel",         "-ll",   KaString,  &logLevel,          KaOpt, 0,         KA_NL,    KA_NL,    "log level (ERR|WARN|INFO|INFO|VERBOSE|TRACE|DEBUG" },
  { "--logToScreen",      "-ls",   KaBool,    &logToScreen,       KaOpt, KFALSE,    KA_NL,    KA_NL,    "log to screen"                                     },
  { "--fixme",            "-fix",  KaBool,    &fixme,             KaOpt, KFALSE,    KA_NL,    KA_NL,    "FIXME messages"                                    },

  //
  // Broker options
  //
  { "--port",             "-p",    KaUShort,  &ldPort,            KaOpt, _i 7701,   _i 1027,  _i 65535, "TCP port for incoming requests"                    },
  { "--httpsKey",         "-k",    KaString,  &httpsKey,          KaOpt, NULL,      KA_NL,    KA_NL,    "https key file"                                    },
  { "--httpsCertificate", "-c",    KaString,  &httpsCertificate,  KaOpt, NULL,      KA_NL,    KA_NL,    "https certificate file"                            },

  { "--mhdPoolSize",      "-mps",  KaUInt,    &mhdPoolSize,       KaOpt, _i 8,      _i 0,     _i 1024,  "MHD request thread pool size"                      },
  { "--mhdMemoryLimit",   "-mlim", KaUInt,    &mhdMemoryLimit,    KaOpt, _i 64,     _i 0,     _i 1024,  "MHD memory limit (in kb)"                          },
  { "--mhdTimeout",       "-mtmo", KaUInt,    &mhdTimeout,        KaOpt, _i 2000,   _i 0,     KA_NL,    "MHD connection timeout (in milliseconds)"          },
  { "--mhdConnections",   "-mcon", KaUInt,    &mhdMaxConnections, KaOpt, _i 512,    _i 1,     KA_NL,    "Max number of MHD connections"                     },

  { "--distOps",          "-dops", KaBool,    &distributed,       KaOpt, KFALSE,    KA_NL,    KA_NL,    "support for distributed operations"                },

  KARGS_END
};



// -----------------------------------------------------------------------------
//
// NHD variables
//
__thread unsigned int   payloadBodySize = 0;
__thread unsigned int   contentLength   = 0;
__thread char*          payloadBody     = NULL;
__thread KjNode*        payloadTree     = NULL;
__thread Verb           verb            = HTTP_NOVERB;
__thread char*          urlPath         = NULL;
__thread char*          responseText    = NULL;



// -----------------------------------------------------------------------------
//
// K-Lib variables
//
__thread char           kallocBuffer[2 * 1024];
__thread KAlloc         kalloc;
__thread Kjson          kjson;
__thread Kjson*         kjsonP;



// -----------------------------------------------------------------------------
//
// Other global variables
//
KjNode*                 dumpArray = NULL;
__thread KjNode*        httpHeaders = NULL;
__thread KjNode*        uriParams   = NULL;



// -----------------------------------------------------------------------------
//
// postDdsSub -
//
KjNode* postDdsSub(int* statusCodeP)
{
  KjNode*      ddsTopicTypeNodeP  = (uriParams         != NULL)? kjLookup(uriParams, "ddsTopicType") : NULL;
  const char*  ddsTopicType       = (ddsTopicTypeNodeP != NULL)? ddsTopicTypeNodeP->value.s : NULL;
  KjNode*      ddsTopicNameNodeP  = (uriParams         != NULL)? kjLookup(uriParams, "ddsTopicName") : NULL;
  const char*  ddsTopicName       = (ddsTopicNameNodeP != NULL)? ddsTopicNameNodeP->value.s : NULL;

  if (ddsTopicName == NULL || ddsTopicType == NULL)
  {
    KT_E("Both Name and Type of the topic should not be null");
    return NULL;
  }

  KT_V("Creating DDS Subcription for the topic %s:%s", ddsTopicType, ddsTopicName);
  ddsSubscribe(ddsTopicType, ddsTopicName);

  return NULL;
}



// -----------------------------------------------------------------------------
//
// postDdsPub -
//
KjNode* postDdsPub(int* statusCodeP)
{
  KjNode*      ddsTopicTypeNodeP  = (uriParams         != NULL)? kjLookup(uriParams, "ddsTopicType") : NULL;
  const char*  ddsTopicType       = (ddsTopicTypeNodeP != NULL)? ddsTopicTypeNodeP->value.s : NULL;
  KjNode*      ddsTopicNameNodeP  = (uriParams         != NULL)? kjLookup(uriParams, "ddsTopicName") : NULL;
  const char*  ddsTopicName       = (ddsTopicNameNodeP != NULL)? ddsTopicNameNodeP->value.s : NULL;

  if (ddsTopicName == NULL || ddsTopicType == NULL)
  {
    KT_E("Both Name and Type of the topic should not be null");
    return NULL;
  }

  KT_V("Publishing on DDS for the topic %s:%s", ddsTopicType, ddsTopicName);
  ddsPublish(ddsTopicType, ddsTopicName, payloadTree);

  return NULL;
}



// -----------------------------------------------------------------------------
//
// serviceV -
//
FtService serviceV[] =
{
  { HTTP_GET,      "/dump",      getDump    },
  { HTTP_DELETE,   "/dump",      deleteDump },
  { HTTP_GET,      "/die",       die        },
  { HTTP_POST,     "/dds/sub",   postDdsSub },
  { HTTP_POST,     "/dds/pub",   postDdsPub },
  { HTTP_NOVERB,   NULL,         NULL       }
};



// -----------------------------------------------------------------------------
//
// headerReceive -
//
static MHD_Result headerReceive(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  KT_V("Got an HTTP Header: '%s': '%s'", key, value);
  if (strcasecmp(key, "Content-Length") == 0)
  {
    contentLength = atoi(value);
    KT_V("contentLength: %d", contentLength);
  }

  if (httpHeaders == NULL)
    httpHeaders = kjObject(NULL, "headers");

  KjNode* headerP = kjString(NULL, key, value);
  kjChildAdd(httpHeaders, headerP);

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// uriParamReceive -
//
MHD_Result uriParamReceive(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  KT_V("Got a URL Parameter: '%s': '%s'", key, value);

  if (uriParams == NULL)
    uriParams = kjObject(NULL, "params");

  KjNode* paramP = kjString(NULL, key, value);
  kjChildAdd(uriParams, paramP);

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// mhdRequestInit -
//
static MHD_Result mhdRequestInit(MHD_Connection* connection, const char* url, const char* method, const char* version, void** con_cls)
{
  bzero(&kjson,        sizeof(kjson));
  bzero(&kalloc,       sizeof(kalloc));
  bzero(&kallocBuffer, sizeof(kallocBuffer));

  kaBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 32 * 1024, NULL, "Global KAlloc buffer");
  kjsonP = kjBufferCreate(&kjson, &kalloc);
  KT_T(StRequest, "kjsonP at %p", kjsonP);

  payloadBodySize  = 0;
  contentLength    = 0;
  payloadBody      = NULL;
  payloadTree      = NULL;
  urlPath          = (char*) url;
  verb             = verbFromString(method);

  MHD_get_connection_values(connection, MHD_HEADER_KIND,       headerReceive,   NULL);
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriParamReceive, NULL);

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// mhdRequestBodyRead -
//
MHD_Result mhdRequestBodyRead(size_t* upload_data_size, const char* upload_data)
{
  size_t  dataLen = *upload_data_size;

  //
  // First call with payload - use the pre-allocated  "orionldState.preallocReqBuf" if possible,
  // otherwise allocate a bigger buffer
  //
  // FIXME P1: This could be done in "Part I" instead, saving an "if" for each "Part II" call
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  if (payloadBodySize == 0)  // First call with payload
  {
    KT_V("Allocating %d bytes for payload body", contentLength + 1);
    payloadBody = (char*) malloc(contentLength + 1);
    if (payloadBody == NULL)
    {
      KT_E("Out of memory!!!");
      return MHD_NO;
    }
  }

  // Copy the chunk
  KT_V("Copying to payloadBody[%d], %d bytes", payloadBodySize, dataLen);
  memcpy(&payloadBody[payloadBodySize], upload_data, dataLen);

  // Add to the size of the accumulated read buffer
  payloadBodySize += contentLength + 1;

  // Zero-terminate the payloadBody
  payloadBody[payloadBodySize - 1] = 0;

  // Acknowledge the data and return
  *upload_data_size = 0;

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// mhdRequestTreat -
//
char* mhdRequestTreat(int* statusCodeP)
{
  KT_T(StRequest, "In mhdRequestTreat");
  int ix   = 0;

  // Parse the incoming payload body, if present
  if (payloadBody != NULL)
  {
    KT_T(StRequest, "Parsing incoming payload body '%s'", payloadBody);
    payloadTree = kjParse(kjsonP, payloadBody);
    KT_T(StRequest, "payloadTree at %p ", payloadTree);
  }

  // Lookup the service routine and execute it
  while (serviceV[ix].treatP != NULL)
  {
    if ((verb == serviceV[ix].verb) && (strcmp(urlPath, serviceV[ix].url) == 0))
    {
      KT_T(StRequest, "Found the service");
      KjNode* responseTree = serviceV[ix].treatP(statusCodeP);

      if (responseTree == NULL)
        return responseText;

      int     bufSize      = kjRenderSize(kjsonP, responseTree) + 1024;
      char*   buf          = kaAlloc(kjsonP->kallocP, bufSize);

      kjRender(kjsonP, responseTree, buf, bufSize);
      return buf;
    }

    ++ix;
  }

  // Service not found - accumulate
  KjNode* dump    = kjObject(NULL, "item");  // No name as part of array
  KjNode* verbP   = kjString(NULL, "verb", verbToString(verb));
  KjNode* path    = kjString(NULL, "url",  urlPath);

  kjChildAdd(dump, verbP);
  kjChildAdd(dump, path);

  if (uriParams != NULL)
    kjChildAdd(dump, uriParams);
  if (httpHeaders != NULL)
    kjChildAdd(dump, httpHeaders);

  if (dumpArray == NULL)
    dumpArray = kjArray(NULL, "dumpArray");

  if (payloadTree != NULL)
  {
    payloadTree->name = (char*) "body";
    kjChildAdd(dump, payloadTree);
  }

  kjChildAdd(dumpArray, dump);

  *statusCodeP = 200;
  return (char*) "";
}



// -----------------------------------------------------------------------------
//
// mhdRequestEnded -
//
void mhdRequestEnded
(
  void*                       cls,
  MHD_Connection*             connection,
  void**                      con_cls,
  MHD_RequestTerminationCode  toe
)
{
  KT_T(StRequest, "Request ended");

  payloadBodySize  = 0;
  contentLength    = 0;
  payloadBody      = NULL;
  payloadTree      = NULL;
  verb             = HTTP_NOVERB;
  urlPath          = NULL;
  httpHeaders      = NULL;
  uriParams        = NULL;
  responseText     = NULL;

  // Reset kjson/kalloc
  // kaBufferReset(&kalloc, KFALSE);
  // free(kjsonP);
}



// -----------------------------------------------------------------------------
//
// klibLogBuffer -
//
char klibLogBuffer[4 * 1024];



// -----------------------------------------------------------------------------
//
// klibLogFunction -
//
static void klibLogFunction
(
  int          severity,              // 1: Error, 2: Warning, 3: Info, 4: Verbose, 5: Trace
  int          level,                 // Trace level || Error code || Info Code
  const char*  fileName,
  int          lineNo,
  const char*  functionName,
  const char*  format,
  ...
)
{
  va_list  args;

  /* "Parse" the variable arguments */
  va_start(args, format);

  /* Print message to variable */
  vsnprintf(klibLogBuffer, sizeof(klibLogBuffer), format, args);
  va_end(args);

  // LM_K(("Got a lib log message, severity: %d: %s", severity, libLogBuffer));

  if (severity == 1)
    ktOut(fileName, lineNo, functionName, 'E', 0, "klib: %s", klibLogBuffer);
  else if (severity == 2)
    ktOut(fileName, lineNo, functionName, 'W', 0, "klib: %s", klibLogBuffer);
  else if (severity == 3)
    ktOut(fileName, lineNo, functionName, 'I', 0, "klib: %s", klibLogBuffer);
  else if (severity == 4)
    ktOut(fileName, lineNo, functionName, 'V', 0, "klib: %s", klibLogBuffer);
  else if (severity == 5)
    ktOut(fileName, lineNo, functionName, 'T', level + 1000, "klib: %s", klibLogBuffer);
}



// -----------------------------------------------------------------------------
//
// main -
//
int main(int argC, char* argV[])
{
  KArgsStatus ks;
  const char* progName = "ftClient";

  ks = kargsInit(progName, kargs, "FTCLIENT");
  if (ks != KargsOk)
  {
    fprintf(stderr, "error reading CLI parameters\n");
    exit(1);
  }

  ks = kargsParse(argC, argV);
  if (ks != KargsOk)
  {
    kargsUsage();
    exit(1);
  }

  int kt = ktInit(progName, logDir, logToScreen, logLevel, traceLevels, kaBuiltinVerbose, kaBuiltinDebug, fixme);

  if (kt != 0)
  {
    fprintf(stderr, "Error initializing logging library\n");
    exit(1);
  }

  kaInit(klibLogFunction);


  //
  // Initialize the KJSON library
  // This sets up the global kjson instance with preallocated kalloc buffer
  //
  // kaBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 32 * 1024, NULL, "Global KAlloc buffer");
  // kjsonP = kjBufferCreate(&kjson, &kalloc);


  //
  // Traces for eProsima FastDDS libraries
  //
  // EPROS:
  //   For now, all traces from the eProsima libraries are kept in a separate logfile.
  //   This needs to change. It's not acceptable that a library demands to have its own log file.
  //   Orion-LD uses perhaps 25 different libraries. Should we have 25 different logfiles?
  //   No, we should not.
  //
  //   We need a new type for "Log Consumer", one that accepts a callback function pointer for the application
  //   that uses the eProsima libraries to do whatever needs to be done with the log data.
  //   Proposed definition of the callback function:
  //
  //     void ddsLog(const char* file, const char* function, int lineNo, int logLevel, int traceLevel, const char* logMsg);
  //
  std::string ddsLogFile = std::string(logDir) + "/ftClient_dds.log";
  std::unique_ptr<FileConsumer> append_file_consumer(new FileConsumer(ddsLogFile, true));
  Log::RegisterConsumer(std::move(append_file_consumer));


  //
  // Perhaps the most important feature of ftClient is the ability to report on received notifications.
  // For this purpose, the dumpArray contains all payloads received as notifications.
  //
  // NOTE: not only notifications, also forwarded requests, or just about anything received out of the defined API it supports for
  //       configuration.
  //
  dumpArray = kjArray(NULL, "dumpArray");

  KT_V("Serving requests on port %d", ldPort);
  KT_D("%s version:                   %s", progName, FTCLIENT_VERSION);
  if (mhdStart(ldPort, 4, mhdRequestInit, mhdRequestBodyRead, mhdRequestTreat, mhdRequestEnded) == false)
    KT_X(1, "Unable to start REST interface on port %d", ldPort);

  while (1)
  {
    sleep(1);
  }
}
