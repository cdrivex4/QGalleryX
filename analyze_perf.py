import os
import re
import csv

def analyze_logs():
    log_dir = "logs"
    if not os.path.exists(log_dir):
        print("Logs directory not found.")
        return

    crash_log = os.path.join(log_dir, "crash.log")
    if os.path.exists(crash_log):
        print(f"--- Analyzing {crash_log} ---")
        slow_loads = [] 
        dir_stats = {} 
        adaptive_events = []
        drive_concurrency = {} 

        # Optimized line-by-line parsing without heavy regex in hot loop
        with open(crash_log, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                if "SLOW LOAD detected:" in line:
                    # [Performance] SLOW LOAD detected: 123 ms for ...
                    parts = line.split(" detected: ")
                    if len(parts) > 1:
                        sub = parts[1].split(" ms for ")
                        if len(sub) > 1:
                            dur = int(sub[0])
                            path = sub[1].strip().strip('"')
                            slow_loads.append((dur, path))
                            dirname = os.path.dirname(path)
                            dir_stats.setdefault(dirname, []).append(dur)

                if "[AdaptiveIO]" in line:
                    # [AdaptiveIO] Stats: I:/ Avg: 123.4 ms Limit: 2 Active: 1
                    # [AdaptiveIO] LIMIT CHANGE: I:/ 2 -> 1 (850.5 ms)
                    if "Stats:" in line:
                        parts = line.split("Stats:")[1].strip().split()
                        if len(parts) >= 6:
                            drive = parts[0]
                            avg_lat = float(parts[2])
                            limit = int(parts[4])
                            active = int(parts[6])
                            adaptive_events.append({"drive": drive, "latency": avg_lat, "limit": limit, "active": active})
                            drive_concurrency[drive] = limit
                    elif "LIMIT CHANGE:" in line:
                        print("EVENT: " + line.strip())

        if slow_loads:
            print(f"Slow Loads Total: {len(slow_loads)}")
            all_durs = [x[0] for x in slow_loads]
            print(f"  Avg Duration: {sum(all_durs)/len(all_durs):.1f} ms")
            
        print("\n--- Drive Performance Summary ---")
        if not drive_concurrency:
            print("No drive statistics recorded.")
        for drive, limit in drive_concurrency.items():
            drive_events = [e for e in adaptive_events if e['drive'] == drive]
            if drive_events:
                avg_lat = sum(e['latency'] for e in drive_events) / len(drive_events)
                print(f"Drive {drive}: Current Limit: {limit} | Avg Logged Latency: {avg_lat:.1f} ms | Samples: {len(drive_events)}")

    # 2. Analyze latest CSV
    csv_files = [f for f in os.listdir(log_dir) if f.endswith(".csv") and "perf_" in f]
    if csv_files:
        latest_csv = max([os.path.join(log_dir, f) for f in csv_files], key=os.path.getmtime)
        print(f"\n--- Analyzing Latest CSV: {latest_csv} ---")
        try:
            with open(latest_csv, 'r') as f:
                reader = csv.DictReader(f)
                rows = list(reader)
                if rows:
                    fps = [float(r['FPS']) for r in rows if 'FPS' in r]
                    cpu = [float(r['CPU']) for r in rows if 'CPU' in r]
                    print(f"FPS Avg: {sum(fps)/len(fps):.1f} | Min: {min(fps)}")
                    print(f"CPU Avg: {sum(cpu)/len(cpu):.1f}%")
        except Exception as e:
            print(f"CSV error: {e}")

if __name__ == "__main__":
    analyze_logs()
