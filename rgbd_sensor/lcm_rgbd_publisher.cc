#include "rgbd_sensor/lcm_rgbd_publisher.h"

#include "sys/time.h"

#include <memory>
#include <stdexcept>

#include <drake/common/text_logging.h>
#include <drake/lcmt_image.hpp>
#include <drake/lcmt_image_array.hpp>
#include <zlib.h>
#include "rgbd_sensor/lcm_rgbd_common.h"
#include "rs2_lcm/camera_description_t.hpp"

namespace rs2_lcm {

LcmRgbdPublisher::LcmRgbdPublisher(
    const std::vector<ImageType>& types, const std::string& camera_name,
    const std::string& lcm_description_channel_name,
    const std::string& lcm_channel_name, const RGBDSensor* sensor,
    lcm::LCM* lcm)
    : types_(types),
      camera_name_(camera_name),
      lcm_description_channel_name_(lcm_description_channel_name),
      lcm_channel_name_(lcm_channel_name),
      sensor_(sensor),
      lcm_(lcm) {
  for (ImageType type : types_) {
    image_seq_[type] = 0;
  }

  drake::log()->info("Publishing descriptions on {} data on {}",
                     lcm_description_channel_name_, lcm_channel_name_);

  bool depth_registration_requested =
      std::find(types.begin(), types.end(),
                ImageType::RECT_RGB_ALIGNED_DEPTH) != types.end();
  bool hardware_registration_enabled =
      sensor->is_enabled(ImageType::RECT_RGB_ALIGNED_DEPTH);
  enabled_software_registration_ =
      depth_registration_requested && !hardware_registration_enabled;
}

LcmRgbdPublisher::~LcmRgbdPublisher() {}

void LcmRgbdPublisher::PublishDescription() {
  rs2_lcm::camera_description_t desc{};
  desc.camera_name = camera_name_;
  desc.lcm_channel_name = lcm_channel_name_;
  desc.num_image_types = types_.size();
  for (ImageType type : types_) {
    rs2_lcm::image_description_t image_desc{};
    image_desc.frame_name = ImageTypeToFrameName(type);
    image_desc.type = ImageTypeToDescriptionType(type);

    ImageType sensor_query_type = type;
    if (enabled_software_registration_ &&
        type == ImageType::RECT_RGB_ALIGNED_DEPTH)
      sensor_query_type = sensor_->is_enabled(ImageType::RECT_RGB)
                              ? ImageType::RECT_RGB
                              : ImageType::RGB;

    image_desc.intrinsics = SerializeIntrinsics(
        sensor_->get_intrinsics(sensor_query_type));
    for (ImageType to_type : types_) {
      if (sensor_->has_extrinsics(type, to_type)) {
        image_desc.extrinsics.push_back(
            SerializeExtrinsics(type, to_type,
                                sensor_->get_extrinsics(type, to_type)));
      }
    }
    image_desc.num_extrinsics = image_desc.extrinsics.size();
    desc.image_types.push_back(image_desc);
  }

  lcm_->publish<rs2_lcm::camera_description_t>(lcm_description_channel_name_,
                                            &desc);
}

namespace {

void build_lcm_image_header(int32_t sequence, int64_t timestamp,
                            const std::string& frame_name,
                            drake::lcmt_image* image) {
  image->header.seq = sequence;
  image->header.utime = timestamp;
  image->header.frame_name = frame_name;
}

void build_lcm_image_message(const cv::Mat& image_mat, int expected_mat_type,
                             bool bigendian, int8_t pixel_format,
                             int8_t channel_type, int8_t compression_method,
                             drake::lcmt_image* image) {
  if (image_mat.type() != expected_mat_type) {
    throw std::runtime_error("Unexpected image type in LcmRgbdPublisher");
  }

  image->height = image_mat.rows;
  image->width = image_mat.cols;

  // Drake uses FLOAT32 for depth messages, but since we're
  // already in UINT16 just stay there.
  image->row_stride = image_mat.elemSize() * image->width;
  image->bigendian = bigendian;
  image->pixel_format = pixel_format;
  image->channel_type = channel_type;
  image->compression_method = compression_method;
  switch (compression_method) {
    case drake::lcmt_image::COMPRESSION_METHOD_PNG: {
      cv::imencode(".png", image_mat, image->data);
      break;
    }
    case drake::lcmt_image::COMPRESSION_METHOD_JPEG: {
      cv::imencode(".jpg", image_mat, image->data);
      break;
    }
    case drake::lcmt_image::COMPRESSION_METHOD_ZLIB: {
      const int source_size =
          image->width * image->height * image_mat.elemSize();

      // The destination buf_size must be slightly larger than the source
      // size.
      // http://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/zlib-compress2-1.html
      size_t buf_size = source_size * 1.001 + 12;
      std::unique_ptr<uint8_t[]> buf(new uint8_t[buf_size]);

      auto compress_status = compress2(
          buf.get(), &buf_size,
          reinterpret_cast<const Bytef*>(image_mat.ptr()),
          source_size, Z_BEST_SPEED);
      if (compress_status != Z_OK) {
        throw std::runtime_error("zlib compression failed");
      }

      image->data.resize(buf_size);
      memcpy(&image->data[0], buf.get(), buf_size);
      break;
    }
  }
  image->size = image->data.size();
}

}  // namespace

void LcmRgbdPublisher::PublishImages() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t utime = (tv.tv_sec * 1000000) + tv.tv_usec;

  std::shared_ptr<const RawImageData> depth_image, color_image;
  drake::lcmt_image_array images{};
  images.header.seq = seq_++;
  images.header.utime = utime;
  uint64_t timestamp = 0;
  cv::Mat depth_float;
  for (ImageType type : types_) {
    if (!sensor_->is_enabled(type)) {
      continue;
    }

    auto img = sensor_->GetLatestImage(type, &timestamp);
    if (!img) {
      continue;
    }

    images.images.push_back(drake::lcmt_image());
    drake::lcmt_image& image = images.images.back();

    build_lcm_image_header(image_seq_.at(type), timestamp,
                           ImageTypeToFrameName(type), &image);
    switch (type) {
      case ImageType::RGB:
      case ImageType::RECT_RGB:
      case ImageType::DEPTH_ALIGNED_RGB: {
        cv::Mat rgb_mat = img->MakeCvImage(CV_8UC3);
        cv::Mat bgr_mat;
        cv::cvtColor(rgb_mat, bgr_mat, CV_RGB2BGR);
        build_lcm_image_message(
            bgr_mat, CV_8UC3, false, drake::lcmt_image::PIXEL_FORMAT_RGB,
            drake::lcmt_image::CHANNEL_TYPE_UINT8,
            drake::lcmt_image::COMPRESSION_METHOD_JPEG, &image);
        color_image = img;
        break;
      }
      case ImageType::DEPTH: {
        cv::Mat image_mat = img->MakeCvImage(CV_16UC1);
        build_lcm_image_message(
            image_mat, CV_16UC1, false,
            drake::lcmt_image::PIXEL_FORMAT_DEPTH,
            drake::lcmt_image::CHANNEL_TYPE_UINT16,
            drake::lcmt_image::COMPRESSION_METHOD_ZLIB, &image);
        if (enabled_software_registration_) {
          depth_image = img;
        }
        break;
      }
      case ImageType::RECT_RGB_ALIGNED_DEPTH: {
        cv::Mat image_mat = img->MakeCvImage(CV_16UC1);
        build_lcm_image_message(
            image_mat, CV_16UC1, false,
            drake::lcmt_image::PIXEL_FORMAT_DEPTH,
            // TODO(duy): It should be float but why float does
            // not work with Linemod?
            drake::lcmt_image::CHANNEL_TYPE_UINT16,
            drake::lcmt_image::COMPRESSION_METHOD_ZLIB, &image);
        break;
      }
      case ImageType::IR:
      case ImageType::IR_STEREO: {
        cv::Mat image_mat = img->MakeCvImage(CV_16UC1);
        build_lcm_image_message(
            image_mat, CV_16UC1, false,
            drake::lcmt_image::PIXEL_FORMAT_GRAY,
            drake::lcmt_image::CHANNEL_TYPE_UINT16,
            drake::lcmt_image::COMPRESSION_METHOD_PNG, &image);
        break;
      }
    }
  }

  if (enabled_software_registration_) {
    if (std::find(types_.begin(), types_.end(), ImageType::DEPTH) ==
        types_.end()) {
      depth_image = sensor_->GetLatestImage(ImageType::DEPTH, &timestamp);
    }

    if (depth_image && color_image) {
      const auto color_intrinsics = sensor_->get_intrinsics(ImageType::RGB);
      const auto depth_intrinsics = sensor_->get_intrinsics(ImageType::DEPTH);
      const auto X_rgb_depth =
          sensor_->get_extrinsics(ImageType::DEPTH, ImageType::RGB);
      RawImageData depth_registered =
          DoRegisterDepthToColor(color_intrinsics, depth_intrinsics,
                                 X_rgb_depth, *color_image, *depth_image,
                                 ImageType::DEPTH);
      images.images.push_back(drake::lcmt_image());
      drake::lcmt_image& image = images.images.back();
      build_lcm_image_header(
          image_seq_.at(ImageType::DEPTH), timestamp,
          ImageTypeToFrameName(ImageType::RECT_RGB_ALIGNED_DEPTH), &image);
      build_lcm_image_message(
          depth_registered.MakeCvImage(CV_16UC1), CV_16UC1, false,
          drake::lcmt_image::PIXEL_FORMAT_DEPTH,
          // TODO(duy): It should be float but why float does
          // not work with Linemod?
          drake::lcmt_image::CHANNEL_TYPE_UINT16,
          drake::lcmt_image::COMPRESSION_METHOD_PNG, &image);
    }
  }

  images.num_images = images.images.size();
  lcm_->publish<drake::lcmt_image_array>(lcm_channel_name_, &images);
}

}  // namespace rs2_lcm
