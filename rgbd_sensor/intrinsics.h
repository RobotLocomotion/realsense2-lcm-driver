#pragma once

#include <array>
#include <string>

#include <Eigen/Core>

namespace rs2_lcm {

/// Structure representing the intrinsic image properties for an
/// ImageType.
class Intrinsics {
 public:
  enum class DistortionModel {
    NONE = 0,
    MODIFIED_BROWN_CONRADY,
    INVERSE_BROWN_CONRADY,
    FTHETA,
    BROWN_CONRADY,
  };

  /**
   * Default constructor. All member variables defaults to zero.
   */
  Intrinsics() {}

  Intrinsics(int width, int height, float fx, float fy, float ppx, float ppy,
             DistortionModel model, const std::array<float, 5>& coeffs);

  /**
   * Distortion model defaults to NONE and, coeffecients to zero.
   */
  Intrinsics(int width, int height, float fx, float fy, float ppx, float ppy);

  /**
   * fx = intrinsics(0, 0), fy = intrinsics(1, 1), ppx = intrinsics(0, 2), and
   * ppy = intrinsics(1, 2), with default distortion related variables.
   */
  Intrinsics(int width, int height, const Eigen::Matrix3f& intrinsics);

  /**
   * Projects @p point onto the image plane.
   */
  Eigen::Vector2f Project(const Eigen::Vector3f& point) const;

  /**
   * Reconstruct a 3D point from a pixel and its associated depth value.
   */
  Eigen::Vector3f BackProject(const Eigen::Vector2f& pixel, float depth) const;

  // Getters
  DistortionModel distortion_model() const { return model_; }
  const std::array<float, 5>& distortion_coeffs() const { return coeffs_; }
  int width() const { return width_; }
  int height() const { return height_; }
  float fx() const { return fx_; }
  float fy() const { return fy_; }
  float ppx() const { return ppx_; }
  float ppy() const { return ppy_; }

  // Please do not provide partial setters for the member variables.

 private:
  int width_{0};
  int height_{0};
  float fx_{0.};   ///< Focal length x
  float fy_{0.};   ///< Focal length y
  float ppx_{0.};  ///< Principal point x
  float ppy_{0.};  ///< Principal point y

  DistortionModel model_{DistortionModel::NONE};
  std::array<float, 5> coeffs_{};
};

std::string to_string(Intrinsics::DistortionModel);

std::ostream& operator<<(std::ostream& os, const Intrinsics& in);

}  // namespace rs2_lcm
