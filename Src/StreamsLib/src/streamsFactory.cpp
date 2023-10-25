#include "../include/streamsFactory.h"

using namespace libCZIStreamsLib;

bool GetStreamInfoForClass(int index, StreamClassInfo& stream_info)
{
  return false;
}

int GetStreamInfoCount()
{
  return 1;
}

std::shared_ptr<libCZI::IStream> CreateStream(const CreateStreamInfo& stream_info)
{
  return {};
}