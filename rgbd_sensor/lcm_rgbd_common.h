#pragma once

#include <array>
#include <string>

#include "rgbd_sensor/rgbd_sensor.h"
#include "rs2_lcm/extrinsics_t.hpp"
#include "rs2_lcm/intrinsics_t.hpp"

namespace rs2_lcm {

/// Convert image type from rs2_lcm::image_description_t to ImageType.
ImageType DescriptionTypeToImageType(int8_t type);

/// Convert image type from ImageType to rs2_lcm::image_description_t.
int8_t ImageTypeToDescriptionType(ImageType type);

/// Convert image type from ImageType to frame_name (as used in
/// rs2_lcm::image_description_t and lcmt_image::header)
std::string ImageTypeToFrameName(ImageType type);
ImageType FrameNameToImageType(const std::string& name);

Intrinsics DeserializeIntrinsics(const intrinsics_t& intrinsics);

intrinsics_t SerializeIntrinsics(const Intrinsics& intrinsics);

void DeserializeExtrinsics(const extrinsics_t& extrinsics,
                           ImageType* from, ImageType* to,
                           Eigen::Isometry3f* isometry);

extrinsics_t SerializeExtrinsics(ImageType from, ImageType to,
                                 const Eigen::Isometry3f& isometry);

}  // namespace rs2_lcm
