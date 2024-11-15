#!/bin/bash

# Check if the correct number of arguments is provided
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <server_address> <output_device>"
    exit 1
fi

# Arguments
SERVER_ADDRESS=$1
OUTPUT_DEVICE=$2

# Service file path
SERVICE_NAME="server_daemon@$SERVER_ADDRESS:$OUTPUT_DEVICE.service"
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME"

# Set the current directory and user
WORKING_DIRECTORY="$(pwd)"
SERVER_EXECUTABLE="$WORKING_DIRECTORY/server"  # Assuming the server binary is in the current directory
LOG_FILE="$WORKING_DIRECTORY/server.log"
USER="$(whoami)"  # Use the current user

# Create the service file
sudo bash -c "cat > $SERVICE_FILE" << EOL
[Unit]
Description=Custom Server Daemon for $SERVER_ADDRESS $OUTPUT_DEVICE
After=graphical.target
Requires=graphical.target

[Service]
Type=simple
ExecStart=$SERVER_EXECUTABLE $SERVER_ADDRESS $OUTPUT_DEVICE
WorkingDirectory=$WORKING_DIRECTORY
Environment=DISPLAY=:0
User=$USER
Restart=on-failure
StandardOutput=append:$LOG_FILE
StandardError=append:$LOG_FILE

[Install]
WantedBy=default.target
EOL


# Reload systemd, enable, and start the service
sudo systemctl daemon-reload
sudo systemctl enable "$SERVICE_NAME"
sudo systemctl start "$SERVICE_NAME"

echo "Service $SERVICE_NAME created, enabled, and started."
echo "Check the status with: sudo systemctl status $SERVICE_NAME"
