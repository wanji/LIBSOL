/*********************************************************************************
*     File Name           :     online_linear_model.cc
*     Created By          :     yuewu
*     Creation Date       :     [2016-02-18 16:47]
*     Last Modified       :     [2016-03-09 19:23]
*     Description         :     online linear model
**********************************************************************************/
#include "lsol/model/online_linear_model.h"

#include <algorithm>

#include "lsol/util/util.h"

using namespace std;
using namespace lsol::math;
using namespace lsol::pario;

namespace lsol {
namespace model {

OnlineLinearModel::OnlineLinearModel(int class_num)
    : OnlineModel(class_num, "online_linear"),
      weights_(nullptr),
      gradients_(nullptr) {
  this->weights_ = new Vector<real_t>[this->clf_num_];
  this->gradients_ = new real_t[this->clf_num_];

  for (int i = 0; i < this->clf_num_; ++i) {
    this->weights(i).resize(this->dim_);
  }
}

OnlineLinearModel::~OnlineLinearModel() {
  DeleteArray(this->weights_);
  DeleteArray(this->gradients_);
}

void OnlineLinearModel::BeginTrain() {
  OnlineModel::BeginTrain();

  // reset weight vector
  for (int i = 0; i < this->clf_num_; ++i) {
    this->weights(i) = 0;
    this->gradients_[i] = 0;
  }
}

label_t OnlineLinearModel::Iterate(const DataPoint& x, float* predict) {
  OnlineModel::Iterate(x, predict);

  label_t label = this->Predict(x, predict);
  label_t gd_label = this->CalibrateLabel(x.label());
  if (this->aggressive_ || label != gd_label) {
    this->loss_->gradient(gd_label, predict, this->gradients_, this->clf_num_);
    this->Update(x);
  }
  return label;
}

label_t OnlineLinearModel::Predict(const pario::DataPoint& x, float* predicts) {
  size_t feat_num = x.indexes().size();
  for (int c = 0; c < this->clf_num_; ++c) {
    Vector<real_t>& w = this->weights(c);
    predicts[c] = expr::dotmul(w, x.data()) + w[0];
  }
  if (this->clf_num_ == 1) {
    return loss::Loss::Sign(*predicts);
  } else {
    return label_t(max_element(predicts, predicts + this->clf_num_) - predicts);
  }
}

void OnlineLinearModel::update_dim(index_t dim) {
  if (dim >= this->dim_) {
    ++dim;  // reserve the 0-th
    for (int i = 0; i < this->clf_num_; ++i) {
      auto& w = this->weights(i);
      w.resize(dim);
      // set the new value to zero
      for (real_t* iter = w.begin() + this->dim_; iter != w.end(); ++iter)
        *iter = 0;
    }
    OnlineModel::update_dim(dim);
  }
}

void OnlineLinearModel::GetModelParam(Json::Value& root) const {
  ostringstream oss;
  for (int c = 0; c < this->clf_num_; ++c) {
    oss << this->weights(c) << "\n";
  }
  root["params"] = oss.str();
}

int OnlineLinearModel::SetModelParam(const Json::Value& root) {
  istringstream iss(root["params"].asString());
  for (int c = 0; c < this->clf_num_; ++c) {
    iss >> this->weights(c);
  }
  return Status_OK;
}

}  // namespace model
}  // namespace lsol