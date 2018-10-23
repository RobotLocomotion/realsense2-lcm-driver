#include "rgbd_sensor/image.h"

#include <gtest/gtest.h>

namespace rs2_lcm {

GTEST_TEST(ImageTest, Test) {
  const int kRows = 4;
  const int kCols = 5;
  const int kChannels = 2;

  std::vector<int16_t> raw(kRows * kCols * kChannels);

  for (size_t i = 0; i < raw.size(); i++) raw[i] = i;

  RawImageData raw_img(kRows, kCols, kChannels, sizeof(int16_t) * kChannels,
                       raw.data());
  cv::Mat cv_img = raw_img.MakeCvImage(CV_MAKETYPE(CV_16S, kChannels));

  // Dimension checks.
  EXPECT_EQ(raw_img.cols(), kCols);
  EXPECT_EQ(raw_img.rows(), kRows);
  EXPECT_EQ(raw_img.channels(), kChannels);
  EXPECT_EQ(raw_img.scalar_size(), sizeof(int16_t));

  EXPECT_EQ(cv_img.cols, kCols);
  EXPECT_EQ(cv_img.rows, kRows);
  EXPECT_EQ(cv_img.channels(), kChannels);
  EXPECT_EQ(cv_img.elemSize(), sizeof(int16_t) * kChannels);
  EXPECT_EQ(cv_img.type(), CV_MAKETYPE(CV_16S, kChannels));

  // Data check.
  for (int d = 0; d < kChannels; d++) {
    auto eigen_view = raw_img.slice<int16_t>(d);
    EXPECT_EQ(eigen_view.cols(), kCols);
    EXPECT_EQ(eigen_view.rows(), kRows);

    for (int r = 0; r < kRows; r++) {
      for (int c = 0; c < kCols; c++) {
        const int16_t expected = d + c * kChannels + r * kCols * kChannels;
        EXPECT_EQ(raw_img.at<int16_t>(r, c, d), expected);
        EXPECT_EQ(eigen_view(r, c), expected);
      }
    }
  }

  for (int r = 0; r < kRows; r++) {
    for (int c = 0; c < kCols; c++) {
      cv::Scalar expected(c * kChannels + r * kCols * kChannels,
                          c * kChannels + r * kCols * kChannels + 1);
      auto cv_val = cv_img.at<cv::Vec2s>(r, c);
      EXPECT_EQ(expected[0], cv_val[0]);
      EXPECT_EQ(expected[1], cv_val[1]);
    }
  }

  // Mutable setter
  const int kMagicNum = 233;

  raw_img.at<int16_t>(0, 1, 1) = kMagicNum;
  EXPECT_EQ(raw_img.at<int16_t>(0, 1, 1), kMagicNum);
  EXPECT_EQ(raw_img.slice<int16_t>(1)(0, 1), kMagicNum);

  raw_img.mutable_slice<int16_t>(0)(2, 2) = kMagicNum;
  EXPECT_EQ(raw_img.at<int16_t>(2, 2), kMagicNum);
  EXPECT_EQ(raw_img.slice<int16_t>()(2, 2), kMagicNum);

  // The cv mat should be untouched.
  auto cv_val = cv_img.at<cv::Vec2s>(0, 1);
  EXPECT_EQ(cv_val[1], 1 * kChannels + 1);
  cv_val = cv_img.at<cv::Vec2s>(2, 2);
  EXPECT_EQ(cv_val[0], 2 * kChannels + 2 * kCols * kChannels);
}

}  // namespace rs2_lcm
