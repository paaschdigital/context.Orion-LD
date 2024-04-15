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
* Author: David Campo, Ken Zangelin
*/
#include <string.h>
#include <pthread.h>                                        // pthread_exit

extern "C"
{
#include "ktrace/kTrace.h"                                  // trace messages - ktrace library
}

#include "orionld/dds/NgsildSubscriber.h"



// -----------------------------------------------------------------------------
//
// ddsSubscribe -
//
// EPROS: We would like to have one single subscriber, that subscribes to all DDS notifications
//        Obviously, we'd need a way to add topic to that subscriber "on the fly"
//
typedef struct X
{
  char* topicType;
  char* topicName;
} X;

static void* ddsSubscribe2(void* vP)
{
  X* xP = (X*) vP;

  KT_V("Creating a subscription on '%s/%s'", xP->topicType, xP->topicName);

  NgsildSubscriber* subP = new NgsildSubscriber();

  if (subP->init(xP->topicType, xP->topicName))
  {
    subP->run(1400000);  // EPROS: one single subscriber ... run forever ...
  }

  KT_V("Deleting the subscription on '%s/%s'", xP->topicType, xP->topicName);
  delete subP;

  return NULL;
}

void ddsSubscribe(const char* topicType, const char* topicName)
{
  pthread_t  tid;
  X*         xP = (X*) malloc(sizeof(X));

  xP->topicType = strdup(topicType);
  xP->topicName = strdup(topicName);

  KT_V("Starting thread for DDS subscription on %s/%s", xP->topicType, xP->topicName);

  pthread_create(&tid, NULL, ddsSubscribe2, xP);
  KT_V("Continue the execution of father thread");
}
