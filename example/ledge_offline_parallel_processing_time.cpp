#include <yarp/os/all.h>
#include <event-driven/core.h>
#include <event-driven/vis.h>
#include <opencv2/opencv.hpp>
#include <ledge/ledge.h>
#include <filesystem>
#include <thread>
#include <numeric>


namespace fs = std::filesystem;
using yarp::os::Value;


void detectLineSegments(LEDGE &ledge, bool &main_process_flg, std::vector<double> &times_ms_detect, ev::offlineLoader<ev::AE>::iterator &cur_ev, const double &start_time, const double &start_record_time) {
    // Time counter
    double start, detection;
    double time_ms_detect;

    while (main_process_flg) {
        start = yarp::os::Time::now();

        // Detection process
        ledge.detectLines();

        // Update processsing time
        detection = yarp::os::Time::now();
        time_ms_detect = (detection - start) * 1e3;
        if (cur_ev.timestamp() - start_time > start_record_time) times_ms_detect.push_back(time_ms_detect);
    }
}


void trackLineSegments(LEDGE &ledge, bool &main_process_flg, std::vector<double> &times_ms_track, ev::offlineLoader<ev::AE>::iterator &cur_ev, const double &start_time, const double &start_record_time) {
    // Time counter
    double start, tracking;
    double time_ms_track;

    while (main_process_flg) {
        start = yarp::os::Time::now();

        // Tracking & Managing process
        ledge.trackLinesAllBlock();
        ledge.checkDetectionProhibitions();
        ledge.manageLineAdmins();

        // Update processsing time
        tracking = yarp::os::Time::now();
        time_ms_track = (tracking - start) * 1e3;
        if (cur_ev.timestamp() - start_time > start_record_time) times_ms_track.push_back(time_ms_track);
    }
}


int main(int argc, char *argv[]) {
    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setVerbose(false);
    rf.configure(argc, argv);

    if (rf.check("help")) {
        yInfo() << "--data\t<string>\t: path to input event dataset";
        yInfo() << "--csv\t<string>\t: csv out path to record computational time";
        yInfo() << "--block_size \t<int>\t: ledge block size";
        yInfo() << "--inlier \t<double>\t: inlier threshold value for line fitting";
        yInfo() << "--alpha \t<double>\t: size ratio of scarf ring buffer";
        yInfo() << "--ch --cw \t<int>\t: height and width of camera resolution";
        yInfo() << "--fit_thresh \t<double>\t: threshold to determine if line segments are good";
        yInfo() << "--dp \t<double>\t: Perturbation size for tracking";
        yInfo() << "--start_recording_time \t<double>\t: Start time for recording";
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
    double start_time_record = rf.check("start_recording_time", Value(5)).asFloat64();
    double seconds = rf.check("stop_time", Value(10)).asFloat64();
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

    // Initialize CSV file to record processing times
    fs::path csv_path = fs::path(rf.check("csv", Value("")).asString());
    std::ofstream csv_file;
    if (csv_path.empty()) {
        yInfo() << "Processing time are not recorded into CSV.";
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
    bool progress = true, main_process_flg = true, rec_process_flg = true;
    cv::Mat img;

    // Initialize Time counter for main loop
    double yarp_start; // Unit: sec
    double event_start_time;
    double curr_time = 0; // Unit: sec
    double tic = 0, toc = 0, time_scarf = 0;
    std::vector<double> times_ms_detect, times_ms_track;

    // Initialize thread
    ev::offlineLoader<ev::AE>::iterator v;
    eloader.synchroniseRealtimeRead(yarp::os::Time::now());
    v = eloader.begin();
    event_start_time = eloader.getStartTime();
    std::thread track_thread;
    if (line_track) {
        track_thread = std::thread(trackLineSegments, std::ref(ledge), std::ref(main_process_flg), std::ref(times_ms_track), std::ref(v), std::ref(event_start_time), std::ref(start_time_record));
    }
    std::thread detect_thread(detectLineSegments, std::ref(ledge), std::ref(main_process_flg), std::ref(times_ms_detect), std::ref(v), std::ref(event_start_time), std::ref(start_time_record));
    std::thread record_csv_thread;

    // ---------- Loop process ----------
    yarp_start = yarp::os::Time::now();
    eloader.synchroniseRealtimeRead(yarp::os::Time::now());

    while (eloader.incrementReadTill(yarp::os::Time::now())) {
        // Update SCARF
        for (v = eloader.begin(); v != eloader.end(); v++) {
            ledge.update(v->x, v->y, v->p);
        }
    }

    // Finialize threads
    main_process_flg = false;
    if (line_track) {
        track_thread.join();
    }
    detect_thread.join();

    // ---------- Check SCARF event rate ----------
    ev::offlineLoader<ev::AE> eloader_max;
    yInfo() << "Loading data ... ";
    if (!eloader_max.load(datapath, seconds)) {
        yError() << "Could not open data file" << datapath;
        return false;
    }
    eloader_max.synchroniseRealtimeRead(0.0);
    // Skip until start_time record
    eloader_max.incrementReadTill(start_time_record);
    for (v = eloader_max.begin(); v != eloader_max.end(); v++) {
        ledge.update(v->x, v->y, v->p);
    }

    // Update SCARF
    eloader_max.incrementReadTill(seconds);
    tic = yarp::os::Time::now();
    for (v = eloader_max.begin(); v != eloader_max.end(); v++) {
        ledge.update(v->x, v->y, v->p);
    }
    toc = yarp::os::Time::now();
    time_scarf = toc - tic;

    // Count num of events
    int num_events = 0;
    for (v = eloader_max.begin(); v != eloader_max.end(); v++) {
        num_events++;
    }

    // Extract processing times
    yInfo() << "--------------- Final results ---------------";
    yInfo() << "Dataset event rate: " << num_events / (seconds - start_time_record) * 1e-6 << " [Mev/s] \n";
    yInfo() << "SCARF process rate: " << num_events / time_scarf * 1e-6 << " [Mev/s] \n";
    yInfo() << "Detection: " << 1000.0 / (std::accumulate(times_ms_detect.begin(), times_ms_detect.end(), 0.0) / (double)(times_ms_detect.size())) << " [Hz] \n";
    if (line_track) {
        yInfo() << "Tracking:" << 1000.0 / (std::accumulate(times_ms_track.begin(), times_ms_track.end(), 0.0) / (double)(times_ms_track.size())) << " [Hz]";
    }

    // Export processing times into CSV
    if (csv_file.is_open()) {
        csv_file << num_events / seconds * 1e-6 << ","
                 << num_events / time_scarf * 1e-6 << ","
                 << 1000.0 / (std::accumulate(times_ms_detect.begin(), times_ms_detect.end(), 0.0) / (double)(times_ms_detect.size())) << ","
                 << 1000.0 / (std::accumulate(times_ms_track.begin(), times_ms_track.end(), 0.0) / (double)(times_ms_track.size()));
    }

    return true;
}