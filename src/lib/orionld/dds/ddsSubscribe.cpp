extern "C"
{
#include "ktrace/kTrace.h"                                  // trace messages - ktrace library
}

#include "orionld/dds/NgsildSubscriber.h"

void ddsSubscribe(const char* topic)
{
  KT_V("Here");
  NgsildSubscriber* subP = new NgsildSubscriber();
  KT_V("Here");
  if(subP->init(topic))
    {
      KT_V("Here");
      subP->run(1400000);
      KT_V("Here");
    }
  KT_V("Deleting subscription");
  delete subP;
}
