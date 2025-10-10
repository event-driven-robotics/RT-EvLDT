#include <yarp/os/all.h>
#include <event-driven/core.h>
#include <event-driven/vis.h>
#include <opencv2/opencv.hpp>
#include <ledge/ledge.h>
#include <filesystem>
#include <thread>


namespace fs = std::filesystem;
using yarp::os::Value;


void recordDisplay(cv::VideoWriter &vw, cv::Mat &img, cv::Size &save_size) {
    cv::Mat rs;
    cv::resize(img, rs, save_size);
    vw << rs;
}


void recordLedgeCSV(LEDGE &ledge, std::ofstream &csv_file, const cv::Size &ledge_res, ev::offlineLoader<ev::AE>::iterator &cur_ev, const double &start_time, bool &main_process_flg, bool &rec_process_flg) {
    // Loop process for recording
    while (main_process_flg) {
        // Record into CSV
        csv_file << cur_ev.timestamp() - start_time << ",";
        for (int rf_y = 0; rf_y < ledge_res.height; rf_y++) {
            for (int rf_x = 0; rf_x < ledge_res.width; rf_x++) {
                std::string ls_str = ledge.returnLineSegmentAsString(rf_x, rf_y);
                if (ls_str != "") {
                    csv_file << rf_x << "," << rf_y << "," << ls_str;
                }
            }
        }
        csv_file << std::endl;
    }
    rec_process_flg = false;
}


void visualizeDisplay(LEDGE &ledge, const std::string &window_name, double &display_fps, cv::Mat &img, bool &main_process_flg, bool &vis_process_flg, cv::VideoWriter &vw, cv::Size &save_size) {
    // Time counter
    double start, visualization;
    double time_sec_vis; // Unit: sec
    static char c = 0;

    // Loop process for visualization
    while (main_process_flg) {
        start = yarp::os::Time::now();

        // Visualization process
        c = ledge.visualizeDisplay(window_name, img);
        if (c == '\e') break;

        // Record display to video
        recordDisplay(vw, img, save_size);

        // Keep visualization at 30 Hz
        visualization = yarp::os::Time::now();

        time_sec_vis = visualization - start;
        yarp::os::Time::delay((1 / display_fps) - time_sec_vis);
    }
    vis_process_flg = false;
}


void detectLineSegments(LEDGE &ledge, bool &main_process_flg) {
    // Time counter
    double start, detection;
    double time_ms_detect; // Unit: ms

    while (main_process_flg) {
        start = yarp::os::Time::now();

        // Detection process
        ledge.detectLines();

        // Update processsing time
        detection = yarp::os::Time::now();
        time_ms_detect = (detection - start) * 1e3;
    }
}


void trackLineSegments(LEDGE &ledge, bool &main_process_flg) {
    // Time counter
    double start, tracking;
    double time_ms_track; // Unit: ms

    while (main_process_flg) {
        start = yarp::os::Time::now();

        // Tracking & Managing process
        ledge.trackLinesAllBlock();
        ledge.checkDetectionProhibitions();
        ledge.manageLineAdmins();

        // Update processsing time
        tracking = yarp::os::Time::now();
        time_ms_track = (tracking - start) * 1e3;
    }
}


int main(int argc, char *argv[]) {
    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setVerbose(false);
    rf.configure(argc, argv);

    if (rf.check("help")) {
        yInfo() << "--data\t<string>\t: path to input event dataset";
        yInfo() << "--vout\t<string>\t: video out path";
        yInfo() << "--csv\t<string>\t: csv out path for all line segments";
        yInfo() << "--block_size \t<int>\t: ledge block size";
        yInfo() << "--inlier \t<double>\t: inlier threshold value for line fitting";
        yInfo() << "--alpha \t<double>\t: size ratio of scarf ring buffer";
        yInfo() << "--ch --cw \t<int>\t: height and width of camera resolution";
        yInfo() << "--fit_thresh \t<double>\t: threshold to determine if line segments are good";
        yInfo() << "--dp \t<double>\t: Perturbation size for tracking";
        yInfo() << "--stop_time \t<double>\t: Stop time for LEDGE";
        return false;
    }

    // Parameter setting
    double fit_thresh = rf.check("fit_thresh", Value(0.2)).asFloat64();
    int block_size = rf.check("block_size", Value(14)).asInt32();
    double inlier = rf.check("inlier", Value(0.2)).asFloat64();
    double alpha = rf.check("alpha", Value(1.0)).asFloat64();
    bool line_track = rf.check("line_track", Value(true)).asBool();
    std::string datapath = rf.check("data", Value("/home/iit.local/aglover/data/m3ed/spot_indoor_building_loop_data/leftdvs/data.log")).asString();
    cv::Size res(rf.check("cw", Value(1280)).asInt32(), rf.check("ch", Value(720)).asInt32());
    double dp = rf.check("dp", Value(1.1)).asFloat64();
    cv::Size savesize = {res.width * 2 / 2, res.height / 2}; // Put 2 images horizontally and change the resolution in half
    double seconds = rf.check("stop_time", Value(30)).asFloat64();
    double display_fps = 30;

    // Load .log event data
    ev::offlineLoader<ev::AE> eloader;
    yInfo() << "Loading data ... ";
    if (!eloader.load(datapath, seconds)) {
        yError() << "Could not open data file" << datapath;
        return false;
    }
    else {
        yInfo() << eloader.getinfo();
    }

    // Initialize LEDGE
    LEDGE ledge;
    ledge.initialise(res, block_size, alpha, inlier, fit_thresh, line_track, dp);
    cv::Size ledge_res;
    std::tie(std::ignore, std::ignore, ledge_res, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore) = ledge.returnLedgeParameters();


    // Initialize window
    std::string window_name = "Left: SCARF | Right: LEDGE";
    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    cv::resizeWindow(window_name, {res.width * 2, res.height}); // Put 2 images horizontally

    // Initialize recorded video
    fs::path defaultpath = fs::path(getenv("HOME")) / fs::path("Downloads/ledge-offline-par.mp4");
    fs::path vout_path = fs::path(rf.check("vout", Value(defaultpath.string())).asString());
    if (!fs::exists(vout_path.parent_path())) {
        if (fs::create_directories(vout_path.parent_path())) {
            yInfo() << "Directory to save ledge-offline-par.mp4 created successfully.";
        }
        else {
            yError() << "Failed to create directory";
            return false;
        }
    }
    else {
        yInfo() << "Directory already exists.";
    }
    cv::VideoWriter vw;
    vw.open(vout_path.string(),
            cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
            display_fps, savesize, true);

    // Initialize CSV file to record ledge line segments
    fs::path csv_path = fs::path(rf.check("csv", Value("")).asString());
    std::ofstream csv_file;
    if (csv_path.empty()) {
        yInfo() << "LEDGE results are not recorded into CSV.";
    }
    else {
        if (!fs::exists(csv_path.parent_path())) {
            if (fs::create_directories(csv_path.parent_path())) {
                yInfo() << "Directory to save ledge-record.csv created successfully.";
            }
            else {
                yError() << "Failed to create directory";
                return false;
            }
        }
        else {
            yInfo() << "Directory already exists.";
        }
        csv_file.open(csv_path.string());
        if (!csv_file.is_open()) {
            std::cerr << "Error: Could not open the file!" << std::endl;
        }
    }

    // Initialize process parameters
    bool progress = true, main_process_flg = true, vis_process_flg = true, rec_process_flg = true;
    cv::Mat img;

    // Initialize Time counter for main loop
    double yarp_start; // Unit: sec
    double event_start_time;
    double curr_time = 0; // Unit: sec

    // Initialize thread
    ev::offlineLoader<ev::AE>::iterator v;
    std::thread vis_disp_thread(visualizeDisplay, std::ref(ledge), std::ref(window_name), std::ref(display_fps), std::ref(img), std::ref(main_process_flg), std::ref(vis_process_flg), std::ref(vw), std::ref(savesize));
    std::thread track_thread(trackLineSegments, std::ref(ledge), std::ref(main_process_flg));
    std::thread detect_thread(detectLineSegments, std::ref(ledge), std::ref(main_process_flg));
    std::thread record_csv_thread;
    if (csv_file.is_open()) {
        eloader.synchroniseRealtimeRead(yarp::os::Time::now());
        v = eloader.begin();
        event_start_time = eloader.getStartTime();
        record_csv_thread = std::thread(recordLedgeCSV, std::ref(ledge), std::ref(csv_file), std::ref(ledge_res), std::ref(v), std::ref(event_start_time), std::ref(main_process_flg), std::ref(rec_process_flg));
    }

    // ---------- Loop process ----------
    yarp_start = yarp::os::Time::now();
    eloader.synchroniseRealtimeRead(yarp::os::Time::now());

    while (eloader.incrementReadTill(yarp::os::Time::now()) && vis_process_flg) {
        // Update SCARF
        for (v = eloader.begin(); v != eloader.end(); v++) {
            ledge.update(v->x, v->y, v->p);
        }

        curr_time = yarp::os::Time::now() - yarp_start;
        yInfo() << "Time [s]: " << curr_time << "\n";
    }

    // Finialize whole process
    main_process_flg = false;
    vis_disp_thread.join();
    track_thread.join();
    detect_thread.join();
    if (csv_file.is_open()) {
        record_csv_thread.join();
    }

    return true;
}