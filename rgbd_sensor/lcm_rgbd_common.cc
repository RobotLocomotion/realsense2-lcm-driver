#include "rgbd_sensor/lcm_rgbd_common.h"

#include "rs2_lcm/image_description_t.hpp"

namespace rs2_lcm {

ImageType DescriptionTypeToImageType(int8_t type) {
  switch (type) {
    case rs2_lcm::image_description_t::RGB: {
      return ImageType::RGB;
    }
    case rs2_lcm::image_description_t::DEPTH: {
      return ImageType::DEPTH;
    }
    case rs2_lcm::image_description_t::IR: {
      return ImageType::IR;
    }
    case rs2_lcm::image_description_t::IR_STEREO: {
      return ImageType::IR_STEREO;
    }
    case rs2_lcm::image_description_t::RECT_RGB: {
      return ImageType::RECT_RGB;
    }
    case rs2_lcm::image_description_t::RECT_RGB_ALIGNED_DEPTH: {
      return ImageType::RECT_RGB_ALIGNED_DEPTH;
    }
    case rs2_lcm::image_description_t::DEPTH_ALIGNED_RGB: {
      return ImageType::DEPTH_ALIGNED_RGB;
    }
  }
  throw std::runtime_error("Unrecognized image type from LCM description");
}

int8_t ImageTypeToDescriptionType(ImageType type) {
  switch (type) {
    case ImageType::RGB: {
      return rs2_lcm::image_description_t::RGB;
    }
    case ImageType::DEPTH: {
      return rs2_lcm::image_description_t::DEPTH;
    }
    case ImageType::IR: {
      return rs2_lcm::image_description_t::IR;
    }
    case ImageType::IR_STEREO: {
      return rs2_lcm::image_description_t::IR_STEREO;
    }
    case ImageType::RECT_RGB: {
      return rs2_lcm::image_description_t::RECT_RGB;
    }
    case ImageType::RECT_RGB_ALIGNED_DEPTH: {
      return rs2_lcm::image_description_t::RECT_RGB_ALIGNED_DEPTH;
    }
    case ImageType::DEPTH_ALIGNED_RGB: {
      return rs2_lcm::image_description_t::DEPTH_ALIGNED_RGB;
    }
  }
  throw std::runtime_error("Unrecognized image type");
}

std::string ImageTypeToFrameName(ImageType type) {
  switch (type) {
    case ImageType::RGB: {
      return "color";
    }
    case ImageType::DEPTH: {
      return "depth";
    }
    case ImageType::IR: {
      return "infrared";
    }
    case ImageType::IR_STEREO: {
      return "infrared_stereo";
    }
    case ImageType::RECT_RGB: {
      return "rect_color";
    }
    case ImageType::RECT_RGB_ALIGNED_DEPTH: {
      return "rect_color_aligned_depth";
    }
    case ImageType::DEPTH_ALIGNED_RGB: {
      return "depth_aligned_rgb";
    }
  }
  throw std::runtime_error("Unrecognized image type");
}

ImageType FrameNameToImageType(const std::string& name) {
  if (name == "color") {
    return ImageType::RGB;
  } else if (name == "depth") {
    return ImageType::DEPTH;
  } else if (name == "infrared") {
    return ImageType::IR;
  } else if (name == "infrared_stereo") {
    return ImageType::IR_STEREO;
  } else if (name == "rect_color") {
    return ImageType::RECT_RGB;
  } else if (name == "rect_color_aligned_depth") {
    return ImageType::RECT_RGB_ALIGNED_DEPTH;
  } else if (name == "depth_aligned_rgb") {
    return ImageType::DEPTH_ALIGNED_RGB;
  }
  throw std::runtime_error("Unrecognized image type");
}

Intrinsics DeserializeIntrinsics(const intrinsics_t& intrinsics) {
  Intrinsics::DistortionModel distortion{};

  switch (intrinsics.distortion_model) {
    case intrinsics_t::DISTORTION_NONE: {
      distortion = Intrinsics::DistortionModel::NONE;
      break;
    }

    case intrinsics_t::DISTORTION_MODIFIED_BROWN_CONRADY: {
      distortion = Intrinsics::DistortionModel::MODIFIED_BROWN_CONRADY;
      break;
    }

    case intrinsics_t::DISTORTION_INVERSE_BROWN_CONRADY: {
      distortion = Intrinsics::DistortionModel::INVERSE_BROWN_CONRADY;
      break;
    }

    case intrinsics_t::DISTORTION_FTHETA: {
      distortion = Intrinsics::DistortionModel::FTHETA;
      break;
    }

    case intrinsics_t::DISTORTION_BROWN_CONRADY: {
      distortion = Intrinsics::DistortionModel::BROWN_CONRADY;
      break;
    }
    default: {
      throw std::runtime_error(
          std::string("Received intrinsics_t with unknown distortion "
                      "model: ") + std::to_string(static_cast<int>(
                          intrinsics.distortion_model)));
    }
  }

  std::array<float, 5> coeffs;
  memcpy(coeffs.data(), intrinsics.distortion_coeffs, 5 * sizeof(float));

  return Intrinsics(
      intrinsics.width,
      intrinsics.height,
      intrinsics.focal_length_x,
      intrinsics.focal_length_y,
      intrinsics.principal_point_x,
      intrinsics.principal_point_y,
      distortion, coeffs);
}

intrinsics_t SerializeIntrinsics(const Intrinsics& intrinsics) {
  intrinsics_t ret{};
  ret.width = intrinsics.width();
  ret.height = intrinsics.height();
  ret.focal_length_x = intrinsics.fx();
  ret.focal_length_y = intrinsics.fy();
  ret.principal_point_x = intrinsics.ppx();
  ret.principal_point_y = intrinsics.ppy();

  switch (intrinsics.distortion_model()) {
    case Intrinsics::DistortionModel::NONE: {
      ret.distortion_model = intrinsics_t::DISTORTION_NONE;
      break;
    }
    case Intrinsics::DistortionModel::MODIFIED_BROWN_CONRADY: {
      ret.distortion_model = intrinsics_t::DISTORTION_MODIFIED_BROWN_CONRADY;
      break;
    }
    case Intrinsics::DistortionModel::INVERSE_BROWN_CONRADY: {
      ret.distortion_model = intrinsics_t::DISTORTION_INVERSE_BROWN_CONRADY;
      break;
    }
    case Intrinsics::DistortionModel::FTHETA: {
      ret.distortion_model = intrinsics_t::DISTORTION_FTHETA;
      break;
    }
    case Intrinsics::DistortionModel::BROWN_CONRADY: {
      ret.distortion_model = intrinsics_t::DISTORTION_BROWN_CONRADY;
      break;
    }
  }

  memcpy(ret.distortion_coeffs, intrinsics.distortion_coeffs().data(),
         5 * sizeof(float));

  return ret;
}

void DeserializeExtrinsics(const extrinsics_t& extrinsics,
                           ImageType* from, ImageType* to,
                           Eigen::Isometry3f* isometry) {
  *from = DescriptionTypeToImageType(extrinsics.image_from);
  *to = DescriptionTypeToImageType(extrinsics.image_to);
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      isometry->linear()(row, col) = extrinsics.rotation[col * 3 + row];
    }
    isometry->translation()[row] = extrinsics.translation[row];
  }
  isometry->makeAffine();
}

extrinsics_t SerializeExtrinsics(ImageType from, ImageType to,
                                 const Eigen::Isometry3f& isometry) {
  extrinsics_t extrinsics{};

  extrinsics.image_from = ImageTypeToDescriptionType(from);
  extrinsics.image_to = ImageTypeToDescriptionType(to);
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      extrinsics.rotation[col * 3 + row] = isometry.linear()(row, col);
    }
    extrinsics.translation[row] = isometry.translation()[row];
  }
  return extrinsics;
}

}  // namespace rs2_lcm
