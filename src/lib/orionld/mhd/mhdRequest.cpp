/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include <stdlib.h>
#include <microhttpd.h>                                          // MHD

extern "C"
{
#include "ktrace/kTrace.h"                                       // KT_*
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjChildAdd, ...
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/traceLevels.h"                          // Tl*
#include "orionld/mhd/mhdReply.h"                                // mhdReply
#include "orionld/mhd/mhdRequest.h"                              // Own interface


__thread int mhdCalls = 0;

unsigned int payloadSize   = 0;
unsigned int contentLength = 0;
char*        payload       = NULL;


void ftDump(void)
{
  printf("Dumping...\n");
}

char* ftTreat(const char* url, const char* method)
{
  if (payload != NULL)
    KT_V("Payload body: %s", payload);
  
  if (strcmp(url, "/dump") == 0)
    ftDump();
  else
    KT_E("Unknown URL: %s", url);

  return (char*) "xyztmp";
}



static MHD_Result mhdPayloadRead
(
  size_t*          upload_data_size,
  const char*      upload_data
)
{
  size_t  dataLen = *upload_data_size;

  //
  // First call with payload - use the pre-allocated  "orionldState.preallocReqBuf" if possible,
  // otherwise allocate a bigger buffer
  //
  // FIXME P1: This could be done in "Part I" instead, saving an "if" for each "Part II" call
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  if (payloadSize == 0)  // First call with payload
  {
    payload = (char*) malloc(contentLength + 1);
    if (payload == NULL)
      {
        KT_E("Out of memory!!!");
        return MHD_NO;
      }
  }

  // Copy the chunk
  memcpy(&payload[payloadSize], upload_data, dataLen);

  // Add to the size of the accumulated read buffer
  payloadSize += dataLen;

  // Zero-terminate the payload
  payload[payloadSize] = 0;

  // Acknowledge the data and return
  *upload_data_size = 0;

  return MHD_YES;
}



static MHD_Result headerReceive(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  KT_V("Got an HTTP Header: '%s': '%s'", key, value);
  if (strcasecmp(key, "Content-Length") == 0)
    contentLength = atoi(value);

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// mhdRequestInit -
//
MHD_Result mhdRequestInit
(
  MHD_Connection*  connection,
  const char*      url,
  const char*      method,
  const char*      version,
  void**           con_cls
)
{
  if (payload != NULL)
    free(payload);
  
  MHD_get_connection_values(connection, MHD_HEADER_KIND, headerReceive, NULL);
  payloadSize   = 0;
  contentLength = 0;
  payload       = NULL;

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// mhdRequest -
//
MHD_Result mhdRequest
(
   void*            cls,
   MHD_Connection*  connection,
   const char*      url,
   const char*      method,
   const char*      version,
   const char*      upload_data,
   size_t*          upload_data_size,
   void**           con_cls
)
{
  ++mhdCalls;
  if (mhdCalls > 15)
    KT_X(1, "More than 15 consecutive calls");

  if (*con_cls == NULL)
  {
    KT_T(StRequest, "Incoming request: %s %s, type I (*con_cls == %p)", method, url, *con_cls);
    *con_cls = &cls;  // to "acknowledge" the first call
    
    return mhdRequestInit(connection, url, method, version, con_cls);
  }
  else if (*upload_data_size != 0)
  {
    KT_T(StRequest, "Incoming request: %s %s, type II - body (*con_cls == %p)", method, url, *con_cls);
    return mhdPayloadRead(upload_data_size, upload_data);
  }
  else
  {
    KT_T(StRequest, "Incoming request: %s %s, type III - last call (*con_cls == %p)", method, url, *con_cls);
    KT_T(StRequest, "Incoming request: %s %s", method, url);
    *upload_data_size = 0;  // Mark the request as "finished"
    // return mhdConnectionTreat();

#if 0
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);
    KjNode* nodeP = kjString(orionldState.kjsonP, "status", "ok");
    kjChildAdd(orionldState.responseTree, nodeP);
    nodeP = kjString(orionldState.kjsonP, "url", url);
    kjChildAdd(orionldState.responseTree, nodeP);
    char buf[1024];
    kjFastRender(orionldState.responseTree, buf);
    KT_T(StRequest, "Response: %s", buf);
    // KT_T(StRequest, "Calling mhdReply");
    // mhdReply(orionldState.responseTree);
    // KT_T(StRequest, "After mhdReply");
#endif
    char* buf = ftTreat(url, method);
    MHD_Response*  response;
    response = MHD_create_response_from_buffer(strlen(buf), buf, MHD_RESPMEM_MUST_COPY);
    MHD_queue_response(connection, 200, response);
    MHD_destroy_response(response);
  }

  return MHD_YES;
}

