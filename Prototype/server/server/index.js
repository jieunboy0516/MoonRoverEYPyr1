const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const path = require("path");

const app = express();
const port = 3000;

// Where we will keep books
let books = [];

app.use(cors());

// Configuring body parser middleware
app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

app.post("/book", (req, res) => {
  // We will be coding here
});

app.get("/", function (req, res) {
  res.sendFile(path.join(__dirname, "/joystick.html"));
});

app.listen(port, () =>
  console.log(`Hello world app listening on port ${port}!`)
);

var httpapp = require("http").createServer(handler);
var io = require("socket.io")(httpapp);
var fs = require("fs");

var mySocket = 0;

httpapp.listen(4000); //Which port are we going to listen to?

function handler(req, res) {
  fs.readFile(
    __dirname + "/rover.html", //Load and display outputs to the index.html file
    function (err, data) {
      if (err) {
        res.writeHead(500);
        return res.end("Error loading index.html");
      }
      res.writeHead(200);
      res.end(data);
    }
  );
}

io.sockets.on("connection", function (socket) {
  console.log("Webpage connected"); //Confirmation that the socket has connection to the webpage
  io.sockets.emit("myip", serverip);
  //mySocket = socket;
  //io.sockets.emit('field', 'test SUCCESS');
});

//UDP server on 41181
var dgram = require("dgram");
var server = dgram.createSocket("udp4");

var roverip = "192.168.26.108";

server.on("message", function (msg, rinfo) {
  console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);
  // /console.log("Broadcasting Message: " + msg); //Display the message coming from the terminal to the command line for debugging

  if (msg == "ACK") {
    return;
  }

  if (msg.toString().substring(0, 3) == "ZDB") {

    roverip = msg.toString().slice(3);
    console.log(`updated rover ip: ${roverip}`);

    //send status and roverip
    io.sockets.emit("roverip", roverip);


    //send my ip
    //io.sockets.emit("myip", serverip);

    return;
  }
  // if (mySocket != 0) {
  //    mySocket.emit('field', "" + msg);
  //    mySocket.broadcast.emit('field', "" + msg); //Display the message from the terminal to the webpage
  // }

  // io.sockets.emit('field', msg.toString());

  //if (rinfo.address == roverip && isJson(msg.toString())) {
    if (isJson(msg.toString()))
    io.sockets.emit("field", JSON.stringify(processdata(msg)));
  //}

  //io.sockets.emit('field', JSON.stringify(processdata(JSON.stringify(testValues))));
});

server.on("listening", function () {
  var address = server.address(); //IPAddress of the server
  console.log(
    "UDP server listening to " + address.address + ":" + address.port
  );
});

server.bind(2390);

var message = new Buffer("ZDB100000");


var testValues = {
  radio61k: 151.53,
  magnetic: 238.96,
  infrared: 571.1,
  magnetic : 540
};

function confidence_factor(freq, ideal) {
  if (freq > ideal - 5 && freq < ideal + 5) {
    return 1;
  } else if (freq < ideal) {
    return freq / ideal;
  } else {
    return ideal / freq;
  }
}

var confidenceValues = {
  gaborium: 0,
  lathwaite: 0,
  adamantine: 0,
  xirang: 0,
  thiotimoline: 0,
  netherite: 0,
};

function processdata(input) {
  input = JSON.parse(input.toString("utf8"));

  // Data analysis
  if (input.magnetic >= 540) { // If magnetic field up
    confidenceValues["adamantine"] = 1;
  } else if (input.magnetic <= 533) { // If magnetic field down
    confidenceValues["xirang"] = 1;
  } else { // If no magnetic field
    confidenceValues["xirang"] = 0;
    confidenceValues["adamantine"] = 0;
    confidenceValues["gaborium"] = confidence_factor(input.radio61k, 151);
    confidenceValues["lathwaite"] = confidence_factor(input.radio61k, 239);
    confidenceValues["thiotimoline"] = confidence_factor(input.infrared, 353);
    confidenceValues["netherite"] = confidence_factor(input.infrared, 571);
  }

  var identifiedMineral = "none";

  for (var key in confidenceValues) {
    if (confidenceValues[key] > 0.95) {
      if (identifiedMineral == "none") {
        identifiedMineral = key;
      } else {
        identifiedMineral = "multiple";
        break;
      }
    }
  }

  /**
   console.log(`GaboriumConfidence ${GaboriumConfidence*100} % \n`);
   console.log(`LathwaiteConfidence ${LathwaiteConfidence*100} % \n`);
   console.log(`AdamantineConfidence ${AdamantineConfidence*100} % \n`);
   console.log(`XirangConfidence ${XirangConfidence*100} % \n`);
   console.log(`ThiotimolineConfidence ${ThiotimolineConfidence*100} % \n`);
   console.log(`NetheriteConfidence ${NetheriteConfidence*100} % \n`);
   **/

  for (var key in confidenceValues) {
    //console.log(key + " confidence value = " + confidenceValues[key])
  }

  //var maxConfidence = Math.max(GaboriumConfidence, LathwaiteConfidence, AdamantineConfidence, XirangConfidence, ThiotimolineConfidence, NetheriteConfidence)

  /**
   if(maxConfidence == GaboriumConfidence) material = "Gaborium";
   if(maxConfidence == LathwaiteConfidence) material = "Lathwaite";
   if(maxConfidence == AdamantineConfidence) material = "Adamantine";
   if(maxConfidence == XirangConfidence) material = "Xirang";
   if(maxConfidence == ThiotimolineConfidence) material = "Thiotimoline";
   if(maxConfidence == NetheriteConfidence) material = "Netherite";
   **/

  var result = JSON.parse(JSON.stringify(input));

  //result["roverip"] = roverip;
  result["material"] = identifiedMineral;
  result["chosen_material_confidence"] = confidenceValues[identifiedMineral];
  result["GaboriumConfidence"] = confidenceValues["gaborium"];
  result["LathwaiteConfidence"] = confidenceValues["lathwaite"];
  result["AdamantineConfidence"] = confidenceValues["adamantine"];
  result["XirangConfidence"] = confidenceValues["xirang"];
  result["ThiotimolineConfidence"] = confidenceValues["thiotimoline"];
  result["NetheriteConfidence"] = confidenceValues["netherite"];

  //these lines is for testing, remove it
  // result.material = "adamantine";
  // result.chosen_material_confidence = 0.2;

  console.log(result);
  return result;
}

function isJson(str) {
  try {
    JSON.parse(str);
  } catch (e) {
    return false;
  }
  return true;
}

console.log(processdata(JSON.stringify(testValues)));

//setInterval(requestData, 250);


function requestData() {

  if (roverip == null) return;

  server.send(
    message,
    0,
    message.length,
    2390,
    roverip,
    function (err, bytes) {
      if (err) throw err;
      //console.log('UDP message sent to ' + "192.168.5.108" +':'+ "2390");
    }
  );
}



var os = require('os');

var networkInterfaces = os.networkInterfaces();

var serverip;

for (var key in networkInterfaces) {
  if (networkInterfaces.hasOwnProperty(key)) {
    networkInterfaces[key].forEach(function(element){
        if(element.address.substring(0,3) == "192") {
          serverip = element.address
          console.log(serverip)
        }
    })
  }
}

//console.log(networkInterfaces);