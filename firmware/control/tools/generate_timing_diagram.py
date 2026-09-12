#!/usr/bin/env python3
"""
Generate a Mermaid timing diagram from structured `sc_trace` UART logs.

Usage:
  ./tools/generate_timing_diagram.py \
      --input monitor.log \
      --output ../../docs/implementation/SC_P0_TaskTiming.md
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
TRACE_RE = re.compile(
    r"ts_us=(?P<ts>\d+)\s+task=(?P<task>\S+)\s+evt=(?P<evt>\S+)\s+v=(?P<val>-?\d+)"
)


@dataclass
class TraceEvent:
    ts_us: int
    task: str
    evt: str
    val: int


def parse_trace_events(log_path: Path) -> List[TraceEvent]:
    events: List[TraceEvent] = []
    for line in log_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        clean = ANSI_RE.sub("", line)
        m = TRACE_RE.search(clean)
        if not m:
            continue
        events.append(
            TraceEvent(
                ts_us=int(m.group("ts")),
                task=m.group("task"),
                evt=m.group("evt"),
                val=int(m.group("val")),
            )
        )
    events.sort(key=lambda e: e.ts_us)
    return events


def build_schedule(events: List[TraceEvent], window_ms: int) -> Tuple[List[str], List[Tuple[str, int, int]], List[Tuple[str, str, int, int]]]:
    if not events:
        return [], [], []

    t0 = events[0].ts_us
    tasks: Dict[str, None] = {}
    spans: List[Tuple[str, int, int]] = []  # task, start_ms, dur_ms
    marks: List[Tuple[str, str, int, int]] = []  # task, label, at_ms, value
    open_span: Dict[str, int] = {}

    for ev in events:
        rel_ms = int((ev.ts_us - t0) / 1000)
        if rel_ms > window_ms:
            break
        tasks[ev.task] = None

        if ev.evt == "work_start":
            open_span[ev.task] = rel_ms
        elif ev.evt == "work_end":
            start = open_span.pop(ev.task, rel_ms)
            dur = max(1, rel_ms - start)
            spans.append((ev.task, start, dur))
        else:
            marks.append((ev.task, ev.evt, rel_ms, ev.val))

    # Close unmatched spans at end of window.
    for task, start in open_span.items():
        dur = max(1, window_ms - start)
        spans.append((task, start, dur))

    return sorted(tasks.keys()), spans, marks


def render_markdown(tasks: List[str], spans: List[Tuple[str, int, int]], marks: List[Tuple[str, str, int, int]], source_file: Path, window_ms: int) -> str:
    lines: List[str] = []
    lines.append("# SleepCube P0 Task Timing Diagram")
    lines.append("")
    lines.append(f"- Source log: `{source_file}`")
    lines.append(f"- Window: first `{window_ms}` ms of parsed trace data")
    lines.append("")
    lines.append("```mermaid")
    lines.append("gantt")
    lines.append("    title SleepCube P0 Task Schedule (trace-derived)")
    lines.append("    dateFormat  X")
    lines.append("    axisFormat %Lms")

    # Render spans per task.
    span_by_task: Dict[str, List[Tuple[int, int]]] = {t: [] for t in tasks}
    for task, start, dur in spans:
        span_by_task.setdefault(task, []).append((start, dur))

    for task in tasks:
        lines.append(f"    section {task}")
        for idx, (start, dur) in enumerate(span_by_task.get(task, []), start=1):
            lines.append(f"    run_{idx} : {start}, {dur}ms")

    lines.append("```")
    lines.append("")
    lines.append("## Event Markers")
    lines.append("")
    lines.append("| t (ms) | task | event | value |")
    lines.append("| --- | --- | --- | --- |")
    for task, evt, at_ms, value in marks[:200]:
        lines.append(f"| {at_ms} | {task} | {evt} | {value} |")
    if len(marks) > 200:
        lines.append(f"| ... | ... | ... | ... ({len(marks) - 200} more rows omitted) |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Mermaid task timing diagram from sc_trace logs")
    parser.add_argument("--input", required=True, help="Path to idf.py monitor log file")
    parser.add_argument("--output", required=True, help="Output markdown file path")
    parser.add_argument("--window-ms", type=int, default=15000, help="Time window from first trace event (default: 15000)")
    args = parser.parse_args()

    log_path = Path(args.input)
    out_path = Path(args.output)

    events = parse_trace_events(log_path)
    if not events:
        raw = log_path.read_text(encoding="utf-8", errors="ignore")
        if "sc_trace" not in raw and "ts_us=" not in raw:
            raise SystemExit(
                "No trace markers found. Check CONFIG_SC_TRACE_TIMING=y and reflashing before capture."
            )
        raise SystemExit(
            "Trace-like lines were found but not parsed. Please share 5 raw lines containing `sc_trace`."
        )

    tasks, spans, marks = build_schedule(events, window_ms=args.window_ms)
    md = render_markdown(tasks, spans, marks, source_file=log_path, window_ms=args.window_ms)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(md, encoding="utf-8")
    print(f"Wrote {out_path} ({len(events)} trace events parsed)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
