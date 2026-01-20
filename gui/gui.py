import tkinter as tk
from PIL import Image, ImageTk
from pathlib import Path
import time
import os
import buzzer
import platform
import socket

# Global variables
repo_root = None
current_image_path = None
image_label = None  # We will store the label here
tk_image = None     # We MUST store a global reference to the PhotoImage object
fc_buzzer = buzzer.Buzzer()
emulator_socket = None
emulator_file = None
host = "127.0.0.1"
port = 5100
read_buffer = ""

frame_x_size = int(1435 / 2)
frame_y_size = int(681 / 2)

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
    img = img.resize((frame_x_size, frame_y_size), Image.Resampling.LANCZOS)
    tk_image = ImageTk.PhotoImage(img) # Update the global PhotoImage reference

    # Configure the label to use the new image
    image_label.config(image=tk_image)
    # The label needs to keep a local reference as well if the global one wasn't enough in complex scenarios
    image_label.image = tk_image

def pipe_handler():
    global read_buffer
    
    latest_led_color = None
    latest_buzzer_status = None
    
    try:
        # Read whatever data is available (non-blocking)
        chunk = emulator_socket.recv(4096).decode('utf-8')
        
        if not chunk:  # Connection closed
            print("Emulator connection closed.")
            return
        
        # Add to buffer
        read_buffer += chunk
        
        # Process all complete lines in the buffer
        while '\n' in read_buffer:
            line, read_buffer = read_buffer.split('\n', 1)
            in_str = line.rstrip('\r')  # Handle both \n and \r\n
            
            if not in_str:  # Skip empty lines
                continue
                
            # print(in_str)
            
            if in_str.startswith('LED: '):
                match in_str:
                    case 'LED: OFF' | 'LED: RESET':
                        latest_led_color = 0
                    case 'LED: GREEN':
                        latest_led_color = 1
                    case 'LED: RED':
                        latest_led_color = 2
                    case 'LED: BLUE':
                        latest_led_color = 3
                    case 'LED: CYAN':
                        latest_led_color = 4
                    case 'LED: PURPLE':
                        latest_led_color = 5
                    case 'LED: YELLOW':
                        latest_led_color = 6
                    case 'LED: WHITE':
                        latest_led_color = 7
            elif in_str == 'BUZZER: ON':
                latest_buzzer_status = 1
                # fc_buzzer.start_tone()
            elif in_str == 'BUZZER: OFF':
                latest_buzzer_status = 0
                # fc_buzzer.stop_tone()
        
        # Update UI if we got new values
        if latest_led_color is not None:
            set_status_led(latest_led_color)
            
    except BlockingIOError:
        # No data available right now, that's fine
        pass
    except Exception as e:
        print(f"Error in pipe_handler: {e}")
    
    # Schedule next check
    root.after(10, pipe_handler)

# Determine fw repository root
script_path = Path(__file__).resolve()
script_dir = script_path.parent
repo_root = script_dir.parent.parent

# Pick initial image
current_image_path = str(repo_root) + "/emulator/resources/fc-power-off.png"

# Open socket
time.sleep(1)
emulator_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
emulator_socket.connect((host, port))
emulator_file = emulator_socket.makefile('r', encoding='utf-8')

# Main Tkinter setup
root = tk.Tk()
root.title("SDR HW Emulator")

# Start after 10 seconds
#root.after(10_000, rotate_status_led_test)

# Initial image setup (using PIL for flexibility)
initial_img = Image.open(current_image_path)
initial_img = initial_img.resize((frame_x_size, frame_y_size), Image.Resampling.LANCZOS)
tk_image = ImageTk.PhotoImage(initial_img)

# Create a Label widget to display the image
image_label = tk.Label(root, image=tk_image)
image_label.pack()

# Set up pipe handler
root.after(100, pipe_handler)

# Start the Tkinter event loop
root.mainloop()