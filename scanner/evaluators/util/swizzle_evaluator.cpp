/* Copyright 2016 Carnegie Mellon University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "scanner/evaluators/util/swizzle_evaluator.h"

#include "scanner/util/common.h"
#include "scanner/util/memory.h"
#include "scanner/util/util.h"

#include <cassert>

namespace scanner {

SwizzleEvaluator::SwizzleEvaluator(const EvaluatorConfig& config,
                                   DeviceType device_type, i32 device_id,
                                   const std::vector<i32>& output_to_input_idx)
    : config_(config),
      device_type_(device_type),
      device_id_(device_id),
      output_to_input_idx_(output_to_input_idx) {}

void SwizzleEvaluator::configure(const DatasetItemMetadata& metadata) {
  metadata_ = metadata;
}

void SwizzleEvaluator::evaluate(
    const std::vector<std::vector<u8 *>> &input_buffers,
    const std::vector<std::vector<size_t>> &input_sizes,
    std::vector<std::vector<u8 *>> &output_buffers,
    std::vector<std::vector<size_t>> &output_sizes) {
  i32 input_count = static_cast<i32>(input_buffers[0].size());

  size_t num_outputs = output_to_input_idx_.size();
  for (size_t i = 0; i < num_outputs; ++i) {
    i32 input_idx = output_to_input_idx_[i];
    assert(input_idx < input_buffers.size());
    for (i32 b = 0; b < input_count; ++b) {
      size_t size = input_sizes[input_idx][b];
      u8* input_buffer = input_buffers[input_idx][b];

      u8* output_buffer = nullptr;
      if (device_type_ == DeviceType::GPU) {
#ifdef HAVE_CUDA
        cudaMalloc((void**)&output_buffer, size);
        cudaMemcpy(output_buffer, input_buffer, size, cudaMemcpyDefault);
#else
        LOG(FATAL) << "Not built with CUDA support.";
#endif
      } else {
        output_buffer = new u8[size];
        memcpy(output_buffer, input_buffer, size);
      }
      assert(output_buffer != nullptr);
      output_buffers[i].push_back(output_buffer);
      output_sizes[i].push_back(size);
    }
  }
}

SwizzleEvaluatorFactory::SwizzleEvaluatorFactory(
    DeviceType device_type, const std::vector<i32> &output_to_input_idx,
    const std::vector<std::string> &output_names)
    : device_type_(device_type),
      output_to_input_idx_(output_to_input_idx),
      output_names_(output_names) {
  assert(output_names.size() == output_to_input_idx.size());
}

EvaluatorCapabilities SwizzleEvaluatorFactory::get_capabilities() {
  EvaluatorCapabilities caps;
  caps.device_type = device_type_;
  caps.max_devices = 1;
  caps.warmup_size = 0;
  return caps;
}

std::vector<std::string> SwizzleEvaluatorFactory::get_output_names() {
  return output_names_;
}

Evaluator *SwizzleEvaluatorFactory::new_evaluator(
    const EvaluatorConfig &config) {
  return new SwizzleEvaluator(config, device_type_, 0, output_to_input_idx_);
}
}