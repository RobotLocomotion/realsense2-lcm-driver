#include "rgbd_sensor/intrinsics.h"

#include <array>

#include <Eigen/Core>

namespace rs2_lcm {

Intrinsics::Intrinsics(int width, int height, float fx, float fy, float ppx,
                       float ppy, DistortionModel model,
                       const std::array<float, 5>& coeffs)
    : width_(width),
      height_(height),
      fx_(fx),
      fy_(fy),
      ppx_(ppx),
      ppy_(ppy),
      model_(model),
      coeffs_(coeffs) {}

Intrinsics::Intrinsics(int width, int height, float fx, float fy, float ppx,
                       float ppy)
    : Intrinsics(width, height, fx, fy, ppx, ppy, DistortionModel::NONE, {}) {}

Intrinsics::Intrinsics(int width, int height, const Eigen::Matrix3f& intrinsics)
    : Intrinsics(width, height, intrinsics(0, 0), intrinsics(1, 1),
                 intrinsics(0, 2), intrinsics(1, 2)) {}

Eigen::Vector2f Intrinsics::Project(const Eigen::Vector3f& point) const {
  float x = point[0] / point[2];
  float y = point[1] / point[2];

  if (model_ == DistortionModel::MODIFIED_BROWN_CONRADY) {
    float r2 = x * x + y * y;
    float f =
        1 + coeffs_[0] * r2 + coeffs_[1] * r2 * r2 + coeffs_[4] * r2 * r2 * r2;
    x *= f;
    y *= f;
    float dx = x + 2 * coeffs_[2] * x * y + coeffs_[3] * (r2 + 2 * x * x);
    float dy = y + 2 * coeffs_[3] * x * y + coeffs_[2] * (r2 + 2 * y * y);
    x = dx;
    y = dy;
  } else if (model_ == DistortionModel::FTHETA) {
    float r = sqrt(x * x + y * y);
    float rd = (1.0f / coeffs_[0] * atan(2 * r * tan(coeffs_[0] / 2.0f)));
    x *= rd / r;
    y *= rd / r;
  }

  return Eigen::Vector2f(x * fx_ + ppx_, y * fy_ + ppy_);
}

Eigen::Vector3f Intrinsics::BackProject(const Eigen::Vector2f& pixel,
                                        float depth) const {
  if (model_ == DistortionModel::MODIFIED_BROWN_CONRADY) {
    throw std::runtime_error("Cannot back project with MODIFIED_BROWN_CONRADY");
  }
  if (model_ == DistortionModel::FTHETA) {
    throw std::runtime_error("Cannot back project with FTHETA");
  }

  float x = (pixel[0] - ppx_) / fx_;
  float y = (pixel[1] - ppy_) / fy_;
  if (model_ == DistortionModel::INVERSE_BROWN_CONRADY) {
    float r2 = x * x + y * y;
    float f =
        1 + coeffs_[0] * r2 + coeffs_[1] * r2 * r2 + coeffs_[4] * r2 * r2 * r2;
    float ux = x * f + 2 * coeffs_[2] * x * y + coeffs_[3] * (r2 + 2 * x * x);
    float uy = y * f + 2 * coeffs_[3] * x * y + coeffs_[2] * (r2 + 2 * y * y);
    x = ux;
    y = uy;
  }
  return Eigen::Vector3f(depth * x, depth * y, depth);
}

std::ostream& operator<<(std::ostream& os, const Intrinsics& in) {
  os << "w: " << in.width() << ", h: " << in.height() << ", fx: " << in.fx()
     << ", fy: " << in.fy() << ", ppx: " << in.ppx() << ", ppy: " << in.ppy()
     << ", distortion model: " << to_string(in.distortion_model())
     << ", coeffs: ";
  const auto& coeffs = in.distortion_coeffs();
  for (size_t i = 0; i < coeffs.size(); i++) {
    if (i != coeffs.size() - 1) {
      os << coeffs.at(i) << ", ";
    } else {
      os << coeffs.at(i);
    }
  }
  return os;
}

std::string to_string(Intrinsics::DistortionModel model) {
  switch (model) {
    case Intrinsics::DistortionModel::NONE:
      return "NONE";
    case Intrinsics::DistortionModel::MODIFIED_BROWN_CONRADY:
      return "MODIFIED_BROWN_CONRADY";
    case Intrinsics::DistortionModel::INVERSE_BROWN_CONRADY:
      return "INVERSE_BROWN_CONRADY";
    case Intrinsics::DistortionModel::FTHETA:
      return "FTHETA";
    case Intrinsics::DistortionModel::BROWN_CONRADY:
      return "BROWN_CONRADY";
    default:
      throw std::runtime_error("Invalid DistortionModel");
  }
}

}  // namespace rs2_lcm
