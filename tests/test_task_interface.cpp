#include "TaskInterface.hpp"
#include "common.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

class ConcreteTask : public TaskInterface {
 public:
    TaskType GetTaskType() override {
        return TaskType::ObjectDetection;
    }

    std::vector<neuriplo_tasks::Result> ProcessMetadata(
        NvDsFrameMeta* /*frame_meta*/,
        const cv::Mat& /*frame*/) override {
        return {};
    }
};

std::string WriteTempFile(const std::string& name, const std::string& contents) {
    const std::string path = ::testing::TempDir() + name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << contents;
    out.close();
    return path;
}

void RemoveFile(const std::string& path) {
    std::remove(path.c_str());
}

TEST(TaskInterfaceTest, GetTaskTypeReturnsObjectDetection) {
    ConcreteTask task;
    EXPECT_EQ(task.GetTaskType(), TaskType::ObjectDetection);
}

TEST(TaskInterfaceTest, ReadLabelNamesPreservesOrder) {
    const std::string path =
        WriteTempFile("deep_infer_labels_order.txt", "person\ncar\nbicycle\n");
    ConcreteTask task;

    const std::vector<std::string> names = task.ReadLabelNames(path);
    RemoveFile(path);

    EXPECT_EQ(names, (std::vector<std::string>{"person", "car", "bicycle"}));
}

TEST(TaskInterfaceTest, ReadLabelNamesMissingFileIsEmpty) {
    const std::string path =
        ::testing::TempDir() + "deep_infer_labels_missing_987654.txt";
    RemoveFile(path);
    ConcreteTask task;

    EXPECT_TRUE(task.ReadLabelNames(path).empty());
}

TEST(TaskInterfaceTest, ReadLabelNamesEmptyFileIsEmpty) {
    const std::string path = WriteTempFile("deep_infer_labels_empty.txt", "");
    ConcreteTask task;

    const std::vector<std::string> names = task.ReadLabelNames(path);
    RemoveFile(path);

    EXPECT_TRUE(names.empty());
}

TEST(TaskInterfaceTest, ReadLabelNamesKeepsBlankLine) {
    const std::string path =
        WriteTempFile("deep_infer_labels_blank.txt", "a\n\nb\n");
    ConcreteTask task;

    const std::vector<std::string> names = task.ReadLabelNames(path);
    RemoveFile(path);

    EXPECT_EQ(names, (std::vector<std::string>{"a", "", "b"}));
}

}  // namespace
