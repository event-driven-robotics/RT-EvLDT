import argparse
import pathlib
from rosbags.highlevel import AnyReader
import numpy as np
from typing import List
import sys
from tqdm import tqdm

### Import bimvee from local environment
sys.path.append("/app/LEDGE/submodule/bimvee")
from bimvee.exportIitYarp import exportIitYarp


def convert_dvsrosbag2yarp(args: argparse.Namespace) -> None:
    # Params
    rosbag_path: pathlib.Path = pathlib.Path(args.rosbag_path)
    container: dict = {}
    xs: List[int] = []
    ys: List[int] = []
    pols: List[bool] = []
    tss: List[float] = []
    total_messages: int = 0
    start_time_ns: int = 0
    end_time_ns: int = 0

    # Read .bag file
    with AnyReader([rosbag_path]) as reader:
        connections = [x for x in reader.connections if x.topic == args.event_topic]
        if args.start_time is not None:
            start_time_ns = reader.start_time + args.start_time * 1e9
        else:
            start_time_ns = reader.start_time
        if args.end_time is not None:
            end_time_ns = reader.start_time + args.end_time * 1e9
        else:
            end_time_ns = reader.end_time
        total_messages = sum(
            1
            for _ in reader.messages(
                connections=connections, start=start_time_ns, stop=end_time_ns
            )
        )
    with AnyReader([rosbag_path]) as reader:
        connections = [x for x in reader.connections if x.topic == args.event_topic]
        for connection, _, rawdata in tqdm(
            reader.messages(
                connections=connections, start=start_time_ns, stop=end_time_ns
            ),
            desc="Processing messages",
            total=total_messages,
        ):
            msg = reader.deserialize(rawdata, connection.msgtype)
            x_values = [event.x for event in msg.events]  # type: ignore[attr-defined]
            y_values = [event.y for event in msg.events]  # type: ignore[attr-defined]
            pol_values = [event.polarity for event in msg.events]  # type: ignore[attr-defined]
            ts_values = [
                (event.ts.sec + event.ts.nanosec * 1e-9)
                for event in msg.events  # type: ignore[attr-defined]
            ]
            xs.extend(x_values)
            ys.extend(y_values)
            pols.extend(pol_values)
            tss.extend(ts_values)

    container["data"] = {"ch0": {"dvs": {"ts": np.array(tss).astype(np.float64)}}}
    container["data"]["ch0"]["dvs"]["x"] = np.array(xs).astype(np.uint16)  # type: ignore
    container["data"]["ch0"]["dvs"]["y"] = np.array(ys).astype(np.uint16)  # type: ignore
    container["data"]["ch0"]["dvs"]["pol"] = np.array(pols).astype(bool)  # type: ignore
    container["info"] = {}
    container["info"]["filePathOrName"] = args.output_path

    del reader, xs, ys, pols, tss

    # Export yarp .log file
    exportIitYarp(
        container,
        exportFilePath=args.output_path,
        exportAsEv2=True,
        exportTimestamps=False,
        minTimeStepPerBottle=5e-4,
        viewerApp=False,
        protectedWrite=False,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description=(
            "Convert rosbag file to yarp .log file.\n\n"
            "Example:\n"
            "  uv run scripts/convert_dvsrosbag2yarp.py"
            " -p /data/rosbag/Scaramuzza/slider_close.bag"
            " -o /data/rosbag2log/slider_close"
        ),
    )
    parser.add_argument(
        "-p",
        "--rosbag_path",
        help="Absolute or relative Path to .bag file",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-o", "--output_path", help="Output folder path", type=str, required=True
    )
    parser.add_argument(
        "-s", "--start_time", help="start time [s]", type=int, default=None
    )
    parser.add_argument("-e", "--end_time", help="end time [s]", type=int, default=None)
    parser.add_argument(
        "--event_topic", help="event topic name", type=str, default="/dvs/events"
    )

    args = parser.parse_args()

    convert_dvsrosbag2yarp(args)
