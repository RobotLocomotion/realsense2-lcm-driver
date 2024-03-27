#include "rgbd_sensor/rgbd_sensor.h"
#include <spdlog/fmt/ostr.h>
#include <drake/common/text_logging.h>

namespace rs2_lcm {

RGBDSensor::RGBDSensor(const std::vector<ImageType>& supported_types)
    : supported_types_(supported_types) {
  bool supports_depth = false;
  bool supports_color = false;

  for (const auto& from_type : supported_types) {
    for (const auto& to_type : supported_types) {
      set_extrinsics(from_type, to_type, Eigen::Isometry3f::Identity());
    }

    if (is_depth_image(from_type)) supports_depth = true;
    if (is_color_image(from_type)) supports_color = true;
  }

  if (!supports_depth) {
    throw std::runtime_error(
        "RGBDSensor has to support at least one depth ImageType.");
  }

  if (!supports_color) {
    throw std::runtime_error(
        "RGBDSensor has to support at least one color ImageType.");
  }
}

void RGBDSensor::Start(const std::vector<ImageType>& types) {
  for (const auto& type : types) {
    if (!supports(type)) {
      throw std::runtime_error("Does not support type: " +
                               ImageTypeToString(type));
    }
  }

  // Initialize the images and cloud to nullptrs.
  {
    std::map<const ImageType, TimeStampedImage> images;
    for (const auto& type : types) images[type] = TimeStampedImage();

    std::unique_lock<std::mutex> lock(data_lock_);
    images_ = images;
  }

  DoStart(types);

  std::vector<ImageType> enabled_types = get_enabled_image_types();

  // Prints intrinsics and extrinsics for all the enabled streams.
  for (const auto& type : enabled_types) {
    if (!has_intrinsics(type)) {
      throw std::runtime_error("Missing intrinsics for: " +
                               ImageTypeToString(type));
    }
    drake::log()->info("{} enabled with intrinsics: {}",
                       ImageTypeToString(type), get_intrinsics(type));

    for (const auto& to_type : enabled_types) {
      drake::log()->info("{} to {} with extrinsics:\n{}",
                         ImageTypeToString(type), ImageTypeToString(to_type),
                         get_extrinsics(type, to_type).matrix());
    }
  }
}

void RGBDSensor::Stop() {
  DoStop();

  // Clears all the images and point clouds.
  std::unique_lock<std::mutex> lock(data_lock_);
  images_ = {};
}

bool RGBDSensor::supports(const ImageType type) const {
  return std::find(supported_types_.begin(), supported_types_.end(), type) !=
         supported_types_.end();
}

void RGBDSensor::UpdateImages(
    const std::map<const ImageType, TimeStampedImage>& new_images) {
  std::unique_lock<std::mutex> lock(data_lock_);
  // Only update the new images.
  for (const auto& new_pair : new_images) {
    images_[new_pair.first] = new_pair.second;
  }
}

std::shared_ptr<const RawImageData> RGBDSensor::GetLatestImage(
    const ImageType type, uint64_t* timestamp) const {
  std::unique_lock<std::mutex> lock(data_lock_);
  auto it = images_.find(type);
  if (it == images_.end()) {
    *timestamp = 0;
    return nullptr;
  }
  const TimeStampedImage& image = it->second;
  *timestamp = image.timestamp;
  return image.data;
}

RawImageData DoRegisterDepthToColor(const Intrinsics& color_intrinsics,
                                    const Intrinsics& depth_intrinsics,
                                    const Eigen::Isometry3f& X_rgb_depth,
                                    const RawImageData& color,
                                    const RawImageData& depth,
                                    ImageType depth_type) {
  if (depth_intrinsics.width() != depth.cols() ||
      depth_intrinsics.height() != depth.rows()) {
    throw std::runtime_error("Depth image dimension mismatch");
  }
  if (depth.channels() != 1 || depth.scalar_size() != 2) {
    throw std::runtime_error("Depth image format is incorrect");
  }
  if (color_intrinsics.width() != color.cols() ||
      color_intrinsics.height() != color.rows()) {
    throw std::runtime_error("Color image dimension mismatch");
  }
  if (color.channels() != 3 || color.scalar_size() != 1) {
    throw std::runtime_error("Color image format is incorrect");
  }

  if (depth_type == ImageType::RECT_RGB_ALIGNED_DEPTH) {
    return depth;
  }

  RawImageData depth_registered(color.rows(), color.cols(), 1, 2);
  auto depth_registered_view = depth_registered.mutable_slice<uint16_t>();
  auto depth_view = depth.slice<uint16_t>();
  depth_registered_view.setZero();

  for (int v = 0; v < depth.rows(); v++) {
    for (int u = 0; u < depth.cols(); u++) {
      const float z = depth_view(v, u) / 1000.;

      // Skip over pixels with a depth value of zero, which is used to
      // indicate no data
      if (z == 0) {
        continue;
      }

      const Eigen::Vector3f P_depth =
          depth_intrinsics.BackProject(Eigen::Vector2f(u, v), z);
      const Eigen::Vector3f P_rgb = X_rgb_depth * P_depth;
      const Eigen::Vector2f p_rgb = color_intrinsics.Project(P_rgb);
      const int px = static_cast<int>(std::round(p_rgb(0)));
      const int py = static_cast<int>(std::round(p_rgb(1)));
      if (px >= 0 && py >= 0 && px < color.cols() && py < color.rows()) {
        if (depth_registered_view(py, px) == 0 ||
            static_cast<float>(depth_registered_view(py, px)) >
                P_rgb[2] * 1000.0) {
          depth_registered_view(py, px) =
              static_cast<uint16_t>(std::round(P_rgb[2] * 1000.0));
        }
      }
    }
  }
  // TODO(duy): Apply bilateral or Gaussian filter to interpolate pixels with no
  // depth due to discretization error
  return depth_registered;
}

}  // namespace rs2_lcm
