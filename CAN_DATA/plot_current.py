#!/usr/bin/env python3
"""
Parse a PCAN-View .trc (v2.0) log, pull out CAN ID 0x293 frames with
byte0 == 0x02, and make two graphs of pack current, voltage and power:

  Graph 1: current = bytes 4-5 little-endian signed int16, raw counts
           power   = voltage x (raw * CURR_SCALE_LE)
  Graph 2: current = bytes 4-5 big-endian signed int16, divided by 100 (A)
           power   = voltage x current

Voltage is bytes 2-3 little-endian unsigned int16 x VOLT_SCALE in both.

Usage:
    python plot_0293_current.py drive.trc
"""
import struct
import sys

import matplotlib.pyplot as plt

TARGET_ID = 0x0293
TARGET_B0 = 0x02

VOLT_SCALE = 0.1       # V per count (350 V pack -> 3546 counts = 354.6 V)
CURR_SCALE_LE = 1    # A per count for graph 1's power -- ASSUMED
CURR_DIV_BE = 100.0    # graph 2: big-endian raw / 100 -> A


def parse_trc(path):
    """Yield (time_ms, can_id, data_bytes) for every data frame in the file."""
    with open(path, "r", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith(";"):
                continue

            # Columns: msg#  time_ms  type  id  Rx/Tx  len  data...
            parts = line.split()
            if len(parts) < 7:
                continue
            try:
                time_ms = float(parts[1])
                can_id = int(parts[3], 16)
                dlc = int(parts[5])
                data = bytes(int(b, 16) for b in parts[6:6 + dlc])
            except ValueError:
                continue

            yield time_ms, can_id, data


def plot_graph(t, volt_v, curr, power_kw, curr_label, power_label, title):
    """One figure: current, voltage and power, each with its own y-axis."""
    c_cur, c_volt, c_pow = "tab:blue", "tab:red", "tab:green"

    fig, ax_i = plt.subplots(figsize=(13, 6))
    ax_v = ax_i.twinx()
    ax_p = ax_i.twinx()
    ax_p.spines["right"].set_position(("axes", 1.09))

    l_i, = ax_i.plot(t, curr, color=c_cur, linewidth=1, label=curr_label)
    l_v, = ax_v.plot(t, volt_v, color=c_volt, linewidth=1, label="Voltage (V)")
    l_p, = ax_p.plot(t, power_kw, color=c_pow, linewidth=1, label=power_label)

    ax_i.axhline(0, color="gray", linewidth=0.8)
    ax_i.set_xlabel("Time (s)")
    ax_i.set_ylabel(curr_label, color=c_cur)
    ax_v.set_ylabel("Pack voltage (V)", color=c_volt)
    ax_p.set_ylabel(power_label, color=c_pow)
    ax_i.tick_params(axis="y", colors=c_cur)
    ax_v.tick_params(axis="y", colors=c_volt)
    ax_p.tick_params(axis="y", colors=c_pow)

    ax_i.set_title(title)
    ax_i.grid(True, alpha=0.3)
    ax_i.legend(handles=[l_i, l_v, l_p], loc="upper right")
    fig.tight_layout()


def main():
    if len(sys.argv) != 2:
        sys.exit(f"Usage: {sys.argv[0]} <file.trc>")

    t, volt_v = [], []
    i_le_raw, p_le_kw = [], []     # graph 1
    i_be_amps, p_be_kw = [], []    # graph 2

    for time_ms, can_id, data in parse_trc(sys.argv[1]):
        if can_id != TARGET_ID or len(data) < 6 or data[0] != TARGET_B0:
            continue

        v = struct.unpack_from("<H", data, 2)[0] * VOLT_SCALE
        i_le = struct.unpack_from("<h", data, 4)[0]            # little-endian signed
        i_be = struct.unpack_from(">h", data, 4)[0] / CURR_DIV_BE  # big-endian signed, A

        t.append(time_ms / 1000.0)
        volt_v.append(v)
        i_le_raw.append(i_le)
        p_le_kw.append(v * i_le * CURR_SCALE_LE / 1000.0)
        i_be_amps.append(i_be)
        p_be_kw.append(v * i_be / 1000.0)

    if not t:
        sys.exit("No matching 0x293 frames with b0 = 02 found.")

    n = len(t)
    print(f"Frames decoded: {n}")
    print(f"Voltage (V)            min: {min(volt_v):.1f}  max: {max(volt_v):.1f}")
    print(f"[LE] Current (raw)     min: {min(i_le_raw)}  max: {max(i_le_raw)}  "
          f"mean: {sum(i_le_raw) / n:.1f}")
    print(f"[LE] Power (kW)        min: {min(p_le_kw):.2f}  max: {max(p_le_kw):.2f}  "
          f"(at {CURR_SCALE_LE} A/count)")
    print(f"[BE] Current (A, /100) min: {min(i_be_amps):.2f}  max: {max(i_be_amps):.2f}  "
          f"mean: {sum(i_be_amps) / n:.2f}")
    print(f"[BE] Power (kW)        min: {min(p_be_kw):.2f}  max: {max(p_be_kw):.2f}")

    plot_graph(
        t, volt_v, i_le_raw, p_le_kw,
        curr_label="Pack current (raw int16 counts, little-endian)",
        power_label=f"Pack power (kW, at {CURR_SCALE_LE} A/count)",
        title="CAN 0x293 (b0 = 02): little-endian signed int16 current",
    )
    plot_graph(
        t, volt_v, i_be_amps, p_be_kw,
        curr_label=f"Pack current (A, big-endian int16 / {CURR_DIV_BE:g})",
        power_label="Pack power (kW)",
        title=f"CAN 0x293 (b0 = 02): big-endian signed int16 / {CURR_DIV_BE:g} current",
    )
    plt.show()


if __name__ == "__main__":
    main()