// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <string>
#include <vector>

#include "paddle/fluid/eager/eager_tensor.h"
#include "paddle/fluid/eager/legacy/type_def.h"
#include "paddle/fluid/framework/ddim.h"
#include "paddle/fluid/framework/operator.h"
#include "paddle/fluid/framework/shape_inference.h"
#include "paddle/fluid/framework/type_defs.h"
#include "paddle/fluid/framework/var_type.h"
namespace egr {
namespace legacy {

class EagerInferShapeContext : public paddle::framework::InferShapeContext {
  using DDim = paddle::framework::DDim;

 public:
  EagerInferShapeContext(const NameTensorMap* in, const NameTensorMap* out,
                         const paddle::framework::AttributeMap* attr,
                         const paddle::framework::AttributeMap* default_attr,
                         const std::string op_type)
      : tensor_in_(in),
        tensor_out_(out),
        attrs_(attr),
        default_attrs_(default_attr),
        op_type_(op_type) {}

  bool HasInput(const std::string& name) const override {
    // has only one input
    auto it = tensor_in_->find(name);

    if (it == tensor_in_->end()) {
      return false;
    }
    const auto& in = it->second;
    if (in.size() == 0) return false;
    PADDLE_ENFORCE_EQ(
        in.size(), 1UL,
        paddle::platform::errors::PreconditionNotMet(
            "Input %s should not have more than one inputs", name));
    return in[0] != nullptr;
  }

  bool HasOutput(const std::string& name) const override {
    // has only one output
    auto it = tensor_out_->find(name);
    if (it == tensor_out_->end()) {
      return false;
    }
    const auto& out = it->second;
    if (out.size() == 0) {
      return false;
    }
    PADDLE_ENFORCE_EQ(
        out.size(), 1UL,
        paddle::platform::errors::PreconditionNotMet(
            "Output %s should not have more than one outputs", name));
    return out[0] != nullptr;
  }

  bool HasInputs(const std::string& name) const override {
    auto it = tensor_in_->find(name);
    if (it == tensor_in_->end() || it->second.empty()) {
      return false;
    }
    for (auto& input : it->second) {
      if (input == nullptr) {
        return false;
      }
    }
    return true;
  }

  bool HasOutputs(const std::string& name) const override {
    auto it = tensor_out_->find(name);
    if (it == tensor_out_->end() || it->second.empty()) {
      return false;
    }
    for (auto& output : it->second) {
      if (output == nullptr) {
        return false;
      }
    }
    return true;
  }

  paddle::framework::AttrReader Attrs() const override {
    return paddle::framework::AttrReader(*attrs_, *default_attrs_);
  }

  std::vector<std::string> Inputs(const std::string& name) const override {
    std::vector<std::string> vec_res;
    auto it = tensor_in_->find(name);
    PADDLE_ENFORCE_NE(
        it, tensor_in_->end(),
        paddle::platform::errors::NotFound("can not find [%s] in input", name));

    vec_res.reserve(it->second.size());
    for (auto& var : it->second) {
      if (var) {
        vec_res.push_back(var->name());
      } else {
        vec_res.push_back(paddle::framework::kEmptyVarName);
      }
    }

    return vec_res;
  }

  std::vector<std::string> Outputs(const std::string& name) const override {
    std::vector<std::string> vec_res;
    auto it = tensor_out_->find(name);
    PADDLE_ENFORCE_NE(it, tensor_out_->end(),
                      paddle::platform::errors::NotFound(
                          "can not find [%s] in output", name));

    vec_res.reserve(it->second.size());
    for (auto& var : it->second) {
      if (var) {
        vec_res.push_back(var->name());
      } else {
        vec_res.push_back(paddle::framework::kEmptyVarName);
      }
    }

    return vec_res;
  }
  std::string GetInputNameByIdx(size_t idx) const override {
    auto& op_proto =
        paddle::framework::OpInfoMap::Instance().Get(op_type_).proto_;
    PADDLE_ENFORCE_LT(idx, op_proto->inputs().size(),
                      paddle::platform::errors::OutOfRange(
                          "The index should be less than the size of inputs of "
                          "operator %s, but got index is %d and size is %d",
                          op_type_, idx, op_proto->inputs().size()));
    return op_proto->inputs()[idx].name();
  }

  std::string GetOutputNameByIdx(size_t idx) const override {
    auto& op_proto =
        paddle::framework::OpInfoMap::Instance().Get(op_type_).proto_;
    PADDLE_ENFORCE_LT(
        idx, op_proto->outputs().size(),
        paddle::platform::errors::OutOfRange(
            "The index should be less than the size of outputs of "
            "operator %s, but got index is %d and size is %d",
            op_type_, idx, op_proto->outputs().size()));
    return op_proto->outputs()[idx].name();
  }

  void ShareDim(const std::string& in, const std::string& out, size_t i = 0,
                size_t j = 0) override {
    auto in_it = tensor_in_->find(in);
    auto out_it = tensor_out_->find(out);
    PADDLE_ENFORCE_NE(
        in_it, tensor_in_->end(),
        paddle::platform::errors::NotFound("can not found [%s] in input", in));
    PADDLE_ENFORCE_GT(in_it->second.size(), i,
                      paddle::platform::errors::PreconditionNotMet(
                          "Inputs %s should have %llu argument", in, i));
    PADDLE_ENFORCE_NE(
        out_it, tensor_out_->end(),
        paddle::platform::errors::NotFound("can not found [%s] in input", in));
    PADDLE_ENFORCE_GT(out_it->second.size(), j,
                      paddle::platform::errors::PreconditionNotMet(
                          "Outputs %s should have %llu argument", out, j));

    paddle::framework::Variable* in_var = in_it->second[i]->MutableVar();
    paddle::framework::Variable* out_var = out_it->second[j]->MutableVar();

    PADDLE_ENFORCE_EQ(in_var->Type(), out_var->Type(),
                      paddle::platform::errors::PreconditionNotMet(
                          "The type of %s and %s is not the same.", in, out));

    if (in_var->IsType<paddle::framework::LoDTensor>()) {
      auto& in_lod_tensor = in_var->Get<paddle::framework::LoDTensor>();
      auto* out_lod_tensor =
          out_var->GetMutable<paddle::framework::LoDTensor>();
      out_lod_tensor->Resize(in_lod_tensor.dims());
    } else {
      auto& in_sele_rows = in_var->Get<paddle::framework::SelectedRows>();
      auto out_sele_rows =
          out_var->GetMutable<paddle::framework::SelectedRows>();
      out_sele_rows->mutable_value()->Resize(in_sele_rows.value().dims());
      out_sele_rows->set_rows(in_sele_rows.rows());
      out_sele_rows->set_height(in_sele_rows.height());
    }
  }

  void ShareAllLoD(const std::string& in,
                   const std::string& out) const override {
    // do nothing
  }
  void ShareLoD(const std::string& in, const std::string& out, size_t i = 0,
                size_t j = 0) const override {
    // do nothing
  }

  bool IsRuntime() const override { return true; }

  // TODO(paddle-dev): Can this be template?
  std::vector<paddle::framework::InferShapeVarPtr> GetInputVarPtrs(
      const std::string& name) override {
    PADDLE_THROW(paddle::platform::errors::PermissionDenied(
        "GetInputVarPtrs not support in dygraph runtime context"));
  }

  std::vector<paddle::framework::InferShapeVarPtr> GetOutputVarPtrs(
      const std::string& name) override {
    PADDLE_THROW(paddle::platform::errors::PermissionDenied(
        "GetOutputVarPtrs not support in dygraph runtime context"));
  }

  DDim GetInputDim(const std::string& name) const override {
    auto it = tensor_in_->find(name);
    PADDLE_ENFORCE_NE(
        it, tensor_in_->end(),
        paddle::platform::errors::NotFound("can not find [%s] in input", name));
    PADDLE_ENFORCE_EQ(
        it->second.size(), 1UL,
        paddle::platform::errors::PreconditionNotMet(
            "Input(%s) should hold one element, but now it holds %d", name,
            it->second.size()));
    return this->GetDim(it->second[0]->MutableVar());
  }

  std::vector<DDim> GetInputsDim(const std::string& name) const override {
    // const std::vector<Variable*>& vars = InputVars(name);
    std::vector<DDim> vec_res;
    auto it = tensor_in_->find(name);
    PADDLE_ENFORCE_NE(it, tensor_in_->end(),
                      paddle::platform::errors::NotFound(
                          "can not find [%s] in output", name));
    vec_res.reserve(it->second.size());
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i]) {
        vec_res.emplace_back(GetDim(it->second[i]->MutableVar()));
      } else {
        vec_res.emplace_back();
      }
    }

    return vec_res;
  }

  std::vector<paddle::framework::proto::VarType::Type> GetInputsVarType(
      const std::string& name) const override {
    std::vector<paddle::framework::proto::VarType::Type> vec_res;
    auto it = tensor_in_->find(name);
    PADDLE_ENFORCE_NE(
        it, tensor_in_->end(),
        paddle::platform::errors::NotFound("can not find [%s] in input", name));
    vec_res.reserve(it->second.size());
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i]) {
        vec_res.emplace_back(
            paddle::framework::ToVarType(it->second[i]->MutableVar()->Type()));
      } else {
        vec_res.emplace_back();
      }
    }
    return vec_res;
  }

  std::vector<paddle::framework::proto::VarType::Type> GetOutputsVarType(
      const std::string& name) const override {
    std::vector<paddle::framework::proto::VarType::Type> vec_res;
    auto it = tensor_out_->find(name);
    PADDLE_ENFORCE_NE(it, tensor_out_->end(),
                      paddle::platform::errors::NotFound(
                          "can not find [%s] in output", name));
    vec_res.reserve(it->second.size());
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i]) {
        vec_res.emplace_back(
            paddle::framework::ToVarType(it->second[i]->MutableVar()->Type()));
      } else {
        vec_res.emplace_back(
            static_cast<paddle::framework::proto::VarType::Type>(-1));
      }
    }
    return vec_res;
  }

  void SetOutputDim(const std::string& name, const DDim& dim) override {
    auto it = tensor_out_->find(name);
    PADDLE_ENFORCE_NE(it, tensor_out_->end(),
                      paddle::platform::errors::NotFound(
                          "can not find [%s] in output", name));

    if (it->second[0]) {
      SetDim(it->second[0]->MutableVar(), dim);
    }
  }

  void SetOutputsDim(const std::string& name,
                     const std::vector<DDim>& dims) override {
    auto it = tensor_out_->find(name);
    PADDLE_ENFORCE_NE(it, tensor_out_->end(),
                      paddle::platform::errors::NotFound(
                          "can not find [%s] in output", name));

    PADDLE_ENFORCE_EQ(dims.size(), it->second.size(),
                      paddle::platform::errors::InvalidArgument(
                          "The number of dims is expected to be equal to the "
                          "number of Outputs(%s). But receieved: the number of "
                          "dims = %d, the number of Outputs(%s) = %d.",
                          name, dims.size(), name, it->second.size()));

    for (size_t i = 0; i < dims.size(); ++i) {
      if (it->second[i]) {
        SetDim(it->second[i]->MutableVar(), dims[i]);
      }
    }
  }

  int32_t GetLoDLevel(const std::string& in, size_t i = 0) const override {
    PADDLE_THROW(paddle::platform::errors::PermissionDenied(
        "GetLoDLevel function not support in dygraph mode"));
  }

  void SetLoDLevel(const std::string& out, int32_t lod_level,
                   size_t j = 0) const override {
    PADDLE_THROW(paddle::platform::errors::PermissionDenied(
        "SetLoDLevel function not support in dygraph mode"));
  }

 protected:
  DDim GetDim(paddle::framework::Variable* var) const {
    PADDLE_ENFORCE_NOT_NULL(var, paddle::platform::errors::PreconditionNotMet(
                                     "Input variable should not be null"));
    if (var->IsType<paddle::framework::LoDTensor>()) {
      return var->Get<paddle::framework::LoDTensor>().dims();
    } else if (var->IsType<paddle::framework::SelectedRows>()) {
      return var->Get<paddle::framework::SelectedRows>().GetCompleteDims();
    } else {
      PADDLE_THROW(paddle::platform::errors::PermissionDenied(
          "Only LoDTensor/SelectedRows support 'GetDim', but Variables "
          "type_id is xx."));
    }
  }

  std::vector<DDim> GetRepeatedDims(const std::string& name) const override {
    PADDLE_THROW(paddle::platform::errors::PermissionDenied(
        "GetRepeatedDims not support in dygraph runtime"));
  }

  void SetDim(paddle::framework::Variable* var, const DDim& dim) {
    if (var->IsType<paddle::framework::LoDTensor>()) {
      var->GetMutable<paddle::framework::LoDTensor>()->Resize(dim);
    } else if (var->IsType<paddle::framework::SelectedRows>()) {
      var->GetMutable<paddle::framework::SelectedRows>()->set_height(dim[0]);
    } else {
      PADDLE_THROW(paddle::platform::errors::PermissionDenied(
          "Variable type_id %s, expect LoDTensor/SelectedRows."));
    }
  }

  void SetDims(const std::vector<paddle::framework::Variable*>& vars,
               const std::vector<DDim>& dims) {
    size_t length = vars.size();
    PADDLE_ENFORCE_EQ(
        length, dims.size(),
        paddle::platform::errors::PreconditionNotMet(
            "Vars number [%d] should be equal with dims number [%d]", length,
            dims.size()));
    for (size_t i = 0; i < length; ++i) {
      if (vars[i] == nullptr) {
        continue;
      }
      SetDim(vars[i], dims[i]);
    }
  }

  void SetRepeatedDims(const std::string& name,
                       const std::vector<DDim>& dims) override {
    PADDLE_THROW(paddle::platform::errors::PermissionDenied(
        "SetRepeatedDims not support in dygraph runtime"));
  }

 private:
  const NameTensorMap* tensor_in_;
  const NameTensorMap* tensor_out_;
  const paddle::framework::AttributeMap* attrs_;
  const paddle::framework::AttributeMap* default_attrs_;
  const std::string op_type_;
};

}  // namespace legacy
}  // namespace egr
