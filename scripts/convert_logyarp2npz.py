import pathlib
import numpy as np
import sys
import argparse
from tqdm import tqdm
from typing import Any, Optional

### Import bimvee from local environment
sys.path.append("/app/LEDGE/submodule/bimvee")
from bimvee.importIitYarp import importIitYarp


def convert_logyarp2npz(args: argparse.Namespace) -> None:
    # Params
    log_folder_path: pathlib.Path = pathlib.Path(args.log_folder_path)
    output_folder_path: pathlib.Path = pathlib.Path(args.output_folder_path)
    time_sec_range: float = args.time_sec_range
    output_time_relation_file: Optional[pathlib.Path] = None
    if args.time_relations:
        output_time_relation_file = output_folder_path / pathlib.Path(
            "time_relations.txt"
        )

    # Read .log file
    yarplog: Any = importIitYarp(
        filePathOrName=log_folder_path.__str__(), template=None, wrapTime=0.0
    )
    events: dict = yarplog["data"]["left"]["dvs"]
    ts_sec_offset: float = float(events["tsOffset"])
    ts_sec_array: np.ndarray = np.array(events["ts"], dtype=np.float64) - ts_sec_offset
    xs_array: np.ndarray = np.array(events["x"], dtype=np.int16)
    ys_array: np.ndarray = np.array(events["y"], dtype=np.int16)
    pols_array: np.ndarray = np.array(events["pol"], dtype=bool)

    # Params for .npz file splitting
    if args.stop_time is None:
        stop_npz_time_sec: float = float(events["ts"][-1])
    else:
        stop_npz_time_sec: float = float(args.stop_time)
    if args.start_time is None:
        start_npz_time_sec: float = 0.0
    else:
        start_npz_time_sec: float = float(args.start_time)
    total_npz_time_sec: float = stop_npz_time_sec - start_npz_time_sec
    num_windows: int = int(np.floor((total_npz_time_sec / time_sec_range)))

    # Export event data to multiple .npz files for 30 ms
    for file_cnt in tqdm(range(num_windows), desc="Export .npz files"):
        npz_start_time_sec: float = start_npz_time_sec + file_cnt * time_sec_range
        npz_end_time_sec: float = npz_start_time_sec + time_sec_range

        # Get events in the time window
        mask_time_window: np.ndarray = np.logical_and(
            ts_sec_array >= npz_start_time_sec, ts_sec_array < npz_end_time_sec
        )
        xs_window: np.ndarray = xs_array[mask_time_window]
        ys_window: np.ndarray = ys_array[mask_time_window]
        pols_window: np.ndarray = pols_array[mask_time_window]
        tss_window: np.ndarray = (
            (ts_sec_array[mask_time_window] - npz_start_time_sec) * 1e9
        ).astype(np.int32)

        # Save to .npz file
        (output_folder_path / "events_npz").mkdir(parents=True, exist_ok=True)
        output_file_path: pathlib.Path = output_folder_path / pathlib.Path(
            f"events_npz/{file_cnt:06d}.npz"
        )
        np.savez(
            output_file_path.__str__(),
            x=xs_window,
            y=ys_window,
            t=tss_window,
            p=pols_window,
        )

        # Save time relationship between image and events
        if output_time_relation_file is not None:
            with output_time_relation_file.open(mode="a") as f:
                f.write(
                    f"{output_file_path.with_suffix('.png').name} {ts_sec_array[mask_time_window][0]} {ts_sec_array[mask_time_window][0]}-{ts_sec_array[mask_time_window][-1]}\n"
                )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="""Convert yarp .log format to multiple .npz file (each 30 ms)
                                     || uv run scripts/convert_logyarp2npz.py -p /data/mikura/UnrealEngine/simple_horizon_mustard_bottle/ch0dvs -o /data/mikura/UnrealEngine/simple_horizon_mustard_bottle 
                                     --time_sec_range 0.03 --start_time 0 --stop_time 10"""
    )
    parser.add_argument(
        "-p",
        "--log_folder_path",
        help="Absolute or relative Path to folder which includes .log file",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-o",
        "--output_folder_path",
        help="Output folder path for .npz files",
        type=str,
        required=True,
    )
    parser.add_argument(
        "--time_sec_range",
        help="Time range for each .npz file [s]",
        type=float,
        default=0.03,
    )
    parser.add_argument(
        "--start_time", help="Start time for recording [s]", type=float, default=None
    )
    parser.add_argument(
        "--stop_time", help="Stop time for recording [s]", type=float, default=None
    )
    parser.add_argument(
        "--time_relations",
        help="Save time relationships between .npz files and images",
        action="store_true",
    )
    args = parser.parse_args()

    convert_logyarp2npz(args)