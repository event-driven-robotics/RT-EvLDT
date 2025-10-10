#include "ledge.h"

// -----Private functions-----
LEDGE::LineSegment::LineSegment(cv::Point2d p1, cv::Point2d p2, double e, int group, Status line_status, cv::Point2i admin, int unrefined_group)
    : p1(p1), p2(p2), e(e), group(group), line_status(line_status), admin(admin), unrefined_group(unrefined_group) {}


void LEDGE::LineSegment::initialize(cv::Point2d newP1, cv::Point2d newP2, double newE, int newGroup, Status new_line_status, cv::Point2i newAdmin) {
    p1 = newP1;
    p2 = newP2;
    e = newE;
    group = newGroup;
    line_status = new_line_status;
    admin = newAdmin;
}


std::vector<cv::Point> LEDGE::getSpecificScarfLists(std::vector<cv::Point2i> scarf_blocks, std::function<std::vector<cv::Point>(int, int)> getScarfList) {
    std::vector<cv::Point> scarf_lists;
    // Get Scarf points
    for (auto scarf_block : scarf_blocks) {
        auto scarf_list = getScarfList(scarf_block.x, scarf_block.y);
        scarf_lists.insert(scarf_lists.end(), scarf_list.begin(), scarf_list.end());
    }
    return scarf_lists;
}


void LEDGE::createImageDetectionTracking(cv::Mat &img, visualizeFlags &vflags, int line_width) {
    img = cv::Mat::zeros({image_res.width * 2, image_res.height}, CV_8UC3);
    //line-fitting (Detection / Bad track / Good track)
    if (vflags.show_fit) {
        cv::Mat hsv = cv::Mat::zeros(image_res, CV_8UC3);
        for (auto &row : LSG) {
            for (auto &L : row) {
                if (track_mode) {
                    //hue(0-179), value, saturation
                    if (L.line_status == Status::Detected)
                        cv::line(hsv, L.p1, L.p2, {135, 255, 220}, line_width);
                    else if (L.line_status == Status::BadTrack && vflags.show_bad_track)
                        cv::line(hsv, L.p1, L.p2, {0, 255, 150}, line_width);
                    else if (L.line_status == Status::GoodTrack)
                        cv::line(hsv, L.p1, L.p2, {70, 255, 220}, line_width);
                }
                else {
                    if (L.e > fit_threshold) {
                        //hue, value, saturation
                        cv::line(hsv, L.p1, L.p2, {135, 255, 220}, line_width);
                    }
                }
            }
        }
        cv::cvtColor(hsv, img(cv::Rect(img.cols / 2, 0, hsv.cols, hsv.rows)), cv::COLOR_HSV2BGR);
    }

    // Line Segment Groups
    if (vflags.show_lsg) {
        static int angle_dstr = 20, angle_diff = 180 / angle_dstr;
        cv::Mat hsv_lsg = cv::Mat::zeros(image_res, CV_8UC3);
        for (auto &row : LSG) {
            for (auto &L : row) {
                if (L.group >= 0 && LSG_scores[L.group] > fit_threshold && LSG_counts[L.group] > 3) {
                    //hue(0-179), value, saturation
                    cv::line(hsv_lsg, L.p1, L.p2, cv::Scalar((L.group / angle_dstr + angle_diff * (L.group % angle_dstr)), 255.0, 150.0), 3);
                }
            }
        }
        cv::cvtColor(hsv_lsg, img(cv::Rect(0, 0, hsv_lsg.cols, hsv_lsg.rows)), cv::COLOR_HSV2BGR);
    }

    // Scarf
    if (vflags.show_ev) {
        cv::Mat img32 = scarf.getSurface();
        cv::Mat img8U, imgBGR;
        img32.convertTo(img8U, CV_8U, 255);
        cv::cvtColor(img8U, imgBGR, cv::COLOR_GRAY2BGR);
        img(cv::Rect(0, 0, imgBGR.cols, imgBGR.rows)) += imgBGR;
    }

    // LEDGE Grid
    if (vflags.show_grid) {
        cv::Mat grid = cv::Mat::zeros(image_res, CV_8UC3);
        for (auto i = 0; i < LSG.size(); i++) {
            if (i == 0 || i == LSG.size() - 1) {
                cv::line(grid, {0, i * rf_res.height + rf_border_shift.height}, {image_res.width, i * rf_res.height + rf_border_shift.height}, {0, 160, 0});
            }
            else {
                cv::line(grid, {0, i * rf_res.height + rf_border_shift.height}, {image_res.width, i * rf_res.height + rf_border_shift.height}, {160, 0, 0});
            }
        }
        for (auto i = 0; i < LSG[0].size(); i++) {
            if (i == 0 || i == LSG[0].size() - 1) {
                cv::line(grid, {i * rf_res.width + rf_border_shift.width, 0}, {i * rf_res.width + rf_border_shift.width, image_res.height}, {0, 160, 0});
            }
            else {
                cv::line(grid, {i * rf_res.width + rf_border_shift.width, 0}, {i * rf_res.width + rf_border_shift.width, image_res.height}, {160, 0, 0});
            }
        }
        img(cv::Rect(0, 0, grid.cols, grid.rows)) += grid;
        img(cv::Rect(img.cols / 2, 0, grid.cols, grid.rows)) += grid;
    }
}


// -----Public functions-----
void LEDGE::initialise(cv::Size img_res, int block_size, double alpha, double inlier, double fit, bool track_mode, double dp) {
    // Initialize scarf
    scarf.initialise(img_res, block_size, alpha);

    // Reflect scarf params into ledge params
    std::tie(ledge_res, rf_res, N, rf_border_shift) = scarf.getScarfParams();
    image_res = img_res;
    LSG.resize(ledge_res.height);
    for (auto &r : LSG)
        r.resize(ledge_res.width, LineSegment());
    this->alpha = alpha;
    inlier_threshold = inlier;
    fit_threshold = fit;
    this->track_mode = track_mode;
    this->dp = dp;
    LSG_counts.resize(ledge_res.area());
    LSG_unrefined_counts.resize(ledge_res.area());
    LSG_scores.resize(ledge_res.area());
}


void LEDGE::initialise(cv::Size img_res, cv::Size ledge_res_, double alpha, double inlier, double fit, bool track_mode, double dp) {
    // Initialize scarf
    scarf.initialise(img_res, ledge_res_, alpha);

    // Reflect scarf params into ledge params
    std::tie(ledge_res, rf_res, N, rf_border_shift) = scarf.getScarfParams();
    image_res = img_res;
    LSG.resize(ledge_res.height);
    for (auto &r : LSG)
        r.resize(ledge_res.width, LineSegment());
    this->alpha = alpha;
    inlier_threshold = inlier;
    fit_threshold = fit;
    this->track_mode = track_mode;
    this->dp = dp;
    LSG_counts.resize(ledge_res.area());
    LSG_unrefined_counts.resize(ledge_res.area());
    LSG_scores.resize(ledge_res.area());
}


char LEDGE::visualizeDisplay(const std::string &window_name, cv::Mat &img, int line_width, double wait) {
    static visualizeFlags vflags;

    createImageDetectionTracking(img, vflags, line_width);

    char c = 0;
    cv::imshow(window_name, img);
    if (wait >= 0.0) {
        c = cv::waitKey(wait);
        if (c == '1') vflags.show_ev = !vflags.show_ev;
        if (c == '2') vflags.show_fit = !vflags.show_fit;
        if (c == '3') vflags.show_grid = !vflags.show_grid;
        if (c == '4') vflags.show_lsg = !vflags.show_lsg;
        if (c == 'g') vflags.show_bad_track = !vflags.show_bad_track;
    }
    return c;
}


double LEDGE::calcLineLength(LEDGE::LineSegment &L) {
    double length = sqrt((L.p2.y - L.p1.y) * (L.p2.y - L.p1.y) + (L.p2.x - L.p1.x) * (L.p2.x - L.p1.x));
    return length;
}


std::tuple<cv::Size, cv::Size, cv::Size, int, double, double, double, double> LEDGE::returnLedgeParameters() {
    std::tuple params = {
        rf_res,
        image_res,
        ledge_res,
        N,
        alpha,
        inlier_threshold,
        fit_threshold,
        dp};
    return params;
}


std::tuple<cv::Point2d, cv::Point2d, double, int, int, cv::Point2i, int> LEDGE::returnLineSegment(int rf_x, int rf_y) {
    LineSegment ls = LSG[rf_y][rf_x];
    std::tuple line_segment = {
        ls.p1,
        ls.p2,
        ls.e,
        ls.group,
        LineStatusToInt(ls.line_status),
        ls.admin,
        ls.unrefined_group};
    return line_segment;
}


std::string LEDGE::returnLineSegmentAsString(int rf_x, int rf_y) {
    LineSegment ls = LSG[rf_y][rf_x];
    std::stringstream ss;
    if (ls.line_status == LEDGE::Status::Detected || ls.line_status == LEDGE::Status::BadTrack || ls.line_status == LEDGE::Status::GoodTrack) {
        ss << ls.p1.x << "," << ls.p1.y << ","
           << ls.p2.x << "," << ls.p2.y << ","
           << ls.e << ","
           << ls.group << ","
           << LineStatusToInt(ls.line_status) << ","
           << ls.admin.x << "," << ls.admin.y << ",";
    }
    return ss.str();
}


// Function for test ledge core
std::vector<cv::Point> LEDGE::getSpecificScarfLists(std::vector<cv::Point2i> scarf_blocks, std::string event_type) {
    std::function<std::vector<cv::Point>(int, int)> getScarfList;
    if (event_type == "active")
        getScarfList = [this](int u, int v) { return scarf.getList(u, v); };
    else if (event_type == "all")
        getScarfList = [this](int u, int v) { return scarf.getAll(u, v); };
    else
        getScarfList = [this](int u, int v) { return scarf.getList(u, v); };
    return getSpecificScarfLists(scarf_blocks, getScarfList);
}