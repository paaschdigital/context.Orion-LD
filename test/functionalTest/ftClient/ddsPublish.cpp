#include <chrono>
#include <thread>
#include <unistd.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

extern "C"
{
#include "ktrace/kTrace.h"                                  // trace messages - ktrace library
}

#include "dds/NgsildEntityPubSubTypes.h"
#include "dds/NgsildEntity.h"

using namespace eprosima::fastdds::dds;

class NgsildEntityPublisher
{
private:
  
  NgsildEntity entity_;
  
  DomainParticipant* participant_;
  
  Publisher* publisher_;
  
  Topic* topic_;
  
  DataWriter* writer_;
  
  TypeSupport type_;
  
  class PubListener : public DataWriterListener
  {
  public:
    
    PubListener()
      : matched_(0)
    {
    }
    
    ~PubListener() override
    {
    }
    
    void on_publication_matched(
				DataWriter*,
				const PublicationMatchedStatus& info) override
    {
      KT_V("info.current_count_change: %d", info.current_count_change);
      if (info.current_count_change == 1)
	{
	  matched_ = info.total_count;
	  std::cout << "Publisher matched." << std::endl;
	}
      else if (info.current_count_change == -1)
	{
	  matched_ = info.total_count;
	  std::cout << "Publisher unmatched." << std::endl;
	}
      else
	{
	  std::cout << info.current_count_change
		    << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
	}
    }
    
    std::atomic_int matched_;
    
  } listener_;
  
public:
  
  NgsildEntityPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new NgsildEntityPubSubType())
  {
  }
  
  virtual ~NgsildEntityPublisher()
  {
    if (writer_ != nullptr)
      {
	publisher_->delete_datawriter(writer_);
      }
    if (publisher_ != nullptr)
      {
	participant_->delete_publisher(publisher_);
      }
    if (topic_ != nullptr)
      {
	participant_->delete_topic(topic_);
      }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
  }

  
  
  //!Initialize the publisher
  bool init(const char* topicType, const char* topicName)
  {
    entity_.id("0");
    entity_.type("T1");
    
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
    KT_V("creating topic: '%s' with type 'topicType'", topicName, topicType);
    topic_ = participant_->create_topic(topicName, topicType, TOPIC_QOS_DEFAULT);
    
    if (topic_ == nullptr)
      {
	KT_E("Create topic failed");	
	return false;
      }
    
    // Create the Publisher
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    
    if (publisher_ == nullptr)
      {
	KT_E("Create publisher failed");
	return false;
      }
    
    // Create the DataWriter
    writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);
    
    if (writer_ == nullptr)
      {
	KT_E("Create DataWriter failed");
	return false;
      }
    
    KT_V("Init done");
    
    return true;
  }
  
  //!Send a publication
  bool publish()
  {
    if (listener_.matched_ > 0)
      {
	bool b=writer_->write(&entity_);
	if(b==false)
	  KT_E("Not able to publish");
	else
	  KT_V("Published");
	return true;
      }
    else
      KT_W("listener not matched");
    return false;
  }
};



void ddsPublish(const char* topicType, const char* topicName)
{
  NgsildEntityPublisher* mypub = new NgsildEntityPublisher();
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
