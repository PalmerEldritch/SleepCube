#!/usr/bin/env python3
"""Analyze M8 Control trace logs against the SRS timing limits."""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from collections import defaultdict, deque
from dataclasses import dataclass
from pathlib import Path
from typing import Deque, Iterable


TOUCH_LIMIT_US = 100_000
COMMAND_LIMIT_US = 250_000
UI_AUDIO_TOGGLE = 1
UI_VOLUME_SET = 4

ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
TRACE_RE = re.compile(
    r"ts_us=(?P<ts>\d+)\s+task=(?P<task>\S+)\s+"
    r"evt=(?P<event>\S+)\s+v=(?P<value>-?\d+)"
)
RESPONSE_RE = re.compile(
    r"response=(?P<request>PLAY|STOP|SET_VOLUME)\s+seq=(?P<sequence>\d+)\s+"
    r"latency_us=(?P<latency>\d+)\s+rx_ts_us=(?P<rx_ts>\d+)"
)


@dataclass(frozen=True)
class TimingAnalysis:
    touch_accept_us: tuple[int, ...]
    command_ack_us: tuple[int, ...]
    transport_rtt_us: tuple[int, ...]
    command_names: tuple[str, ...]


def analyze_lines(lines: Iterable[str]) -> TimingAnalysis:
    touch_pending: dict[int, Deque[int]] = defaultdict(deque)
    toggle_pending: Deque[int] = deque()
    volume_pending: Deque[int] = deque()
    touch_samples: list[int] = []
    command_samples: list[int] = []
    transport_samples: list[int] = []
    command_names: list[str] = []

    for raw_line in lines:
        line = ANSI_RE.sub("", raw_line)
        trace = TRACE_RE.search(line)
        if trace:
            timestamp = int(trace.group("ts"))
            task = trace.group("task")
            event = trace.group("event")
            value = int(trace.group("value"))
            if task == "ui_touch" and event == "evt_emit":
                touch_pending[value].append(timestamp)
                if value == UI_AUDIO_TOGGLE:
                    toggle_pending.append(timestamp)
                elif value == UI_VOLUME_SET:
                    volume_pending.append(timestamp)
            elif task == "app_core" and event == "evt_rx":
                if touch_pending[value]:
                    touch_samples.append(timestamp - touch_pending[value].popleft())

        response = RESPONSE_RE.search(line)
        if not response:
            continue

        request = response.group("request")
        response_timestamp = int(response.group("rx_ts"))
        transport_samples.append(int(response.group("latency")))

        user_timestamp: int | None = None
        if request in {"PLAY", "STOP"} and toggle_pending:
            user_timestamp = toggle_pending.popleft()
        elif request == "SET_VOLUME" and volume_pending:
            # The facade intentionally coalesces slider traffic; the transmitted
            # value corresponds to the most recent user event before the request.
            user_timestamp = volume_pending[-1]
            volume_pending.clear()

        if user_timestamp is not None:
            command_samples.append(response_timestamp - user_timestamp)
            command_names.append(request)

    return TimingAnalysis(
        touch_accept_us=tuple(touch_samples),
        command_ack_us=tuple(command_samples),
        transport_rtt_us=tuple(transport_samples),
        command_names=tuple(command_names),
    )


def summarize(samples: tuple[int, ...]) -> dict[str, float | int | None]:
    if not samples:
        return {"count": 0, "min_us": None, "p50_us": None,
                "p95_us": None, "max_us": None}
    ordered = sorted(samples)

    def percentile(fraction: float) -> int:
        index = max(0, math.ceil(fraction * len(ordered)) - 1)
        return ordered[index]

    return {
        "count": len(ordered),
        "min_us": ordered[0],
        "p50_us": percentile(0.50),
        "p95_us": percentile(0.95),
        "max_us": ordered[-1],
    }


def format_summary(name: str, values: dict[str, float | int | None],
                   limit_us: int) -> str:
    if values["count"] == 0:
        return f"{name}: no samples (limit {limit_us / 1000:.0f} ms)"
    return (
        f"{name}: n={values['count']} min={values['min_us'] / 1000:.3f} ms "
        f"p50={values['p50_us'] / 1000:.3f} ms "
        f"p95={values['p95_us'] / 1000:.3f} ms "
        f"max={values['max_us'] / 1000:.3f} ms "
        f"limit={limit_us / 1000:.0f} ms"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="captured ESP-IDF monitor log")
    parser.add_argument("--min-touch", type=int, default=20,
                        help="minimum accepted touch samples (default: 20)")
    parser.add_argument("--min-command", type=int, default=20,
                        help="minimum user-to-ack samples (default: 20)")
    parser.add_argument("--json", action="store_true",
                        help="emit machine-readable JSON")
    args = parser.parse_args()

    with args.log.open("r", encoding="utf-8", errors="replace") as handle:
        analysis = analyze_lines(handle)

    touch = summarize(analysis.touch_accept_us)
    command = summarize(analysis.command_ack_us)
    transport = summarize(analysis.transport_rtt_us)
    passed = (
        touch["count"] >= args.min_touch
        and command["count"] >= args.min_command
        and touch["max_us"] is not None
        and touch["max_us"] <= TOUCH_LIMIT_US
        and command["max_us"] is not None
        and command["max_us"] <= COMMAND_LIMIT_US
    )

    if args.json:
        print(json.dumps({
            "pass": passed,
            "limits_us": {"touch_accept": TOUCH_LIMIT_US,
                          "user_command_ack": COMMAND_LIMIT_US},
            "touch_accept": touch,
            "user_command_ack": command,
            "transport_rtt": transport,
            "command_names": analysis.command_names,
        }, indent=2))
    else:
        print(format_summary("touch acceptance", touch, TOUCH_LIMIT_US))
        print(format_summary("user command to acknowledgement", command,
                             COMMAND_LIMIT_US))
        print(format_summary("UART request/response RTT", transport,
                             COMMAND_LIMIT_US))
        print(f"M8 timing gate: {'PASS' if passed else 'FAIL'}")

    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
