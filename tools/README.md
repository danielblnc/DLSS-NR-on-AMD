# `summarize_log.py`

Python 3.10 or newer is supported. From the repository root, summarize a log into
JSON on standard output:

```powershell
python -B tools/summarize_log.py "logs\dlssnr_on_amd.log" > "..\dlssnr-summary.json"
```

Use an output filename distinct from the input and outside the checkout, so the
original log remains available for inspection and the derived report is not committed
accidentally. The tool has no runtime package dependencies and needs no GPU,
administrator rights, network access, or upload account. It does not modify the log or upload anything automatically.

The parser recognizes the observed banner, HIP, initialization, FSR hook/dispatch,
job timing, counters, fault, and warning formats used by current reports. Supported
formats are best effort, so an absent or unfamiliar line may be omitted. When a banner
exists, preamble text is discarded and every banner starts a segment, including
duplicates. Headerless logs with recognized records produce one unversioned segment;
segments do not prove process-session boundaries.

Output contract: `last_counters` is the latest counters record, not an aggregate.
`job_time_ms` gives min/median/max for observed nonnegative accepted samples. Faults
are deduplicated by `(code, module, offset, last_job)` after basename sanitization;
paths, thread IDs, and absolute addresses are discarded. Warnings are fixed,
deduplicated labels. The CLI exits 2 for invalid, unreadable, empty, or unrecognized
input.

Read and inspect the JSON before sharing it. `false` means a pattern was not observed,
and `null` means the value was unavailable; neither proves absence. Timing samples may
be sparse or duplicated, so the count is samples rather than the largest job ID. Fault
output is bounded to 64 distinct faults per segment. A summary does not prove image
quality, FPS, or successful in-game behavior. This is intended to make diagnostics
easier to review, including reports such as [upstream issue #132](https://github.com/danielblnc/DLSS-NR-on-AMD/issues/132).

Run tests from the repository root:

```powershell
python -B -m unittest discover -s tests -v
```
