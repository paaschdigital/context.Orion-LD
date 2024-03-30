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
extern "C"
{
#include "ktrace/kTrace.h"                                  // trace messages - ktrace library
}

#include "orionld/dds/NgsildEntityPubSubTypes.h"
#include "orionld/dds/NgsildEntity.h"
#include "orionld/dds/NgsildPublisher.h"

using namespace eprosima::fastdds::dds;



// -----------------------------------------------------------------------------
//
// ddsPublish -
//
void ddsPublish(const char* topicType, const char* topicName)
{
  NgsildPublisher* mypub = new NgsildPublisher();

  KT_V("Initializing publisher for topicName '%s', topicType '%s'", topicName, topicType);
  if(mypub->init(topicType, topicName))
    {
      usleep(1000);
      KT_V("Publishing on topicName '%s', topicType '%s'", topicName, topicType);
      if(mypub->publish())
	KT_V("Published on topicName '%s', topicType '%s'", topicName, topicType);
      else
	KT_V("Error publishing on topicName '%s', topicType '%s'", topicName, topicType);
    }
  KT_V("Deleting publisher");
  delete mypub;
}
