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
#include <microhttpd.h>                                          // MHD

extern "C"
{
#include "ktrace/kTrace.h"                                       // KT_*
}

#include "orionld/common/traceLevels.h"                          // Tl*
#include "orionld/mhd/mhdConnectionInit.h"                       // mhdRequestInit
#include "orionld/mhd/mhdRequest.h"                              // Own interface


__thread int mhdCalls = 0;

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
  if (mhdCalls > 10)
    KT_X(1, "Too many MHD calls to mhdRequest - something is wrong");

  if (*con_cls == NULL)
  {
    KT_T(StRequest, "Incoming request: %s %s, type I (*con_cls == %p)", method, url, *con_cls);
    *con_cls = &cls;  // to "acknowledge" the first call
    
    // return mhdConnectionInit(connection, url, method, version, con_cls);
  }
  else if (*upload_data_size != 0)
  {
    KT_T(StRequest, "Incoming request: %s %s, type II (*con_cls == %p)", method, url, *con_cls);
    // return mhdConnectionPayloadRead(upload_data_size, upload_data);
  }
  else
  {
    KT_T(StRequest, "Incoming request: %s %s, type III (*con_cls == %p)", method, url, *con_cls);
    *upload_data_size = 0;  // Mark the request as "finished"
    // return mhdConnectionTreat();
    MHD_Response*  response;
    response = MHD_create_response_from_buffer(9, (void*) "All Good\n", MHD_RESPMEM_MUST_COPY);
    MHD_queue_response(connection, 200, response);
    MHD_destroy_response(response);
  }

  return MHD_YES;
}
