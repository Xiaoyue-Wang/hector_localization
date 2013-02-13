//=================================================================================================
// Copyright (c) 2011, Johannes Meyer, TU Darmstadt
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the Flight Systems and Automatic Control group,
//       TU Darmstadt, nor the names of its contributors may be used to
//       endorse or promote products derived from this software without
//       specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//=================================================================================================

#include <hector_pose_estimation/measurements/height.h>
#include <hector_pose_estimation/pose_estimation.h>
#include <hector_pose_estimation/global_reference.h>
#include <ros/console.h>

#include <boost/bind.hpp>

namespace hector_pose_estimation {

HeightModel::HeightModel()
{
  stddev_ = 10.0;
  elevation_ = 0.0;
  parameters().add("stddev", stddev_);
}

HeightModel::~HeightModel() {}

SystemStatus HeightModel::getStatusFlags() const {
  return STATE_Z_POSITION;
}

void HeightModel::getMeasurementNoise(NoiseVariance& R, const State&, bool init)
{
  if (init) {
    R(0,0) = pow(stddev_, 2);
  }
}

void HeightModel::getExpectedValue(MeasurementVector& y_pred, const State& state)
{
  y_pred(0) = state.getPosition().z() + getElevation();
}

void HeightModel::getStateJacobian(MeasurementMatrix& C, const State& state)
{
  if (state.getPositionIndex() >= 0) {
    C(0,State::POSITION_Z) = 1.0;
  }
}

HeightBaroCommon::HeightBaroCommon(Measurement* parent)
{
  auto_elevation_ = true;
  elevation_initialized_ = false;
  parent->parameters().add("auto_elevation", auto_elevation_);
}

HeightBaroCommon::~HeightBaroCommon() {}

void HeightBaroCommon::onReset() {
  elevation_initialized_ = false;
}

double HeightBaroCommon::resetElevation(const State &state, boost::function<double()> altitude_func) {
  if (!elevation_initialized_) {
    GlobalReference::Instance()->setAltitude(altitude_func() - state.getPosition().z());
    elevation_initialized_ = true;
    ROS_INFO("Set new reference altitude to %f", GlobalReference::Instance()->position().altitude);
  }

  return  GlobalReference::Instance()->position().altitude;
}

void Height::onReset() {
  HeightBaroCommon::onReset();
}

template <typename T> struct functor_wrapper
{
  functor_wrapper(const T& value) : value(value) {}
  T& operator()() { return value; }
  const T& operator()() const { return value; }
private:
  T value;
};

bool Height::beforeUpdate(State &state, const Update_<HeightModel> &update) {
  setElevation(resetElevation(state, functor_wrapper<double>(update.getVector()(0))));
  return true;
}

} // namespace hector_pose_estimation
