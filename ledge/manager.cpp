#include "ledge.h"

// -----Private functions-----
void LEDGE::manageLineAdmin(LineSegment &line) {
    static int new_admin_x, new_admin_y;
    static cv::Point2d line_center;
    line_center.x = (line.p1.x + line.p2.x) / 2.0;
    line_center.y = (line.p1.y + line.p2.y) / 2.0;
    new_admin_x = (int)((line_center.x - rf_border_shift.width) / rf_res.width);
    if (new_admin_x < 0 || ledge_res.width <= new_admin_x) return;
    new_admin_y = (int)((line_center.y - rf_border_shift.height) / rf_res.height);
    if (new_admin_y < 0 || ledge_res.height <= new_admin_y) return;
    // Take over admin to neighbor
    if (line.admin.x != new_admin_x || line.admin.y != new_admin_y) {
        LSG[new_admin_y][new_admin_x].initialize(line.p1, line.p2, line.e, line.group, line.line_status, {new_admin_x, new_admin_y});
        line.initialize();
    }
    // Update admin in related scarf blocks
    auto scarf_blocks = getScarfBlocks(LSG[new_admin_y][new_admin_x]);
    for (auto scarf_block : scarf_blocks) {
        if (LSG[scarf_block.y][scarf_block.x].line_status == Status::NoDetection) {
            LSG[scarf_block.y][scarf_block.x].admin = {new_admin_x, new_admin_y};
        }
    }
}


void LEDGE::checkDetectionProhibition(LineSegment &line, double threshold) {
    static double range = 0.5 * (rf_res.height + rf_res.width) * threshold * 2;
    static cv::Point2d mean_pt;
    static double rf_x, rf_y;
    mean_pt.x = (line.p1.x + line.p2.x) / 2.0;
    mean_pt.y = (line.p1.y + line.p2.y) / 2.0;
    for (double x = mean_pt.x - range; x <= mean_pt.x + range; x += 2 * range) {
        rf_x = (int)((x - rf_border_shift.width) / rf_res.width);
        if (rf_x >= 0 && rf_x < ledge_res.width && rf_x != line.admin.x && LSG[line.admin.y][rf_x].line_status == Status::NoDetection) {
            LSG[line.admin.y][rf_x].line_status = Status::ProhibitDetection;
        }
    }
    for (double y = mean_pt.y - range; y <= mean_pt.y + range; y += 2 * range) {
        rf_y = (int)((y - rf_border_shift.height) / rf_res.height);
        if (rf_y >= 0 && rf_y < ledge_res.height && rf_y != line.admin.y && LSG[rf_y][line.admin.x].line_status == Status::NoDetection) {
            LSG[rf_y][line.admin.x].line_status = Status::ProhibitDetection;
        }
    }
}

// ---------------------------


// -----Public functions------
void LEDGE::manageLineAdmins() {
    for (int y = 0; y < LSG.size(); y++) {
        for (int x = 0; x < LSG[y].size(); x++) {
            // Update administration
            if (LSG[y][x].line_status == Status::BadTrack || LSG[y][x].line_status == Status::GoodTrack) {
                manageLineAdmin(LSG[y][x]);
            }
        }
    }
}


void LEDGE::checkDetectionProhibitions() {
    for (int y = 0; y < LSG.size(); y++) {
        for (int x = 0; x < LSG[y].size(); x++) {
            // Prohibit detection in neighboring block
            if (LSG[y][x].line_status == Status::BadTrack || LSG[y][x].line_status == Status::GoodTrack) {
                checkDetectionProhibition(LSG[y][x], inlier_threshold);
            }
        }
    }
}


// Function for test ledge manager
void LEDGE::manageLineAdmin(cv::Point2d &line_p1, cv::Point2d &line_p2, int &lsid) {
    LEDGE::LineSegment ls = LEDGE::LineSegment(line_p1, line_p2, 0.0, lsid);
    manageLineAdmin(ls);
}


void LEDGE::checkDetectionProhibition(cv::Point2d &line_p1, cv::Point2d &line_p2, cv::Point2i &admin, double threshold) {
    LEDGE::LineSegment ls = LEDGE::LineSegment(line_p1, line_p2, 0, 0, Status::NoDetection, admin);
    checkDetectionProhibition(ls, threshold);
}
// ---------------------------