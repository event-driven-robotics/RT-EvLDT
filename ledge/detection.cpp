#include "ledge.h"

// -----Private functions-----
LEDGE::LineSegment LEDGE::computePoints(cv::Point2d p0, cv::Point2d v0, cv::Vec2d X, cv::Vec2d Y) {
    LEDGE::LineSegment ls;
    double b = v0.x;
    double a = -v0.y;
    double c = v0.y * p0.x - v0.x * p0.y;

    if (fabs(v0.y) < 1e-3) {
        ls.p1 = {X[0], p0.y};
        ls.p2 = {X[1], p0.y};
    }
    else if (fabs(v0.x) < 1e-3) {
        ls.p1 = {p0.x, Y[0]};
        ls.p2 = {p0.x, Y[1]};
    }
    else {
        std::array<cv::Point2d, 4> pots;
        pots[0] = {(double)X[0], (a * X[0] + c) / -b};
        pots[1] = {(double)X[1], (a * X[1] + c) / -b};
        pots[2] = {(b * Y[0] + c) / -a, (double)Y[0]};
        pots[3] = {(b * Y[1] + c) / -a, (double)Y[1]};
        std::sort(pots.begin(), pots.end(), [](const cv::Point2d &a, const cv::Point2d &b) { return a.x < b.x; });
        ls.p1 = pots[1];
        ls.p2 = pots[2];
    }

    return ls;
}


void LEDGE::computeFitMeasure(LineSegment &L, const std::vector<cv::Point> &elc, double threshold) {
    // Initialize variables
    static int max_line = sqrt(rf_res.width * rf_res.width + rf_res.height * rf_res.height) + 1;
    static std::vector<double> tokens(max_line, 0);
    static int mean_len = 0.5 * (rf_res.width + rf_res.height);

    std::fill(tokens.begin(), tokens.end(), 0.0);
    double Y = L.p2.y - L.p1.y;
    double X = L.p2.x - L.p1.x;
    double length = sqrt(X * X + Y * Y);
    double iK = 1.0 / length;
    double T = mean_len * threshold;
    double count = 0;

    // Calculate fitting score
    for (auto &p : elc) {
        double d1 = iK * fabs((L.p1.y - p.y) * X - (L.p1.x - p.x) * Y); //distance from point to line
        int d2 = iK * fabs((p.x - L.p1.x) * X + (p.y - L.p1.y) * Y);    //distance along line (dot product)
        if (d1 < T) {
            count++;
            if (d2 >= 0 && d2 < tokens.size()) {
                tokens[d2] = 1.0;
            }
        }
    }
    L.e = std::accumulate(tokens.begin(), tokens.end(), 0.0) * iK * count / N;
}
// ---------------------------


// -----Public functions-----
void LEDGE::detectLines() {
    // Initialize variables
    cv::Vec4f l;
    static int max_line = sqrt(rf_res.width * rf_res.width + rf_res.height * rf_res.height) + 1;
    std::vector<double> tokens(max_line, 0);

    // Detect line segments
    for (int y = 0; y < LSG.size(); y++) {
        for (int x = 0; x < LSG[y].size(); x++) {
            if (track_mode) {
                if (LSG[y][x].line_status == Status::Detected || LSG[y][x].line_status == Status::BadTrack || LSG[y][x].line_status == Status::GoodTrack)
                    continue;
                else if (LSG[y][x].line_status == Status::ProhibitDetection) {
                    LSG[y][x].line_status = Status::NoDetection;
                    continue;
                }
            }
            auto elc = scarf.getList(x, y);
            if (elc.size() < std::max(scarf.getN() / 10, 3)) {
                LSG[y][x].e = 0.0;
                LSG[y][x].group = 0;
                continue;
            }
            cv::fitLine(elc, l, cv::DIST_FAIR, 0, 0.02, 0.02);

            auto new_line = computePoints({l[2], l[3]}, {l[0], l[1]},
                                          {(double)x * rf_res.width + rf_border_shift.width, (double)(x + 1) * rf_res.width + rf_border_shift.width},
                                          {(double)y * rf_res.height + rf_border_shift.height, (double)(y + 1) * rf_res.height + rf_border_shift.height});
            computeFitMeasure(new_line, elc, inlier_threshold);

            // Tracking is being activated
            if (new_line.e > fit_threshold) {
                LSG[y][x] = new_line;
                LSG[y][x].line_status = Status::Detected;
                LSG[y][x].admin = {x, y};
                LSG[y][x].group = latest_ls_id++;
            }
        }
    }
}
// --------------------------