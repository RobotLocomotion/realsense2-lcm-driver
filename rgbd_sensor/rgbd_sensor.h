#pragma once

#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Dense>
#include "rgbd_sensor/image.h"
#include "rgbd_sensor/intrinsics.h"

namespace rs2_lcm {

class RGBDSensor {
 public:
  virtual ~RGBDSensor() {}

  /// Starts streaming images from the camera.
  ///
  /// @param types which ImageTypes to enable on the camera.  Not all
  /// devices support all image types.
  ///
  void Start(const std::vector<ImageType>& types);

  void Stop();

  /**
   * For rgb image, the channels are in RGB order.
   * For depth image, each element is 16bits, in units of mm.
   * For ir image, each element is 16 bits.
   */
  std::shared_ptr<const RawImageData> GetLatestImage(const ImageType type,
                                                     uint64_t* timestamp) const;

  const std::vector<ImageType>& get_supported_image_types() const {
    return supported_types_;
  }

  std::vector<ImageType> get_enabled_image_types() const {
    const auto& supported = get_supported_image_types();
    std::vector<ImageType> enabled;
    for (const auto& type : supported) {
      if (is_enabled(type)) {
        enabled.push_back(type);
      }
    }
    return enabled;
  }

  /**
   * Returns true if @p type is supported.
   */
  bool supports(const ImageType type) const;

  /**
   * Returns true if @p type has been started by by Start().
   */
  virtual bool is_enabled(const ImageType type) const {
    std::unique_lock<std::mutex> lock(data_lock_);
    return images_.count(type);
  }

  /**
   * Returns true if there are intrinsics associated with @p type.
   */
  bool has_intrinsics(ImageType type) const {
    std::unique_lock<std::mutex> lock(params_lock_);
    return intrinsics_.count(type) > 0;
  }

  /**
   * Returns the intrinsics associated with @p type.
   * @throws std::runtime_error if there is none.
   */
  Intrinsics get_intrinsics(ImageType type) const {
    std::unique_lock<std::mutex> lock(params_lock_);
    return intrinsics_.at(type);
  }

  void set_intrinsics(ImageType type, const Intrinsics& intrinsics) {
    std::unique_lock<std::mutex> lock(params_lock_);
    intrinsics_[type] = intrinsics;
  }

  bool has_extrinsics(ImageType from, ImageType to) const {
    std::unique_lock<std::mutex> lock(params_lock_);
    return extrinsics_.count(std::pair<ImageType, ImageType>(from, to)) > 0;
  }

  /**
   * Returns X_to_from.
   * @throws std::runtime_error if there is no transformation between @p from
   * and @p to.
   */
  Eigen::Isometry3f get_extrinsics(ImageType from, ImageType to) const {
    std::unique_lock<std::mutex> lock(params_lock_);
    return extrinsics_.at(std::pair<ImageType, ImageType>(from, to));
  }

  /**
   * Sets the extrinsics of X_to_from to @p extrinsics, and X_from_to_ to
   * the inverse of @p extrinsics.
   */
  void set_extrinsics(ImageType from, ImageType to,
                      const Eigen::Isometry3f& extrinsics) {
    std::unique_lock<std::mutex> lock(params_lock_);
    extrinsics_[std::pair<ImageType, ImageType>(from, to)] = extrinsics;
    extrinsics_[std::pair<ImageType, ImageType>(to, from)] =
        extrinsics.inverse();
  }

  /// @return a string identifying the camera model (e.g. "realsense_d400").
  virtual std::string camera_model() const = 0;

  /// @return a unique string identifying the camera (typically a
  /// serial number).
  virtual const std::string& camera_id() const = 0;

 protected:
  template <typename DataType>
  struct TimeStampedData {
    DataType data{};
    uint64_t timestamp{0};
  };

  /**
   * @p supported_types has to contain at least one depth ImageType and one
   * color ImageType.
   * Defaults extrinsics between all supported ImageType to identity.
   * Derived class should override extrinsics_ when the information is
   * available.
   */
  explicit RGBDSensor(const std::vector<ImageType>& supported_types);

  typedef TimeStampedData<std::shared_ptr<const RawImageData>> TimeStampedImage;

  virtual void DoStart(const std::vector<ImageType>& types) = 0;
  virtual void DoStop() = 0;

  /**
   * This function adds all entries from @p images to images_.
   */
  void UpdateImages(
      const std::map<const ImageType, TimeStampedImage>& images);

 private:
  const std::vector<ImageType> supported_types_;

  mutable std::mutex params_lock_;
  std::map<ImageType, Intrinsics> intrinsics_;
  std::map<std::pair<ImageType, ImageType>, Eigen::Isometry3f> extrinsics_;

  mutable std::mutex data_lock_;
  std::map<const ImageType, TimeStampedImage> images_;
};

/**
 * Return a color-aligned depth image in the color camera frame.
 * @param depth_type ImageType of the input depth image. If it's
 * ImageType::RECT_RGB_ALIGNED_DEPTH, nothing will be done and a copy of
 * the already-aligned depth image will be returned.
 */
RawImageData DoRegisterDepthToColor(const Intrinsics& color_intrinsics,
                                    const Intrinsics& depth_intrinsics,
                                    const Eigen::Isometry3f& X_rgb_depth,
                                    const RawImageData& color,
                                    const RawImageData& depth,
                                    ImageType depth_type);
}  // namespace rs2_lcm
