#include <yarp/os/all.h>
#include <event-driven/core.h>
#include <event-driven/vis.h>
#include <opencv2/opencv.hpp>
#include <ledge/ledge.h>
#include <filesystem>
#include <thread>


namespace fs = std::filesystem;
using yarp::os::Value;


void showProgressBar(double progress, double total) {
    int barWidth = 50;
    double progressRatio = progress / total;
    int pos = static_cast<int>(barWidth * progressRatio);

    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] Record progress: " << int(progressRatio * 100.0) << " %\r";
    std::cout.flush();
}


void recordLedgeCSV(LEDGE &ledge, std::ofstream &csv_file, const cv::Size &ledge_res, const double &timer) {
    csv_file << timer << ",";
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


void recordDisplay(cv::VideoWriter &vw, cv::Mat &img, cv::Size &save_size) {
    cv::Mat rs;
    cv::resize(img, rs, save_size);
    vw << rs;
}


int main(int argc, char *argv[]) {
    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setVerbose(false);
    rf.configure(argc, argv);

    if (rf.check("help")) {
        yInfo() << "--data\t<string>\t: path to input event dataset";
        yInfo() << "--fps\t<double>\t: process frequency";
        yInfo() << "--vout\t<string>\t: video out path";
        yInfo() << "--csv\t<string>\t: csv out path to record line segments";
        yInfo() << "--time_csv\t<string>\t: csv out path to record processing time";
        yInfo() << "--block_size \t<int>\t: ledge block size";
        yInfo() << "--inlier \t<double>\t: inlier threshold value for line fitting";
        yInfo() << "--alpha \t<double>\t: size ratio of scarf ring buffer";
        yInfo() << "--ch --cw \t<int>\t: height and width of camera resolution";
        yInfo() << "--fit_thresh \t<double>\t: threshold to determine if line segments are good";
        yInfo() << "--dp \t<double>\t: Perturbation size for tracking";
        yInfo() << "--line_width \t<int>\t: Line width for visualization: Default 3 [px]";
        return false;
    }

    // Parameter setting
    double fps = rf.check("fps", Value(24)).asFloat64();
    double fit_thresh = rf.check("fit_thresh", Value(0.2)).asFloat64();
    int block_size = rf.check("block_size", Value(14)).asInt32();
    double inlier = rf.check("inlier", Value(0.2)).asFloat64();
    double alpha = rf.check("alpha", Value(1.0)).asFloat64();
    bool line_track = rf.check("line_track", Value(true)).asBool();
    std::string datapath = rf.check("data", Value("/home/iit.local/aglover/data/m3ed/spot_indoor_building_loop_data/leftdvs/data.log")).asString();
    cv::Size res(rf.check("cw", Value(1280)).asInt32(), rf.check("ch", Value(720)).asInt32());
    double dp = rf.check("dp", Value(1.1)).asFloat64();
    int line_width = rf.check("line_width", Value(3)).asInt32();
    cv::Size savesize = {res.width * 2, res.height}; // Put 2 images horizontally
    double seconds = 30;
    double display_fps = 30;
    double data_timelength = 0;

    // Load .log event data
    ev::offlineLoader<ev::AE> eloader;
    yInfo() << "Loading data ... ";
    if (!eloader.load(datapath, seconds)) {
        yError() << "Could not open data file" << datapath;
        return false;
    }
    else {
        yInfo() << eloader.getinfo();
        data_timelength = eloader.getLength();
        yInfo() << "Data time length [s]: " << data_timelength;
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
    fs::path defaultpath = fs::path(getenv("HOME")) / fs::path("Downloads/ledge-offline-seq.mp4");
    fs::path vout_path = fs::path(rf.check("vout", Value(defaultpath.string())).asString());
    if (!fs::exists(vout_path.parent_path())) {
        if (fs::create_directories(vout_path.parent_path())) {
            yInfo() << "Directory to save ledge-offline-seq.mp4 created successfully.";
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

    // Initialize CSV file to record ledge results
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

    // Initialize CSV file to record ledge results
    fs::path time_csv_path = fs::path(rf.check("time_csv", Value("")).asString());
    std::ofstream time_csv_file;
    if (time_csv_path.empty()) {
        yInfo() << "Processing times of LEDGE are not recorded into CSV.";
    }
    else {
        if (!fs::exists(time_csv_path.parent_path())) {
            if (fs::create_directories(time_csv_path.parent_path())) {
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
        time_csv_file.open(time_csv_path.string());
        if (!time_csv_file.is_open()) {
            std::cerr << "Error: Could not open the file!" << std::endl;
        }
    }

    // Initialize process parameters
    bool progress = true;
    cv::Mat img;
    double timer = 0.0, visualize_timer = 0.0;
    char c;
    eloader.synchroniseRealtimeRead(0.0);
    eloader.incrementReadTill(timer);
    int num_events = 0;
    double tic, toc;
    double process_time_tracking = 0, process_time_detection = 0, process_time_scarf = 0;

    // ---------- Loop process ----------
    while (eloader.incrementReadTill(timer)) {
        // Update SCARF
        for (ev::offlineLoader<ev::AE>::iterator v = eloader.begin(); v != eloader.end(); v++) {
            tic = yarp::os::Time::now();
            ledge.update(v->x, v->y, v->p);
            toc = yarp::os::Time::now();
            process_time_scarf += toc - tic;
            num_events++;
        }

        // Track line segments
        if (line_track) {
            tic = yarp::os::Time::now();
            ledge.trackLinesAllBlock();
            ledge.checkDetectionProhibitions();
            ledge.manageLineAdmins();
            toc = yarp::os::Time::now();
            process_time_tracking = toc - tic;
        }

        // Detect line segments
        tic = yarp::os::Time::now();
        ledge.detectLines();
        toc = yarp::os::Time::now();
        process_time_detection = toc - tic;

        // Visualize and record display
        if (timer >= visualize_timer) {
            c = ledge.visualizeDisplay(window_name, img, line_width);
            if (c == '\e') break;
            recordDisplay(vw, img, savesize);
            if (!csv_path.empty()) {
                recordLedgeCSV(ledge, csv_file, ledge_res, timer);
                showProgressBar(timer, data_timelength);
            }
            else
                yInfo() << "Time [s]: " << timer << "\n";

            if (!time_csv_path.empty()) {
                time_csv_file << timer << "," << num_events << ","
                              << process_time_scarf << "," << process_time_detection
                              << "," << process_time_tracking << "\n";
                num_events = 0;
                process_time_scarf = 0;
            }
            visualize_timer += 1 / display_fps;
        }

        // Update timer
        timer += 1 / fps;
    }

    return true;
}