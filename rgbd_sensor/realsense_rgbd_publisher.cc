/// @file
///
/// Open RealSense cameras and run the RGBD publisher.
#include <chrono>
#include <string>

#include <drake/common/text_logging.h>
#include <gflags/gflags.h>
#include "rgbd_sensor/lcm_rgbd_common.h"
#include "rgbd_sensor/lcm_rgbd_publisher.h"
#include "rgbd_sensor/real_sense_d400.h"

DEFINE_string(intrinsic_path, "",
              "Path to intrinsic param folders for all cameras");
DEFINE_bool(dump_camera_ids, false,
            "Print the ids of all cameras and terminate.");
DEFINE_string(id_print_prefix, "Camera id: ",
              "Prefix to print before camera id when --dump_camera_ids "
              "is specified");

DEFINE_int32(num_cameras, 1, "Number of cameras to attempt to open");
DEFINE_string(serial, "", "Use serial number. --num_cameras must be 1 if set.");
DEFINE_bool(hardware_depth_registration, false,
            "Enable hardware depth registration");
DEFINE_bool(software_depth_registration, false,
            "Enable software depth registration");
DEFINE_bool(ir, true, "Publish IR images along with RGB and DEPTH");
DEFINE_bool(use_high_res, false,
            "Use in high res mode (1280X720) instead of the default (848X480)");
DEFINE_string(
    json_config_file, "",
    "JSON configuration file for camera settings. Note that this "
    "should only publish one type of camera (d415 or d435) if this is "
    "specified.");

namespace rs2_lcm {
namespace {

int RunRgbdPublisher(const std::vector<std::unique_ptr<RGBDSensor>>& devices,
                     const std::vector<ImageType>& image_types,
                     ImageType depth_type, bool request_software_registration) {
  drake::log()->info("Request software depth registration: {}",
                     request_software_registration);

  for (size_t i = 0; i < devices.size(); ++i) {
    RGBDSensor* sensor = devices[i].get();
    sensor->Start(image_types);
  }

  if (FLAGS_dump_camera_ids) {
    for (size_t i = 0; i < devices.size(); ++i) {
      RGBDSensor* sensor = devices[i].get();
      std::cout << FLAGS_id_print_prefix << sensor->camera_id() << std::endl;
      sensor->Stop();
    }
    return 0;
  }


  lcm::LCM lcm;
  std::vector<LcmRgbdPublisher> publishers;
  for (size_t i = 0; i < devices.size(); ++i) {
    RGBDSensor* sensor = devices[i].get();
    const std::vector<ImageType> enabled_image_types =
        sensor->get_enabled_image_types();

    std::vector<ImageType> requested_image_types = enabled_image_types;
    if (request_software_registration) {
      requested_image_types.push_back(ImageType::RECT_RGB_ALIGNED_DEPTH);
    }

    publishers.emplace_back(
        requested_image_types, sensor->camera_id(), "DRAKE_RGBD_CAMERAS",
        "DRAKE_RGBD_CAMERA_IMAGES_" + sensor->camera_id(), sensor, &lcm);
  }

  // Set the last description time in the past so that we publish immediately.
  auto last_description_sent =
      std::chrono::system_clock::now() - std::chrono::hours(1);
  std::vector<uint64_t> last_depth_timestamp(devices.size(), 0);
  while (true) {
    auto now = std::chrono::system_clock::now();
    if (now - last_description_sent > std::chrono::milliseconds(500)) {
      for (LcmRgbdPublisher& publisher : publishers) {
        publisher.PublishDescription();
        last_description_sent = now;
      }
    }

    uint64_t depth_timestamp = 0;
    for (size_t i = 0; i < devices.size(); ++i) {
      devices[i]->GetLatestImage(depth_type, &depth_timestamp);
      if (depth_timestamp != last_depth_timestamp[i]) {
        publishers[i].PublishImages();
        last_depth_timestamp[i] = depth_timestamp;
      }
    }
    // Run at 200hz (arbitrary).
    lcm.handleTimeout(5);
  }

  return 0;
}

int DoMain() {
  ImageType hardware_depth_type = FLAGS_hardware_depth_registration
                                      ? ImageType::RECT_RGB_ALIGNED_DEPTH
                                      : ImageType::DEPTH;

  bool request_software_registration =
      FLAGS_software_depth_registration && !FLAGS_hardware_depth_registration;

  std::vector<ImageType> image_types;
  image_types.push_back(ImageType::RGB);
  image_types.push_back(hardware_depth_type);
  if (FLAGS_ir) {
    image_types.push_back(ImageType::IR);
    image_types.push_back(ImageType::IR_STEREO);
  }

  if (!FLAGS_serial.empty() && FLAGS_num_cameras != 1) {
    throw std::runtime_error("--serial requires --num_cameras=1.");
  }

  std::vector<std::unique_ptr<RGBDSensor>> sensors;
  int i = 0;
  while (static_cast<int>(sensors.size()) < FLAGS_num_cameras) {
    std::unique_ptr<RGBDSensor> sensor;
    sensor = std::make_unique<RealSenseD400>(
        i, FLAGS_use_high_res, FLAGS_json_config_file);
    ++i;

    if (!FLAGS_serial.empty()) {
      // Since we have not stated the camera, discarding the existing sensor
      // instance should not create a resource conflict.
      if (sensor->camera_id() != FLAGS_serial)
        continue;
    }
    sensors.push_back(std::move(sensor));
  }

  if (sensors.empty()) {
    throw std::runtime_error("No cameras found!");
  }

  return RunRgbdPublisher(sensors, image_types, hardware_depth_type,
                          request_software_registration);
}

}  // namespace
}  // namespace rs2_lcm

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return rs2_lcm::DoMain();
}
