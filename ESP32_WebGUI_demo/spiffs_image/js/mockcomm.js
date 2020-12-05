function MockCommunicator() {
    var _valueHandler = null;
    var _invervalHandler = null;
    var thiz = this;

    this.setValueHandler = function (handler) {
        _valueHandler = handler;
    };

    this.sendData = function (value, timeout) {
        if (!value) return;
        thiz.x = thiz.x || 128;

        return new Promise(function (resolve) {
            if (value == C.CMD_ATTENTION) {
                resolve('');     // no reply to attention command ('AT'), indicates no live connection!
                                 // (reply with 'OK' to indicate a live connection)
            } else if (value.indexOf(C.CMD_START_DATA) > -1) {
                clearInterval(_invervalHandler);
                _invervalHandler = setInterval(function () {
                    if (L.isFunction(_valueHandler)) {
                        thiz.valueString=C.LIVE_VALUE_CONSTANT;
                        for (i=0;i<=512;i++) {
                            thiz.x += getRandomInt(-5,5);
                            if(thiz.x > 240) thiz.x = 240; 
                            if(thiz.x < 10) thiz.x=10;
                            thiz.valueString+=thiz.x+',';
                        }
                        _valueHandler(thiz.valueString);
                    }
                }, 100);
            } else if (value.indexOf(C.CMD_STOP_DATA) > -1) {
                clearInterval(_invervalHandler);
            } else if (value.indexOf(C.CMD_LOAD_CONFIG) > -1) {
                var cmds = '';
                C.CONFIG_MAPPING.forEach( item => cmds += item.cmd + ' ' + item.value +'\n');
                resolve(cmds);
            }
            setTimeout(function () {
                resolve();
            }, timeout);
        });
    };
}

function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function getRandomInt2(factor) {
    return Math.floor((Math.random() - Math.random())*factor);
}