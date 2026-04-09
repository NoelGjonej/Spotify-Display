# Spotify-Display
A device that shows current playing songs on spotify.

Here are some photos of the finished product on Fusion 360
<img width="940" height="677" alt="image" src="https://github.com/user-attachments/assets/dcd69c4a-7fc0-42b8-8f2d-c87416ab078f" />
<img width="893" height="756" alt="image" src="https://github.com/user-attachments/assets/803b7d1f-e8a8-4ae4-b3b3-218ce01bec18" />

# Wiring instructions

ST7735 TFT to ESP32 <br>
<img width="739" height="354" alt="image" src="https://github.com/user-attachments/assets/52c2be6e-6b02-476e-8a17-303c5f4126ad" />
<br>
<br>
<br>
EC11E Encoder to ESP32 <br>
<img width="730" height="261" alt="image" src="https://github.com/user-attachments/assets/1d50002e-0371-4b6f-814a-85d33aeb350f" />
<br>
<br>
<br>
Key Switches pin of each switch goes to a spare GPIO (configured as INPUT_PULLUP in code), the other pin goes to GND. 
