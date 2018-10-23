#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <opencv2/opencv.hpp>

namespace rs2_lcm {

enum class ImageType {
  RGB = 0,
  DEPTH,
  IR,
  IR_STEREO,
  RECT_RGB,
  RECT_RGB_ALIGNED_DEPTH,
  DEPTH_ALIGNED_RGB,
};

std::string ImageTypeToString(const ImageType type);
bool is_color_image(const ImageType type);
bool is_depth_image(const ImageType type);
bool is_infrared_image(const ImageType type);

/**
 * A primitive data storage for dense images. The underlying structure is a row
 * major std::vector<uint8_t>. Each pixel (element) can have multiple channels,
 * e.g. RGB image would have 3 channels, and the layout looks like RGBRGBRGB...
 */
class RawImageData {
 public:
  template <typename T>
  static std::shared_ptr<RawImageData> MakeSharedRawImageData(
      int rows, int cols, int channels, const void* data = nullptr) {
    return std::make_shared<RawImageData>(
        rows, cols, channels, sizeof(T) * channels, data);
  }

  /**
   * Allocates @p rows * @p cols * @p element_size number of bytes, and does
   * memcpy from @p data if it's non-null. The terminology is taken from opencv.
   * @p rows Height of an image.
   * @p cols Width of an image.
   * @p channels Number of scalars in each pixel (element).
   * @p element_size Number of bytes per pixel (element).
   */
  RawImageData(int rows, int cols, int channels, int element_size,
               const void* data = nullptr);

  /**
   * Constructs an RawImageData from @p cvimg, involves deep copy of @p cvimg's
   * data.
   */
  explicit RawImageData(const cv::Mat& cvimg);

  /**
   * Returns a const Eigen matrix view of the image specified by @p channel.
   * @throws if @p channel is out of bound.
   * @throws if @p T has a different size than the declared scalar size.
   */
  template <typename T>
  Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>,
             Eigen::RowMajor, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>
  slice(int channel = 0) const {
    if (channel < 0 || channel >= channels_) {
      throw std::runtime_error("invalid channel");
    }
    if (sizeof(T) != scalar_size_) {
      std::cout << scalar_size_ << ", " << sizeof(T) << "\n";
      throw std::runtime_error("scalar size mismatch.");
    }
    return Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>,
                      Eigen::RowMajor,
                      Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>(
        reinterpret_cast<const T*>(data_.data()) + channel, rows_, cols_,
        Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>(channels_,
                                                      cols_ * channels_));
  }

  /**
   * Returns a mutable Eigen matrix view of the image specified by @p channel.
   * @throws if @p channel is out of bound.
   * @throws if @p T has a different size than the declared scalar size.
   */
  template <typename T>
  Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>, Eigen::RowMajor,
             Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>
  mutable_slice(int channel = 0) {
    if (channel < 0 || channel >= channels_) {
      throw std::runtime_error("invalid channel");
    }
    if (sizeof(T) != scalar_size_) {
      std::cout << scalar_size_ << ", " << sizeof(T) << "\n";
      throw std::runtime_error("scalar size mismatch.");
    }
    return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>,
                      Eigen::RowMajor,
                      Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>(
        reinterpret_cast<T*>(data_.data()) + channel, rows_, cols_,
        Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>(channels_,
                                                      cols_ * channels_));
  }

  /**
   * Returns a const reference to scalar at (@p row, @p col, @p channel).
   * @throws if @p row, @p col or @p channel is out of bound.
   * @throws if @p T has a different size than the declared scalar size.
   */
  template <typename T>
  const T& at(int row, int col, int channel = 0) const {
    const int offset = ComputeOffset<T>(row, col, channel);
    const T* addr = reinterpret_cast<const T*>(data_.data() + offset);
    return *addr;
  }

  /**
   * Returns a mutable reference to scalar at (@p row, @p col, @p channel).
   * @throws if @p row, @p col or @p channel is out of bound
   * @throws if @p T has a different size than the declared scalar size.
   */
  template <typename T>
  T& at(int row, int col, int channel = 0) {
    const int offset = ComputeOffset<T>(row, col, channel);
    T* addr = reinterpret_cast<T*>(data_.data() + offset);
    return *addr;
  }

  /**
   * Creates a new cv::Mat of @p cv_type. The internal data is memcpy to the
   * to-be-returned cv::Mat.
   * @throws if the number of channels or element size specified by
   * @p cv_type does not match the internal values.
   */
  cv::Mat MakeCvImage(int cv_type) const;

  int cols() const { return cols_; }
  int rows() const { return rows_; }
  int channels() const { return channels_; }

  /**
   * Returns the number of bytes per scalar.
   */
  int scalar_size() const { return scalar_size_; }
  const uint8_t* data() const { return data_.data(); }
  uint8_t* data() { return data_.data(); }

 private:
  template <typename T>
  int ComputeOffset(int row, int col, int channel) const {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_ || channel < 0 ||
        channel >= channels_) {
      throw std::runtime_error("invalid index");
    }
    if (sizeof(T) != scalar_size_) {
      std::cout << scalar_size_ << ", " << sizeof(T) << "\n";
      throw std::runtime_error("scalar size mismatch.");
    }

    return channel * scalar_size_ + col * element_size_ +
           row * cols_ * element_size_;
  }

  std::vector<uint8_t> data_;
  const int rows_;
  const int cols_;
  const int channels_;
  const int element_size_;
  const int scalar_size_;
};

}  // namespace rs2_lcm
