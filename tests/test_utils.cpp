#include "utils.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

const std::vector<std::string> kEmpty;

}  // namespace

TEST(SplitTest, SplitsOnDelimiter) {
    EXPECT_EQ(Split("a,b,c", ','), (std::vector<std::string>{"a", "b", "c"}));
}

TEST(SplitTest, EmptyInputYieldsNoTokens) {
    EXPECT_EQ(Split("", ','), kEmpty);
}

TEST(SplitTest, KeepsLeadingEmptyToken) {
    EXPECT_EQ(Split(",a", ','), (std::vector<std::string>{"", "a"}));
}

TEST(SplitTest, KeepsInteriorEmptyTokens) {
    EXPECT_EQ(Split("a,,b", ','), (std::vector<std::string>{"a", "", "b"}));
}

TEST(SplitTest, DropsTrailingEmptyToken) {
    EXPECT_EQ(Split("a,b,", ','), (std::vector<std::string>{"a", "b"}));
}

TEST(SplitTest, NoDelimiterReturnsWholeString) {
    EXPECT_EQ(Split("abc", ','), (std::vector<std::string>{"abc"}));
}

TEST(ToLowerTest, LowercasesAscii) {
    EXPECT_EQ(ToLower("HeLLo"), "hello");
}

TEST(ToLowerTest, LeavesEmptyString) {
    EXPECT_EQ(ToLower(""), "");
}

TEST(ToLowerTest, LeavesDigitsAndPunctuation) {
    EXPECT_EQ(ToLower("Model-1_v2.ONNX"), "model-1_v2.onnx");
}

TEST(ToLowerTest, DoesNotCorruptHighBytes) {
    // std::tolower must be called with an unsigned char; a signed-char
    // implementation would corrupt bytes above 0x7F.
    const std::string high("\xC3\xA9");
    EXPECT_EQ(ToLower(high), high);
}

TEST(IsImageFileTest, RecognizesCommonExtensions) {
    EXPECT_TRUE(IsImageFile("photo.jpg"));
    EXPECT_TRUE(IsImageFile("photo.jpeg"));
    EXPECT_TRUE(IsImageFile("photo.png"));
    EXPECT_TRUE(IsImageFile("photo.bmp"));
}

TEST(IsImageFileTest, IsCaseInsensitive) {
    EXPECT_TRUE(IsImageFile("PHOTO.JPG"));
    EXPECT_TRUE(IsImageFile("Photo.PnG"));
}

TEST(IsImageFileTest, RejectsNonImages) {
    EXPECT_FALSE(IsImageFile("clip.mp4"));
    EXPECT_FALSE(IsImageFile("labels.txt"));
    EXPECT_FALSE(IsImageFile("noextension"));
}

TEST(IsVideoFileTest, RecognizesFileExtensions) {
    EXPECT_TRUE(IsVideoFile("clip.mp4"));
    EXPECT_TRUE(IsVideoFile("clip.avi"));
    EXPECT_TRUE(IsVideoFile("clip.mkv"));
    EXPECT_TRUE(IsVideoFile("clip.mov"));
}

TEST(IsVideoFileTest, RecognizesStreamUrls) {
    EXPECT_TRUE(IsVideoFile("rtsp://camera:8554/stream"));
    EXPECT_TRUE(IsVideoFile("rtmp://server/app"));
}

TEST(IsVideoFileTest, IsCaseInsensitive) {
    EXPECT_TRUE(IsVideoFile("CLIP.MP4"));
    EXPECT_TRUE(IsVideoFile("RTSP://CAMERA/STREAM"));
}

TEST(IsVideoFileTest, RejectsNonVideo) {
    EXPECT_FALSE(IsVideoFile("photo.png"));
    EXPECT_FALSE(IsVideoFile("labels.txt"));
}

TEST(GenerateRandomColorsTest, ReturnsRequestedCount) {
    EXPECT_EQ(GenerateRandomColors(0).size(), 0u);
    EXPECT_EQ(GenerateRandomColors(5).size(), 5u);
    EXPECT_EQ(GenerateRandomColors(80).size(), 80u);
}

TEST(GenerateRandomColorsTest, IsDeterministic) {
    // The function seeds its RNG, so repeated calls produce identical output.
    EXPECT_EQ(GenerateRandomColors(10), GenerateRandomColors(10));
}

TEST(GenerateRandomColorsTest, ChannelsAreInByteRange) {
    for (const auto& color : GenerateRandomColors(32)) {
        for (int i = 0; i < 4; ++i) {
            EXPECT_GE(color[i], 0.0);
            EXPECT_LE(color[i], 255.0);
        }
    }
}

TEST(DrawLabelTest, ModifiesImage) {
    cv::Mat image(120, 200, CV_8UC3, cv::Scalar(0, 0, 0));
    const cv::Scalar before = cv::sum(image);

    DrawLabel(image, "person", 0.87f, 10, 40);

    const cv::Scalar after = cv::sum(image);
    EXPECT_NE(before, after);
}

TEST(DrawLabelTest, ClampsTopEdgeWithoutCrashing) {
    cv::Mat image(120, 200, CV_8UC3, cv::Scalar(0, 0, 0));
    ASSERT_NO_THROW(DrawLabel(image, "car", 0.5f, 0, 1));
}

TEST(Nv12ToBgrTest, ConvertsToBgrFrame) {
    constexpr int width = 4;
    constexpr int height = 2;
    std::vector<uint8_t> nv12(static_cast<size_t>(width) * height * 3 / 2, 128);

    cv::Mat bgr = Nv12ToBgr(nv12.data(), width, height);

    EXPECT_EQ(bgr.rows, height);
    EXPECT_EQ(bgr.cols, width);
    EXPECT_EQ(bgr.type(), CV_8UC3);
}
