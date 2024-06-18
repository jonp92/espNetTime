let prevSecond = -1;
function setClock() {
  const now = new Date();
  const seconds = now.getSeconds();
  const minutes = now.getMinutes();
  const hours = now.getHours();

  const secondDegrees = ((seconds / 60) * 360);
  if (prevSecond === 59 && seconds === 0) {
    document.querySelector('.second-hand').style.transition = 'none';
    document.querySelector('.second-hand').style.transform = `rotate(1deg)`;
    void document.querySelector('.second-hand').offsetWidth; // Trigger reflow
    document.querySelector('.second-hand').style.transition = '';
  } else {
    document.querySelector('.second-hand').style.transform = `rotate(${secondDegrees}deg)`;
    }
  const minuteDegrees = ((minutes / 60) * 360) + ((seconds / 60) * 6);
  const hourDegrees = (((hours % 12) || 12) / 12 * 360) + ((minutes / 60) * 30);


  document.querySelector('.minute-hand').style.transform = `rotate(${minuteDegrees}deg)`;
  document.querySelector('.hour-hand').style.transform = `rotate(${hourDegrees}deg)`;

  prevSecond = seconds;
}

setInterval(setClock, 1000);
setClock();

function updateClock() {
  const now = new Date();
  let hours = now.getHours();
  let minutes = now.getMinutes();
  let seconds = now.getSeconds();
  // Format time to display as two digits
  hours = hours < 10 ? '0' + hours : hours;
  minutes = minutes < 10 ? '0' + minutes : minutes;
  seconds = seconds < 10 ? '0' + seconds : seconds;
  // Display the time
  document.getElementById('clock').innerHTML = hours + ':' + minutes + ':' + seconds;

  let gpsTimeString = document.getElementById('gps_time').innerHTML;
  console.log("gpsTimeString: " + gpsTimeString);
  let gpsTimeParts = gpsTimeString.split(':');
  gpsTimeParts[2] = gpsTimeParts[2].split('.')[0];
  console.log("gpsTimeParts: " + gpsTimeParts);
  let gpsHours = parseInt(gpsTimeParts[0], 10);
  console.log("gpsHours: " + gpsHours);
  let gpsMinutes = parseInt(gpsTimeParts[1], 10);
  console.log("gpsMinutes: " + gpsMinutes);
  let gpsSeconds = parseInt(gpsTimeParts[2], 10);
  console.log("gpsSeconds: " + gpsSeconds);
  let gpsTime = new Date(Date.UTC(now.getUTCFullYear(), now.getUTCMonth(), now.getUTCDate(), gpsHours, gpsMinutes, gpsSeconds, 0));
  console.log("gpsTime: " + gpsTime);
  let diff = now - gpsTime;
  console.log("Difference in ms: " + diff);
  // Assuming 'diff' is the difference in milliseconds
  if (diff < 1000) {
    // Less than a second difference
    console.log("Less than a second difference");
    document.getElementById("time_offset").innerHTML = diff + "ms";
    return;
  } else {
    let totalSeconds = Math.abs(diff) / 1000; // Convert to seconds and ensure it's positive
    let offsetHours = Math.floor(totalSeconds / 3600); // Get hours
    totalSeconds %= 3600; // Remove hours from the total seconds
    let offsetMinutes = Math.floor(totalSeconds / 60); // Get minutes
    let offsetSeconds = Math.floor(totalSeconds % 60); // Remaining seconds

    // Pad with zeros to ensure two digits
    let formattedHours = offsetHours.toString().padStart(2, '0');
    let formattedMinutes = offsetMinutes.toString().padStart(2, '0');
    let formattedSeconds = offsetSeconds.toString().padStart(2, '0');

    // Concatenate for final string
    let formattedDiff = `${formattedHours}:${formattedMinutes}:${formattedSeconds}`;
    console.log("Formatted Difference: " + formattedDiff);

    // Update the HTML element with the formatted time
    document.getElementById("time_offset").innerHTML = formattedDiff;
  }
}

// Update the clock every second
setInterval(updateClock, 1000);

// Initialize the clock
updateClock();

if (!!window.EventSource) {
  var source = new EventSource('/events');
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);
  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  source.addEventListener('time', function(e) {
    console.log("time", e.data);
    let hours = e.data.split(':')[0];
    let minutes = e.data.split(':')[1];
    let seconds = e.data.split(':')[2];
    document.getElementById('gps_time').innerHTML = hours + ':' + minutes + ':' + seconds;
  }, false);
}
else {
  console.log("No EventSource");
}