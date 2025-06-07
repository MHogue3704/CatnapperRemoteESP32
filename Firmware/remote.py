# Catnapper Remote (MicroPython)
# ------------------------------
# Reads two buttons, displays status on OLED, measures battery,
# sends commands via ESP-NOW, then deep-sleeps until a button is pressed.

import network
import espnow
import machine
import esp32
import time
from machine import Pin, ADC, I2C
import ssd1306

# --- Configuration ------------------------------------------------------
BUTTON_UP_PIN    = 4
BUTTON_DOWN_PIN  = 5
BATTERY_PIN      = 0    # ADC0
CHARGING_PIN     = 10   # Digital input
I2C_SDA_PIN      = 7
I2C_SCL_PIN      = 6
SLEEP_TIMEOUT_MS = 15000  # 15 seconds

# Receiver MAC (update to your peer)
PEER_MAC = b"\x24\x6F\x28\xAA\xBB\xCC"

# --- Hardware Setup -----------------------------------------------------
# OLED display
i2c = I2C(scl=Pin(I2C_SCL_PIN), sda=Pin(I2C_SDA_PIN))
oled = ssd1306.SSD1306_I2C(128, 64, i2c)

def display_status(batt, volt, charging, up, down):
    oled.fill(0)
    oled.text(f"Batt: {batt}%", 0, 0)
    oled.text(f"Volt: {volt:.2f}V", 0, 10)
    if charging:
        oled.text("Charging...", 0, 20)
    oled.text("UP: ON" if up else "UP: OFF", 0, 40)
    oled.text("DN: ON" if down else "DN: OFF", 64, 40)
    oled.show()

# Buttons
btn_up   = Pin(BUTTON_UP_PIN, Pin.IN, Pin.PULL_UP)
btn_down = Pin(BUTTON_DOWN_PIN, Pin.IN, Pin.PULL_UP)
# Charging detect
chg_pin  = Pin(CHARGING_PIN, Pin.IN)
# Battery ADC
bat_adc  = ADC(Pin(BATTERY_PIN))
bat_adc.atten(ADC.ATTN_11DB)

# --- ESP-NOW Setup ------------------------------------------------------
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

esp = espnow.ESPNow()
esp.init()
esp.add_peer(PEER_MAC)

# --- State Variables ---------------------------------------------------
last_up     = False
last_down   = False
last_active = time.ticks_ms()

# --- Helper: read battery ------------------------------------------------
def read_battery():
    raw = bat_adc.read()
    volt = raw / 4095 * 3.3 * 2.0
    pct  = int((volt * 100 - 300) / (420 - 300) * 100)
    return max(0, min(pct, 100)), volt

# --- Main Loop ----------------------------------------------------------
while True:
    up   = not btn_up.value()
    down = not btn_down.value()

    # Send only on state change
    if up != last_up or down != last_down:
        esp.send(PEER_MAC, bytes([1 if up else 0, 1 if down else 0]))
        last_up     = up
        last_down   = down
        last_active = time.ticks_ms()

    # Update display once per second
    if time.ticks_diff(time.ticks_ms(), last_active) % 1000 < 50:
        batt, volt = read_battery()
        charging   = (chg_pin.value() == 0)
        display_status(batt, volt, charging, up, down)

    # Enter deep-sleep after timeout
    if time.ticks_diff(time.ticks_ms(), last_active) > SLEEP_TIMEOUT_MS:
        oled.poweroff()
        wlan.active(False)
        esp = None
        # Wake on either button (active low)
        esp32.wake_on_ext1(pins=[btn_up, btn_down], level=esp32.WAKEUP_ANY_LOW)
        machine.deepsleep()

    time.sleep_ms(200)
