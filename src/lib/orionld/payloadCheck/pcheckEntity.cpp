/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
* Author: Ken Zangelin, Gabriel Quaresma
*/
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/service/orionldServiceInit.h"                  // orionldHostName, orionldHostNameLen
#include "orionld/context/orionldCoreContext.h"                  // orionldDefaultUrl, orionldCoreContext
#include "orionld/payloadCheck/pcheckName.h"                     // pcheckName
#include "orionld/payloadCheck/pcheckEntity.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// checkEntityIdFieldExistis -
//
static bool checkEntityIdFieldExists(void)
{
  if (orionldState.payloadIdNode == NULL)
  {
    orionldError(OrionldBadRequestData, "Entity id is missing", "The 'id' field is mandatory", 400);
    return false;
  }

  if (orionldState.payloadTypeNode == NULL)
  {
    const char* entityStartTitle       = "Entity - ";
    const char* idEntityFromPayload    = orionldState.payloadIdNode->value.s;
    char*       titleExpanded;

    titleExpanded = kaAlloc(&orionldState.kalloc, (1 + strlen(entityStartTitle) + strlen(idEntityFromPayload)));
    strcpy(titleExpanded, entityStartTitle);
    strcat(titleExpanded, idEntityFromPayload);

    orionldError(OrionldBadRequestData, titleExpanded, "The 'type' field is mandatory", 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckEntity -
//
bool pcheckEntity
(
  KjNode*   kNodeP,
  KjNode**  locationNodePP,
  KjNode**  observationSpaceNodePP,
  KjNode**  operationSpaceNodePP,
  KjNode**  createdAtPP,
  KjNode**  modifiedAtPP,
  bool      isBatchOperation
)
{
  if (isBatchOperation == false)
  {
    OBJECT_CHECK(orionldState.requestTree, "toplevel");

    //
    // Check presence of mandatory fields "id" and "type"
    //
    if (checkEntityIdFieldExists() == false)
      return false;
  }

  char*    detailsP;
  KjNode*  locationNodeP          = NULL;
  KjNode*  observationSpaceNodeP  = NULL;
  KjNode*  operationSpaceNodeP    = NULL;
  KjNode*  createdAtP             = NULL;
  KjNode*  modifiedAtP            = NULL;
  KjNode*  batchIdP               = NULL;
  KjNode*  batchTypeP             = NULL;

  //
  // Check for duplicated items and that data types are correct
  //
  while (kNodeP != NULL)
  {
    if (isBatchOperation == true)
    {
      if (strcmp(kNodeP->name, "id") == 0 || strcmp(kNodeP->name, "@id") == 0)
      {
        DUPLICATE_CHECK(batchIdP, "id", kNodeP);
        STRING_CHECK(batchIdP, "id");
        URI_CHECK(batchIdP->value.s, "id", true);
        orionldState.payloadIdNode = kNodeP;  // FIXME: Is this necessary?
      }
      else if (strcmp(kNodeP->name, "type") == 0 || strcmp(kNodeP->name, "@type") == 0)
      {
        DUPLICATE_CHECK(batchTypeP, "type", kNodeP);
        STRING_CHECK(batchTypeP, "type");
        URI_CHECK(batchTypeP->value.s, "type", false);
        orionldState.payloadTypeNode = kNodeP;  // FIXME: Is this necessary?
      }
    }

    if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(locationNodeP, "location", kNodeP);
      // FIXME: check validity of location - GeoProperty - Issue #256
    }
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceNodeP, "observationSpace", kNodeP);
      // FIXME: check validity of observationSpace - GeoProperty
    }
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceNodeP, "operationSpace", kNodeP);
      // FIXME: check validity of operationSpaceP - GeoProperty
    }
    else if (SCOMPARE10(kNodeP->name, 'c', 'r', 'e', 'a', 't', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(createdAtP, "createdAt", kNodeP);
      STRING_CHECK(kNodeP, "createdAt");
    }
    else if (SCOMPARE11(kNodeP->name, 'm', 'o', 'd', 'i', 'f', 'i', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(modifiedAtP, "modifiedAt", kNodeP);
      STRING_CHECK(kNodeP, "modifiedAt");
    }
    else  // Property/Relationshiop - must check chars in the name of the attribute
    {
      if (strcmp(kNodeP->name, "@context") != 0)
      {
        if (pcheckName(kNodeP->name, &detailsP) == false)
        {
          orionldError(OrionldBadRequestData, "Invalid Property/Relationship name", kNodeP->name, 400);
          return false;
        }
      }
    }
    kNodeP = kNodeP->next;
  }

  if ((isBatchOperation == true) && (checkEntityIdFieldExists() == false))
    return false;


  //
  // Prepare output
  //
  if (locationNodePP         != NULL)  *locationNodePP         = locationNodeP;
  if (observationSpaceNodePP != NULL)  *observationSpaceNodePP = observationSpaceNodeP;
  if (operationSpaceNodePP   != NULL)  *operationSpaceNodePP   = operationSpaceNodeP;
  if (createdAtPP            != NULL)  *createdAtPP            = createdAtP;
  if (modifiedAtPP           != NULL)  *modifiedAtPP           = modifiedAtP;

  return true;
}
