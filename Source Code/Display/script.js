const update_interval = 2000 // (ms) Interval on which to poll the server for information
var data,start=0,stop,hints,pause,time=0;

var receiver = new XMLHttpRequest();
var hintsBox = document.getElementById('hints');
var timerBox = document.getElementById('clock');

// Process data when server answers
receiver.onreadystatechange = function() {
  if (receiver.readyState==4 && receiver.status==200) {
    data  = receiver.responseText.split(',');
    start = parseInt(data[0]);
    stop  = parseInt(data[1]);
    hints = parseInt(data[2]);
    pause = parseInt(data[3]);
    
    hintsBox.innerHTML = "Hints: " + hints; // Update hints
  }
};

function zeroPad(x) {
  if (x<10)
    return '0'+x.toString();
  else
    return x.toString();
}

function updateTimer() {
  var seconds = Math.floor(time/10)%60;
  var minutes = Math.floor(time/600)%60;
  var hours   = Math.floor(time/36000);

  timerBox.innerHTML = hours.toString() + ':' + zeroPad(minutes) + ':' + zeroPad(seconds);
}

function update_time() {
  if (start==0) {             // Timer hasn't started yet
    time=0;
  } else if (stop!=0) {       // Timer is stopped
    if (pause==0)
      time = stop-start;  // Paused before stopping
    else
      time = pause-start; // Not paused before stopping
  } else if (pause!=0) {      // Timer is paused
    time = pause-start;
  } else {                    // Timer is running
    time = Math.floor(Date.now()/100)-start
  }
  updateTimer();
}

// Poll the server for information on an interval
setInterval(function() {
  receiver.open('GET',"getdata.php");
  receiver.send();
},update_interval);

// Update timer every second
setInterval(update_time,1000);
