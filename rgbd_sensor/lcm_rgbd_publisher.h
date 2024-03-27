#pragma once

#include <map>
#include <string>
#include <vector>

#include <lcm/lcm-cpp.hpp>
#include "rgbd_sensor/rgbd_sensor.h"

namespace rs2_lcm {

/// Class which polls for updates from a camera and publishes the
/// images over lcm.
class LcmRgbdPublisher {
 public:
  /// @param types List of image types to publish.
  ///
  /// @param camera_name The name of this camera (will be published in
  /// description messages).
  ///
  /// @param lcm_channel_name The name of the LCM channel to publish
  /// camera descriptions on.
  ///
  /// @param lcm_channel_name The name of the LCM channel to publish
  /// images on.
  ///
  /// @param sensor Sensor to read images from.  This parameter is
  /// aliased and must be valid for the lifetime of this object.
  ///
  /// @param lcm An LCM object to use to publish messages.  This
  /// parameter is aliased and must be valid for the lifetime of this
  /// object
  LcmRgbdPublisher(const std::vector<ImageType>& types,
                   const std::string& camera_name,
                   const std::string& lcm_description_channel_name,
                   const std::string& lcm_channel_name,
                   const RGBDSensor* sensor,
                   lcm::LCM* lcm);

  ~LcmRgbdPublisher();

  /// Publish a description of this camera.
  void PublishDescription();

  /// Publish the current set of images.
  void PublishImages();

 private:
  const std::vector<ImageType> types_;
  const std::string camera_name_;
  const std::string lcm_description_channel_name_;
  const std::string lcm_channel_name_;
  const RGBDSensor* sensor_;
  bool enabled_software_registration_;

  lcm::LCM* lcm_{nullptr};
  int32_t seq_{0};
  std::map <ImageType, int32_t> image_seq_;
};

}  // namespace rs2_lcm
