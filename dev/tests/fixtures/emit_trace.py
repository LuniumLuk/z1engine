import json
import sys
from pathlib import Path


def main():
	if len(sys.argv) < 4:
		return 2
	trace_path = Path(sys.argv[1])
	label = sys.argv[2]
	durations = [float(v) for v in sys.argv[3:]]
	trace_path.parent.mkdir(parents=True, exist_ok=True)
	events = []
	ts = 1000
	for idx, dur in enumerate(durations):
		events.append({
			"cat": "function",
			"dur": dur,
			"name": label,
			"ph": "X",
			"pid": 0,
			"tid": 1,
			"ts": ts + idx * 100,
		})
	with trace_path.open("w", encoding="utf-8") as f:
		json.dump({"otherData": {}, "traceEvents": events}, f)
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
