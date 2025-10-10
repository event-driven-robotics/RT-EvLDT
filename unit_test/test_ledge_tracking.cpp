#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <tuple>
#include <ledge/ledge.h>
#include "fixture.h"


// ------------------------------------------------------
// Perturbation of endpoints in line segment
// ------------------------------------------------------
// Parameterized fixture
class TestLEDGEPerturbation : public TestLEDGE,
                              public testing::WithParamInterface<std::tuple<
                                  cv::Point2d,                                       // endpoint0 of line segment
                                  cv::Point2d,                                       // endpoint1 of line segment
                                  std::array<std::pair<cv::Point2d, cv::Point2d>, 5> // ground truth of perturbated line segments
                                  >> {
};

INSTANTIATE_TEST_SUITE_P(
    TestLEDGESuite,
    TestLEDGEPerturbation,
    testing::Values(
        std::make_tuple(
            cv::Point2d(231, 119),
            cv::Point2d(245, 119),
            std::array<std::pair<cv::Point2d, cv::Point2d>, 5>{{std::make_pair(cv::Point2d(231, 119), cv::Point2d(245, 119)),
                                                                std::make_pair(cv::Point2d(231, 119 - 1.1), cv::Point2d(245, 119 - 1.1)),
                                                                std::make_pair(cv::Point2d(231, 119 - 1.1), cv::Point2d(245, 119 + 1.1)),
                                                                std::make_pair(cv::Point2d(231, 119 + 1.1), cv::Point2d(245, 119 - 1.1)),
                                                                std::make_pair(cv::Point2d(231, 119 + 1.1), cv::Point2d(245, 119 + 1.1))}}),
        std::make_tuple(
            cv::Point2d(231, 119),
            cv::Point2d(231, 133),
            std::array<std::pair<cv::Point2d, cv::Point2d>, 5>{{
                // p1 and p2 are switched depending on values of x
                std::make_pair(cv::Point2d(231, 119), cv::Point2d(231, 133)),
                std::make_pair(cv::Point2d(231 - 1.1, 119), cv::Point2d(231 - 1.1, 133)),
                std::make_pair(cv::Point2d(231 - 1.1, 119), cv::Point2d(231 + 1.1, 133)),
                std::make_pair(cv::Point2d(231 - 1.1, 133), cv::Point2d(231 + 1.1, 119)),
                std::make_pair(cv::Point2d(231 + 1.1, 119), cv::Point2d(231 + 1.1, 133)),
            }}),
        std::make_tuple(
            cv::Point2d(21, 34.4),
            cv::Point2d(27, 21),
            std::array<std::pair<cv::Point2d, cv::Point2d>, 5>{{std::make_pair(cv::Point2d(21, 34.4), cv::Point2d(27, 21)),
                                                                std::make_pair(cv::Point2d(21 - 1.1, 35), cv::Point2d(27 - 1.1, 21)),
                                                                std::make_pair(cv::Point2d(21 - 1.1, 35), cv::Point2d(27 + 1.1, 21)),
                                                                std::make_pair(cv::Point2d(21 + 1.1, 35), cv::Point2d(27 - 1.1, 21)),
                                                                std::make_pair(cv::Point2d(21 + 1.1, 35), cv::Point2d(27 + 1.1, 21))}}),
        std::make_tuple(
            cv::Point2d(21.3, 21),
            cv::Point2d(35, 30),
            std::array<std::pair<cv::Point2d, cv::Point2d>, 5>{{std::make_pair(cv::Point2d(21.3, 21), cv::Point2d(35, 30)),
                                                                std::make_pair(cv::Point2d(21, 21 - 1.1), cv::Point2d(35, 30 - 1.1)),
                                                                std::make_pair(cv::Point2d(21, 21 - 1.1), cv::Point2d(35, 30 + 1.1)),
                                                                std::make_pair(cv::Point2d(21, 21 + 1.1), cv::Point2d(35, 30 - 1.1)),
                                                                std::make_pair(cv::Point2d(21, 21 + 1.1), cv::Point2d(35, 30 + 1.1))}})));

/**
 * @brief Test for perturbating endpoint
 */
TEST_P(TestLEDGEPerturbation, perturbateEndPoint) {
    // Get parameters
    auto [endpt_p, endpt_2_p, gt_perturb_ls] = GetParam();

    // Prepare gt and result
    auto est_perturb_ls = ledge.perturbateLine(endpt_p, endpt_2_p);

    // Evaluation
    EXPECT_EQ(est_perturb_ls, gt_perturb_ls);
}


// ------------------------------------------------------
// Perturbation of line segment IDs
// ------------------------------------------------------
// Parameterized fixture
class TestLEDGEPerturbatedIDs : public TestLEDGE,
                                public testing::WithParamInterface<std::tuple<
                                    cv::Point2d,       // endpoint0 of line segment
                                    cv::Point2d,       // endpoint1 of line segment
                                    int,               // line segment ID
                                    std::array<int, 5> // ground truth of perturbated line segment IDs
                                    >> {
};

INSTANTIATE_TEST_SUITE_P(
    TestLEDGESuite,
    TestLEDGEPerturbatedIDs,
    testing::Values(
        std::make_tuple(
            cv::Point2d(231, 119),
            cv::Point2d(245, 119),
            256,
            std::array<int, 5>{256, 256, 256, 256, 256})));

/**
 * @brief Test for perturbated line segment IDs
 */
TEST_P(TestLEDGEPerturbatedIDs, perturbatedLineSegmentIDs) {
    // Get parameters
    auto [endpt_p, endpt_2_p, lsid, gt_perturb_lsids] = GetParam();

    // Prepare gt and result
    auto est_perturb_lsids = ledge.checkPerturbatedLineID(endpt_p, endpt_2_p, lsid);

    // Evaluation
    EXPECT_EQ(est_perturb_lsids, gt_perturb_lsids);
}


// ------------------------------------------------------
// Get scarf blocks
// ------------------------------------------------------
// Parameterized fixture for getScarfBlocks
class TestLEDGEGetScarfBlocks : public TestLEDGE,
                                public testing::WithParamInterface<std::tuple<
                                    cv::Point2d,             // endpoint 1 of line segment
                                    cv::Point2d,             // endpoint 2 of line segment
                                    std::vector<cv::Point2i> // ground truth of scarf blocks
                                    >> {
};

INSTANTIATE_TEST_SUITE_P(
    TestLEDGESuite,
    TestLEDGEGetScarfBlocks,
    testing::Values(
        std::make_tuple(cv::Point2d(7, 10), cv::Point2d(21, 19), std::vector<cv::Point2i>{cv::Point2i(0, 0)}),
        std::make_tuple(cv::Point2d(19, 7), cv::Point2d(9, 21), std::vector<cv::Point2i>{cv::Point2i(0, 0)}),
        std::make_tuple(cv::Point2d(7, 10), cv::Point2d(63, 20), std::vector<cv::Point2i>{cv::Point2i(0, 0), cv::Point2i(1, 0), cv::Point2i(2, 0), cv::Point2i(3, 0)}),
        std::make_tuple(cv::Point2d(231, 119), cv::Point2d(245, 119), std::vector<cv::Point2i>{cv::Point2i(16, 8)}),
        std::make_tuple(cv::Point2d(231, 119), cv::Point2d(231, 133), std::vector<cv::Point2i>{cv::Point2i(16, 8)}),
        std::make_tuple(cv::Point2d(230, 119), cv::Point2d(245, 119), std::vector<cv::Point2i>{cv::Point2i(15, 8), cv::Point2i(16, 8)})));

/**
 * @brief Test for getting scarf blocks from gline
 */
TEST_P(TestLEDGEGetScarfBlocks, getScarfBlocks) {
    // Get parameters
    auto [line_p1, line_p2, gt_scarf_blocks] = GetParam();

    // Prepare results from function
    auto est_scarf_blocks = ledge.getScarfBlocks(line_p1, line_p2);

    // Evaluation
    EXPECT_EQ(gt_scarf_blocks, est_scarf_blocks);
}


// ------------------------------------------------------
// Get events with getSpecificScarfLists
// ------------------------------------------------------
// Parameterized fixture for getSpecificScarfLists
class TestLEDGEScarfLists : public TestLEDGE,
                            public testing::WithParamInterface<
                                std::tuple<
                                    std::vector<cv::Point2i>, // coordinate of scarf blocks
                                    std::vector<cv::Point2i>, // events
                                    std::string,              // selected type of events "active"/"all"
                                    std::vector<cv::Point>    // ground truth of selected scarf lists
                                    >> {
};

INSTANTIATE_TEST_SUITE_P(
    TestLEDGESuite,
    TestLEDGEScarfLists,
    testing::Values(
        std::make_tuple(std::vector<cv::Point2i>{cv::Point2i(0, 0)},
                        std::vector<cv::Point2i>{cv::Point2i(10, 10), cv::Point2i(1, 1)},
                        "active",
                        std::vector<cv::Point>{cv::Point2i(10, 10)}),
        std::make_tuple(std::vector<cv::Point2i>{cv::Point2i(0, 0)},
                        std::vector<cv::Point2i>{cv::Point2i(10, 10), cv::Point2i(1, 1)},
                        "all",
                        std::vector<cv::Point>{cv::Point2i(10, 10), cv::Point2i(1, 1)}),
        std::make_tuple(std::vector<cv::Point2i>{cv::Point2i(16, 8), cv::Point2i(17, 8)},
                        std::vector<cv::Point2i>{
                            cv::Point2i(220, 130),
                            cv::Point2i(229, 124),
                            cv::Point2i(233, 130),
                            cv::Point2i(240, 120),
                            cv::Point2i(247, 119),
                            cv::Point2i(255, 133),
                            cv::Point2i(259, 130),
                            cv::Point2i(269, 120)},
                        "active",
                        std::vector<cv::Point>{
                            cv::Point2i(233, 130), cv::Point2i(240, 120), cv::Point2i(247, 119)}),
        std::make_tuple(std::vector<cv::Point2i>{cv::Point2i(16, 8), cv::Point2i(17, 8)},
                        std::vector<cv::Point2i>{
                            cv::Point2i(220, 130),
                            cv::Point2i(229, 124),
                            cv::Point2i(233, 130),
                            cv::Point2i(240, 120),
                            cv::Point2i(247, 119),
                            cv::Point2i(255, 133),
                            cv::Point2i(259, 130),
                            cv::Point2i(269, 120)},
                        "all",
                        std::vector<cv::Point>{
                            cv::Point2i(229, 124), cv::Point2i(233, 130), cv::Point2i(240, 120), cv::Point2i(247, 119),
                            cv::Point2i(240, 120), cv::Point2i(247, 119), cv::Point2i(255, 133), cv::Point2i(259, 130)})));

/**
 * @brief Test for getSpecificScarfLists
 * 
 */
TEST_P(TestLEDGEScarfLists, getSpecificScarfLists) {
    // Get parameters
    auto [scarf_blocks, scarf_events, event_type, gt_scarf_lists] = GetParam();

    // Prepare results from function
    for (auto e : scarf_events) ledge.update(e.x, e.y, 1);
    auto est_scarf_lists_with_zeros = ledge.getSpecificScarfLists(scarf_blocks, event_type);
    std::vector<cv::Point> est_scarf_lists;
    for (auto est_e : est_scarf_lists_with_zeros) {
        if (est_e.x != 0 && est_e.y != 0) est_scarf_lists.push_back(est_e);
    }

    // Evaluation
    EXPECT_EQ(gt_scarf_lists, est_scarf_lists);
}