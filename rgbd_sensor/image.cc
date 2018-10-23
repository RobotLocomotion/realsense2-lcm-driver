#include "rgbd_sensor/image.h"

namespace rs2_lcm {

std::string ImageTypeToString(const ImageType type) {
  switch (type) {
    case ImageType::RGB:
      return "RGB";
    case ImageType::DEPTH:
      return "DEPTH";
    case ImageType::IR:
      return "INFRARED";
    case ImageType::IR_STEREO:
      return "INFRARED_STEREO";
    case ImageType::RECT_RGB:
      return "RECTIFIED_RGB";
    case ImageType::RECT_RGB_ALIGNED_DEPTH:
      return "DEPTH_ALIGNED_TO_RECTIFIED_RGB";
    case ImageType::DEPTH_ALIGNED_RGB:
      return "RGB_ALIGNED_TO_DEPTH";
    default:
      throw std::runtime_error("Unknown ImageType");
  }
}

bool is_color_image(const ImageType type) {
  switch (type) {
    case ImageType::RGB:
    case ImageType::RECT_RGB:
    case ImageType::DEPTH_ALIGNED_RGB:
      return true;
    case ImageType::IR:
    case ImageType::IR_STEREO:
    case ImageType::DEPTH:
    case ImageType::RECT_RGB_ALIGNED_DEPTH:
      return false;
    default:
      throw std::runtime_error("Unknown ImageType");
  }
}

bool is_depth_image(const ImageType type) {
  switch (type) {
    case ImageType::IR:
    case ImageType::IR_STEREO:
    case ImageType::RGB:
    case ImageType::RECT_RGB:
    case ImageType::DEPTH_ALIGNED_RGB:
      return false;
    case ImageType::DEPTH:
    case ImageType::RECT_RGB_ALIGNED_DEPTH:
      return true;
    default:
      throw std::runtime_error("Unknown ImageType");
  }
}

bool is_infrared_image(const ImageType type) {
  switch (type) {
    case ImageType::IR:
    case ImageType::IR_STEREO:
      return true;
    case ImageType::RGB:
    case ImageType::RECT_RGB:
    case ImageType::DEPTH_ALIGNED_RGB:
    case ImageType::DEPTH:
    case ImageType::RECT_RGB_ALIGNED_DEPTH:
      return false;
    default:
      throw std::runtime_error("Unknown ImageType");
  }
}

RawImageData::RawImageData(int rows, int cols, int channels, int element_size,
                           const void* data)
    : rows_(rows),
      cols_(cols),
      channels_(channels),
      element_size_(element_size),
      scalar_size_(element_size_ / channels_) {
  data_.resize(rows_ * cols_ * element_size_);
  if (data) memcpy(data_.data(), data, data_.size());
}

RawImageData::RawImageData(const cv::Mat& img)
    : RawImageData(img.rows, img.cols, img.channels(), img.elemSize(),
                   img.data) {}

cv::Mat RawImageData::MakeCvImage(int cv_type) const {
  if (element_size_ != CV_ELEM_SIZE(cv_type) ||
      channels_ != CV_MAT_CN(cv_type)) {
    throw std::runtime_error("invalid conversion");
  }

  cv::Mat ret(rows_, cols_, cv_type);
  memcpy(ret.data, data_.data(), data_.size());
  return ret;
}

}  // namespace rs2_lcm
