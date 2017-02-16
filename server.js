// ===============
// Dependencies
// ===============

var express = require("express"),
    app = express(),
    router = express.Router(),
    http = require('http').Server(app),
    server = require('http').createServer(app),
    bodyParser = require('body-parser'),
    cookieParser = require('cookie-parser'),
    methodOverride = require('method-override'),
    SerialPort = require('serialport'),
    errorHandler = require('errorhandler'),
    hostname = process.env.HOSTNAME || 'localhost',
    PORT = process.env.PORT || 8081,
    publicDir = process.argv[2] || __dirname + '/public',
    path = require('path'),
    io = require("socket.io")(http, {
        serveClient: false
    }),
    exphbs = require('express-handlebars'),
    session = require('express-session');



// ====================
// Express Config
// ====================

app.engine('handlebars', exphbs({}));
app.set('view engine', 'handlebars');
app.set('port', PORT);

app.use(methodOverride('X-HTTP-Method-Override'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
    extended: true
}));
app.use(express.static(publicDir));
app.use(errorHandler({
    dumpExceptions: true,
    showStack: true
}));

// ====================
// Express Routes
// ====================

router.get('/', function(req, res) {
    res.render('panel', []);
});

app.use(router);

// ====================
// ArTank Serial Communication
// ====================

var tank = new SerialPort('/dev/ttyACM0', {
    baudRate: 115200
}, function(err) {
    if (err) {
        return console.log('Error: ', err.message);
    }
});

tank.on('data', function(data) {
    console.log('Data: ' + data);
});

// ====================
// Socket.io
// ====================
var xy = [0, 0];
var controllingSocket = null;
var controllingTimestamp = 0;
var waitingSockets = [];
const CONTROLTIME = 15000;

//get time it takes for socket to become controller
var waitingTimeCheck = function(socket) {
    //check if socket is controlling
    if (socket.id === controllingSocket) {
        return 0;
    }

    //get remaining controlling time for current controller
    var currentControllerRemainingTime = CONTROLTIME - (Date.now() - controllingTimestamp);

    //check position in waiting list and multiply with controltime
    var waitingTime = 0;
    var waitingListIndex = waitingSockets.indexOf(socket.id);

    if (waitingListIndex > -1) {
        waitingTime = waitingListIndex * CONTROLTIME;
    }

    return waitingTime + currentControllerRemainingTime;
};

//function to check if controllingSockets turn is over
var controllerCheck = function() {
    //check for multiple id's in case of reload
    for (var i = 0; i < waitingSockets.length; i++) {
        //delete waiting socket if duplicate of controlling socket
        if (waitingSockets[i] === controllingSocket) {
            waitingSockets.splice(i, 1);
        }

        //delete socket if it is a duplicate of a socket on the waiting list which appeared earlier
        if (waitingSockets.indexOf(waitingSockets[i]) < i) {
            waitingSockets.splice(i, 1);
        }
    }

    //check if controlling Socket is free
    if (controllingSocket === null) {
        //make first socket in waitinglist controller
        controllingSocket = waitingSockets[0];
        controllingTimestamp = Date.now();

        //inform new controlling socket
        io.to(controllingSocket).emit('controlling', {
            control: true
        });

        //remove first socket from waiting list
        waitingSockets.splice(0, 1);
    }

    //replace controller if he is controlling for longer than the allowed controlling time
    if (controllingSocket !== "" && Date.now() - controllingTimestamp > CONTROLTIME) {
        //inform current controlling socket of controller change
        io.to(controllingSocket).emit('controlling', {
            control: false
        });

        //add current controller at the end of the waiting list
        waitingSockets.push(controllingSocket);

        //make first socket in waiting list the controller and set timestamp
        controllingSocket = waitingSockets[0];
        controllingTimestamp = Date.now();

        //inform new controlling socket
        io.to(controllingSocket).emit('controlling', {
            control: true
        });

        //remove first socket from waiting list
        waitingSockets.splice(0, 1);
    }
};

//connection handling
io.on('connection', function(socket) {

    //add socket to waiting pool
    waitingSockets.push(socket.id);

    //check current controller
    controllerCheck();

    //tell socket whether it is the controller or not
    socket.emit('controlling', {
        control: (socket.id === controllingSocket),
        waitingTime: waitingTimeCheck(socket)
    });

    //send initial values to new user
    socket.emit('initialize', {
        'xy' : xy
    });

    //let clients check if the controller has changed
    socket.on('controllercheck', function() {
        controllerCheck();

        //if socket is not controlling, send it its waiting time
        if (controllingSocket !== socket.id) {
            socket.emit('controlling', {
                control: false,
                waitingTime: waitingTimeCheck(socket)
            });
        }
    });

    //replace controller on disconnect/ remove socket from waitinglist
    socket.on('disconnect', function() {
        //check if socket is controller
        if (socket.id === controllingSocket) {

            //make first socket in waitinglist controller
            controllingSocket = waitingSockets[0];
            controllingTimestamp = Date.now();

            //inform new controlling socket
            io.to(controllingSocket).emit('controlling', {
                control: true
            });
        }

        //check if socket is in waiting list
        removeWaitingIndex = waitingSockets.indexOf(socket.id);

        if (removeWaitingIndex > -1) {
            //remove socket from waiting list
            waitingSockets.splice(removeWaitingIndex, 1);
        }
    });

    //handle incoming values
    socket.on('xy', function(msg) {
        //only process input from currently controlling socket
        if (socket.id === controllingSocket) {

          //broadcast current position to all sockets
          socket.broadcast.emit('xy', {
              'xy': msg.xy
          });

            //save and validate new position, format for arduino
            xy = [parseInt(msg.xy[0]), parseInt(msg.xy[1])];
            xy.forEach(function(e, i, a) {
              e = Math.floor(e);
              e = e*5;
              e = Math.min(255, e);
              e = Math.max(-255, e);
              e = -e;

              xy[i] = e;
            });
            //console.log('R Value: ' + msg);

            //send current xy values to tank
            console.log(xy);
            tank.write(new Buffer(xy[0] + ';' + xy[1] + '*'));
        }
    });
});
// ====================
// Start the Server
// ====================

//start socket.io listener
http.listen(PORT, function() {
    console.log("Server showing %s listening at http://%s:%s", publicDir, hostname, PORT);
});

//start express
app.start = app.listen = function() {
    return server.listen.apply(server, arguments);
};
