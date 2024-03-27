#include "rgbd_sensor/real_sense_d400.h"

#include <fstream>
#include <iostream>

#include <boost/make_shared.hpp>
#include <drake/common/scoped_singleton.h>
#include <librealsense2/rs_advanced_mode.hpp>
#include <librealsense2/rsutil.h>
#include "rgbd_sensor/real_sense_common.h"

#include "drake/common/text_logging.h"

namespace rs2_lcm {
namespace {

ImageType StreamProfileToImageType(const rs2::stream_profile& profile) {
  switch (profile.stream_type()) {
    case RS2_STREAM_COLOR:
      return ImageType::RGB;
    case RS2_STREAM_DEPTH:
      return ImageType::DEPTH;
    case RS2_STREAM_INFRARED:
      if (profile.stream_index() == 1) return ImageType::IR;
      if (profile.stream_index() == 2) return ImageType::IR_STEREO;
      break;
    default:
      break;
  }
  throw std::runtime_error("Unknown convertion from " + profile.stream_name() +
                           " to ImageType");
}

Intrinsics MakeIntrinsics(const rs2_intrinsics& rs_intrin) {
  Intrinsics::DistortionModel model;
  switch (rs_intrin.model) {
    case RS2_DISTORTION_NONE:
      model = Intrinsics::DistortionModel::NONE;
      break;
    case RS2_DISTORTION_MODIFIED_BROWN_CONRADY:
      model = Intrinsics::DistortionModel::MODIFIED_BROWN_CONRADY;
      break;
    case RS2_DISTORTION_INVERSE_BROWN_CONRADY:
      model = Intrinsics::DistortionModel::INVERSE_BROWN_CONRADY;
      break;
    case RS2_DISTORTION_FTHETA:
      model = Intrinsics::DistortionModel::FTHETA;
      break;
    case RS2_DISTORTION_BROWN_CONRADY:
      model = Intrinsics::DistortionModel::BROWN_CONRADY;
      break;
    default:
      throw std::runtime_error("Unknown distortion model.");
  }

  std::array<float, 5> coeffs;
  for (size_t i = 0; i < coeffs.size(); i++) coeffs.at(i) = rs_intrin.coeffs[i];

  return Intrinsics(rs_intrin.width, rs_intrin.height, rs_intrin.fx,
                    rs_intrin.fy, rs_intrin.ppx, rs_intrin.ppy, model, coeffs);
}

std::shared_ptr<RawImageData> MakeImg(const void* src, int width, int height,
                                      rs2_format pixel_type) {
  switch (pixel_type) {
    case RS2_FORMAT_Z16:
    case RS2_FORMAT_DISPARITY16:
    case RS2_FORMAT_Y16:
    case RS2_FORMAT_RAW16:
      return RawImageData::MakeSharedRawImageData<uint16_t>(
          height, width, 1, src);
    case RS2_FORMAT_Y8:
    case RS2_FORMAT_RAW8:
      return RawImageData::MakeSharedRawImageData<uint8_t>(
          height, width, 1, src);
    case RS2_FORMAT_RGB8:
    case RS2_FORMAT_BGR8:
      return RawImageData::MakeSharedRawImageData<uint8_t>(
          height, width, 3, src);
    case RS2_FORMAT_RGBA8:
    case RS2_FORMAT_BGRA8:
      return RawImageData::MakeSharedRawImageData<uint8_t>(
          height, width, 4, src);
    default:
      throw std::runtime_error("Doesn't support this type");
  }
}

std::shared_ptr<rs2::context> GetRealSense2Context() {
  return drake::GetScopedSingleton<rs2::context>();
}

}  // namespace

RealSenseD400::RealSenseD400(int camera_id, bool use_high_res,
                             const std::string& json_config_file)
    : RGBDSensor({ImageType::RGB, ImageType::DEPTH, ImageType::IR,
                  ImageType::IR_STEREO}),
      context_(GetRealSense2Context()),
      pipeline_(*context_),
      camera_(context_->query_devices()[camera_id]),
      depth_sensor_(camera_.first<rs2::depth_sensor>()),
      camera_name_(camera_.get_info(RS2_CAMERA_INFO_NAME)),
      serial_number_(camera_.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)),
      use_high_res_(use_high_res) {
  const bool is_d435 = camera_name_ == "Intel RealSense D435"
    || camera_name_ == "Intel RealSense D435I";
  const bool is_d455 = camera_name_ == "Intel RealSense D455";

  if (camera_name_ == "Intel RealSense D415") {
    LoadJsonConfig(json_config_file.empty() ? "cfg/d415_high_density.json"
                                            : json_config_file);
    post_process_ = true;
  } else if (is_d435) {
    LoadJsonConfig(json_config_file.empty() ? "cfg/d435_medium_density.json"
                                            : json_config_file);
    post_process_ = false;
  } else if (is_d455) {
    LoadJsonConfig(json_config_file.empty() ? "cfg/d455_medium_density.json"
                                            : json_config_file);
    post_process_ = false;
  } else {
    throw std::runtime_error(camera_name_ + " is not a D415, D435/D435I or D455");
  }

  // Get intrinsics and extrinsics for all the supported streams.
  const auto& supported_types = get_supported_image_types();
  rs2::config config = MakeRealSenseConfig(supported_types);
  auto pipeline_profile = config.resolve(pipeline_);

  // Getting depth scale.
  depth_scale_ = depth_sensor_.get_option(RS2_OPTION_DEPTH_UNITS);

  // Find camera intrinsics
  std::vector<rs2::stream_profile> profiles = pipeline_profile.get_streams();
  std::map<ImageType, rs2_intrinsics> rs_intrinsics;
  for (const auto& profile : profiles) {
    ImageType type = StreamProfileToImageType(profile);
    supported_streams_[type] = profile;
    rs_intrinsics[type] =
        profile.as<rs2::video_stream_profile>().get_intrinsics();
  }

  for (const auto& type : supported_types) {
    // Read camera's onboard intrinsics.
    set_intrinsics(type, MakeIntrinsics(rs_intrinsics.at(type)));

    // Read camera's onboard extrinsics.
    for (const auto& to_type : supported_types) {
      const auto rs_extrinsics = supported_streams_.at(type).get_extrinsics_to(
          supported_streams_.at(to_type));
      set_extrinsics(type, to_type,
                     real_sense::rs_extrinsics_to_eigen(rs_extrinsics));
    }
  }
}

void RealSenseD400::LoadJsonConfig(const std::string& json_path) {
  drake::log()->debug("Loading config from {}", json_path);
  if (camera_.is<rs400::advanced_mode>()) {
    std::ifstream config_stream;
    config_stream.open(json_path);

    auto advanced_mode_dev = camera_.as<rs400::advanced_mode>();
    if (!advanced_mode_dev.is_enabled()) {
      advanced_mode_dev.toggle_advanced_mode(true);
    }

    std::stringstream buffer;
    buffer << config_stream.rdbuf();
    std::string json_config = buffer.str();
    drake::log()->debug("{}", json_config);
    advanced_mode_dev.load_json(json_config);
  } else {
    drake::log()->warn("{} did not load config.", camera_name_);
  }
}

rs2::config RealSenseD400::MakeRealSenseConfig(
    const std::vector<ImageType>& desired_types) const {
  rs2::config config;
  config.enable_device(camera_.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

  int width = 848, height = 480;

  if (use_high_res_) {
    width = 1280;
    height = 720;
  }

  const bool is_d435 = camera_name_ == "Intel RealSense D435" ||
      camera_name_ == "Intel RealSense D435I";

  const bool is_d455 = camera_name_ == "Intel RealSense D455";

  if ((is_d435 || is_d455) && !use_high_res_) {
    // Using a smaller width for shorter min range.
    width = 640;
    height = 480;
  }

  // Always enable rgb and depth.
  config.enable_stream(RS2_STREAM_COLOR, -1, width, height, RS2_FORMAT_RGB8,
                       30);
  config.enable_stream(RS2_STREAM_DEPTH, -1, width, height, RS2_FORMAT_ANY, 30);
  if (std::find(desired_types.begin(), desired_types.end(), ImageType::IR) !=
      desired_types.end()) {
    config.enable_stream(RS2_STREAM_INFRARED, 1, width, height, RS2_FORMAT_ANY,
                         30);
  }
  if (std::find(desired_types.begin(), desired_types.end(),
                ImageType::IR_STEREO) != desired_types.end()) {
    config.enable_stream(RS2_STREAM_INFRARED, 2, width, height, RS2_FORMAT_ANY,
                         30);
  }

  return config;
}

void RealSenseD400::SetMode(const rs2_rs400_visual_preset mode) {
  throw std::runtime_error("Unimplemented");
  // TODO(siyuan): figure out why this doesn't seem to do anything.
  //  depth_sensor_.set_option(RS2_OPTION_VISUAL_PRESET, mode);
  //  auto val = depth_sensor_.get_option(RS2_OPTION_VISUAL_PRESET);
  //  std::cout <<
  //  depth_sensor_.get_option_description(RS2_OPTION_VISUAL_PRESET) << ": "
  //            <<
  //            depth_sensor_.get_option_value_description(RS2_OPTION_VISUAL_PRESET,
  //            val) << "\n";
}

int RealSenseD400::get_number_of_cameras() {
  return GetRealSense2Context()->query_devices().size();
}

void RealSenseD400::DoStart(const std::vector<ImageType>& types) {
  if (run_) {
    return;
  }

  run_ = true;

  drake::log()->info("{} {} starting.", camera_name_, serial_number_);
  auto config = MakeRealSenseConfig(types);
  pipeline_.start(config);

  thread_ = std::thread(&RealSenseD400::PollingThread, this);
}

void RealSenseD400::DoStop() {
  run_ = false;
  thread_.join();
  pipeline_.stop();
}

void RealSenseD400::PollingThread() {
  std::map<const ImageType, TimeStampedImage> images;
  std::vector<ImageType> enabled_types = get_enabled_image_types();
  for (const auto& type : enabled_types) {
    images[type] = TimeStampedImage();
  }

  rs2::frameset frameset;
  std::map<ImageType, rs2::frame> frames;

  rs2::temporal_filter low_pass_filter;
  rs2::spatial_filter spatial_filter;
  rs2::disparity_transform depth_to_disparity(true);
  rs2::disparity_transform disparity_to_depth(false);
  low_pass_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.4);
  low_pass_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20);

  while (run_) {
    // Block until all frames have arrived.
    frameset = pipeline_.wait_for_frames();

    if (frameset.size() != enabled_types.size()) continue;

    for (const auto& frame : frameset) {
      frames[StreamProfileToImageType(frame.get_profile())] = frame;
    }

    // Raw color and depth img.
    rs2::frame& depth_frame = frames.at(ImageType::DEPTH);
    if (post_process_) {
      depth_frame = depth_to_disparity.process(depth_frame);
      depth_frame = spatial_filter.process(depth_frame);
      depth_frame = low_pass_filter.process(depth_frame);
      depth_frame = disparity_to_depth.process(depth_frame);
    }

    // Make images.
    for (auto& pair : images) {
      const ImageType type = pair.first;
      const rs2::video_frame frame = frames.at(type).as<rs2::video_frame>();
      pair.second.timestamp = (uint64_t)frame.get_timestamp();
      std::shared_ptr<RawImageData> img;
      if (!is_infrared_image(type)) {
        img = MakeImg(frame.get_data(), frame.get_width(), frame.get_height(),
                  supported_streams_.at(type).format());
      } else {
        // d400 returns images in 8 bits. but rgbd sensor wants 16 bits.
        img = RawImageData::MakeSharedRawImageData<uint16_t>(
            frame.get_height(), frame.get_width(), 1);
        auto addr = reinterpret_cast<const uint8_t*>(frame.get_data());
        auto img_view = img->mutable_slice<uint16_t>();
        for (int x = 0; x < frame.get_width(); x++) {
          for (int y = 0; y < frame.get_height(); y++) {
            img_view(y, x) = addr[x + y * frame.get_width()] * 256;
          }
        }
      }
      // Scale depth image to units of mm.
      // This could cause on overflow if distance is > 65 ish meters.
      if (is_depth_image(pair.first)) {
        auto depth_view = img->mutable_slice<uint16_t>();
        // Note: Can't do depth_view *= scale, where scale < 0. I think eigen
        // casts scale to uint16_t first.
        for (int x = 0; x < depth_view.cols(); x++) {
          for (int y = 0; y < depth_view.rows(); y++) {
            depth_view(y, x) =
                static_cast<uint16_t>(depth_view(y, x) * depth_scale_ * 1e3);
          }
        }
      }

      pair.second.data = img;
    }

    UpdateImages(images);
  }
}

}  // namespace rs2_lcm
