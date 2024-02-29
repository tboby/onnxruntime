// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <unordered_map>

#include "core/common/common.h"
#include "core/platform/ort_mutex.h"
#include "core/providers/cuda/cuda_pch.h"

namespace onnxruntime {

using CudaGraphAnnotation_t = int;
using CudaGraphSet_t = std::unordered_map<CudaGraphAnnotation_t, cudaGraphExec_t>;

constexpr CudaGraphAnnotation_t kCudaGraphAnnotationSkip = -1;
constexpr CudaGraphAnnotation_t kCudaGraphAnnotationDefault = 0;

struct CudaGraphSet {
  CudaGraphSet(){};
  ~CudaGraphSet();

  bool IsEmpty() const;
  void Clear();
  bool Contains(CudaGraphAnnotation_t cuda_graph_annotation_id) const;
  void Put(CudaGraphAnnotation_t cuda_graph_annotation_id, cudaGraphExec_t graph_exec);
  bool Get(CudaGraphAnnotation_t cuda_graph_annotation_id, cudaGraphExec_t& graph_exec);

 private:
  CudaGraphSet_t cuda_graphs_;
};

struct CUDAGraph {
  CUDAGraph(){};
  CUDAGraph(cudaStream_t stream);
  ~CUDAGraph();

  void SetStream(cudaStream_t stream);
  void SetGraphAnnotationId(CudaGraphAnnotation_t cuda_graph_annotation_id);

  void CaptureBegin();
  void CaptureEnd();
  Status Replay();

  void Reset();

  bool IsGraphCaptureAllowedOnRun() const;
  bool IsGraphCaptured(CudaGraphAnnotation_t cuda_graph_annotation_id) const;

 private:
  cudaGraph_t graph_ = NULL;

  bool has_graph_ = false;
  bool has_graph_exec_ = false;

  CudaGraphSet cuda_graph_set_;
  CudaGraphAnnotation_t cuda_graph_annotation_id_ = kCudaGraphAnnotationDefault;

  cudaStream_t stream_ = nullptr;  // Does not own the stream
};

}  // namespace onnxruntime
