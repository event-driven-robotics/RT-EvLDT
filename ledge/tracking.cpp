#include "ledge.h"

// -----Private functions-----
std::vector<cv::Point2i> LEDGE::getScarfBlocks(LEDGE::LineSegment &line) {
    // Sort endpoints
    if (line.p1.x > line.p2.x) {
        std::swap(line.p1, line.p2);
    }
    // Variables
    std::vector<cv::Point2i> scarf_blocks;
    static cv::Point2d rf_crd, rf_crd_p2;
    static cv::Point2i range_top_left, range_bottom_right;
    static std::array<cv::Point2d, 4> block_corners;
    static std::array<double, 4> side_corners;
    rf_crd = {(line.p1.x - rf_border_shift.width) / (double)rf_res.width, (line.p1.y - rf_border_shift.height) / (double)rf_res.height};
    rf_crd_p2 = {(line.p2.x - rf_border_shift.width) / (double)rf_res.width, (line.p2.y - rf_border_shift.height) / (double)rf_res.height};
    range_top_left = {(int)std::floor(std::min(rf_crd.x, rf_crd_p2.x)), (int)std::floor(std::min(rf_crd.y, rf_crd_p2.y))};
    range_bottom_right = {(int)std::ceil(std::max(rf_crd.x, rf_crd_p2.x)), (int)std::ceil(std::max(rf_crd.y, rf_crd_p2.y))};

    // Line segment is on border
    if (range_top_left.x == range_bottom_right.x) range_bottom_right.x++;
    if (range_top_left.y == range_bottom_right.y) range_bottom_right.y++;

    // Add return value as scarf_blocks
    for (int i = std::max(0, range_top_left.x); i < std::min(ledge_res.width, range_bottom_right.x); i++) {
        for (int j = std::max(0, range_top_left.y); j < std::min(ledge_res.height, range_bottom_right.y); j++) {
            scarf_blocks.emplace_back(i, j);
        }
    }
    return scarf_blocks;
}


inline bool LEDGE::normal_direct_X(const cv::Point2d &endpoint, const cv::Point2d &other_endpt) {
    auto line_vec = other_endpt - endpoint;
    double cos_angle = line_vec.x / cv::norm(line_vec);
    if (-1 / sqrt(2) < cos_angle && cos_angle < 1 / sqrt(2))
        return true;
    else
        return false;
}


std::array<cv::Point2d, 2> LEDGE::perturbateEndPoint(const cv::Point2d &endpoint, const cv::Point2d &other_endpt, const cv::Point2d &admin) {
    std::array<cv::Point2d, 2> perturbated_endpoint = {endpoint, endpoint};

    // Endpoint is on X axis of scarf block
    if (std::abs(std::fmod(endpoint.x - rf_border_shift.width, (double)rf_res.width)) < 1e-10) {
        // Endpoint is close to the vertex of block
        if (std::abs((endpoint.y - rf_border_shift.height) - std::round((endpoint.y - rf_border_shift.height) / rf_res.height) * rf_res.height) < 1.0) {
            // Normal vector of line segment is X direction
            if (normal_direct_X(endpoint, other_endpt)) {
                // Perturbated in X direction
                perturbated_endpoint[0] = {endpoint.x - dp, std::round((endpoint.y - rf_border_shift.height) / rf_res.height) * rf_res.height + rf_border_shift.height};
                perturbated_endpoint[1] = {endpoint.x + dp, std::round((endpoint.y - rf_border_shift.height) / rf_res.height) * rf_res.height + rf_border_shift.height};
            }
            // Normal vector of line segment is Y direction
            else {
                // Perturbated in Y direction
                perturbated_endpoint[0] = {endpoint.x, endpoint.y - dp};
                perturbated_endpoint[1] = {endpoint.x, endpoint.y + dp};
            }
        }
        // Endpoint is far enough from the vertex of block
        else {
            // Perturbated in Y direction
            perturbated_endpoint[0] = {endpoint.x, endpoint.y - dp};
            perturbated_endpoint[1] = {endpoint.x, endpoint.y + dp};
        }
    }
    // Endpoint is on Y axis of scarf block
    else if (std::abs(std::fmod(endpoint.y - rf_border_shift.height, (double)rf_res.height)) < 1e-10) {
        // Endpoint is close to the vertex of block
        if (std::abs((endpoint.x - rf_border_shift.width) - std::round((endpoint.x - rf_border_shift.width) / rf_res.width) * rf_res.width) < 1.0) {
            // Normal vector of line segment is X direction
            if (normal_direct_X(endpoint, other_endpt)) {
                // Perturbated in X direction
                perturbated_endpoint[0] = {endpoint.x - dp, endpoint.y};
                perturbated_endpoint[1] = {endpoint.x + dp, endpoint.y};
            }
            // Normal vector of line segment is Y direction
            else {
                // Perturbated in Y direction
                perturbated_endpoint[0] = {std::round((endpoint.x - rf_border_shift.width) / rf_res.width) * rf_res.width + rf_border_shift.width, endpoint.y - dp};
                perturbated_endpoint[1] = {std::round((endpoint.x - rf_border_shift.width) / rf_res.width) * rf_res.width + rf_border_shift.width, endpoint.y + dp};
            }
        }
        else {
            // Perturbated in X direction
            perturbated_endpoint[0] = {endpoint.x - dp, endpoint.y};
            perturbated_endpoint[1] = {endpoint.x + dp, endpoint.y};
        }
    }
    // Check range of perturbation
    for (auto it = perturbated_endpoint.begin(); it != perturbated_endpoint.end(); it++) {
        it->x = std::min(std::max((double)rf_border_shift.width, it->x), (double)image_res.width);
        it->y = std::min(std::max((double)rf_border_shift.height, it->y), (double)image_res.height);
    }
    return perturbated_endpoint;
}


std::array<LEDGE::LineSegment, 5> LEDGE::perturbateLine(const LEDGE::LineSegment &line) {
    std::array<cv::Point2d, 2> p1_perturbation = perturbateEndPoint(line.p1, line.p2, line.admin);
    std::array<cv::Point2d, 2> p2_perturbation = perturbateEndPoint(line.p2, line.p1, line.admin);
    std::array<LEDGE::LineSegment, 5> perturbated_lines;

    perturbated_lines[0] = line;
    for (size_t i = 0; i < p1_perturbation.size(); i++) {
        for (size_t j = 0; j < p2_perturbation.size(); j++) {
            if (p1_perturbation[i].x <= p2_perturbation[j].x) {
                perturbated_lines[i * p1_perturbation.size() + j + 1] = LEDGE::LineSegment(p1_perturbation[i], p2_perturbation[j], line.e, line.group, line.line_status, line.admin);
            }
            else {
                perturbated_lines[i * p1_perturbation.size() + j + 1] = LEDGE::LineSegment(p2_perturbation[j], p1_perturbation[i], line.e, line.group, line.line_status, line.admin);
            }
        }
    }
    return perturbated_lines;
}


// Compute fitting score with multiple scarf blocks and multiple line segments
void LEDGE::computeFitMeasureMultiBlocksMultiPerturbation(std::array<LineSegment, 5> &Ls, const std::vector<cv::Point> &elcs_all, double threshold) {
    static int max_line = (int)(calcLineLength(Ls[0]) + 1.0);
    static std::vector<double> tokens(max_line, 0);
    static int mean_len = 0.5 * (rf_res.width + rf_res.height);
    static double T = mean_len * threshold;
    static double Tp = T + dp;
    static double X, Y, length, iK, count;

    std::vector<bool> elcs_process(elcs_all.size(), true);

    // Calculate fitting scores in each line segments
    for (size_t i = 0; i < Ls.size(); i++) {
        std::fill(tokens.begin(), tokens.end(), 0.0);
        Y = Ls[i].p2.y - Ls[i].p1.y;
        X = Ls[i].p2.x - Ls[i].p1.x;
        length = sqrt(X * X + Y * Y);
        iK = 1.0 / length;
        count = 0;
        for (size_t elcs_idx = 0; elcs_idx < elcs_all.size(); elcs_idx++) {
            if (!elcs_process[elcs_idx]) continue;
            auto p = elcs_all[elcs_idx];
            double d1 = iK * fabs((Ls[i].p1.y - p.y) * X - (Ls[i].p1.x - p.x) * Y); //distance from point to line
            if (i == 0 && Tp < d1) {
                elcs_process[elcs_idx] = false;
                continue;
            }
            int d2 = iK * fabs((p.x - Ls[i].p1.x) * X + (p.y - Ls[i].p1.y) * Y); //distance along line (dot product)
            if (d1 < T && d2 >= 0 && d2 < tokens.size()) {
                count++;
                tokens[d2] = 1.0;
            }
        }

        // TODO: Optimize how to calculate fitting score even with muliple blocks
        Ls[i].e = (std::accumulate(tokens.begin(), tokens.end(), 0.0) * iK) * (count / N); // * (length / mean_len);
    }
}
// ---------------------------


// -----Public functions-----
void LEDGE::trackLinesAllBlock() {
    static int mean_len = 0.5 * (rf_res.width + rf_res.height);
    for (int y = 0; y < LSG.size(); y++) {
        for (int x = 0; x < LSG[y].size(); x++) {
            if (LSG[y][x].line_status == Status::NoDetection || LSG[y][x].line_status == Status::ProhibitDetection)
                continue;
            else {
                // ------Tracking process------
                std::vector<cv::Point2i> scarf_blocks = getScarfBlocks(LSG[y][x]);

                // Check if active scarf includes enough events
                auto elcs_active = getSpecificScarfLists(scarf_blocks, [this](int u, int v) { return scarf.getList(u, v); });
                if (elcs_active.size() < std::max(scarf.getN() / 10, 3) && LSG[y][x].admin.x == x && LSG[y][x].admin.y == y) {
                    LSG[y][x].initialize();
                    continue;
                }

                // Check if the length of line segment is enough
                if (calcLineLength(LSG[y][x]) < 0.5 * mean_len) {
                    LSG[y][x].initialize();
                    continue;
                }

                // Get all (active and inactive) events in all blocks on line segment
                std::vector<cv::Point> elcs_all;
                if (scarf_blocks.size() > 1)
                    elcs_all = getSpecificScarfLists(scarf_blocks, [this](int u, int v) { return scarf.getList(u, v); });
                else
                    elcs_all = getSpecificScarfLists(scarf_blocks, [this](int u, int v) { return scarf.getAll(u, v); });

                // Perturbates line segment
                std::array<LineSegment, 5> perturbated_lines = perturbateLine(LSG[y][x]);
                computeFitMeasureMultiBlocksMultiPerturbation(perturbated_lines, elcs_all, inlier_threshold);

                // Select best perturbated line segment
                auto best_perturbated_line_itr = std::max_element(perturbated_lines.begin(), perturbated_lines.end(),
                                                                  [](const LineSegment &a, const LineSegment &b) { return a.e < b.e; });
                LSG[y][x] = *best_perturbated_line_itr;

                // Change track flg depending on fitting score
                if (LSG[y][x].e > fit_threshold)
                    LSG[y][x].line_status = Status::GoodTrack;
                else
                    LSG[y][x].line_status = Status::BadTrack;
            }
        }
    }
}


// Function for test ledge tracking
std::array<std::pair<cv::Point2d, cv::Point2d>, 5> LEDGE::perturbateLine(cv::Point2d line_p1, cv::Point2d line_p2) {
    LEDGE::LineSegment ls = LEDGE::LineSegment(line_p1, line_p2);
    auto perturbated_ls = perturbateLine(ls);
    std::array<std::pair<cv::Point2d, cv::Point2d>, 5> perturbated_endpts;
    for (int i = 0; i < perturbated_ls.size(); i++) {
        perturbated_endpts[i].first = perturbated_ls[i].p1;
        perturbated_endpts[i].second = perturbated_ls[i].p2;
    }
    return perturbated_endpts;
}


std::array<int, 5> LEDGE::checkPerturbatedLineID(cv::Point2d line_p1, cv::Point2d line_p2, int lsid) {
    LEDGE::LineSegment ls = LEDGE::LineSegment(line_p1, line_p2, 0.0, lsid);
    auto perturbated_ls = perturbateLine(ls);
    std::array<int, 5> perturbated_line_ids;
    for (int i = 0; i < perturbated_line_ids.size(); i++) {
        perturbated_line_ids[i] = perturbated_ls[i].group;
    }
    return perturbated_line_ids;
}


std::vector<cv::Point2i> LEDGE::getScarfBlocks(cv::Point2d line_p1, cv::Point2d line_p2) {
    LEDGE::LineSegment ls = LEDGE::LineSegment(line_p1, line_p2);
    return getScarfBlocks(ls);
}
// --------------------------