#!/bin/python3 
import os, sys, time, math
from threading import Thread, Lock
from PySide2.QtWidgets import QApplication, QWidget, QHBoxLayout, QVBoxLayout, QLabel, QTextEdit, QPushButton, QMessageBox, QDialog, QListWidget
from PySide2.QtGui import Qt
from http.client import HTTPConnection

CONFIG_PATH = "creator_panel.conf"
config = dict()


# Load configuration file
def load_config():
  global config
  
  # Abort if config file doesn't exist
  if not os.path.exists(CONFIG_PATH):
    return False
  
  check_interval_set = False
  address_set        = False
  port_set           = False
  
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
      if key=="check-interval":
        check_interval_set = True
      elif key=="address":
        address_set = True
      elif key=="port":
        port_set = True
  # Abort if not all required values were set
  if not (check_interval_set and address_set and port_set):
    return False
  
  # Convert transition time to an floating point number, or abort if it's invalid or out of range
  # Also convert color hexcodes to color classes, and port to integer
  try:
    config["check-interval"] = int(config["check-interval"])
    config["port"] = int(config["port"])
  except:
    return False
  
  # Abort if port out of range
  if config["port"]<0 or config["port"]>65535:
    return False
  
  # Everything was successful
  return True


# Window class
class mainWindow(QWidget):
  def __init__(self):
    super().__init__()
    self.lock = Lock()
    self.uilock = Lock()
    # Set main window properties
    self.setWindowTitle("Escape Room - Creator Panel")
    self.resize(410,0)
    self.mainLayout = QHBoxLayout(self)
    
    # Info section setup
    self.infoLayout = QVBoxLayout(self)
    self.mainLayout.addLayout(self.infoLayout)
    # Time display
    self.time = QLabel(self)
    self.infoLayout.addWidget(self.time)
    self.time.setTextFormat(Qt.RichText)
    self.time.setText("<b>Time:</b> ")
    # Hints display
    self.hints = QLabel(self)
    self.infoLayout.addWidget(self.hints)
    self.hints.setTextFormat(Qt.RichText)
    self.hints.setText("<b>Hints:</b> ")
    # Score display
    self.score = QLabel(self)
    self.infoLayout.addWidget(self.score)
    self.score.setTextFormat(Qt.RichText)
    self.score.setText("<b>Score:</b>")
    # History
    self.history = QTextEdit(self)
    self.infoLayout.addWidget(self.history)
    self.history.setReadOnly(True)
    self.history.setAcceptDrops(False)
    self.history.setPlaceholderText("Action history will be displayed here.")
    
    # Button section setup
    self.buttonLayout = QVBoxLayout(self)
    self.mainLayout.addLayout(self.buttonLayout)
    # Start/pause button
    self.startButton = QPushButton(self)
    self.buttonLayout.addWidget(self.startButton)
    self.startButton.setText("Start")
    # Reset button
    self.resetButton = QPushButton(self)
    self.buttonLayout.addWidget(self.resetButton)
    self.resetButton.setText("Reset")
    # Add hint button
    self.addHintButton = QPushButton(self)
    self.buttonLayout.addWidget(self.addHintButton)
    self.addHintButton.setText("Add Hint")
    # Remove hint button
    self.removeHintButton = QPushButton(self)
    self.buttonLayout.addWidget(self.removeHintButton)
    self.removeHintButton.setText("Remove Hint")
    # Victory lights button
    self.victoryLightsButton = QPushButton(self)
    self.buttonLayout.addWidget(self.victoryLightsButton)
    self.victoryLightsButton.setText("Victory Lights")
    # Reset lights button
    self.resetLightsButton = QPushButton(self)
    self.buttonLayout.addWidget(self.resetLightsButton)
    self.resetLightsButton.setText("Reset Lights")
    # Screen blanking button
    self.screenBlankButton = QPushButton(self)
    self.buttonLayout.addWidget(self.screenBlankButton)
    self.screenBlankButton.setText("Blank Screen")
    # Slideshow button
    self.slideshowButton = QPushButton(self)
    self.buttonLayout.addWidget(self.slideshowButton)
    self.slideshowButton.setText("Slideshow")
    self.buttonLayout.addStretch()
    
    # Slideshow dialog
    self.slideshowDialog = QDialog(self)
    self.slideshowDialog.setWindowTitle("Select slideshow")
    self.slideshowDialog.resize(250,100)
    self.slideshowDialog.mainLayout = QVBoxLayout(self.slideshowDialog)
    # List
    self.slideshowDialog.list = QListWidget(self.slideshowDialog)
    self.slideshowDialog.mainLayout.addWidget(self.slideshowDialog.list)
    self.slideshowDialog.list.addItem("Item 8 Hint 1")
    self.slideshowDialog.list.addItem("Item 8 Hint 2")
    self.slideshowDialog.list.setCurrentRow(0)
    # Buttons
    self.slideshowDialog.buttonLayout = QHBoxLayout(self.slideshowDialog)
    self.slideshowDialog.mainLayout.addLayout(self.slideshowDialog.buttonLayout)
    self.slideshowDialog.buttonLayout.addStretch()
    # OK
    self.slideshowDialog.OKButton = QPushButton(self)
    self.slideshowDialog.buttonLayout.addWidget(self.slideshowDialog.OKButton)
    self.slideshowDialog.OKButton.setText("OK")
    # Cancel
    self.slideshowDialog.cancelButton = QPushButton(self)
    self.slideshowDialog.buttonLayout.addWidget(self.slideshowDialog.cancelButton)
    self.slideshowDialog.cancelButton.setText("Cancel")
    
    # Set up slots and signals
    self.startButton.clicked.connect(self.startSignal)
    self.resetButton.clicked.connect(self.resetSignal)
    self.addHintButton.clicked.connect(self.hintAddSignal)
    self.removeHintButton.clicked.connect(self.hintRemoveSignal)
    self.victoryLightsButton.clicked.connect(self.victoryLightsSignal)
    self.resetLightsButton.clicked.connect(self.resetLightsSignal)
    self.screenBlankButton.clicked.connect(self.screenBlankSignal)
    self.slideshowButton.clicked.connect(self.slideshowDialog.exec_)
    self.slideshowDialog.OKButton.clicked.connect(self.slideshowDialog.accept)
    self.slideshowDialog.cancelButton.clicked.connect(self.slideshowDialog.reject)
    self.slideshowDialog.accepted.connect(self.sendSlideshow)
  
  # Updates UI with given time and hint data
  def updateUi(self, start, pause, hints, update_all=False):
    if update_all:
      self.uilock.acquire()
    # Timer hasn't started
    if start==0:
      t=0
      if update_all:
        self.startButton.setText("Start")
    # Timer is started but not paused
    elif pause==0:
      t = math.floor(time.time()*10) - start
      if update_all:
        self.startButton.setText("Pause")
    # Timer is paused
    else:
      t = pause-start
      if update_all:
        self.startButton.setText("Resume")
    if update_all:
      self.uilock.release()
    
    # Create human readable time string
    hours = str(t//36000)
    mins  = str((t//600)%60).zfill(2)
    secs  = str((t//10)%60).zfill(2)
    time_str = f"{hours}:{mins}:{secs}"
    
    # Store time string for use by other functions
    self.lock.acquire()
    self.time_str = time_str
    self.lock.release()
    # Calculate score
    if t==0 or hints<0:
      score = "N/A"
    else:
      score = str(round(10000000/(t*((hints/4)+1)),3))
    # Set display text
    self.uilock.acquire()
    self.time.setText(f"<b>Time:</b> {time_str} ({str(t/10)}s)")
    self.hints.setText(f"<b>Hints:</b> {str(hints)}")
    self.score.setText(f"<b>Score:</b> {score}")
    self.uilock.release()
  
  # Adds action to history
  def historyAdd(self, text):
    self.lock.acquire()
    gametime_str = self.time_str
    self.lock.release()
    timestamp = time.strftime(f"<i><font color='gray'>%H:%M sys, </font></i><font color='#007FFF'>{gametime_str} game</font> ")
    self.uilock.acquire()
    self.history.append(timestamp+text)
    self.uilock.release()
  
  # Signal handlers
  def startSignal(self):
    self.historyAdd("Toggled <b>timer</b>.")
    self.start_pointer()
    self.ledstrip_send_pointer("base")
  
  def resetSignal(self):
    self.historyAdd("<b>Reset</b>.")
    self.reset_pointer()
    self.ledstrip_send_pointer("base")
  
  def hintAddSignal(self):
    self.historyAdd("Added <b>hint</b>.")
    self.hintadd_pointer()
    self.ledstrip_send_pointer("hint")
  
  def hintRemoveSignal(self):
    self.historyAdd("Removed <b>hint</b>.")
    self.hintremove_pointer()
  
  def victoryLightsSignal(self):
    self.historyAdd("Set victory <b>lights</b>.")
    self.ledstrip_send_pointer("victory")
  
  def resetLightsSignal(self):
    self.historyAdd("Reset <b>lights</b>.")
    self.ledstrip_send_pointer("base")
  
  def screenBlankSignal(self):
    self.historyAdd("Blanked <b>screen</b>.")
    self.ledstrip_send_pointer("blank")
  
  def sendSlideshow(self):
    command = ["8.1", "8.2"]
    pos = self.slideshowDialog.list.currentRow()
    self.historyAdd(f"Sent <b>slideshow</b> {command[pos]}.")
    self.ledstrip_send_pointer(f"show-{command[pos]}")
  
  # Halts time watch object when app is closing
  def closeEvent(self,e):
    self.timewatch_halt_pointer()


# Class that handles reading and modifying files which hold time/hint data
class timeWatch():
  def __init__(self):
    self.lock = Lock()
    self.start = 0
    self.pause = 0
    self.hints = 0
    self.active = False
    self.second_iterator_thread = Thread(target=self.secondIterator, daemon=True)
    self.file_watch_thread = Thread(target=self.fileWatch, daemon=True)
  
  # Reads time and hint values
  def getValues(self):
    # Make sure no other thread is interfering right now
    self.lock.acquire()
    start_f, pause_f, hints_f = None, None, None
    # Try reading values from files
    try:
      start_f = open("start.txt","r")
      pause_f = open("pause.txt","r")
      hints_f = open("hints.txt","r")
      self.start = int(start_f.read())
      self.pause = int(pause_f.read())
      self.hints = int(hints_f.read())
      start_f.close()
      pause_f.close()
      hints_f.close()
    # If there's an error, close any opened files and set negative values so that the user suspects that something went wrong
    except:
      if start_f!=None:
        start_f.close()
      if pause_f!=None:
        pause_f.close()
      if hints_f!=None:
        hints_f.close()
      self.start, self.pause = -1, -1
    # Signal UI to update
    self.update_ui_pointer(self.start, self.pause, self.hints, update_all=True)
    # Determine if timer is running
    self.active = not ((self.start==0 and self.pause==0) or (self.start==-1 or self.pause==-1))
    # Allow other threads to do stuff now that we're done
    self.lock.release()
  
  # Iterates time every second
  def secondIterator(self):
    while True:
      sleep_time = 0.1
      if self.active and self.lock.acquire(blocking=False):
        #sleep_time = 0.1
        self.update_ui_pointer(self.start, self.pause, self.hints)
        # Calculate how much time is left until the next second (not needed anymore)
        #sleep_time = 0.1-(time.time()-self.start/10)%0.1
        self.lock.release()
      # Sleep until the next second
      time.sleep(sleep_time)
  
  # Periodically checks files for any changes made by external applications
  def fileWatch(self):
    while True:
      self.getValues()
      time.sleep(config['check-interval'])
  
  # Starts, pauses or resumes timer
  def startPauseResume(self):
    # Abort if time values are invalid
    if self.start==-1 or self.pause==-1:
      return;
    # Make sure no other thread is interfering right now
    self.lock.acquire()
    
    t = math.floor(time.time()*10)
    
    # Start
    if self.start==0 and self.pause==0:
      start_f = None
      try:
        start_f = open("start.txt","w")
        start_f.write(str(t))
        start_f.close()
      except:
        if start_f != None:
          start_f.close()
    
    # Pause
    elif self.pause==0:
      pause_f = None
      try:
        pause_f = open("pause.txt","w")
        pause_f.write(str(t))
        pause_f.close()
      except:
        if pause_f != None:
          pause_f.close()
    
    # Resume
    else:
      start_f = None
      pause_f = None
      
      try:
        # First get values from start and pause files, so we can do some calculations
        start_f = open("start.txt","r")
        pause_f = open("pause.txt","r")
        start = int(start_f.read())
        pause = int(pause_f.read())
        start_f.close()
        pause_f.close()
        
        # Find how long has the timer been paused for
        diff = t-pause
        # Move start time forward by that amount
        start += diff
        
        # Write new values to files
        start_f = open("start.txt","w")
        pause_f = open("pause.txt","w")
        start_f.write(str(start))
        pause_f.write("0")
        start_f.close()
        pause_f.close()
      except:
        if start_f != None:
          start_f.close()
        if pause_f != None:
          pause_f.close()
    
    # Allow other threads to do stuff now that we're done
    self.lock.release()
    # Trigger a file check, now that we made a change
    self.getValues()
  
  # Resets timer
  def reset(self):
    # Get latest values
    self.getValues()
    # Create new file with a summary of this game
    output_f = open(time.strftime("total_%Y%m%d_%H%M%S.txt"),"w")
    # Calculate score and make time human readable
    if self.pause==0:
      t = math.floor(time.time()*10 - self.start)
    else:
      t = math.floor(self.pause - self.start)
    hours = str(t//36000)
    mins  = str((t//600)%60).zfill(2)
    secs  = str(t/10).zfill(4)
    if t==0 or self.hints<0:
      score = "N/A"
    else:
      score = str(round(10000000/(t*((self.hints/4)+1)),3))
    # Write summary to file
    output_f.write(f"Time : {hours}:{mins}:{secs} ({str(t/10)}s)\n")
    output_f.write(f"Hints: {str(self.hints)}\n")
    output_f.write(f"Score: {score}\n")
    output_f.close()
    
    # Make sure no other thread is interfering
    self.lock.acquire()
    # Open all files
    start_f, pause_f, hints_f = None, None, None
    try:
      start_f = open("start.txt","w")
      pause_f = open("pause.txt","w")
      hints_f = open("hints.txt","w")
      # Reset all values
      start_f.write("0")
      pause_f.write("0")
      hints_f.write("0")
      # Close files
      start_f.close()
      pause_f.close()
      hints_f.close()
    except:
      if start_f != None:
        start_f.close()
      if pause_f != None:
        pause_f.close()
      if hints_f != None:
        hints_f.close()
    # Allow other threads to do stuff now that we're done
    self.lock.release()
    # Refresh values now that we changed them
    self.getValues()
  
  # Adds hint
  def hintAdd(self):
    # Make sure no one else is interfering
    self.lock.acquire()
    
    hints_f = None
    try:
      # Check how many hints we have
      hints_f = open("hints.txt","r")
      hints = int(hints_f.read())
      hints_f.close()
      
      # Add one hint to that number
      hints_f = open("hints.txt","w")
      hints_f.write(str(hints+1))
      hints_f.close()
    except:
      if hints_f != None:
        hints_f.close()
    
    # Allow other threads to do stuff now that we're done
    self.lock.release()
    # Refresh values now that we changed them
    self.getValues()
  
  # Removes hint
  def hintRemove(self):
    # Make sure no one else is interfering
    self.lock.acquire()
    
    hints_f = None
    try:
      # Check how many hints we have
      hints_f = open("hints.txt","r")
      hints = int(hints_f.read())
      hints_f.close()
      
      # Add one hint to that number
      hints_f = open("hints.txt","w")
      hints_f.write(str(hints-1))
      hints_f.close()
    except:
      if hints_f != None:
        hints_f.close()
    
    # Allow other threads to do stuff now that we're done
    self.lock.release()
    # Refresh values now that we changed them
    self.getValues()
  
  # Halts execution when app is exiting
  def halt(self):
    self.lock.acquire()


# Class that handles communicating with the light control server
class ledstripCommunicator():
  def __init__(self):
    self.lock = Lock()
  
  def __comms_thread(self,switch_to):
    # Prevent multiple commands from being sent at the same time
    self.lock.acquire()
    try:
      # Send request to the server
      comms = HTTPConnection(config["address"], config["port"], timeout=20)
      comms.request('GET',"/"+switch_to)
      comms.getresponse()
    except ConnectionRefusedError:
      self.historyadd_pointer("<font color='red'>Couldn't connect to light control server:</font> Connection refused")
    except OSError as e:
      self.historyadd_pointer(f"<font color='red'>Couldn't connect to light control server:</font> {e}")
    except TimeoutError:
      self.historyadd_pointer("<font color='red'>Couldn't connect to light control server:</font> Timed out")
    except BaseException as e:
      self.historyadd_pointer("<font color='red'>Couldn't connect to light control server.</font>")
      self.lock.release()
      raise e
    self.lock.release()
  
  def send(self,switch_to):
    # Create thread that handles connection with server
    Thread(target=self.__comms_thread, args=(switch_to,)).start()


# Main
def main():
  # Create objects
  app = QApplication()
  window = mainWindow()
  time_watch = timeWatch()
  ledstrip_comms = ledstripCommunicator()
  
  # Load config
  if not load_config():
    # If we can't load the config file, show an error message and quit.
    title = "Escape Room - Creator Panel"
    message = f"""Could not load configuration file; it might be missing, invalid or incomplete. 
Configuration file should be named '{CONFIG_PATH}', located in the directory where the app is run, and needs to have this format and values:<br/><br/>
<b>check-interval=</b><i><font color='gray'>Integer in seconds that indicates how often to check for timer/hint changes by external applications</font></i><br/>
<b>address=</b><i><font color='gray'>Address of computer where the light control server is running</font></i><br/>
<b>port=</b><i><font color='gray'>Port that the light control server is listening to</font></i>"""
    msgbox = QMessageBox(QMessageBox.Critical, title, message)
    msgbox.setTextFormat(Qt.RichText)
    msgbox.exec()
    sys.exit()
  
  # Connect class functions
  time_watch.update_ui_pointer = window.updateUi
  window.start_pointer = time_watch.startPauseResume
  window.reset_pointer = time_watch.reset
  window.hintadd_pointer = time_watch.hintAdd
  window.hintremove_pointer = time_watch.hintRemove
  window.ledstrip_send_pointer = ledstrip_comms.send
  window.timewatch_halt_pointer = time_watch.halt
  ledstrip_comms.historyadd_pointer = window.historyAdd
  
  # Start things
  time_watch.file_watch_thread.start()
  time_watch.second_iterator_thread.start()
  window.show()
  sys.exit(app.exec_());

# Redirects to main
if __name__=="__main__":
  main()
