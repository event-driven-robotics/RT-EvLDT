#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <event-driven/algs.h>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <map>
#include <tuple>
#include <functional>

class LEDGE {
private:
    ev::SCARF scarf;

    enum class Status {
        NoDetection = 0,       // No detected
        ProhibitDetection = 1, // Detection prohibited
        Detected = 2,          // Detected line segment and start tracking in next loop
        BadTrack = 3,          // Bad tracked line segment (fitting score <= threshold)
        GoodTrack = 4          // Good tracked line segment (fitting score > threshold)
    };

    inline static int LineStatusToInt(Status line_status) { return static_cast<int>(line_status); }

    struct LineSegment {
        // Params
        cv::Point2d p1;
        cv::Point2d p2;
        double e;
        int group;
        int unrefined_group;
        Status line_status;
        cv::Point2i admin; // rf coordinate with admin

        // Constructor
        LineSegment(cv::Point2d p1 = cv::Point2d(), cv::Point2d p2 = cv::Point2d(), double e = 0.0, int group = -1, Status line_status = Status::NoDetection, cv::Point2i admin = cv::Point2i(), int unrefined_group = -1);

        void initialize(cv::Point2d newP1 = cv::Point2d(), cv::Point2d newP2 = cv::Point2d(), double newE = 0.0, int newGroup = -1, Status new_line_status = Status::NoDetection, cv::Point2i newAdmin = cv::Point2i());
    };
    struct visualizeFlags {
        bool show_ev{true};
        bool show_fit{true};
        bool show_grid{false};
        bool show_lsg{true};
        bool show_bad_track{false};
    };

    // Values for Line Segment Groups
    std::vector<std::vector<LineSegment>> LSG;
    std::vector<double> LSG_counts;
    std::vector<double> LSG_unrefined_counts;
    std::vector<double> LSG_scores;

    // Parameters
    cv::Size ledge_res{{0, 0}};       // Resolution of LEDGE blocks
    cv::Size rf_res{{0, 0}};          // Resolution of each Receptive field (blocksize)
    cv::Size rf_border_shift{{0, 0}}; // Border shift size of SCARF [px]
    cv::Size image_res{{0, 0}};       // Resolution of display
    int N{0};                         // Size of ring buffer in each SCARF block
    double alpha{1.0};
    double inlier_threshold{0.2};
    double fit_threshold{0.2};
    bool track_mode = false;
    double dp{1.1}; // Perturbation size
    int latest_ls_id{0};

    // Core functions
    double calcLineLength(LineSegment &L);
    std::vector<cv::Point> getSpecificScarfLists(std::vector<cv::Point2i> scarf_blocks, std::function<std::vector<cv::Point>(int, int)> getScarfList);
    void createImageDetectionTracking(cv::Mat &img, visualizeFlags &vflags, int line_width = 3);

    // Detection functions
    LineSegment computePoints(cv::Point2d p0, cv::Point2d v0, cv::Vec2d X, cv::Vec2d Y);
    void computeFitMeasure(LineSegment &L, const std::vector<cv::Point> &elc, double threshold);

    // Tracking functions
    std::vector<cv::Point2i> getScarfBlocks(LineSegment &line);
    std::array<cv::Point2d, 2> perturbateEndPoint(const cv::Point2d &endpoint, const cv::Point2d &other_endpt, const cv::Point2d &admin);
    std::array<LineSegment, 5> perturbateLine(const LineSegment &line);
    inline bool normal_direct_X(const cv::Point2d &endpoint, const cv::Point2d &other_endpt);
    void computeFitMeasureMultiBlocksMultiPerturbation(std::array<LineSegment, 5> &Ls, const std::vector<cv::Point> &elcs_all, double threshold);

    // Manager functions
    void manageLineAdmin(LineSegment &line);
    void checkDetectionProhibition(LineSegment &line, double threshold);


public:
    // Core functions
    void initialise(cv::Size img_res, int block_size, double alpha = 1.0, double inlier = 0.2, double fit = 0.2, bool track_mode = false, double dp = 1.1);
    void initialise(cv::Size img_res, cv::Size ledge_res_, double alpha = 1.0, double inlier = 0.2, double fit = 0.2, bool track_mode = false, double dp = 1.1);
    inline void update(const int &u, const int &v, const int &p) { scarf.update(u, v, p); }
    char visualizeDisplay(const std::string &window_name, cv::Mat &img, int line_width = 3, double wait = 3.0);
    std::tuple<cv::Size, cv::Size, cv::Size, int, double, double, double, double> returnLedgeParameters();
    std::tuple<cv::Point2d, cv::Point2d, double, int, int, cv::Point2i, int> returnLineSegment(int rf_x, int rf_y);
    std::string returnLineSegmentAsString(int rf_x, int rf_y);

    // Detection functions
    void detectLines();

    // Tracking functions
    void trackLinesAllBlock();

    // Manager functions
    void checkDetectionProhibitions();
    void manageLineAdmins();

    // Test functions
    std::array<std::pair<cv::Point2d, cv::Point2d>, 5> perturbateLine(cv::Point2d line_p1, cv::Point2d line_p2);
    std::array<int, 5> checkPerturbatedLineID(cv::Point2d line_p1, cv::Point2d line_p2, int lsid);
    std::vector<cv::Point2i> getScarfBlocks(cv::Point2d line_p1, cv::Point2d line_p2);
    std::vector<cv::Point> getSpecificScarfLists(std::vector<cv::Point2i> scarf_blocks, std::string event_type);
    void manageLineAdmin(cv::Point2d &line_p1, cv::Point2d &line_p2, int &lsid);
    void checkDetectionProhibition(cv::Point2d &line_p1, cv::Point2d &line_p2, cv::Point2i &admin, double threshold);
};