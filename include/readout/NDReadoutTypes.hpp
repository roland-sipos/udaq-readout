/**
 * @file NDReadoutTypes.hpp Payload type structures for the DUNE Near Detector
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_NDREADOUTTYPES_HPP_
#define READOUT_INCLUDE_READOUT_NDREADOUTTYPES_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "dataformats/FragmentHeader.hpp"
#include "dataformats/GeoID.hpp"
#include "dataformats/pacman/PACMANFrame.hpp"

#include <cstdint>  // uint_t types
#include <memory>   // unique_ptr

namespace dunedaq {
namespace readout {
namespace types {

/**
 * @brief PACMAN frame
 * Size = 816[Bytes] (12*64+1*32+2*8)
 * */
const constexpr std::size_t PACMAN_FRAME_SIZE = 1024 * 1024;
struct PACMAN_MESSAGE_STRUCT
{
  using FrameType = PACMAN_MESSAGE_STRUCT;
  // data
  char data[PACMAN_FRAME_SIZE];
  // comparable based on first timestamp
  bool operator<(const PACMAN_MESSAGE_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data);  // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&other.data);  // NOLINT
    return (thisptr->get_msg_header((void *) &data)->unix_ts) * 1000000000 < (otherptr->get_msg_header((void *) &other.data)->unix_ts) * 1000000000 ? true : false;  // NOLINT
  }

  // message UNIX timestamp - NOT individual packet timestamps
  uint64_t get_timestamp() const  // NOLINT(build/unsigned)
  {
    return ((uint64_t)(reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_header((void *) &data)->unix_ts) * 1000000000);  // NOLINT
  }

  // FIX ME - implement this in the frame later
  void set_timestamp(uint64_t /*ts*/)  // NOLINT(build/unsigned)
  {
    // reinterpret_cast<dunedaq::dataformats::PACMANFrame*>(&data)->set_timestamp(ts); // NOLINT
  }

  uint64_t get_message_type() const  // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_header((void *) &data)->type;  // NOLINT
  }
  void inspect_message() const
  {
    TLOG_DEBUG(1) << "Message timestamp: " << get_timestamp();

    TLOG_DEBUG(1) << "Message Type: " << (char)get_message_type();

    uint16_t numWords = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_header((void *) &data)->words;  // NOLINT

    TLOG_DEBUG(1) << "Num words in message: " << numWords;

    for (unsigned int i = 0; i < numWords; i++)
    {
      TLOG_DEBUG(1) << "Inspecting word " << i;

      dunedaq::dataformats::PACMANFrame::PACMANMessageWord* theWord = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_word((void *) &data, i);  // NOLINT

      TLOG_DEBUG(1) << "Word type: " << (char)theWord->data_word.type;
      TLOG_DEBUG(1) << "PACMAN I/O Channel: " << (char)theWord->data_word.channel_id;
      TLOG_DEBUG(1) << "Word receipt timestamp: " << theWord->data_word.receipt_timestamp;

      dunedaq::dataformats::PACMANFrame::LArPixPacket* thePacket = &(theWord->data_word.larpix_word);

      TLOG_DEBUG(1) << "Inspecting packet";

      TLOG_DEBUG(1) << "Packet Type: " << thePacket->data_packet.type;
      TLOG_DEBUG(1) << "Packet Chip ID: " << thePacket->data_packet.chipid;
      TLOG_DEBUG(1) << "Packet Channel ID: " << thePacket->data_packet.channelid;

      TLOG_DEBUG(1) << "packet timestamp: " << thePacket->data_packet.timestamp;
    }
  }

  FrameType* begin()
  {
    return reinterpret_cast<FrameType*>(&data[0]);  // NOLINT
  }

  FrameType* end()
  {
    return reinterpret_cast<FrameType*>(data + PACMAN_FRAME_SIZE);  // NOLINT
  }

  static const constexpr dataformats::GeoID::SystemType system_type = dataformats::GeoID::SystemType::kNDLArTPC;
  static const constexpr dataformats::FragmentType fragment_type = dataformats::FragmentType::kNDLArTPC;
  static const constexpr size_t frame_size = PACMAN_FRAME_SIZE;

  // Set the right value for this field
  static const constexpr uint64_t tick_dist = 0;  // NOLINT(build/unsigned)

  static const constexpr uint8_t frames_per_element = 1;  // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size;
};

/**
 * Key finder for LBs.
 * */
struct PACMANTimestampGetter
{
  uint64_t operator()(const PACMAN_MESSAGE_STRUCT& data)  // NOLINT(build/unsigned)
  {
    return data.get_timestamp();
  }
};

typedef dunedaq::appfwk::DAQSink<PACMAN_MESSAGE_STRUCT> PACMANFrameSink;
typedef std::unique_ptr<PACMANFrameSink> UniquePACMANFrameSink;
using PACMANFramePtrSink = appfwk::DAQSink<std::unique_ptr<types::PACMAN_MESSAGE_STRUCT>>;
using UniquePACMANFramePtrSink = std::unique_ptr<PACMANFramePtrSink>;

}  // namespace types
}  // namespace readout
}  // namespace dunedaq

#endif  // READOUT_INCLUDE_READOUT_NDREADOUTTYPES_HPP_
