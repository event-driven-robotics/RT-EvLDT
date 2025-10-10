#include <yarp/os/all.h>
#include <event-driven/core.h>
#include <thread>
#include <filesystem>
#include <ledge/ledge.h>


using namespace yarp::os;
namespace fs = std::filesystem;

class ledgeModule : public yarp::os::RFModule {
private:
    LEDGE ledge;
    ev::window<ev::AE> input_port;
    std::thread events_process, detection_process, tracking_process;
    bool streaming{false};
    fs::path vout_path;
    double fit_thresh{0.2}, inlier{0.2}, alpha{1.0}, display_fps{30}, dp{1.1};
    int block_size{14};
    cv::Size res, savesize;
    bool line_track{false};
    std::string window_name;
    cv::Mat disp_img;
    cv::VideoWriter vw;

public:
    bool configure(yarp::os::ResourceFinder &rf) override {
        if (rf.check("help")) {
            yInfo() << "LEDGE online";
            yInfo() << "--ch <int>, --cw <int>\t: camera dimensions";
            yInfo() << "--blocksize <int>     \t: ledge block size";
            yInfo() << "--alpha <float>       \t: ratio of scarf ring buffer";
            yInfo() << "--vout <string>       \t: video out path (press space)";
            yInfo() << "--fit_thresh <double> \t: threshold to get good fitting of line segments";
            yInfo() << "--dp <double>         \t: Perturbation size for tracking";
            return false;
        }
        if (!Network::checkNetwork(2.0)) {
            yError() << "no yarp network";
            return false;
        }

        setName(rf.check("name", Value("/ledge")).asString().c_str());

        // Parameter setting
        fit_thresh = rf.check("fit_thresh", Value(0.2)).asFloat64();
        block_size = rf.check("block_size", Value(14)).asInt32();
        inlier = rf.check("inlier", Value(0.2)).asFloat64();
        alpha = rf.check("alpha", Value(1.0)).asFloat64();
        line_track = rf.check("line_track", Value(true)).asBool();
        res = {rf.check("cw", Value(1280)).asInt32(), rf.check("ch", Value(720)).asInt32()};
        dp = rf.check("dp", Value(1.1)).asFloat64();
        savesize = {res.width * 2 / 2, res.height / 2}; // Put 2 images horizontally and change the resolution in half
        display_fps = 30;

        // Initialize recorded video
        fs::path defaultpath = fs::path(std::getenv("HOME")) / fs::path("Downloads/ledge-online.mp4");
        vout_path = fs::path(rf.check("vout", Value(defaultpath.string())).asString());
        if (!fs::exists(vout_path.parent_path())) {
            if (fs::create_directories(vout_path.parent_path())) {
                yInfo() << "Directory to save ledge-online.mp4 created successfully.";
            }
            else {
                yError() << "Failed to create directory";
                return false;
            }
        }
        else {
            yInfo() << "Directory already exists.";
        }

        // Initialize LEDGE
        ledge.initialise(res, block_size, alpha, inlier, fit_thresh, line_track, dp);

        // Initialize window
        window_name = "Left: SCARF | Right: LEDGE";
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);
        cv::resizeWindow(window_name, {res.width * 2, res.height}); // Put 2 images horizontally

        // Initialize yarp connection
        if (!input_port.open(getName("/AE:i"))) {
            yError() << "could not open input port";
            return false;
        }
        Network::connect("/atis3/AE:o", getName("/AE:i"), "fast_tcp");
        Network::connect("/file/leftdvs:o", getName("/AE:i"), "fast_tcp");

        // Initialize thread
        events_process = std::thread([this] { this->updateSCARF(); });
        detection_process = std::thread([this] { this->detectLineSegments(); });
        tracking_process = std::thread([this] { this->trackManageLineSegments(); });

        return true;
    }


    // Keep main loop at 30 Hz
    double getPeriod() override {
        return 1 / display_fps;
    }


    bool interruptModule() override {
        input_port.stop();
        return true;
    }


    bool close() override {
        input_port.stop();
        events_process.join();
        detection_process.join();
        tracking_process.join();
        return true;
    }


    void updateSCARF() {
        while (!isStopping() && !input_port.isStopping()) {
            // Update SCARF
            ev::info port_stats = input_port.readAll(true);
            for (auto &e : input_port) {
                ledge.update(e.x, e.y, e.p);
            }
        }
    }


    void detectLineSegments() {
        // Time counter
        double start, detection;
        double time_ms_detect; // Unit: ms

        while (!isStopping()) {
            start = yarp::os::Time::now();

            // Detection process
            ledge.detectLines();

            // Update processsing time
            detection = yarp::os::Time::now();
            time_ms_detect = (detection - start) * 1e3;
        }
    }


    void trackManageLineSegments() {
        // Time counter
        double start, tracking;
        double time_ms_tracking; // Unit ms

        while (!isStopping()) {
            start = yarp::os::Time::now();

            // Tracking & Managing process
            ledge.trackLinesAllBlock();
            ledge.checkDetectionProhibitions();
            ledge.manageLineAdmins();

            // Update processing time
            tracking = yarp::os::Time::now();
            time_ms_tracking = (tracking - start) * 1e3;
        }
    }


    void recordDisplay(cv::VideoWriter &vw, cv::Mat &img, cv::Size &save_size) {
        cv::Mat rs;
        cv::resize(img, rs, save_size);
        vw << rs;
    }


    bool updateModule() override {
        static char c = 0;

        // Visualization process
        c = ledge.visualizeDisplay(window_name, disp_img);
        if (c == '\e') {
            stopModule();
            return false;
        };

        // Record display to video
        if (c == ' ') {
            if (!streaming) {
                yInfo() << "[record start]" << vout_path.string();
                vw.open(vout_path.string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), display_fps, savesize, true);
                streaming = true;
            }
            else {
                yInfo() << "[record stop ]" << vout_path.string();
                vw.release();
            }
        }
        if (streaming) recordDisplay(vw, disp_img, savesize);

        return true;
    }
};


int main(int argc, char *argv[]) {
    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setVerbose(false);
    rf.configure(argc, argv);

    /* create LEDGE module */
    ledgeModule instance;
    return instance.runModule(rf);
}