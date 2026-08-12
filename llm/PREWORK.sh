#!/bin/sh
# PREWORK — commands the USER runs (sudo / installs). The LLM cannot run
# sudo or install software; bin/todo_0.md checks these have been run.
# Copy-paste into a terminal before starting a session.

# A. Fresh-machine setup (run once per machine)
sudo apt update
sudo apt upgrade
sudo apt install g++ cmake ninja-build libboost-all-dev python3 python3-venv libicu-dev
export CXX=g++
curl -LsSf https://astral.sh/uv/install.sh | sh
uv tool install graphifyy
python3 -m venv .venv
uv pip install -r llm/requirements.txt
.venv/bin/pip install "clang-format==22.1.8" cmake-format
.venv/bin/pip install pandas tabulate networkx termcolor jellyfish ordered_set colorama matplotlib
.venv/bin/python3 --version
.venv/bin/clang-format --version
.venv/bin/cmake-format --version

# B. Profiling tools (install if missing — needed for Performance mode)
sudo apt install linux-tools-common linux-tools-$(uname -r)
sudo apt install valgrind
sudo apt install util-linux   # provides taskset (usually pre-installed)
sudo apt install heaptrack
git clone https://github.com/brendangregg/FlameGraph.git ~/FlameGraph
git clone https://github.com/andikleen/pmu-tools.git ~/pmu-tools

# C. Benchmark measurement protocol (run before every benchmark session)
sudo systemctl stop unattended-upgrades
sudo systemctl disable unattended-upgrades
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
  echo performance | sudo tee $i
done
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
MAX_FREQ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)
HALF_FREQ=$((MAX_FREQ / 2))
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
# taskset pins the measurement command to cores 1-4 (adjust) — no
# shield needed; pass the CPU list directly:
#   taskset -c 1-4 perf stat -r 10 <cmd>

sudo sysctl kernel.perf_event_paranoid=1
sudo sysctl kernel.kptr_restrict=0