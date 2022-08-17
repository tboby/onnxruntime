// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "python/tools/kernel_explorer/kernels/gemm_rocblas.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>

#include "core/providers/rocm/rocm_common.h"
#include "core/providers/rocm/shared_inc/fpgeneric.h"
#include "python/tools/kernel_explorer/device_array.h"
#include "python/tools/kernel_explorer/kernels/gemm.h"

namespace py = pybind11;

namespace onnxruntime {

// to be moved to onnxruntime once we have a monolithicly tunable gemm wrapper and it is enabled for onnxruntime
template <typename T>
Status RocBlasGemmOp(const GemmParams<T>* params) {
  // NOTE: rocblas assumes the storage is column-majored, swapping A and B makes it have the same interface
  // as those with row-majored convention. That is, if you treat the storage as row-majored but view the matrices as
  // transposed, then by using the property Transpose(A*B) = Tranpose(B)*Transpose(A), the correctness is obvious.
  auto status = rocblasGemmHelper(
      params->handle,
      params->opb == BlasOp::N ? rocblas_operation_none : rocblas_operation_transpose,
      params->opa == BlasOp::N ? rocblas_operation_none : rocblas_operation_transpose,
      params->n, params->m, params->k,
      &(params->alpha),
      params->b, params->ldb,
      params->a, params->lda,
      &(params->beta),
      params->c, params->ldc);
  ORT_RETURN_IF(status != rocblas_status_success, rocblas_status_to_string(status));
  return Status::OK();
}

template <typename T>
class RocBlasGemm : public IKernelExplorer {
 public:
  RocBlasGemm(BlasOp opa, BlasOp opb,
              int64_t m, int64_t n, int64_t k,
              double alpha,
              DeviceArray& a, int64_t lda,
              DeviceArray& b, int64_t ldb,
              double beta,
              DeviceArray& c, int64_t ldc) {
    ROCBLAS_CALL_THROW(rocblas_create_handle(&rocblas_handle_));
    params_.handle = rocblas_handle_;
    params_.opa = opa;
    params_.opb = opb;
    params_.m = m;
    params_.n = n;
    params_.k = k;
    params_.alpha = alpha;
    params_.a = static_cast<T*>(a.ptr());
    params_.lda = lda;
    params_.b = static_cast<T*>(b.ptr());
    params_.ldb = ldb;
    params_.beta = beta;
    params_.c = static_cast<T*>(c.ptr());
    params_.ldc = ldc;
  }

  ~RocBlasGemm() {
    ROCBLAS_CALL_THROW(rocblas_destroy_handle(rocblas_handle_));
    rocblas_handle_ = nullptr;
  }

  void Run() override {
    ORT_THROW_IF_ERROR(impl_(&params_));
  }

  std::vector<std::string> ListImpls() const {
    return {"Rocblas"};
  }

  bool SelectImpl(const std::string& name) {
    return name == "Rocblas";
  }

 private:
  rocblas_handle rocblas_handle_;

  using ParamsT = GemmParams<T>;
  using OpT = contrib::rocm::Op<ParamsT>;

  ParamsT params_{};
  OpT impl_{RocBlasGemmOp<T>};
};

void InitRocBlasGemm(py::module mod) {
  // float
  py::class_<RocBlasGemm<float>>(mod, "RocblasGemm_float")
      .def(py::init<BlasOp, BlasOp, int64_t, int64_t, int64_t, double,
                    DeviceArray&, int64_t, DeviceArray&, int64_t, double, DeviceArray&, int64_t>())
      .def("SetRepeats", &RocBlasGemm<float>::SetRepeats)
      .def("Profile", &RocBlasGemm<float>::Profile)
      .def("Run", &RocBlasGemm<float>::Run)
      .def("ListImpls", &RocBlasGemm<float>::ListImpls)
      .def("SelectImpl", &RocBlasGemm<float>::SelectImpl);

  // half
  py::class_<RocBlasGemm<half>>(mod, "RocblasGemm_half")
      .def(py::init<BlasOp, BlasOp, int64_t, int64_t, int64_t, double,
                    DeviceArray&, int64_t, DeviceArray&, int64_t, double, DeviceArray&, int64_t>())
      .def("SetRepeats", &RocBlasGemm<half>::SetRepeats)
      .def("Profile", &RocBlasGemm<half>::Profile)
      .def("Run", &RocBlasGemm<half>::Run)
      .def("ListImpls", &RocBlasGemm<half>::ListImpls)
      .def("SelectImpl", &RocBlasGemm<half>::SelectImpl);
}

}  // namespace onnxruntime
