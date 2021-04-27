/**
 * @file VariableSizeElementQueue_test.cxx Unit Tests for the VariableSizeElementQueue
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE VariableSizeElementQueue_test // NOLINT

#include "boost/test/unit_test.hpp"

#include "readout/ReadoutTypes.hpp"
#include "VariableSizeElementQueue.hpp"

#include <string>
#include <cstdio>

using namespace dunedaq::readout;

BOOST_AUTO_TEST_SUITE(VariableSizeElementQueue_test)

BOOST_AUTO_TEST_CASE(VariableSizeElementQueue_find) {
  TLOG() << "Fill up a queue and check if the elements can be found" << std::endl;
  VariableSizeElementQueue<types::VariableSizePayloadWrapper> queue(10000);
  for (uint32_t timestamp_counter = 0; timestamp_counter < 10000; timestamp_counter += 10) {
    char* payload = (char*) malloc(1024);
    memcpy(payload, &timestamp_counter, sizeof(uint32_t));
    types::VariableSizePayloadWrapper wrapper(1024, payload);
    bool queue_write = queue.write(std::move(wrapper));
    BOOST_REQUIRE(queue_write);

    for (uint32_t expected_timestamp = 0; expected_timestamp <= timestamp_counter; expected_timestamp += 10) {
      uint32_t timestamp;
      types::VariableSizePayloadWrapper* found_element = queue.find(expected_timestamp);
      BOOST_REQUIRE(found_element);
      std::memcpy(&timestamp, found_element->data.get(), sizeof(timestamp));
      BOOST_REQUIRE_EQUAL(expected_timestamp, timestamp);
    }
  }
}

BOOST_AUTO_TEST_CASE(VariableSizeElementQueue_find_upper) {
  TLOG() << "Search for elements that are not in the queue" << std::endl;
  VariableSizeElementQueue<types::VariableSizePayloadWrapper> queue(10000);
  for (uint32_t timestamp_counter = 10; timestamp_counter < 10000; timestamp_counter += 10) {
    char* payload = (char*) malloc(1024);
    memcpy(payload, &timestamp_counter, sizeof(uint32_t));
    types::VariableSizePayloadWrapper wrapper(1024, payload);
    bool queue_write = queue.write(std::move(wrapper));
    BOOST_REQUIRE(queue_write);

    for (uint32_t expected_timestamp = 5; expected_timestamp <= timestamp_counter; expected_timestamp += 10) {
      uint32_t timestamp;
      types::VariableSizePayloadWrapper* found_element = queue.find(expected_timestamp);
      BOOST_REQUIRE(found_element);
      std::memcpy(&timestamp, found_element->data.get(), sizeof(timestamp));
      BOOST_REQUIRE_EQUAL(expected_timestamp+5, timestamp);
    }
  }
}

BOOST_AUTO_TEST_CASE(VariableSizeElementQueue_find_outside) {
  TLOG() << "Try to find elements that are bigger than the biggest one in the queue" << std::endl;
  VariableSizeElementQueue<types::VariableSizePayloadWrapper> queue(10);
  char* payload = (char*) malloc(1024);
  uint32_t timestamp = 42;
  memcpy(payload, &timestamp, sizeof(uint32_t));
  types::VariableSizePayloadWrapper wrapper(1024, payload);
  bool queue_write = queue.write(std::move(wrapper));
  BOOST_REQUIRE(queue_write);

  types::VariableSizePayloadWrapper* found_element = queue.find(100);
  BOOST_REQUIRE_EQUAL(found_element, nullptr);
}

BOOST_AUTO_TEST_CASE(VariableSizeElementQueue_overrun) {
  TLOG() << "Cause an overrun of the cyclic buffer" << std::endl;
  VariableSizeElementQueue<types::VariableSizePayloadWrapper> queue(1001);
  for (uint32_t timestamp_counter = 0; timestamp_counter < 1000; ++timestamp_counter) {
    char* payload = (char*) malloc(1024);
    memcpy(payload, &timestamp_counter, sizeof(uint32_t));
    types::VariableSizePayloadWrapper wrapper(1024, payload);
    bool queue_write = queue.write(std::move(wrapper));
    BOOST_REQUIRE(queue_write);
  }

  for (int i = 0; i < 500; ++i) {
    types::VariableSizePayloadWrapper wrapper;
    bool read_successful = queue.read(wrapper);
    BOOST_REQUIRE(read_successful);
  }

  for (uint32_t timestamp_counter = 1000; timestamp_counter < 1500; ++timestamp_counter) {
    char* payload = (char*) malloc(1024);
    memcpy(payload, &timestamp_counter, sizeof(uint32_t));
    types::VariableSizePayloadWrapper wrapper(1024, payload);
    bool queue_write = queue.write(std::move(wrapper));
    BOOST_REQUIRE(queue_write);
  }

  for (uint32_t expected_timestamp = 500; expected_timestamp < 1500; ++expected_timestamp) {
    uint32_t timestamp;
    types::VariableSizePayloadWrapper* found_element = queue.find(expected_timestamp);
    BOOST_REQUIRE(found_element);
    std::memcpy(&timestamp, found_element->data.get(), sizeof(timestamp));
    BOOST_REQUIRE_EQUAL(expected_timestamp, timestamp);
  }
}

BOOST_AUTO_TEST_CASE(VariableSizeElementQueue_overrun_find_upper) {
    TLOG() << "Cause an overrun of the cyclic buffer and search for upper timestamps" << std::endl;
    VariableSizeElementQueue<types::VariableSizePayloadWrapper> queue(1001);
    for (uint32_t timestamp_counter = 10; timestamp_counter < 10000; timestamp_counter += 10) {
      char* payload = (char*) malloc(1024);
      memcpy(payload, &timestamp_counter, sizeof(uint32_t));
      types::VariableSizePayloadWrapper wrapper(1024, payload);
      bool queue_write = queue.write(std::move(wrapper));
      BOOST_REQUIRE(queue_write);
    }

    for (int i = 0; i < 500; ++i) {
      types::VariableSizePayloadWrapper wrapper;
      bool read_successful = queue.read(wrapper);
      BOOST_REQUIRE(read_successful);
    }

    for (uint32_t timestamp_counter = 10000; timestamp_counter < 15000; timestamp_counter += 10) {
      char* payload = (char*) malloc(1024);
      memcpy(payload, &timestamp_counter, sizeof(uint32_t));
      types::VariableSizePayloadWrapper wrapper(1024, payload);
      bool queue_write = queue.write(std::move(wrapper));
      BOOST_REQUIRE(queue_write);
    }

    for (uint32_t expected_timestamp = 5005; expected_timestamp < 14995; expected_timestamp += 10) {
      uint32_t timestamp;
      types::VariableSizePayloadWrapper* found_element = queue.find(expected_timestamp);
      BOOST_REQUIRE(found_element);
      std::memcpy(&timestamp, found_element->data.get(), sizeof(timestamp));
      BOOST_REQUIRE_EQUAL(expected_timestamp+5, timestamp);
    }
}


BOOST_AUTO_TEST_SUITE_END()

