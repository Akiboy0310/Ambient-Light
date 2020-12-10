window.tabBasic = {};

window.tabBasic.stopDisplay = function (handler) {
    console.log('stop display of live values was pressed');
    esp32.sendCmdNoResultHandling(C.CMD_STOP_DATA);
    esp32.stopLiveValueListener();
};

window.tabBasic.startDisplay = function (handler) {
    console.log('start display of live values was pressed');
    esp32.sendCmdNoResultHandling(C.CMD_START_DATA);
    esp32.startLiveValueListener(tabBasic.liveValueHandler);
};

window.tabBasic.startRecording = function (handler) {
    console.log('start recording');
    esp32.sendCmdNoResultHandling(C.CMD_RECORD_SOUND);
    esp32.startLiveValueListener(tabBasic.liveValueHandler);
};


var osciData = new Uint8Array(512);
var osciPos = 0;
var signalMode = '1';

window.tabBasic.liveValueHandler = function (data) {
    var valArray;
    var values=512;
    var redraw=10;

    //console.log ("received live data: " + data);
    valArray = data.split(':')[1].split(',');    
    var i;
    for (i=0;i<valArray.length-1;i++) {
        actVal = parseInt(valArray[i]);
        if (actVal>255) actVal=255;
        if (actVal<0)   actVal=0;
        osciData[osciPos] = 255-actVal;
        osciPos++;  
        if (osciPos>=values) osciPos=0; 
        if (osciPos % redraw == 0) window.tabBasic.drawScope(values); 
    }
};


window.tabBasic.drawScope = function(values) {
    var ctx = document.getElementById('scope').getContext('2d');
    var width = ctx.canvas.width;
    var height = ctx.canvas.height;
    var scaling = width / values;
    // var log='draw:';

    ctx.fillStyle = 'rgba(0, 10, 0, 0.8)';
    ctx.fillRect(0, 0, width, height);

    ctx.lineWidth = 1;
    ctx.strokeStyle = 'rgb(0, 250, 0)';
    ctx.beginPath();

    for (var x = 0; x < values; x++) {
        ctx.lineTo(x*scaling, osciData[x]);
    }
    ctx.stroke();
}


