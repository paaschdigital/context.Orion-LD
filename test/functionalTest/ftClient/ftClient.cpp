/*
*
* Copyright 2024 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
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
* Author: Ken Zangelin
*/
#include <unistd.h>                    // >sleep
#include <strings.h>                   // bzero
#include <stdlib.h>                    // exit

extern "C"
{
#include "ktrace/kTrace.h"             // trace messages - ktrace library
#include "ktrace/ktGlobals.h"          // globals
#include "kargs/kargs.h"               // argument parsing - kargs library
}

#include "mhd/mhdStart.h"              // mhdStart - initialize MHD and start receiving REST requests



// -----------------------------------------------------------------------------
//
// Global vars
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
  { "--port",             "-p",    KaUShort,  &ldPort,            KaOpt, _i 1026,   _i 1024,  _i 65535, "port for NGSI-LD requests"                         },
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
// FTCLIENT_VERSION -
//
#define FTCLIENT_VERSION "0.0.1"



// -----------------------------------------------------------------------------
//
// main -
//
int main(int argC, char* argV[])
{
  KArgsStatus ks;
  const char* progName = "ftClient";

  ks = kargsInit(progName, kargs, "FTCLIENT");
  if (ks != KasOk)
  {
    fprintf(stderr, "error reading CLI parameters\n");
    exit(1);
  }

  ks = kargsParse(argC, argV);
  if (ks != KasOk)
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

  KT_V("kaBuiltinDebug: %d", kaBuiltinDebug);
  KT_V("mhdMaxConnections: %d", mhdMaxConnections);
  KT_V("Serving NGSI-LD requests on port: %d", ldPort);
  KT_D("%s version:                   %s", progName, FTCLIENT_VERSION);
  
  if (mhdStart(ldPort, 4) == false)
    KT_X(1, "Unable to start REST interface on port %d (NGSI-LD)", ldPort);

  while (1)
  {
    sleep(1);
  }
} 
