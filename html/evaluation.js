//=========================================
// loadFile
//=========================================
function loadFile(filePathName) {
  // data format:
  // x0,y_a0,y_b0,y_c0,y_d0
  // x1,y_a1,y_b1,y_c1,y_d1
  // x2,y_a2,y_b2,y_c2,y_d2
  var content = null;
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.open("GET", filePathName, false);
  xmlhttp.send();
  if (xmlhttp.status==200) {
    content = xmlhttp.responseText;
  }
  return content;
}

//=========================================
// main
//=========================================
var logData = loadFile("watering_log.txt");
while (logData.indexOf("\r") >= 0) {
  logData = logData.replace("\r", "");
}
var dataLines = logData.split("\n");
var max_x = 1;
var max_y = 1;
var min_x = 100000;
var min_y = 100000;
var numColumns = 0;

var c = document.getElementById("EvaluationCanvas");
var ctx = c.getContext("2d");
var c_legend = document.getElementById("LegendCanvas");
var ctx_legend = c_legend.getContext("2d");

// read all data lines and find out minimum and maximum values for scaling
for (var i = 0; i < dataLines.length; i++) {
  var curLine = dataLines[i];
  var curLineSplit = curLine.split(",");
  if (curLineSplit.length > numColumns) {
    numColumns=curLineSplit.length;
  }
  if (curLineSplit.length > 1) {
    var x=parseInt(curLineSplit[0]);
    if (x > max_x) {
      max_x = x;
    }
    if (x < min_x) {
      min_x = x;
    }
    for (var k = 1; k < curLineSplit.length; k++) {
      var y=parseInt(curLineSplit[k]);
      if (y > 0) {
        if (y > max_y) {
          max_y = y;
        }
        if (y < min_y) {
          min_y = y;
        }
      }
    }
  }
}
// draw data
for (var k=1; k < numColumns; k++) {
  switch (k) {
    case 1:
    ctx.strokeStyle = "#0096E6";
    break;
    case 2:
    ctx.strokeStyle = "#FA1E00";
    break;
    case 3:
    ctx.strokeStyle = "#64C83C";
    break;
    case 4:
    ctx.strokeStyle = "#F0D000";
    break;
    default:
    ctx.strokeStyle = "#000000";
  }
  ctx_legend.strokeStyle = ctx.strokeStyle;
  ctx_legend.beginPath();
  ctx_legend.moveTo(10, k*20);
  ctx_legend.lineTo(190, k*20);
  ctx_legend.stroke();

  ctx.beginPath();
  var first = 1;
  for (var i = 0; i < dataLines.length; i++) {
    var curLine = dataLines[i];
    var curLineSplit = curLine.split(",");
    if (numColumns == curLineSplit.length) {
      var x=(parseInt(curLineSplit[0])-min_x)/(max_x-min_x)*800;
      var y=parseInt(curLineSplit[k]);
      if (y > 0) {
        var y=(y-min_y)/(max_y-min_y)*500;
        if (0 != first) {
          ctx.moveTo(x,y);
          first=0;
        } else {
          ctx.lineTo(x,y);
        }
      }
    }
  }
  ctx.stroke();
}


// user defined selection of log data
var c = document.getElementById("EvaluationSelCanvas");
var ctx = c.getContext("2d");
var dates = loadFile("dates.txt");
while (dates.indexOf("\r") >= 0) {
  dates = dates.replace("\r", "");
}
var datesLines = dates.split("\n");

var parent = document.body;

var selList = document.createElement("select");
selList.id = "dateSelect";
parent.appendChild(selList);

for(var i = 0; i < datesLines.length; i++) {
  var option = document.createElement("option");
  var filePathName=datesLines[i] + "watering_log.txt";
  while (filePathName.indexOf("\n") >= 0) {
    filePathName = filePathName.replace("\n", "");
  }
  var content = null;
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.open("GET", filePathName, false);
  xmlhttp.send();
  if (xmlhttp.status==200) {
    option.value = datesLines[i];
    option.text = datesLines[i];
    selList.appendChild(option);
  }
}


//=========================================
// clearCanvas2
//=========================================
function clearCanvas2() {
  var c2 = document.getElementById("EvaluationSelCanvas");
  var ctx2 = c.getContext("2d");
  //  document.getElementById("clear").addEventListener("click", function() {
  ctx2.clearRect(0, 0, c2.width, c2.height);//}, false);
}


//=========================================
// showSelData
//=========================================
function showSelData() {
  var selDate=document.getElementById("dateSelect").value;

  // data format:
  // x0,y_a0,y_b0,y_c0,y_d0
  // x1,y_a1,y_b1,y_c1,y_d1
  // x2,y_a2,y_b2,y_c2,y_d2
  var filename = selDate + "watering_log.txt";
  while (filename.indexOf("\n") >= 0) {
    filename = filename.replace("\n", "");
  }

  var logData = loadFile(filename);
  while (logData.indexOf("\r") >= 0) {
    logData = logData.replace("\r", "");
  }
  var dataLines = logData.split("\n");
  var max_x = 1;
  var max_y = 1;
  var min_x = 100000;
  var min_y = 100000;
  var numColumns = 0;

  var c = document.getElementById("EvaluationSelCanvas");
  var ctx = c.getContext("2d");


  // read all data lines and find out minimum and maximum values for scaling
  for (var i = 0; i < dataLines.length; i++) {
    var curLine = dataLines[i];
    var curLineSplit = curLine.split(",");
    if (curLineSplit.length > numColumns) {
      numColumns=curLineSplit.length;
    }
    if (curLineSplit.length > 1) {
      var x=parseInt(curLineSplit[0]);
      if (x > max_x) {
        max_x = x;
      }
      if (x < min_x) {
        min_x = x;
      }
      for (var k = 1; k < curLineSplit.length; k++) {
        var y=parseInt(curLineSplit[k]);
        if (y > 0) {
          if (y > max_y) {
            max_y = y;
          }
          if (y < min_y) {
            min_y = y;
          }
        }
      }
    }
  }
  // draw data
  for (var k=1; k < numColumns; k++) {
    switch (k) {
      case 1:
      ctx.strokeStyle = "#0096E6";
      break;
      case 2:
      ctx.strokeStyle = "#FA1E00";
      break;
      case 3:
      ctx.strokeStyle = "#64C83C";
      break;
      case 4:
      ctx.strokeStyle = "#F0D000";
      break;
      default:
      ctx.strokeStyle = "#000000";
    }

    ctx.beginPath();
    var first = 1;
    for (var i = 0; i < dataLines.length; i++) {
      var curLine = dataLines[i];
      var curLineSplit = curLine.split(",");
      if (numColumns == curLineSplit.length) {
        var x=(parseInt(curLineSplit[0])-min_x)/(max_x-min_x)*800;
        var y=parseInt(curLineSplit[k]);
        if (y > 0) {
          var y=(y-min_y)/(max_y-min_y)*500;
          if (0 != first) {
            ctx.moveTo(x,y);
            first=0;
          } else {
            ctx.lineTo(x,y);
          }
        }
      }
    }
    ctx.stroke();
  }
}
