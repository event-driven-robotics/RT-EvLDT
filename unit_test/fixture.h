#pragma once

#include <gtest/gtest.h>
#include <ledge/ledge.h>

// Fixture
class TestLEDGE : public testing::Test {
protected:
    LEDGE ledge, ledge2;

    // Params
    cv::Size image_res, ledge_res;
    int block_size;
    double alpha, inlier, fit_thresh, dp;
    cv::Point2d endpt_1, endpt_2, admin_rf;

    virtual void SetUp() override {
        // Initialize params
        image_res = cv::Size(1280, 720);
        ledge_res = cv::Size(128, 72);
        block_size = 14;
        alpha = 1.0;
        inlier = 0.2;
        fit_thresh = 0.2;
        dp = 1.1;
        endpt_1 = {231, 119};
        endpt_2 = {245, 119};
        admin_rf = {17, 8};

        // Initialize LEDGE
        ledge.initialise(image_res, block_size, alpha, inlier, fit_thresh, dp);
        ledge2.initialise(image_res, ledge_res, alpha, inlier, fit_thresh, dp);
    }
};