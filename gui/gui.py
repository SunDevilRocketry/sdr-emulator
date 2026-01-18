import tkinter as tk
from PIL import Image, ImageTk
from pathlib import Path
import time
import os
import buzzer

# Global variables
repo_root = None
current_image_path = None
image_label = None  # We will store the label here
tk_image = None     # We MUST store a global reference to the PhotoImage object

def set_status_led(led_color: int):
    global current_image_path, tk_image

    match led_color:
        case 0: # LED_NONE
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-none.png"
        case 1: # LED_GREEN
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-g.png"
        case 2: # LED_RED
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-r.png"
        case 3: # LED_BLUE
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-b.png"
        case 4: # LED_CYAN
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-c.png"
        case 5: # LED_PURPLE
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-p.png"
        case 6: # LED_YELLOW
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-y.png"
        case 7: # LED_WHITE
            current_image_path = str(repo_root) + "/emulator/resources/fc-status-w.png"
    
    # Open, resize (optional), and convert the new image
    img = Image.open(current_image_path)
    img = img.resize((1435, 681), Image.Resampling.LANCZOS)
    tk_image = ImageTk.PhotoImage(img) # Update the global PhotoImage reference

    # Configure the label to use the new image
    image_label.config(image=tk_image)
    # The label needs to keep a local reference as well if the global one wasn't enough in complex scenarios
    image_label.image = tk_image

def rotate_status_led_test():
    rotate_status_led_test.i = (rotate_status_led_test.i + 1) % 8
    set_status_led(rotate_status_led_test.i)
    root.after(4000, rotate_status_led_test)  # 4 seconds

rotate_status_led_test.i = -1

# Determine fw repository root
script_path = Path(__file__).resolve()
script_dir = script_path.parent
repo_root = script_dir.parent.parent

# Pick initial image
current_image_path = str(repo_root) + "/emulator/resources/fc-power-off.png"

# Main Tkinter setup
root = tk.Tk()
root.title("SDR HW Emulator")

# Start after 10 seconds
root.after(10_000, rotate_status_led_test)

# Initial image setup (using PIL for flexibility)
initial_img = Image.open(current_image_path)
initial_img = initial_img.resize((1435, 681), Image.Resampling.LANCZOS)
tk_image = ImageTk.PhotoImage(initial_img)

# Create a Label widget to display the image
image_label = tk.Label(root, image=tk_image)
image_label.pack()

## TEST
# fc_buzzer = buzzer.Buzzer()
# fc_buzzer.start_tone()
# time.sleep(0.1)
# fc_buzzer.stop_tone()
# time.sleep(0.1)
# fc_buzzer.start_tone()
# time.sleep(0.1)
# fc_buzzer.stop_tone()

# Start the Tkinter event loop
root.mainloop()