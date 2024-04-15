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
*/

//
// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
extern "C"
{
#include "ktrace/kTrace.h"                                  // trace messages - ktrace library
#include "kjson/KjNode.h"                                   // KjNode
#include "kjson/kjLookup.h"                                 // kjLookup
}

#include "orionld/common/traceLevels.h"                     // Trace Levels
#include "orionld/dds/NgsildPublisher.h"                    // NgsildPublisher



// -----------------------------------------------------------------------------
//
// NgsildPublisher::~NgsildPublisher
//
NgsildPublisher::~NgsildPublisher()
{
  if (writer_ != nullptr)
    publisher_->delete_datawriter(writer_);

  if (publisher_ != nullptr)
    participant_->delete_publisher(publisher_);

  if (topic_ != nullptr)
    participant_->delete_topic(topic_);

  DomainParticipantFactory::get_instance()->delete_participant(participant_);
}



// -----------------------------------------------------------------------------
//
// NgsildPublisher::init -
//
// FIXME: need to move the params to the constructor, as the constructor creates the
//        NgsildEntityPubSubType where currently the topic name is hardcoded to "NgsildEntity".
//
bool NgsildPublisher::init(const char* topicType, const char* topicName)
{
  DomainParticipantQos participantQos;
  participantQos.name("Participant_publisher");
  participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

  if (participant_ == nullptr)
  {
    KT_E("Create participant failed");
    return false;
  }

  // Register the Type
  type_.register_type(participant_);

  // Create the publications Topic
  KT_V("creating topic: '%s' with type '%s'", topicName, topicType);
  topic_ = participant_->create_topic(topicName, topicType, TOPIC_QOS_DEFAULT);

  if (topic_ == nullptr)
  {
    KT_E("Create topic failed");
    return false;
  }
  KT_V("created topic: '%s' with type '%s'!", topicName, topicType);

  // Create the Publisher
  publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
  KT_V("created publisher");

  if (publisher_ == nullptr)
  {
    KT_E("Create publisher failed");
    return false;
  }

  // Create the DataWriter
  KT_V("creating writer");
  writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);

  if (writer_ == nullptr)
  {
    KT_E("Create DataWriter failed");
    return false;
  }
  KT_V("created writer");

  KT_V("Init done");
  return true;
}



// -----------------------------------------------------------------------------
//
// NgsildPublisher::publish -
//
bool NgsildPublisher::publish(KjNode* entityP)
{
  if (listener_.matched_ <= 0)
  {
    KT_W("listener not matched");
    return false;
  }
  if (entityP == NULL)
    KT_X(1, "entityP == NULL");

  KjNode*     idNodeP   = kjLookup(entityP, "id");
  KjNode*     typeNodeP = kjLookup(entityP, "type");
  const char* id        = (idNodeP != NULL)? idNodeP->value.s : "idNodeP is NULL";
  const char* type      = (typeNodeP != NULL)? typeNodeP->value.s : "typeNodeP is NULL";

  KT_V("id: '%s'", id);
  KT_V("type: '%s'", type);

  entity_.id(id);
  entity_.type(type);

  bool b = writer_->write(&entity_);

  if (b == false)
    KT_E("Not able to publish");
  else
    KT_V("Published");

  return true;
}
