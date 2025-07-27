#include <gtest/gtest.h>
#include "hft_core/Config.hpp"
#include <fstream>
#include <filesystem>

using namespace hft::core;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary config file
        test_file_ = "test_config.conf";
        std::ofstream file(test_file_);
        file << "# Test configuration\n";
        file << "string_value=\"test_string\"\n";
        file << "int_value=42\n";
        file << "double_value=3.14\n";
        file << "bool_value=true\n";
        file.close();
    }

    void TearDown() override {
        std::filesystem::remove(test_file_);
    }

    std::string test_file_;
};

TEST_F(ConfigTest, LoadFromFile) {
    auto& config = Config::instance();
    EXPECT_TRUE(config.load_from_file(test_file_));
}

TEST_F(ConfigTest, GetStringValue) {
    auto& config = Config::instance();
    config.load_from_file(test_file_);
    
    EXPECT_EQ(config.get<std::string>("string_value"), "test_string");
    EXPECT_EQ(config.get<std::string>("non_existent", "default"), "default");
}

TEST_F(ConfigTest, GetIntValue) {
    auto& config = Config::instance();
    config.load_from_file(test_file_);
    
    EXPECT_EQ(config.get<int>("int_value"), 42);
    EXPECT_EQ(config.get<int>("non_existent", 100), 100);
}

TEST_F(ConfigTest, GetDoubleValue) {
    auto& config = Config::instance();
    config.load_from_file(test_file_);
    
    EXPECT_DOUBLE_EQ(config.get<double>("double_value"), 3.14);
    EXPECT_DOUBLE_EQ(config.get<double>("non_existent", 2.71), 2.71);
}

TEST_F(ConfigTest, GetBoolValue) {
    auto& config = Config::instance();
    config.load_from_file(test_file_);
    
    EXPECT_TRUE(config.get<bool>("bool_value"));
    EXPECT_FALSE(config.get<bool>("non_existent", false));
}

TEST_F(ConfigTest, SetAndGet) {
    auto& config = Config::instance();
    
    config.set("new_key", std::string("new_value"));
    EXPECT_EQ(config.get<std::string>("new_key"), "new_value");
    
    config.set("new_int", 123);
    EXPECT_EQ(config.get<int>("new_int"), 123);
}

TEST_F(ConfigTest, HasKey) {
    auto& config = Config::instance();
    config.load_from_file(test_file_);
    
    EXPECT_TRUE(config.has("string_value"));
    EXPECT_FALSE(config.has("non_existent"));
}

TEST_F(ConfigTest, RemoveKey) {
    auto& config = Config::instance();
    config.load_from_file(test_file_);
    
    EXPECT_TRUE(config.has("string_value"));
    config.remove("string_value");
    EXPECT_FALSE(config.has("string_value"));
}