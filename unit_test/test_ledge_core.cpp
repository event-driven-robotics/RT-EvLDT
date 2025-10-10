#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <tuple>
#include <ledge/ledge.h>
#include "fixture.h"


// ------------------------------------------------------
// Initialization with block size
// ------------------------------------------------------
/**
 * @brief Test for initialization with block size
 */
TEST_F(TestLEDGE, initialize_block_size) {
    // Prepare params and gt
    cv::Size est_rf_res, est_ledge_res;
    int est_N;
    std::tie(est_rf_res, std::ignore, est_ledge_res, est_N, std::ignore, std::ignore, std::ignore, std::ignore) = ledge.returnLedgeParameters();
    cv::Size gt_rf_res((int)block_size, (int)block_size);
    cv::Size gt_ledge_res((int)image_res.width / block_size - 1, (int)image_res.height / block_size - 1);
    int gt_N = 14 * 14 * 1 / 2;

    // Evaluation
    EXPECT_EQ(est_rf_res, gt_rf_res);
    EXPECT_EQ(est_ledge_res, gt_ledge_res);
    EXPECT_EQ(est_N, gt_N);
}


// ------------------------------------------------------
// Initialization with ledge_res
// ------------------------------------------------------
/**
 * @brief Test for initialization with ledge_res
 */
TEST_F(TestLEDGE, initialize_ledge_res) {
    // Prepare params and gt
    cv::Size est_rf_res, est_ledge_res;
    int est_N;
    std::tie(est_rf_res, std::ignore, est_ledge_res, est_N, std::ignore, std::ignore, std::ignore, std::ignore) = ledge2.returnLedgeParameters();
    cv::Size gt_rf_res = {8, 8}; // 1280/(128+1(border))-1(even number)=8, 720/(72+1(border))-1(even number)=8
    int gt_N = 8 * 8 * 1 / 2;

    // Evaluation
    EXPECT_EQ(est_rf_res, gt_rf_res);
    EXPECT_EQ(est_ledge_res, ledge_res);
    EXPECT_EQ(est_N, gt_N);
}
