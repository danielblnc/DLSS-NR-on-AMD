"""Conservatively summarize DLSS-NR-on-AMD diagnostic logs.

The parser intentionally reports observations only.  It does not infer FPS,
visual quality, or process identity from a log's startup banners.
"""

from __future__ import annotations

import json
import math
import re
import sys
from pathlib import Path
from typing import Any


_LIMITATIONS = [
    "job samples are not a full benchmark or FPS measurement",
    "logs cannot prove visual correctness",
    "segments correspond to startup banners, not process IDs",
    "unknown log formats are not diagnosed",
    "faults are limited to the first 64 distinct signatures per segment",
    "false means not observed, not proof of absence",
    "timing samples may be sparse or repeated and do not count distinct jobs",
]

# Keep inputs bounded before converting to Python integers/floats.  This also
# makes malformed or hostile lines cheap to ignore.
_UINT = r"[0-9]{1,12}"
_HEX = r"[0-9A-Fa-f]{1,16}"
_DECIMAL = r"[0-9]{1,12}(?:\.[0-9]{1,12})?"

_BANNER_RE = re.compile(
    r"^dlssnr_amd(?:\s+(?P<version>v[A-Za-z0-9][A-Za-z0-9._-]{0,63})"
    r"\s+\(build\s+(?P<build>[A-Za-z0-9._-]{1,64})\))?\s+loaded\b"
)
_BANNER_BOUNDARY_RE = re.compile(r"^dlssnr_amd\b.*\bloaded\b", re.IGNORECASE)
_HIP_RE = re.compile(
    rf"^\s*env:\s*HIP:\s*(?P<devices>{_UINT})\s+device\(s\),"
    rf"\s*driver\s+{_UINT},\s*runtime\s+(?P<runtime>{_UINT})\b"
)
_ENGINE_RE = re.compile(r"^\s*engine\s+init\s+ok\s*$", re.IGNORECASE)
_HOOK_RE = re.compile(
    r"^\s*hooked\s+(?:amd_fidelityfx_upscaler_dx12|amd_fidelityfx_dx12|"
    r"ffx_fsr3upscaler_x64)\.dll!(?:ffxDispatch|ffxCreateContext)\b",
    re.IGNORECASE,
)
_DISPATCH_RE = re.compile(
    r"^\s*first\s+ffxDispatch\s+type\s+0x[0-9A-Fa-f]{1,16}\b", re.IGNORECASE
)
_JOB_RE = re.compile(
    rf"^\s*network\s+job\s+{_UINT}\s+done\s+in\s+(?P<ms>{_DECIMAL})\s+ms\b",
    re.IGNORECASE,
)
_COUNTERS_RE = re.compile(
    rf"^\s*(?:[A-Za-z][A-Za-z0-9_.-]*:\s*)?frames\s+(?P<frames>{_UINT})"
    rf"\s+dispatches\s+(?P<dispatches>{_UINT})\s+submitted\s+(?P<submitted>{_UINT})"
    rf"\s+ready\s+(?P<ready>-?[0-9]{{1,12}})\s+route\s+(?P<route>[A-Za-z0-9_.-]{{1,32}})\b",
    re.IGNORECASE,
)
_FAULT_RE = re.compile(
    rf"^\s*FAULT:\s*exception\s+(?P<code>0x{_HEX})\s+at\s+(?:0x)?{_HEX}\s+"
    rf"\((?P<module>[^\r\n]{{1,512}}?)\s+\+\s+(?P<offset>0x{_HEX})\)"
    rf"\s+thread\s+{_UINT},\s+last\s+job\s+(?P<last_job>-?[0-9]{{1,12}})\b",
    re.IGNORECASE,
)

_WARNING_PATTERNS = (
    (
        "invalid kernel file",
        re.compile(
            r"^\s*(?:WARNING\s*:\s*|job\s+" + _UINT + r"\s+GPU\s+errors\s*:[^\r\n]{0,200}?|)"
            r"invalid\s+kernel\s+file\b",
            re.IGNORECASE,
        ),
    ),
    (
        "unsupported colour format",
        re.compile(
            r"^\s*(?:WARNING\s*:\s*)?unsupported\s+colour\s+format(?:[0-9]{1,12})?(?![A-Za-z_])",
            re.IGNORECASE,
        ),
    ),
    (
        "too many frames in flight",
        re.compile(
            r"^\s*(?:(?:WARNING\s*:|inline\s*:)[ \t]*)?too\s+many\s+frames\s+in\s+flight\b",
            re.IGNORECASE,
        ),
    ),
    (
        "GPU wait timeout",
        re.compile(
            r"^\s*(?:(?:WARNING\s*:|inline\s*:)[ \t]*)?(?:[0-9]{1,12}\s+)?gpu\s+wait\s+timeouts?\b",
            re.IGNORECASE,
        ),
    ),
    (
        "zero-output warning",
        re.compile(
            r"^\s*(?:WARNING\s*:\s*[^\r\n]{0,200}\boutput\s+is\s+zero\b|"
            r"zero[- ]output\s+warning\s*$)",
            re.IGNORECASE,
        ),
    ),
)


def _new_segment(version: str | None = None, build: str | None = None) -> dict[str, Any]:
    return {
        "version": version,
        "build": build,
        "hip_runtime": None,
        "engine_initialized": False,
        "fsr_hook_observed": False,
        "fsr_dispatch_observed": False,
        "completed_job_samples": 0,
        "job_time_ms": None,
        "last_counters": None,
        "faults": [],
        "warnings": [],
        "_job_times": [],
    }


def _finite_nonnegative(token: str) -> float | None:
    try:
        value = float(token)
    except (OverflowError, ValueError):
        return None
    if not math.isfinite(value) or value < 0:
        return None
    return value


def _clean_number(value: float) -> int | float:
    return int(value) if value.is_integer() else value


def _basename(path: str) -> str:
    return re.split(r"[\\/]", path.strip())[-1]


def _consume(segment: dict[str, Any], line: str) -> bool:
    recognized = False

    hip = _HIP_RE.search(line)
    if hip:
        recognized = True
        try:
            segment["hip_runtime"] = int(hip.group("runtime"))
        except ValueError:
            pass

    if _ENGINE_RE.search(line):
        recognized = True
        segment["engine_initialized"] = True

    if _HOOK_RE.search(line):
        recognized = True
        segment["fsr_hook_observed"] = True

    if _DISPATCH_RE.search(line):
        recognized = True
        segment["fsr_dispatch_observed"] = True

    job = _JOB_RE.search(line)
    if job:
        recognized = True
        duration = _finite_nonnegative(job.group("ms"))
        if duration is not None:
            times: list[float] = segment["_job_times"]
            times.append(duration)
            segment["completed_job_samples"] = len(times)

    counters = _COUNTERS_RE.search(line)
    if counters:
        recognized = True
        try:
            segment["last_counters"] = {
                "frames": int(counters.group("frames")),
                "dispatches": int(counters.group("dispatches")),
                "submitted": int(counters.group("submitted")),
                "ready": int(counters.group("ready")),
                "route": counters.group("route"),
            }
        except ValueError:
            pass

    fault = _FAULT_RE.search(line)
    if fault:
        recognized = True
        item = {
            "code": fault.group("code").lower(),
            "module": _basename(fault.group("module")),
            "offset": fault.group("offset").lower(),
            "last_job": int(fault.group("last_job")),
        }
        faults: list[dict[str, Any]] = segment["faults"]
        if item not in faults and len(faults) < 64:
            faults.append(item)

    for label, pattern in _WARNING_PATTERNS:
        if pattern.search(line):
            recognized = True
            warnings: list[str] = segment["warnings"]
            if label not in warnings and len(warnings) < 32:
                warnings.append(label)

    return recognized


def _finish(segment: dict[str, Any]) -> dict[str, Any]:
    times: list[float] = segment.pop("_job_times")
    if times:
        ordered = sorted(times)
        middle = len(ordered) // 2
        median = ordered[middle] if len(ordered) % 2 else (ordered[middle - 1] + ordered[middle]) / 2
        segment["job_time_ms"] = {
            "min": _clean_number(ordered[0]),
            "median": _clean_number(median),
            "max": _clean_number(ordered[-1]),
        }
    return segment


def summarize(text: str) -> dict[str, Any]:
    """Return banner-scoped diagnostic facts from *text*.

    ``ValueError`` is raised when the input contains neither a startup banner
    nor any recognized diagnostic record.
    """
    if not isinstance(text, str) or not text:
        raise ValueError("unrecognized log input")

    lines = text.splitlines()
    banner_positions: list[tuple[int, re.Match[str] | None]] = []
    for index, line in enumerate(lines):
        if _BANNER_BOUNDARY_RE.match(line):
            banner_positions.append((index, _BANNER_RE.match(line)))

    segments: list[dict[str, Any]] = []
    if banner_positions:
        for position, (start, banner) in enumerate(banner_positions):
            end = banner_positions[position + 1][0] if position + 1 < len(banner_positions) else len(lines)
            version = banner.group("version") if banner else None
            build = banner.group("build") if banner else None
            segment = _new_segment(version, build)
            for line in lines[start + 1 : end]:
                _consume(segment, line)
            segments.append(_finish(segment))
    else:
        segment = _new_segment()
        recognized = False
        for line in lines:
            recognized = _consume(segment, line) or recognized
        if not recognized:
            raise ValueError("unrecognized log input")
        segments.append(_finish(segment))

    return {"segments": segments, "limitations": list(_LIMITATIONS)}


def _main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("error: expected one log file", file=sys.stderr)
        return 2
    try:
        text = Path(argv[1]).read_text(encoding="utf-8-sig", errors="replace")
        if not text.strip():
            raise ValueError("empty log input")
        result = summarize(text)
    except (OSError, ValueError):
        print("error: unreadable or unrecognized log input", file=sys.stderr)
        return 2
    json.dump(result, sys.stdout, ensure_ascii=True, separators=(",", ":"))
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(_main(sys.argv))
