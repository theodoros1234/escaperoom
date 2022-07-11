#!/bin/python3
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Thread, Lock
import os, sys, time, signal, platform, subprocess

# Show warning if not running on Linux
if platform.system()!="Linux":
  print(f"\033[1m\033[93mUnsupported platform ({platform.system()}) was detected. This script was designed for Linux,\nand might run unreliably on other operating systems.\033[0m\n")
# Show friendly error message if dependencies aren't met.
try:
  import serial
except ModuleNotFoundError:
  print("This script depends on the PySerial library, use the following command to install it:")
  print("pip3 install pyserial")
  sys.exit(2)
try:
  from PySide2.QtWidgets import QApplication, QWidget
  from PySide2.QtGui import Qt
except ModuleNotFoundError:
  print("This script depends on the PySide2 library. It should be available on your distribution's repositories, otherwise use the following command to install it:")
  print("pip3 install pyside2")
  sys.exit(2)

config = dict()
CONFIG_PATH = "light_control_server.conf"
# LED Strip communication and screen blanker objects must be available globally, due to http.server limitations
strip = None
blanker = None

# Color class
class color():
  # Initialize color by decoding a hexcode
  # Accepts hexcodes compatible with my Arduino LED strip controller software
  def __init__(self, hexcode=None):
    self.hexcode = hexcode
    self.hexbytes = bytes(hexcode, 'utf-8')
    prefix = hexcode[0:3]
    # Hexcode indicates rainbow color
    if prefix=="$H#":
      self.cltype = "H"
      try:
        self.value = int(hexcode[3:5],16)
      except:
        self.value = 255
    # Hexcode indicates static color
    elif prefix=="$S#":
      self.cltype = "S"
      try:
        self.red = int(hexcode[3:5],16)
      except:
        self.red = 255
      try:
        self.green = int(hexcode[5:7],16)
      except:
        self.green = 255
      try:
        self.blue = int(hexcode[7:9],16)
      except:
        self.blue = 255
    # Invalid hexcode
    else:
      self.cltype = "S"
      self.red, self.green, self.blue = 255, 255, 255
  
  # Adds ANSI color escape codes to given string
  def escapify(self, text):
    # If it's a static color
    if self.cltype == "S":
      # Set color as background
      background = f"\033[48;2;{str(self.red)};{str(self.green)};{str(self.blue)}m"
      # Decide between bright or dark text depending on background luminance. This formula is inaccurate, but good enough for our purpose
      luminance_estimate = 0.2126*pow(self.red/255,2.4) + 0.7152*pow(self.green/255,2.4) + 0.0722*pow(self.blue/255,2.4)
      if luminance_estimate > 0.2:
        foreground = "\033[30m"
      else:
        foreground = "\033[97m"
      # Return colored text
      return background + foreground + text + "\033[0m"
    # If it's a rainbow color
    else:
      hue = 0
      new_text = str()
      # List of foreground and background colors for each hue value
      background_list = ["\033[48;2;255;0;0m", "\033[48;2;255;255;0m", "\033[48;2;0;255;0m", "\033[48;2;0;255;255m", "\033[48;2;0;0;255m", "\033[48;2;255;0;255m"]
      foreground_list = ["\033[97m", "\033[30m", "\033[30m", "\033[30m", "\033[97m", "\033[30m"]
      # Cycle through colors and assign one for each character
      for char in text:
        new_text += background_list[hue] + foreground_list[hue] + char
        hue = (hue+1)%6
      # Return colored text
      return new_text + "\033[0m"
  

# Load configuration file
def load_config():
  global config
  
  # Abort if config file doesn't exist
  if not os.path.exists(CONFIG_PATH):
    return False
  
  base_color_set        = False
  hint_color_bright_set = False
  hint_color_dark_set   = False
  hint_transition_set   = False
  finish_color_set      = False
  transition_set        = False
  hostname_set          = False
  port_set              = False
  serial_set            = False
  baudrate_set          = False
  samplerate_set        = False
  slideshow8_1_set      = False
  slideshow8_2_set      = False
  
  # Read config file
  with open(CONFIG_PATH,'r') as config_f:
    for line in config_f.readlines():
      separator = line.find('=')
      # Skip line if it doesn't follow the format 'key=value'
      if separator==-1:
        continue
      # Split each line to key and value
      key = line[0:separator].strip()
      value = line[separator+1:].strip()
      config[key] = value
      # Check if this is one of the required keys
      if key=="base-color":
        base_color_set = True
      elif key=="hint-color-bright":
        hint_color_bright_set = True
      elif key=="hint-color-dark":
        hint_color_dark_set = True
      elif key=="hint-transition":
        hint_transition_set = True
      elif key=="victory-color":
        finish_color_set = True
      elif key=="transition":
        transition_set = True
      elif key=="hostname":
        hostname_set = True
      elif key=="port":
        port_set = True
      elif key=="serial":
        serial_set = True
      elif key=="baudrate":
        baudrate_set = True
      elif key=="samplerate":
        samplerate_set = True
      elif key=="slideshow8.1-path":
        slideshow8_1_set = True
      elif key=="slideshow8.2-path":
        slideshow8_2_set = True
      else:
        print(f"Unused value '{line.strip()}' found in config file")
  # Abort if not all required values were set
  if not (base_color_set and hint_color_bright_set and hint_color_dark_set and hint_transition_set and finish_color_set and transition_set and hostname_set and port_set and serial_set and baudrate_set and samplerate_set and slideshow8_1_set and slideshow8_2_set):
    return False
  
  # Convert transition time to an floating point number, or abort if it's invalid or out of range
  # Also convert color hexcodes to color classes, and port to integer
  try:
    config["transition"] = float(config["transition"])
    config["base-color"] = color(config["base-color"])
    config["hint-color-bright"] = color(config["hint-color-bright"])
    config["hint-color-dark"] = color(config["hint-color-dark"])
    config["hint-transition"] = float(config["hint-transition"])
    config["victory-color"] = color(config["victory-color"])
    config["port"] = int(config["port"])
    config["baudrate"] = int(config["baudrate"])
    config["samplerate"] = int(config["samplerate"])
  except:
    return False
  
  # Abort if port out of range
  if config["port"]<0 or config["port"]>65535:
    return False
  
  # Everything was successful
  return True


# Class that handles client requests
class RequestHandler(BaseHTTPRequestHandler):
  global ledstrip
  # This function is called by the http.server class whenever a client makes a request.
  def do_GET(self):
    invalid = False
    # Base color request
    if self.path=="/base":
      print(config['base-color'].escapify("Changing to base color"))
      strip.change("base")
      blanker.hide()
    # Hint color request
    elif self.path=="/hint":
      print(config['hint-color-bright'].escapify("Changing to"),end='')
      print(config['hint-color-dark'].escapify(" hint color"))
      strip.startHintMode()
    # Victory color request
    elif self.path=="/victory":
      print(config['victory-color'].escapify("Changing to victory color"))
      strip.change("victory")
    # Screen blank request
    elif self.path=="/blank":
      print(color('$S#000000').escapify("Blanking screen"))
      blanker.show()
    # Hints that show a slideshow on screen
    elif self.path=="/show-8.1":
      print("Showing slideshow for hint 8.1")
      subprocess.Popen(["/usr/bin/soffice", "--show", config["slideshow8.1-path"]])
    elif self.path=="/show-8.2":
      print("Showing slideshow for hint 8.2")
      subprocess.Popen(["/usr/bin/soffice", "--show", config["slideshow8.2-path"]])
    # Space keystroke request
    elif self.path=="/space":
      print("Pressing space key")
      subprocess.Popen(["xdotool", "key", "space"])
    # Invalid request
    else:
      invalid = True
    
    # Respond based on the validity of the request
    if invalid:
      self.send_response(404)
      self.end_headers()
      self.wfile.write(b"Invalid request.")
    else:
      self.send_response(200)
      self.end_headers()
      self.wfile.write(b"Received request.")

# Class that handles connection to Arduino that controls the LED strip
class ledstrip():
  def __init__(self):
    self.lock = Lock()
    self.hintmode_lock = Lock()
    self.hintmode = False
    # Track if connection was made to LED strip
    self.init_success = False
    try:
      # Create serial connection
      self.port = serial.Serial()
      self.port.port = config["serial"]
      self.port.baudrate = config["baudrate"]
      self.port.open()
      # Set base color
      self.color = config["base-color"]
      self.port.write(self.color.hexbytes)
      self.init_success = True
    except FileNotFoundError:
      print("Could not connect to LED strip: Device was disconnected")
    except PermissionError:
      print("Could not connect to LED strip: Access denied")
    except OSError as e:
      print("Could not connect to LED strip:", end=' ')
      if e.errno==2:
        print("Device not connected")
      elif e.errno==13:
        print("Access denied")
      else:
        print(e)
    except BaseException as e:
      self.port.close()
      raise e
    # Close port, so that if the device gets randomly disconnected and reconnected, it won't cause issues.
    self.port.close()
  
  # Transitions LED strip color from current color to specified one
  def change(self, target, hint_transition=False):
    # Disable hint mode if switching to a non-hint color
    if target[0:4]!="hint":
      self.stopHintMode()
    self.lock.acquire()
    # Track if we successfully did a smooth transition
    success = True
    try:
      # Determines which color to use
      if target=="base":
        new_color = config["base-color"]
      elif target=="hint-bright":
        new_color = config["hint-color-bright"]
      elif target=="hint-dark":
        new_color = config["hint-color-dark"]
      elif target=="victory":
        new_color = config["victory-color"]
      else:
        print("Declined request to change to invalid color target")
        new_color = None
      
      # Skip changing if new color is the same as the old one
      if new_color == self.color:
        new_color = None
        success = False
      
      # Proceed if we decided that we should change colors
      if new_color != None:
        # Connect to Arduino
        self.port.open()
        # Make sure we're only making static-static or rainbow-rainbow transitions
        if self.color.cltype == new_color.cltype:
          if hint_transition:
            total_samples = int(config["samplerate"]*config["hint-transition"])
          else:
            total_samples = int(config["samplerate"]*config["transition"])
          # Static color transition
          if self.color.cltype == 'S':
            dR = new_color.red - self.color.red
            dG = new_color.green - self.color.green
            dB = new_color.blue - self.color.blue
            for i in range(1,total_samples+1):
              r = int(self.color.red + dR*i/total_samples)
              g = int(self.color.green + dG*i/total_samples)
              b = int(self.color.blue + dB*i/total_samples)
              self.port.write(bytes("$S#%02X%02X%02X"%(r,g,b),'utf-8'))
          # Rainbow color transition
          elif self.color.cltype == 'H':
            dV = new_color.value - self.color.value
            for i in range(1,total_samples+1):
              v = int(self.color.value + dV*i/total_samples)
              self.port.write(bytes("$H#%02X"%(v,),'utf-8'))
            
        # If changing between two different type color modes, do an instant change.
        else:
          self.port.write(new_color.hexbytes)
          success = False
        
        self.color = new_color
    except FileNotFoundError:
      print("Could not change LED strip color: Device was disconnected")
      success = False
    except PermissionError:
      print("Could not change LED strip color: Access denied")
      success = False
    except OSError as e:
      print("Could not change LED strip color:", end=' ')
      if e.errno==2:
        print("Device was disconnected")
      elif e.errno==13:
        print("Access denied")
      else:
        print(e)
      success = False
    except BaseException as e:
      self.port.close()
      self.lock.release()
      raise e
    # Disconnect from Arduino
    self.port.close()
    self.lock.release()
    return success
  
  # Pulses hint color
  def startHintMode(self):
    Thread(target=self.__hintModeThread, daemon=True).start()
  
  # Thread that handles hint mode pulsing
  def __hintModeThread(self):
    self.hintmode_lock.acquire()
    self.hintmode = True
    self.hintmode_lock.release()
    run = True
    hint_transition = False
    # Pulses until hint mode is disabled by another thread
    while run:
      if not self.change("hint-bright", hint_transition):
        # If there was no smooth transition or an error happened, wait before next change
        time.sleep(1)
      hint_transition = True
      self.hintmode_lock.acquire()
      if not self.hintmode:
        self.hintmode_lock.release()
        break
      self.hintmode_lock.release()
      if not self.change("hint-dark", hint_transition):
        # Again, if there was no smooth transition or an error happened, wait
        time.sleep(1)
      self.hintmode_lock.acquire()
      run = self.hintmode
      self.hintmode_lock.release()
  
  # Stops pulsing hint colors
  def stopHintMode(self):
    self.hintmode_lock.acquire()
    self.hintmode = False
    self.hintmode_lock.release()


# Screen blanking class
class screenBlanker(QWidget):
  def __init__(self):
    super().__init__()
    self.active = False
  # Track if screen is blanked or not
  def showEvent(self, e):
    self.active = True
    self.setWindowState(self.windowState() | Qt.WindowFullScreen)
  def hideEvent(self, e):
    # If the screen was blanked, ask the desktop manager to lock the session.
    if self.active:
      # We should try to lock the screen after hiding the screen blanking window
      try:
        print("Showing lock screen")
        subprocess.run(["qdbus", "org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver.SetActive", "true"], timeout=15)
      except BaseException as e:
        print(f"Couldn't lock session: {e}")
    self.active = False


def main():
  global strip, blanker
  app = QApplication()
  app.setQuitOnLastWindowClosed(False)
  # Fix Ctrl+C functionality
  signal.signal(signal.SIGINT, signal.SIG_DFL)
  # Load config file
  if load_config():
    print("\033[4mLoaded following configuration values:\033[0m")
    for i in config.items():
      # If this is a color value, color the text.
      if isinstance(i[1], color):
        print(f"{i[0]}: {i[1].escapify(i[1].hexcode)}")
      else:
        print(f"{i[0]}: {str(i[1])}")
    print()
  # Print error message if config file is invalid
  else:
    print("Could not load configuration file; it might be missing, invalid or incomplete.")
    print(f"Configuration file should be named '{CONFIG_PATH}', located in the directory where\nthe script is run, and needs to have this format and values (without parentheses):\n")
    print("base-color= $(S for static color or H for rainbow)#(Hex value color)")
    print("hint-color-bright= $(S for static color or H for rainbow)#(Hex value color)")
    print("hint-color-dark= $(S for static color or H for rainbow)#(Hex value color)")
    print("hint-transition= (Number of seconds, can be a decimal number. Used for transition between bright and dark hint color)")
    print("victory-color= $(S for static color or H for rainbow)#(Hex value color)")
    print("transition= (Number of seconds, can be a decimal number)")
    print("hostname= (Address from which the server should listen. This should be your device's internal IP address)")
    print("port= (Network port for the server to listen to. This should be an integer between 0-65535 inclusive)")
    print("serial= (Path to serial port that connects to an Arduino controlling an LED strip)")
    print("baudrate= (Integer that indicates what baud rate to use for serial communication with the Arduino)")
    print("samplerate= (Integer that indicates how many color samples per second are sent to the LED strip during transitions)")
    print("slideshow8.1-path = (Path to slideshow for hint 8.1)")
    print("slideshow8.2-path = (Path to slideshow for hint 8.2)")
    sys.exit(1)
  
  # Create screen blanking object
  blanker = screenBlanker()
  # Connect to Arduino LED strip
  print("Connecting to LED strip")
  strip = ledstrip()
  # Exit if connection was unsuccessful
  if not strip.init_success:
    sys.exit(3)
  # Start HTTP Server
  print("Starting server")
  server = HTTPServer((config['hostname'],config['port']),RequestHandler)
  # Activate server
  Thread(target=server.serve_forever, daemon=True).start()
  sys.exit(app.exec_())

if __name__=="__main__":
  main()
