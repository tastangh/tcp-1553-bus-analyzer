# TCP 1553 Suite

A comprehensive MIL-STD-1553 Bus Controller (BC) and Bus Monitor (BM) application that communicates over TCP. This suite is specifically designed to replace QEMU in digital satellite simulations, allowing direct interaction with the **Galacsim** simulator.

---

## 🛠 Prerequisites

Before building, ensure your Linux system has the following dependencies installed:

### 1. System Libraries

```bash
sudo apt update
sudo apt install -y build-essential cmake libpcap-dev libwxgtk3.0-gtk3-dev libboost-all-dev libspdlog-dev
```

### 2. Network Permissions

The Bus Monitor uses `libpcap` for passive sniffing on the loopback interface (`lo`). This requires special kernel capabilities.

---

## 🚀 Getting Started

### 1. Build the Project

You can use the provided build script or run CMake manually:

**Using Script:**

```bash
./scripts/build.sh
```

**Manual Build:**

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 2. Run the Application

Simply run the provided script:

```bash
./scripts/run.sh
```

_Note: This script will ask for your **sudo** password once to apply the necessary network capabilities (`setcap`) for passive sniffing._

---

## 🛰 Operation Guide

### 1. Bus Controller (BC)

The BC panel allows you to inject 1553 frames into the simulation bus.

- **TCP Server Mode**: The suite acts as a TCP server on port **5002**. When you start the BC, it waits for the **Simulator (Galacsim)** to connect.
- **Default Frames**: At startup, common satellite frames (RT 17 Torque, RT 7 GPS, RT 15 STR, RT 16 Gyro) are automatically populated.
- **Modes**:
  - **BC to RT (Receive)**: BC sends data to the RT.
  - **RT to BC (Transmit)**: BC requests data from the RT. Data fields are disabled in the UI for this mode as the BC is the receiver.
- **Controls**:
  - **Send Active Frames**: Continuously sends all "Active" frames in the list.
  - **Send Once**: Sends the specific frame a single time.
  - **Repeat**: When ON, the sending loop continues indefinitely.

### 2. Bus Monitor (BM)

The BM panel provides a real-time view of all 1553 traffic on the bus.

- **Passive Sniffing**: It monitors traffic on port 5002 without interfering with the BC-Simulator connection.
- **Live View**: Displays timestamped transactions, including Bus, Type, RT, SA, and Data Words (Hex).
- **RT/SA Tree**: Automatically tracks active Remote Terminals and Subaddresses in the sidebar.

---

## 📝 Protocol Specification

The suite uses a simplified 1553-over-TCP protocol to maintain compatibility with legacy QEMU drivers:

- **Sync Pattern**: `0xAA 0x55` (2 bytes)
- **Header**:
  - `Mode` (1 byte): `0x00` = BC->RT (Rx), `0x01` = RT->BC (Tx)
  - `RT` (1 byte): 0-31
  - `SA` (1 byte): 0-31
  - `WC` (1 byte): 0-32 (0 = 32 words)
- **Payload**: 64 bytes (32 words \* 2 bytes/word).
- **Total Packet Size**: 70 bytes.

---

## 📂 Project Structure

- `src/core`: 1553 logic, BC/BM implementation, and TCP Proxy.
- `src/ui`: wxWidgets based graphical interface.
- `scripts/`: Build, Run, and Permission management scripts.
- `bin/`: Output directory for the compiled binary.
