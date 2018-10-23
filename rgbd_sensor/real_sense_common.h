#pragma once

#include "rgbd_sensor/rgbd_sensor.h"

namespace rs2_lcm {
namespace real_sense {

// Real sense's matrix is color major.
template <typename RsExtrinsics>
Eigen::Isometry3f rs_extrinsics_to_eigen(const RsExtrinsics& ext) {
  Eigen::Isometry3f ret = Eigen::Isometry3f::Identity();
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      ret.linear()(row, col) = ext.rotation[col * 3 + row];
    }
    ret.translation()[row] = ext.translation[row];
  }
  return ret;
}

template <typename RsExtrinsics>
RsExtrinsics eigen_to_rs_extrinsics(const Eigen::Isometry3f& tf) {
  RsExtrinsics ret{};
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      ret.rotation[col * 3 + row] = tf.linear()(row, col);
    }
    ret.translation[row] = tf.translation()[row];
  }
  return ret;
}

}  // namespace real_sense
}  // namespace rs2_lcm
