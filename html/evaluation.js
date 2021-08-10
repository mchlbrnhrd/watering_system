//=========================================
// loadFile
//=========================================
function loadFile(filePathName) {
  // data format:
  // x0, modea0, y_a0, modeb0, y_b0, modec0, y_c0, moded0, y_d0
  // x1, modea1, y_a1, modeb1, y_b1, modec1, y_c1, moded1, y_d1
  // x2, modea2, y_a2, modeb1, y_b2, modec2, y_c2, moded2, y_d2
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
const maxNumSensor = 4;
var showData0 = document.getElementById("showData0");
var showData1 = document.getElementById("showData1");
var showData2 = document.getElementById("showData2");
var showData3 = document.getElementById("showData3");
showData0.checked=true;
showData1.checked=true;
showData2.checked=true;
showData3.checked=true;
showMainData();


function showMainData() {
  var showData0 = document.getElementById("showData0");
  var showData1 = document.getElementById("showData1");
  var showData2 = document.getElementById("showData2");
  var showData3 = document.getElementById("showData3");
  var showData = [showData0.checked, showData1.checked, showData2.checked, showData3.checked];

  var concatPrev0 = document.getElementById("concatPrev0");
  var concatPrev1 = document.getElementById("concatPrev1");
  var concatPrev2 = document.getElementById("concatPrev2");
  var concatPrev = [concatPrev0.checked, concatPrev1.checked, concatPrev2.checked];

  var logData = loadFile("watering_log.txt");
  while (logData.indexOf("\r") >= 0) {
    logData = logData.replace("\r", "");
  }
  var dataLines = logData.split("\n");

  if (concatPrev[0]) {
    // concatinate previous data sets

    //var selDatePrev=document.getElementById("dateSelect").value;
    var sel=document.getElementById("dateSelect");
    var selIndex=sel.selectedIndex;
    var selLength=sel.length;
    //window.alert(selLength.toString());
    for (var idx=selLength-2; idx >= selIndex; --idx) {
      var selDatePrev=sel[idx].value;
      var filename = selDatePrev + "watering_log.txt";

      var logDataPrev0 = loadFile(filename);
      while (logDataPrev0.indexOf("\r") >= 0) {
        logDataPrev0 = logDataPrev0.replace("\r", "");
      }
      var dataLinesPrev0 = logDataPrev0.split("\n");
      var curLinePrev = dataLinesPrev0[dataLinesPrev0.length-2];
      var curLineSplitPrev = curLinePrev.split(",");
      var timeOffset=parseInt(curLineSplitPrev[0], 10);
      // add timeOffset to current data
      for (var k=0; k < dataLines.length; ++k) {
        var curLine = dataLines[k];
        if (curLine != "") {
          var curLineSplit = curLine.split(",");
          cancel=false;
          for (var j=0; j < curLineSplit.length; j+=2) {
            if (0 == curLineSplit[j]) {
              cancel=true;
            }
          }
          if (!cancel) {
            var newTime = parseInt(curLineSplit[0], 10) + timeOffset;
            curLineSplit[0] = newTime.toString();
            var newDataLine = "";
            var length = curLineSplit.length > (maxNumSensor*2)+1 ? (maxNumSensor*2)+1: curLineSplit.length;
            for (var m=0; m < length; ++m) {
              newDataLine += curLineSplit[m];
              if (m+1 < length) {
                newDataLine += ",";
              }
            }
            dataLines[k] = newDataLine;
          }
        }
      }
      dataLines.splice.apply(dataLines, [0, 0].concat(dataLinesPrev0));
    }




  //  for (var k=0; k < logDataPrev0.length; ++k) {
  //    logData.push(logDataPrev0[k]);
  //  }
  }



  var max_x = 1;
  var max_y = 1;
  var min_x = 100000;
  var min_y = 100000;
  var numColumns = 0;

  var c = document.getElementById("EvaluationCanvas");
  var ctx = c.getContext("2d");
  ctx.clearRect(0, 0, c.width, c.height);//}, false);
  var c_legend = document.getElementById("LegendCanvas");
  var ctx_legend = c_legend.getContext("2d");
  ctx_legend.clearRect(0, 0, c_legend.width, c_legend.height);

  // read all data lines and find out minimum and maximum values for scaling
  for (var i = 0; i < dataLines.length; i++) {
    var curLine = dataLines[i];
    if (curLine != "") {
      // example: time, modeA, valA, modeB, valB, modeC, valC,...
      // 3600,1,378,1,453,1,598,1,297,1,277,1,271,1,278,1,281
      // => every second entry is mode and can be 0
      var curLineSplit = curLine.split(",");
      // ignore invalid data sets (at least one entry is 0)
      var cancel=false;
      for (var j=0; j < curLineSplit.length; j+=2) {
        if (0 == curLineSplit[j]) {
          cancel=true;
        }
      }
      if (!cancel) {
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
          // length: time + N * (entryMode + entryValue) = 1 + N*2
          var length = curLineSplit.length > (maxNumSensor*2)+1 ? (maxNumSensor*2)+1 : curLineSplit.length;
          for (var k = 2; k < length; k+=2) {
            var k_simple = k/2-1;
            if (showData[k_simple]) {
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
      }
    }
  }

  // read thresholdData
  // example (human readable version:)
  // Channel: 1
  // [S1] threshold low             : 330
  // [S2] threshold high            : 448
  // [S3] threshold expected change : 2250
  // [T1] timeout pump on           : 5
  // [T2] time pump on max          : 20
  // [T3] time wait                 : 129660
  // [T4] time out pump off         : 86400
  // [T5] time out error state      : 86400
  //
  // example: mashine readable version
  // 330
  // 448
  // 2250
  // 5
  // 20
  // 129660
  // 86400
  // 86400
  //
  // => numSetEntry = 8; idxLow = 0; idxHigh = 1
  const numSetEntry=8;
  const idxLow=0;
  const idxHigh=1;

  var showThreshold = document.getElementById("showThreshold");
  if (showThreshold.checked) {
    var thresholdData = loadFile("watering_threshold.txt");
    while (thresholdData.indexOf("\r") >= 0) {
      thresholdData = thresholdData.replace("\r", "");
    }
    var dataLinesThreshold = thresholdData.split("\n");
    var ctr=0;
    for (var i = 0; i < dataLinesThreshold.length; i++) {
      if (ctr < maxNumSensor) {
        if (((i % numSetEntry == idxLow) || (i % numSetEntry == idxHigh)) && showData[ctr]) {
          var thresh = parseInt(dataLinesThreshold[i]);
          if (thresh > max_y) {
            max_y = thresh;
          }
          if (thresh < min_y) {
            min_y = thresh;
          }
        }
        if (i % numSetEntry == idxHigh) {
          ctr++;
        }
      }
    }

  // draw background
    var threshNumCtr=0;
    ctx.save();
    for (var i = 0; i < dataLinesThreshold.length; i++) {
      if (threshNumCtr < maxNumSensor) {
        switch (threshNumCtr) {
          case 0:
          ctx.strokeStyle = "#0096E6";
          break;
          case 1:
          ctx.strokeStyle = "#FA1E00";
          break;
          case 2:
          ctx.strokeStyle = "#64C83C";
          break;
          case 3:
          ctx.strokeStyle = "#F0D000";
          break;
          default:
          ctx.strokeStyle = "#000000";
        }
        var thresh = parseInt(dataLinesThreshold[i]);
        thresh=(thresh-min_y)/(max_y-min_y)*500;
        if (i % numSetEntry == idxLow) {
          if (showData[threshNumCtr]) {
            ctx.setLineDash([3,10]);
            ctx.beginPath();

            ctx.moveTo(0, thresh);
            ctx.lineTo(800, thresh);
            ctx.stroke();
          }
        }
        if (i % numSetEntry == idxHigh) {
          if (showData[threshNumCtr]) {
            ctx.setLineDash([5,15]);
            ctx.beginPath();
            ctx.moveTo(0, thresh);
            ctx.lineTo(800, thresh);
            ctx.stroke();
          }
          threshNumCtr++;
        }
      }
    }
    ctx.restore();
  }

  // draw data
  if (numColumns > (maxNumSensor*2)+1) {
    numColumns = (maxNumSensor*2)+1;
  }
  for (var k=2; k < numColumns; k+=2) {
    var k_simple=k/2-1;
    switch (k_simple) {
      case 0:
      ctx.strokeStyle = "#0096E6";
      break;
      case 1:
      ctx.strokeStyle = "#FA1E00";
      break;
      case 2:
      ctx.strokeStyle = "#64C83C";
      break;
      case 3:
      ctx.strokeStyle = "#F0D000";
      break;
      default:
      ctx.strokeStyle = "#000000";
    }
    if (showData[k_simple]) {
      ctx_legend.strokeStyle = ctx.strokeStyle;
      ctx_legend.beginPath();
      ctx_legend.moveTo(10, k_simple*20);
      ctx_legend.lineTo(190, k_simple*20);
      ctx_legend.stroke();

      ctx.beginPath();
      var first = 1;
      for (var i = 0; i < dataLines.length; i++) {
        var curLine = dataLines[i];
        // example: time, modeA, valA, modeB, valB, modeC, valC,...
        // 3600,1,378,1,453,1,598,1,297,1,277,1,271,1,278,1,281
        if (curLine != "") {
          var curLineSplit = curLine.split(",");
          // ignore invalid data sets (at least one entry is 0 is invalid)
          cancel=false;
          for (var j=0; j < curLineSplit.length; j+=2) {
            if (0 == curLineSplit[j]) {
              cancel=true;
            }
          }
          if (!cancel) {
            if (curLineSplit.length >= numColumns) {
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
        }
      }
      ctx.stroke();
    }
  }
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
      for (var k = 2; k < curLineSplit.length; k+=2) {
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
  if (numColumns > (maxNumSensor*2)+1) {
    numColumns = (maxNumSensor*2)+1;
  }
  for (var k=2; k < numColumns; k+=2) {
    var k_simple=k/2-1;
    switch (k) {
      case 0:
      ctx.strokeStyle = "#0096E6";
      break;
      case 1:
      ctx.strokeStyle = "#FA1E00";
      break;
      case 2:
      ctx.strokeStyle = "#64C83C";
      break;
      case 3:
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
      if (curLineSplit.length >= numColumns) {
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
