/* Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
#include "paddle/pten/core/convert_utils.h"

// See Note [ Why still include the fluid headers? ]
#include "paddle/fluid/platform/device/gpu/gpu_info.h"

namespace pten {

// TODO(chenweihang): Add other place trans cases later
Backend TransToPtenBackend(const paddle::platform::Place& place) {
  if (paddle::platform::is_cpu_place(place)) {
    return Backend::CPU;
  } else if (paddle::platform::is_gpu_place(place)) {
    return Backend::CUDA;
  } else {
    return Backend::UNDEFINED;
  }
}

paddle::experimental::DataType TransToPtenDataType(
    const paddle::framework::proto::VarType::Type& dtype) {
  // Set the order of case branches according to the frequency with
  // the data type is used
  switch (dtype) {
    case paddle::framework::proto::VarType::FP32:
      return DataType::FLOAT32;
    case paddle::framework::proto::VarType::FP64:
      return DataType::FLOAT64;
    case paddle::framework::proto::VarType::INT64:
      return DataType::INT64;
    case paddle::framework::proto::VarType::INT32:
      return DataType::INT32;
    case paddle::framework::proto::VarType::INT8:
      return DataType::INT8;
    case paddle::framework::proto::VarType::UINT8:
      return DataType::UINT8;
    case paddle::framework::proto::VarType::INT16:
      return DataType::INT16;
    case paddle::framework::proto::VarType::COMPLEX64:
      return DataType::COMPLEX64;
    case paddle::framework::proto::VarType::COMPLEX128:
      return DataType::COMPLEX128;
    case paddle::framework::proto::VarType::FP16:
      return DataType::FLOAT16;
    case paddle::framework::proto::VarType::BF16:
      return DataType::BFLOAT16;
    case paddle::framework::proto::VarType::BOOL:
      return DataType::BOOL;
    default:
      return DataType::UNDEFINED;
  }
}

DataLayout TransToPtenDataLayout(const paddle::framework::DataLayout& layout) {
  switch (layout) {
    case paddle::framework::DataLayout::kNHWC:
      return DataLayout::NHWC;
    case paddle::framework::DataLayout::kNCHW:
      return DataLayout::NCHW;
    case paddle::framework::DataLayout::kAnyLayout:
      return DataLayout::ANY;
    case paddle::framework::DataLayout::kMKLDNN:
      return DataLayout::MKLDNN;
    default:
      return DataLayout::UNDEFINED;
  }
}

paddle::platform::Place TransToFluidPlace(const Backend& backend) {
  // TODO(chenweihang): add other trans cases later
  switch (backend) {
    case pten::Backend::CPU:
      return paddle::platform::CPUPlace();
#if defined(PADDLE_WITH_CUDA) || defined(PADDLE_WITH_HIP)
    case pten::Backend::CUDA:
      return paddle::platform::CUDAPlace(
          paddle::platform::GetCurrentDeviceId());
#endif
#ifdef PADDLE_WITH_MKLDNN
    case pten::Backend::MKLDNN:
      return paddle::platform::CPUPlace();
#endif
#if defined(PADDLE_WITH_CUDA) || defined(PADDLE_WITH_HIP)
    case pten::Backend::CUDNN:
      return paddle::platform::CUDAPlace(
          paddle::platform::GetCurrentDeviceId());
#endif
    default:
      PADDLE_THROW(paddle::platform::errors::Unimplemented(
          "Unsupported backend `%s` when casting it to paddle place type.",
          backend));
  }
}

paddle::framework::proto::VarType::Type TransToProtoVarType(
    const paddle::experimental::DataType& dtype) {
  // Set the order of case branches according to the frequency with
  // the data type is used
  switch (dtype) {
    case DataType::FLOAT32:
      return paddle::framework::proto::VarType::FP32;
    case DataType::FLOAT64:
      return paddle::framework::proto::VarType::FP64;
    case DataType::INT64:
      return paddle::framework::proto::VarType::INT64;
    case DataType::INT32:
      return paddle::framework::proto::VarType::INT32;
    case DataType::INT8:
      return paddle::framework::proto::VarType::INT8;
    case DataType::UINT8:
      return paddle::framework::proto::VarType::UINT8;
    case DataType::INT16:
      return paddle::framework::proto::VarType::INT16;
    case DataType::COMPLEX64:
      return paddle::framework::proto::VarType::COMPLEX64;
    case DataType::COMPLEX128:
      return paddle::framework::proto::VarType::COMPLEX128;
    case DataType::FLOAT16:
      return paddle::framework::proto::VarType::FP16;
    case DataType::BFLOAT16:
      return paddle::framework::proto::VarType::BF16;
    case DataType::BOOL:
      return paddle::framework::proto::VarType::BOOL;
    default:
      PADDLE_THROW(paddle::platform::errors::Unimplemented(
          "Unsupported data type `%s` when casting it into "
          "paddle data type.",
          dtype));
  }
}

paddle::framework::DataLayout TransToFluidDataLayout(const DataLayout& layout) {
  switch (layout) {
    case DataLayout::NHWC:
      return paddle::framework::DataLayout::kNHWC;
    case DataLayout::NCHW:
      return paddle::framework::DataLayout::kNCHW;
    case DataLayout::ANY:
      return paddle::framework::DataLayout::kAnyLayout;
    case DataLayout::MKLDNN:
      return paddle::framework::DataLayout::kMKLDNN;
    default:
      PADDLE_THROW(paddle::platform::errors::Unimplemented(
          "Unsupported data layout `%s` when casting it into "
          "paddle data layout.",
          layout));
  }
}

paddle::framework::LoD TransToFluidLoD(const pten::LoD& lod) {
  paddle::framework::LoD out;
  out.reserve(lod.size());

  for (auto& elem : lod) {
    out.emplace_back(elem);
  }
  return out;
}

pten::LoD TransToPtenLoD(const paddle::framework::LoD& lod) {
  pten::LoD out;
  out.reserve(lod.size());

  for (auto& elem : lod) {
    out.emplace_back(elem);
  }
  return out;
}

size_t DataTypeSize(DataType dtype) {
  switch (dtype) {
    case DataType::UNDEFINED:
      return 0;
    case DataType::BOOL:
      return sizeof(bool);
    case DataType::INT8:
      return sizeof(int8_t);
    case DataType::UINT8:
      return sizeof(uint8_t);
    case DataType::INT16:
      return sizeof(int16_t);
    case DataType::INT32:
      return sizeof(int);
    case DataType::INT64:
      return sizeof(int64_t);
    case DataType::FLOAT16:
      return sizeof(paddle::platform::float16);
    case DataType::FLOAT32:
      return sizeof(float);
    case DataType::FLOAT64:
      return sizeof(double);
    case DataType::COMPLEX64:
      return sizeof(paddle::platform::complex<float>);
    case DataType::COMPLEX128:
      return sizeof(paddle::platform::complex<double>);
    default:
      return 0;
  }
}

DataType String2DataType(const std::string& str) {
  if (str == "bool") {
    return DataType::BOOL;
  } else if (str == "float16") {
    return DataType::FLOAT16;
  } else if (str == "float32") {
    return DataType::FLOAT32;
  } else if (str == "float64") {
    return DataType::FLOAT64;
  } else if (str == "int8") {
    return DataType::INT8;
  } else if (str == "int16") {
    return DataType::INT16;
  } else if (str == "int32") {
    return DataType::INT32;
  } else if (str == "int64") {
    return DataType::INT64;
  } else if (str == "uint8") {
    return DataType::UINT8;
  } else if (str == "complex64") {
    return DataType::COMPLEX64;
  } else if (str == "complex128") {
    return DataType::COMPLEX128;
  } else {
    return DataType::UNDEFINED;
  }
}

std::string DataType2String(DataType dtype) {
  switch (dtype) {
    case DataType::BOOL:
      return "bool";
    case DataType::INT8:
      return "int8";
    case DataType::UINT8:
      return "uint8";
    case DataType::INT16:
      return "int16";
    case DataType::INT32:
      return "int32";
    case DataType::INT64:
      return "int64";
    case DataType::FLOAT16:
      return "float16";
    case DataType::FLOAT32:
      return "float32";
    case DataType::FLOAT64:
      return "float64";
    case DataType::COMPLEX64:
      return "complex64";
    case DataType::COMPLEX128:
      return "complex128";
    default:
      PADDLE_THROW(paddle::platform::errors::InvalidArgument(
          "Unknow pten::DataType, the int value = %d.",
          static_cast<int>(dtype)));
      return "";
  }
}

}  // namespace pten
