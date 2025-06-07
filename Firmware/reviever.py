# Catnapper Receiver (MicroPython)
# --------------------------------
# Listens for ChairCommand over ESP-NOW and drives two relays to control the chair motor.

import network
import espnow
from machine import Pin
import time

# Pin definitions (match your wiring)
RELAY_UP = 2    # Pin to activate 'UP' relay
RELAY_DOWN = 3  # Pin to activate 'DOWN' relay

# Initialize relay outputs
relay_up = Pin(RELAY_UP, Pin.OUT)
relay_down = Pin(RELAY_DOWN, Pin.OUT)
relay_up.value(0)
relay_down.value(0)

# Set up Wi-Fi in station mode (required for ESP-NOW)
w0 = network.WLAN(network.STA_IF)
w0.active(True)

# Initialize ESP-NOW
esp = espnow.ESPNow()
esp.init()

# (Optional) Add peer for filtered reception
# Replace with your remote's MAC address
emote_mac = b'\x24\x6F\x28\xAA\xBB\xCC'
esp.add_peer(remote_mac)

print("Catnapper Receiver Ready")

# Main receive loop
while True:
    host, msg = esp.recv()  # blocking; returns (mac, bytes)
    if msg:
        # Expecting 2-byte payload: [up_flag, down_flag]
        up_flag = bool(msg[0])
        down_flag = bool(msg[1])

        # Drive the relays
        relay_up.value(1 if up_flag else 0)
        relay_down.value(1 if down_flag else 0)

    # Small delay to yield
    time.sleep_ms(100)
