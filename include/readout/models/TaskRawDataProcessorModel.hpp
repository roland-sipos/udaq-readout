/**
 * @file TaskRawDataProcessorModel.hpp Raw processor parallel task and pipeline
 * combination using a vector of std::functions
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_TASKRAWDATAPROCESSORMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_TASKRAWDATAPROCESSORMODEL_HPP_

#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/concepts/RawDataProcessorConcept.hpp"
#include "readout/utils/ReusableThread.hpp"

#include <folly/ProducerConsumerQueue.h>

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class ReadoutType>
class TaskRawDataProcessorModel : public RawDataProcessorConcept<ReadoutType>
{
public:
  explicit TaskRawDataProcessorModel(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : RawDataProcessorConcept<ReadoutType>()
    , m_error_registry(error_registry)
  {}

  ~TaskRawDataProcessorModel() {}

  void conf(const nlohmann::json& cfg) override
  {
    auto config = cfg.get<datalinkhandler::Conf>();
    m_postprocess_queue_sizes = config.postprocess_queue_sizes;

    for (size_t i = 0; i < m_post_process_functions.size(); ++i) {
      m_items_to_postprocess_queues.push_back(
        std::make_unique<folly::ProducerConsumerQueue<const ReadoutType*>>(m_postprocess_queue_sizes));
    }
    RawDataProcessorConcept<ReadoutType>::conf(cfg);
  }

  void start(const nlohmann::json& /*args*/) override
  {
    m_run_marker.store(true);
    for (size_t i = 0; i < m_post_process_threads.size(); ++i) {
      m_post_process_threads[i]->set_work(&TaskRawDataProcessorModel<ReadoutType>::run_post_processing_thread,
                                          this,
                                          std::ref(m_post_process_functions[i]),
                                          std::ref(*m_items_to_postprocess_queues[i]));
    }
  }

  void stop(const nlohmann::json& /*args*/) override
  {
    m_run_marker.store(false);
    for (auto& thread : m_post_process_threads) {
      while (!thread->get_readiness()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
  }

  virtual void get_info(datalinkhandlerinfo::Info& /*info*/)
  {
    // No stats for now, extend later
  }

  void preprocess_item(ReadoutType* item) override { invoke_all_preprocess_functions(item); }

  void postprocess_item(const ReadoutType* item) override
  {
    for (size_t i = 0; i < m_items_to_postprocess_queues.size(); ++i) {
      if (!m_items_to_postprocess_queues[i]->write(item)) {
        ers::warning(PostprocessingNotKeepingUp(ERS_HERE, i));
      }
    }
  }

  template<typename Task>
  void add_preprocess_task(Task&& task)
  {
    m_preprocess_functions.push_back(std::forward<Task>(task));
  }

  template<typename Task>
  void add_postprocess_task(Task&& task)
  {
    m_post_process_threads.emplace_back(std::make_unique<ReusableThread>(0));
    m_post_process_threads.back()->set_name("postprocess", m_post_process_threads.size() - 1);
    m_post_process_functions.push_back(std::forward<Task>(task));
  }

  void invoke_all_preprocess_functions(ReadoutType* item)
  {
    for (auto&& task : m_preprocess_functions) {
      task(item);
    }
  }

  void launch_all_preprocess_functions(ReadoutType* item)
  {
    for (auto&& task : m_preprocess_functions) {
      auto fut = std::async(std::launch::async, task, item);
    }
  }

protected:
  void run_post_processing_thread(std::function<void(const ReadoutType*)>& function,
                                  folly::ProducerConsumerQueue<const ReadoutType*>& queue)
  {
    while (m_run_marker.load() || queue.sizeGuess() > 0) {
      const ReadoutType* element;
      if (queue.read(element)) {
        function(element);
      }
    }
  }

  std::atomic<bool> m_run_marker{ false };
  // Async tasks and
  std::vector<std::function<void(ReadoutType*)>> m_preprocess_functions;
  std::unique_ptr<FrameErrorRegistry>& m_error_registry;

  std::vector<std::function<void(const ReadoutType*)>> m_post_process_functions;
  std::vector<std::unique_ptr<folly::ProducerConsumerQueue<const ReadoutType*>>> m_items_to_postprocess_queues;
  std::vector<std::unique_ptr<ReusableThread>> m_post_process_threads;

  size_t m_postprocess_queue_sizes;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_TASKRAWDATAPROCESSORMODEL_HPP_
