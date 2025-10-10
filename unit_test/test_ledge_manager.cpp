#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <tuple>
#include <ledge/ledge.h>
#include "fixture.h"


// ------------------------------------------------------
// Manage line admin
// ------------------------------------------------------
// Parameterized fixture for manageLineAdmin
class TestLEDGEManageAdmin : public TestLEDGE,
                             public testing::WithParamInterface<std::tuple<
                                 std::array<cv::Point2d, 2>, // endpoints of line segment
                                 int,                        // line segment id
                                 cv::Point2i                 // ground truth of new admin
                                 >> {
};

INSTANTIATE_TEST_SUITE_P(
    TestLEDGESuite,
    TestLEDGEManageAdmin,
    testing::Values(
        std::make_tuple(
            std::array<cv::Point2d, 2>{{cv::Point(18, 35), cv::Point(23, 21)}},
            102,
            cv::Point2i(0, 1)),
        std::make_tuple(
            std::array<cv::Point2d, 2>{{cv::Point(21, 21), cv::Point(35, 35)}},
            23013,
            cv::Point2i(1, 1)),
        std::make_tuple(
            std::array<cv::Point2d, 2>{{cv::Point(21, 32), cv::Point(35, 40)}},
            1923,
            cv::Point2i(1, 2))));

TEST_P(TestLEDGEManageAdmin, manageLineAdmin) {
    // Prepare params
    auto [line_endpts, lsid, gt_new_admin] = GetParam();
    cv::Point2d est_line_endpt0, est_line_endpt1;
    int gt_lsid;

    // Run function
    ledge.manageLineAdmin(line_endpts[0], line_endpts[1], lsid);

    // Evaluation
    std::tie(est_line_endpt0, est_line_endpt1, std::ignore, gt_lsid, std::ignore, std::ignore, std::ignore) = ledge.returnLineSegment(gt_new_admin.x, gt_new_admin.y);
    EXPECT_EQ(line_endpts[0], est_line_endpt0);
    EXPECT_EQ(line_endpts[1], est_line_endpt1);
    EXPECT_EQ(lsid, gt_lsid);
}


// ------------------------------------------------------
// Check detection prohibition
// ------------------------------------------------------
// Parameterized fixture for checkDetectionProhibition
class TestLEDGEDetectionProhibition : public TestLEDGE,
                                      public testing::WithParamInterface<std::tuple<
                                          std::array<cv::Point2d, 2>, // endpoints of line segment
                                          cv::Point2i,                // ground truth of current admin
                                          std::array<int, 4>          // ground truth of prohibition of detection
                                          >> {
};

INSTANTIATE_TEST_SUITE_P(
    TestLEDGESuite,
    TestLEDGEDetectionProhibition,
    testing::Values(
        std::make_tuple(
            std::array<cv::Point2d, 2>{{cv::Point(318, 49), cv::Point(319, 63)}},
            cv::Point2i(22, 3),
            std::array<int, 4>{{0, 1, 0, 0}})));

TEST_P(TestLEDGEDetectionProhibition, checkDetectionProhibition) {
    // Prepare params
    auto [line_endpts, gt_cur_admin, gt_prohibition] = GetParam();
    double thresh = 0.2;
    int est_prohibition_px, est_prohibition_mx, est_prohibition_py, est_prohibition_my;

    // Run function
    ledge.checkDetectionProhibition(line_endpts[0], line_endpts[1], gt_cur_admin, thresh);

    // Evaluation
    std::tie(std::ignore, std::ignore, std::ignore, std::ignore, est_prohibition_px, std::ignore, std::ignore) = ledge.returnLineSegment(gt_cur_admin.x + 1, gt_cur_admin.y);
    std::tie(std::ignore, std::ignore, std::ignore, std::ignore, est_prohibition_mx, std::ignore, std::ignore) = ledge.returnLineSegment(gt_cur_admin.x - 1, gt_cur_admin.y);
    std::tie(std::ignore, std::ignore, std::ignore, std::ignore, est_prohibition_py, std::ignore, std::ignore) = ledge.returnLineSegment(gt_cur_admin.x, gt_cur_admin.y + 1);
    std::tie(std::ignore, std::ignore, std::ignore, std::ignore, est_prohibition_my, std::ignore, std::ignore) = ledge.returnLineSegment(gt_cur_admin.x, gt_cur_admin.y - 1);
    EXPECT_EQ(gt_prohibition[0], est_prohibition_px);
    EXPECT_EQ(gt_prohibition[1], est_prohibition_mx);
    EXPECT_EQ(gt_prohibition[2], est_prohibition_py);
    EXPECT_EQ(gt_prohibition[3], est_prohibition_my);
}