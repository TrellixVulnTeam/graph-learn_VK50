/* Copyright 2020 Alibaba Group Holding Limited. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <vector>
#include "core/operator/sampler/padder/padder.h"
#include "core/operator/sampler/sampler.h"
#include "include/config.h"

namespace graphlearn {
namespace op {

class TopkSampler : public Sampler {
public:
  virtual ~TopkSampler() {}

  Status Sample(const SamplingRequest* req,
                SamplingResponse* res) override {
    int32_t count = req->NeighborCount();
    int32_t batch_size = req->BatchSize();

    res->SetBatchSize(batch_size);
    res->SetNeighborCount(count);
    res->InitNeighborIds(batch_size * count);
    res->InitEdgeIds(batch_size * count);

    const std::string& edge_type = req->Type();
    Graph* graph = graph_store_->GetGraph(edge_type);
    auto storage = graph->GetLocalStorage();

    Status s;
    const int64_t* src_ids = req->GetSrcIds();
    const int64_t* filters = req->GetFilters();
    for (int32_t i = 0; i < batch_size; ++i) {
      int64_t src_id = src_ids[i];
      auto neighbor_ids = storage->GetNeighbors(src_id);
      if (!neighbor_ids) {
        res->FillWith(GLOBAL_FLAG(DefaultNeighborId), -1);
      } else {
        int32_t neighbor_size = neighbor_ids.Size();
        auto edge_ids = storage->GetOutEdges(src_id);
        auto padder = GetPadder(neighbor_ids, edge_ids);
        if (filters) {
          padder->SetFilter(filters[i]);
        }
        s = padder->Pad(res, count);
        if (!s.ok()) {
          return s;
        }
      }
    }
    return s;
  }
};

REGISTER_OPERATOR("TopkSampler", TopkSampler);

}  // namespace op
}  // namespace graphlearn
