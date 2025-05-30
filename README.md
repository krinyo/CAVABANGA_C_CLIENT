# CAVABANGA_C_CLIENT

A lightweight C client for streaming multimedia from the CAVABANGA_C_SERVER, optimized for low-power devices like Raspberry Pi and Orange Pi.

## Features
- Connects to CAVABANGA_C_SERVER for efficient multimedia streaming
- Minimal resource usage for low-power workstations
- Daemon support for background operation
- In active development with planned improvements

## Dependencies
- Listed in `deps.sh`
- Install dependencies:
  ```bash
  bash deps.sh
  ```

## Installation
1. Clone the repository:
   ```bash
   git clone <repository_url>
   ```
2. Install dependencies:
   ```bash
   bash deps.sh
   ```
3. Build the client:
   ```bash
   make
   ```
4. Start the client as a daemon:
   ```bash
   bash start_server_daemon.sh
   ```

## Usage
- Configure the client to connect to the CAVABANGA_C_SERVER (details in server repository).
- Run the daemon script to start streaming in the background.
- Monitor logs for connection and streaming status.

## Notes
- Designed for compatibility with low-power devices.
- Requires CAVABANGA_C_SERVER (see separate repository).
- Daemon script will be enhanced in future updates.
- Refer to [Raspberry Pi documentation](https://www.raspberrypi.org/documentation/) for device-specific setup.

*Developed by a dedicated C programmer crafting efficient multimedia solutions for constrained environments.*

# CAVABANGA_C_CLIENT
make
bash deps.sh
bash start_server_daemon.sh
check
