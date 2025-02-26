# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es

# ---------------------------------------------------------
# External dependency checks
# ---------------------------------------------------------

FIND_LIBRARY (HAVE_GTEST gtest PATHS /usr/lib /usr/lib64 /usr/local/lib64 /usr/local/lib)
if (HAVE_GTEST)
    MESSAGE("Using ${HAVE_GTEST} for Google Test")
else (HAVE_GTEST)
    MESSAGE("Not using the Google Test library")
endif (HAVE_GTEST)

find_library(GTESTMAIN_LIBRARY gtest_main
             PATHS /usr/lib64 /usr/local/lib64 /usr/local/lib)


find_path(GTEST_INCLUDE_DIR gtest/gtest.h
          PATHS /usr/includes 
          PATH_SUFFIXES gtest)

FIND_LIBRARY (HAVE_GMOCK gmock PATHS /usr/lib /usr/lib64 /usr/local/lib64 /usr/local/lib)
if (HAVE_GMOCK)
    MESSAGE("Using ${HAVE_GMOCK} for Google Mock")
else (HAVE_GMOCK)
    MESSAGE("Not using the Google Mock library")
endif (HAVE_GMOCK)

find_library(GMOCKMAIN_LIBRARY gmock_main
             PATHS /usr/lib64 /usr/local/lib64 /usr/local/lib)

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h
          PATHS /usr/includes
          PATH_SUFFIXES gmock)


#
# Stack protection is activated only in debug compilation.
# However, our unit tests don't pass the compilation with "stack protection", so we turn it off using
# remove_definitions().
#
remove_definitions(-fstack-check -fstack-protector)

SET (SOURCES
    main_UnitTest.cpp
    testDataFromFile.cpp
    testInit.cpp
    unittest.cpp

    ngsi9/RegisterContextRequest_test.cpp
    ngsi9/RegisterContextResponse_test.cpp
    ngsi9/DiscoverContextAvailabilityRequest_test.cpp
    ngsi9/DiscoverContextAvailabilityResponse_test.cpp
    ngsi9/SubscribeAvailabilityRequest_test.cpp
    ngsi9/SubscribeContextAvailabilityResponse_test.cpp
    ngsi9/NotifyContextAvailabilityRequest_test.cpp
    ngsi9/NotifyContextAvailabilityResponse_test.cpp
    ngsi9/UnsubscribeContextAvailabilityRequest_test.cpp
    ngsi9/UnsubscribeContextAvailabilityResponse_test.cpp
    ngsi9/UpdateContextAvailabilitySubscriptionRequest_test.cpp
    ngsi9/UpdateContextAvailabilitySubscriptionResponse_test.cpp

    ngsi10/QueryContextRequest_test.cpp
    ngsi10/QueryContextResponse_test.cpp
    ngsi10/NotifyContextRequest_test.cpp
    ngsi10/SubscribeContextRequest_test.cpp
    ngsi10/SubscribeContextResponse_test.cpp
    ngsi10/UnsubscribeContextRequest_test.cpp
    ngsi10/UnsubscribeContextResponse_test.cpp
    ngsi10/UpdateContextRequest_test.cpp
    ngsi10/UpdateContextResponse_test.cpp
    ngsi10/UpdateContextSubscriptionRequest_test.cpp
    ngsi10/UpdateContextSubscriptionResponse_test.cpp

    ngsi/ContextAttribute_test.cpp
    ngsi/ContextElement_test.cpp
    ngsi/NotifyCondition_test.cpp
    ngsi/AttributeDomainName_test.cpp
    ngsi/AttributeList_test.cpp
    ngsi/ConditionValueList_test.cpp
    ngsi/ContextAttributeVector_test.cpp
    ngsi/ContextElementResponse_test.cpp
    ngsi/ContextElementResponseVector_test.cpp
    ngsi/ContextRegistrationAttribute_test.cpp
    ngsi/ContextRegistrationAttributeVector_test.cpp
    ngsi/ContextRegistrationVector_test.cpp
    ngsi/ProvidingApplication_test.cpp
    ngsi/Duration_test.cpp
    ngsi/AttributeExpression_test.cpp
    ngsi/ContextElementVector_test.cpp
    ngsi/ContextRegistrationResponse_test.cpp
    ngsi/ContextRegistrationResponseVector_test.cpp
    ngsi/EntityId_test.cpp
    ngsi/Metadata_test.cpp
    ngsi/MetadataVector_test.cpp
    ngsi/NotifyConditionVector_test.cpp
    ngsi/Originator_test.cpp
    ngsi/Reference_test.cpp
    ngsi/Restriction_test.cpp
    ngsi/Request_test.cpp
    ngsi/RestrictionString_test.cpp
    ngsi/Scope_test.cpp
    ngsi/ScopeVector_test.cpp
    ngsi/StatusCode_test.cpp
    ngsi/SubscribeError_test.cpp
    ngsi/SubscriptionId_test.cpp
    ngsi/Throttling_test.cpp

    orionTypes/EntityType_test.cpp
    orionTypes/EntityTypeResponse_test.cpp
    orionTypes/EntityTypeVector_test.cpp
    orionTypes/EntityTypeVectorResponse_test.cpp

    apiTypesV2/Entities_test.cpp
    apiTypesV2/Entity_test.cpp  
    apiTypesV2/EntityVector_test.cpp

    parse/CompoundValueNode_test.cpp
    parse/compoundValue_test.cpp
    parse/nullTreat_test.cpp
    jsonParse/jsonRequest_test.cpp

    rest/OrionError_test.cpp
    rest/Verb_test.cpp
    rest/restReply_test.cpp
    rest/RestService_test.cpp
    rest/rest_test.cpp
)

SET (HEADERS
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
)

SET (STATIC_LIBS
    ${HAVE_GTEST}
    ${GTESTMAIN_LIBRARY}
    ${HAVE_GMOCK}
    ${GMOCKMAIN_LIBRARY}
    ${ORION_LIBS}
    ${COMMON_STATIC_LIBS}
    ${ORION_LIBS}
)

# Include directories
# ------------------------------------------------------------
include_directories("/usr/include")
include_directories("${LIBBSON_INCLUDE_DIRS}")
include_directories("${LIBMONGOC_INCLUDE_DIRS}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("${PROJECT_SOURCE_DIR}/src/app")
include_directories("${PROJECT_SOURCE_DIR}/test/unittests")
include_directories("${PROJECT_SOURCE_DIR}/test")
include_directories("${PROJECT_SOURCE_DIR}/..")

# Lib directories
# ------------------------------------------------------------
link_directories("/usr/local/lib/")
link_directories("/usr/lib64/")
link_directories("/usr/lib/x86_64-linux-gnu")
link_directories("${Boost_LIBRARY_DIRS}")
link_directories("${LIBMONGOC_LIBRARY_DIRS}")
link_directories("${PROJECT_SOURCE_DIR}/../kjson")
link_directories("${PROJECT_SOURCE_DIR}/../kalloc")
link_directories("${PROJECT_SOURCE_DIR}/../kbase")
link_directories("${PROJECT_SOURCE_DIR}/../khash")
link_directories("${PROJECT_SOURCE_DIR}/../klog")
link_directories("${PROJECT_SOURCE_DIR}/../prometheus-client-c/prom/build")
link_directories("${PROJECT_SOURCE_DIR}/../prometheus-client-c/promhttp/build")
link_directories("${LIBMICROHTTPD_LIBRARY_DIRS}")



# flags
# ------------------------------------------------------------
add_definitions(-DUNIT_TEST)



# Executable declaration
# ------------------------------------------------------------

ADD_EXECUTABLE(unitTest ${SOURCES})
TARGET_LINK_LIBRARIES(unitTest
  ${STATIC_LIBS}
  ${Boost_LIBRARIES}
  ${LIBMICROHTTPD_LIBRARIES}
  ${DYNAMIC_LIBS}
)

IF ((${DISTRO} STREQUAL "Ubuntu_13.04") OR (${DISTRO} STREQUAL "Ubuntu_12.04.1_LTS") OR (${DISTRO} STREQUAL "Ubuntu_12.04.2_LTS"))
  # Pending to check after the refactor
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion-null")  
ENDIF()

