# Example applications with LEDGE library
The following examples shows how to use LEDGE library into the project.
- `ledge_offline_parallel`: Visualize LEDGE (Detected/Tracked line segments) and SCARF with your recorded .log event data. Real-time process due to parallel processing.
- `ledge_offline_sequential`: Visualize LEDGE (Detected/Tracked line segments) and SCARF with your recorded .log event data. Useful for debug due to sequential processing.
- `ledge_online`: Real-time visualization of LEDGE (Detected/Tracked line segments) and SCARF with an online event camera.
- `ledge_offline_parallel_processing_time`: Calculate processing time of detection, tracking, and scarf thread. Additionally, check maximum event rate of SCARF.

## How to build applications
```
cd /app/LEDGE/example
mkdir -p build
cd build
cmake ..
make
```

## RUN LEDGE offline with a recorded data in real-time
Terminal 1 (in docker container)
```
cd /app/LEDGE/example/build
./ledge-offline-par --data /data/recorded_data/gamer-white-paper/ATIS/data.log --fit_thresh 0.2 --block_size 14 --inlier 0.2 --alpha 1.0 --line_track true --cw 640 --ch 480 --dp 2.5
```
- Parameters
  - `--data`    : path of your recorded data in docker container. you can mount a folder from local to docker
  - `--fit_thresh`: threshold to get good fitting of line segments
  - `--block_size`: the size of ledge block
  - `--inlier`: used to get inlier events from line segments (currently should set the same value as `--fit_thresh`)
  - `--alpha`: ratio of scarf ring buffer (1.0 is the max ratio)
  - `--line_track`: flag to use tracking mode of line segments (true: use, false: not use)
  - `--cw`: FoV width of recorded event data
  - `--ch`: FoV height of recorded event data
  - `--csv`: csv path to record all LEDGE results (if you don't specify path, skip recording)
  - `--dp`: Perturbation size for line segment tracking [px]

## RUN LEDGE offline with a recorded data sequentially for debugging
Terminal 1 (in docker container)
```
cd /app/LEDGE/example/build
./ledge-offline-seq --data /data/recorded_data/gamer-white-paper/ATIS/data.log --fps 600 --fit_thresh 0.2 --block_size 14 --inlier 0.2 --alpha 1.0 --line_track true --cw 640 --ch 480 --csv /your/csv/path --dp 2.5
```
- Parameters
  - `--data`    : path of your recorded data in docker container. you can mount a folder from local to docker
  - `--fps`: process frequency
  - `--fit_thresh`: threshold to get good fitting of line segments
  - `--block_size`: the size of ledge block
  - `--inlier`: used to get inlier events from line segments (currently should set the same value as `--fit_thresh`)
  - `--alpha`: ratio of scarf ring buffer (1.0 is the max ratio)
  - `--line_track`: flag to use tracking mode of line segments (true: use, false: not use)
  - `--cw`: FoV width of recorded event data
  - `--ch`: FoV height of recorded event data
  - `--csv`: csv path to record all LEDGE results (if you don't specify path, skip recording)
  - `--dp`: Perturbation size for line segment tracking [px]

## RUN LEDGE online with YARP and an actual event camera
Terminal 1 (in docker container)
- Run YARP server in the background
```
yarpserver &
```

Terminal 2 (in docker container)
- Output event data from the actual event camera as `/atis3/AE:o`
```
atis-bridge-sdk --s 50
```

Terminal 3 (in docker container)
- Run LEDGE online
```
cd /app/LEDGE/example/build
./ledge-online --fit_thresh 0.2 --block_size 14 --inlier 0.2 --alpha 1.0 --line_track true --cw 640 --ch 480 --dp 2.5
```
- Parameters
  - `--fps`: frame rate of process
  - `--fit_thresh`: threshold to get good fitting of line segments
  - `--block_size`: the size of ledge block
  - `--inlier`: used to get inlier events from line segments (currently should set the same value as `--fit_thresh`)
  - `--alpha`: ratio of scarf ring buffer (1.0 is the max ratio)
  - `--line_track`: flag to use tracking mode of line segments (true: use, false: not use)
  - `--cw`: FoV width of recorded event data
  - `--ch`: FoV height of recorded event data
  - `--dp`: Perturbation size for line segment tracking [px]

## RUN LEDGE online with YARP, an actual event camera, and your laptop camera
Terminal 1 (in docker container)
- Run YARP server in the background
```
yarpserver &
```

Terminal 2 (in docker container)
- Output event data from the actual event camera as `/atis3/AE:o`
```
atis-bridge-sdk --s 50
```

Terminal 3 (in docker container)
- Run LEDGE online
```
cd /app/LEDGE/example/build
./ledge-online-laptopcam --fit_thresh 0.2 --block_size 14 --inlier 0.2 --alpha 1.0 --line_track true --cw 640 --ch 480 --dp 2.5 --laptop_cam
```
- Parameters
  - `--fps`: frame rate of process
  - `--fit_thresh`: threshold to get good fitting of line segments
  - `--block_size`: the size of ledge block
  - `--inlier`: used to get inlier events from line segments (currently should set the same value as `--fit_thresh`)
  - `--alpha`: ratio of scarf ring buffer (1.0 is the max ratio)
  - `--line_track`: flag to use tracking mode of line segments (true: use, false: not use)
  - `--cw`: FoV width of recorded event data
  - `--ch`: FoV height of recorded event data
  - `--dp`: Perturbation size for line segment tracking [px]
  - `--laptop_cam`: Flag to use laptop camera

## RUN LEDGE offline to calculate processing time
Terminal 1 (in docker container)
```
cd /app/LEDGE/example/build
./ledge-offline-par-process-time --data /data/recorded_data/gamer-white-paper/ATIS/data.log --fit_thresh 0.2 --block_size 14 --inlier 0.2 --alpha 1.0 --line_track true --cw 640 --ch 480 --dp 2.5 --start_recording_time 5 --stop_time 10
```
- Parameters
  - `--data`    : path of your recorded data in docker container. you can mount a folder from local to docker
  - `--fit_thresh`: threshold to get good fitting of line segments
  - `--block_size`: the size of ledge block
  - `--inlier`: used to get inlier events from line segments (currently should set the same value as `--fit_thresh`)
  - `--alpha`: ratio of scarf ring buffer (1.0 is the max ratio)
  - `--line_track`: flag to use tracking mode of line segments (true: use, false: not use)
  - `--cw`: FoV width of recorded event data
  - `--ch`: FoV height of recorded event data
  - `--csv`: csv path to record processing times (if you don't specify path, skip recording)
  - `--dp`: Perturbation size for line segment tracking [px]
  - `--start_recording_time`: Start recording time to avoid first uninitialized sequences [s]
  - `--stop_time`: Stop recording time [s]
