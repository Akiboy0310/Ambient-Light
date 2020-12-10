function Esp32Comm(initFinished) {
    var thiz = this;
    var _valueHandler = null;
    var _CMD_BUSY_RESPONSE = 'BUSY';
    var _CMD_OK_RESPONSE = 'OK';
    var _CMD_MIN_WAITTIME_MS = 50;
    var _timestampLastCmd = new Date().getTime();
    var _CmdQueue = [];
    var _sendingCmds = false;
    var _communicator;

    /**
     * sends the given command to the ESP32 via Websocket. If sending of the last command is not completed yet, the given command
     * is added to a queue and will be sent later. The time between two sent commands is at least _CMD_MIN_WAITTIME_MS.
     * The order of sending the commands is always equal to the order of calls to this function.
     *
     * @param actCmd
     * @param onlyIfEmptyQueue if set to true, the command is sent only if the queue is empty
     * @param timeout maximum time after the returned promise resolves, regardless if data was received or not. Default 3000ms.
     * @return {Promise}
     */
    thiz.sendCmd = function (actCmd, onlyIfEmptyQueue, timeout) {
        var timeoutResolve = timeout || 3000;
        if(onlyIfEmptyQueue && _CmdQueue.length > 0) {
            console.log('did not send cmd: "' + actCmd + "' because another command is executing.");
            return new Promise(function (resolve, reject) {
                reject(_CMD_BUSY_RESPONSE);
            });
        }
        var promise = new Promise(function(resolve, reject) {
            if (_CmdQueue.length > 0) {
                console.log("adding cmd to queue: " + actCmd);
            }
            _CmdQueue.push({
                cmd: actCmd,
                resolveFn: resolve,
                rejectFn: reject
            });
        });
        if (!_sendingCmds) {
            sendNext();
        }

        function sendNext() {
            _sendingCmds = true;
            if (_CmdQueue.length == 0) {
                _sendingCmds = false;
                return;
            }
            var nextCmd = _CmdQueue.shift();
            //var timeout = _CMD_MIN_WAITTIME_MS - (new Date().getTime() - _timestampLastCmd);
            //timeout = timeout > 0 ? timeout : 0;
            //console.log("waiting for cmd: " + nextCmd.cmd + ", " + timeout + "ms");
            //setTimeout(function () {
            //    _timestampLastCmd = new Date().getTime();
                console.log("sending to ESP32: " + nextCmd.cmd);
                _communicator.sendData(nextCmd.cmd, timeoutResolve).then(nextCmd.resolveFn, nextCmd.rejectFn);
                sendNext();
            //}, timeout);
        }

        return promise;
    };

    thiz.sendCmdNoResultHandling = function(actCmd) {
        thiz.sendCmd(actCmd).then(function () {
        }, function () {
        });
    }

    thiz.sendCmdWithParam = function(actCmd, param, timeout) {
        return thiz.sendCmd(actCmd + ' ' + param, false, timeout);
    };

    /**
     * tests the connection to ESP32. Sends the attention command "AT" and expects reply "OK"
     * @param onlyIfEmptyQueue only sends the test if currently no other tasks are running
     * @return {Promise}
     */
    thiz.testConnection = function (onlyIfEmptyQueue) {
        return new Promise(function(resolve) {
            thiz.sendCmd(C.CMD_ATTENTION, onlyIfEmptyQueue).then(function (response) {
                resolve(response && response.indexOf(_CMD_OK_RESPONSE) > -1 ? true : false);
            }, function (response) {
                resolve(response && response.indexOf(_CMD_BUSY_RESPONSE) > -1 ? true : false);
            });
        });
    };

    thiz.setGuiValue = function (valueConstant, value) {
        i=C.findConfigIndexByCmd(valueConstant);
        if (i>-1) {
            item=C.CONFIG_MAPPING[i];
            console.log("Set gui item "+ item.guiId +":" + value);
            if (item.guiId.indexOf('_SLIDER')>-1) {
                L('#' + item.guiId + '_VAL').innerHTML = value;
                L('#' + item.guiId).value = value;
            } else {
                L('#' + item.guiId).value = value;
            }
            C.CONFIG_MAPPING[i].value=value;
        }  else console.log ("Config index for command "+valueConstant+" not found.");
    };

    thiz.setConfigValue = function (valueConstant, value, sendCmd) {
        i=C.findConfigIndexByGuiId(valueConstant);
        if (i>-1) {
            console.log ("Set config value for "+valueConstant+":"+value);
            C.CONFIG_MAPPING[i].value=value;
            if (sendCmd == true) {
                thiz.sendCmdWithParam(C.CONFIG_MAPPING[i].cmd, value).then(function (response) {
                    // handle received reply (if any):
                    if (response.length > 1) {
                        response=response.trim();
                        console.log ("send command returned: "+response);
                        lastIndex = response.lastIndexOf(" ");
                        actCmd = response.substring(0, lastIndex); // extract command
                        par = response.substring(lastIndex+1);  // extract parameter value
                        thiz.setGuiValue(actCmd,par);
                    }
                }, function () {
                });
            }
        } else console.log ("Config index for GUI element "+valueConstant+" not found.");
    };

    thiz.refreshConfig = function () {
        return new Promise(function(resolve, reject) {
            thiz.sendCmd(C.CMD_LOAD_CONFIG).then(function (response) {
                // handle received configuration data here:
                console.log ("load configuration returned data: "+response);
                var cmdArray = response.split('\n');
                var i;
                for (i=0;i<cmdArray.length-1;i++) {
                    str = cmdArray[i];
                    lastIndex = str.lastIndexOf(" ");
                    actCmd = str.substring(0, lastIndex); // extract command
                    par = str.substring(lastIndex+1);  // extract parameter value
                    thiz.setGuiValue(actCmd,par);
                }
                resolve();
            }, function () {
                console.log("could not get config, using default values!");
                C.CONFIG_MAPPING.forEach( item => thiz.setGuiValue(item.cmd, item.value));
                reject();
            });
        });
    };

    /**
     * saves the configuration to the ESP by sending the data values via the Websocket connection.
     * @return {Promise}
     */
    thiz.save = function (updateProgressHandler) {
        updateProgressHandler = updateProgressHandler || function () {};
        var progress = 0;
        thiz.stopLiveValueListener();
        increaseProgress(30);
        var saveSlotsPromise = new Promise(function (resolve) {
            thiz.testConnection().then(function () {

                C.CONFIG_MAPPING.forEach(function(item) {
                    console.log("Saving "+ item.guiId +":" + item.value);
                    thiz.sendCmdNoResultHandling(item.cmd + ' '+ item.value);
                });
        
                increaseProgress(30);
                thiz.sendCmdNoResultHandling(C.CMD_SAVE_CONFIG);
                resolve();
            });
        });
    

        function increaseProgress(percent) {
            progress += percent;
            updateProgressHandler(progress);
        }

        return new Promise(function (resolve) {
            saveSlotsPromise.then(function () {
                thiz.testConnection().then(function () {
                    increaseProgress(40);
                    thiz.startLiveValueListener();
                    resolve();
                });
            });
        });
    };

    thiz.startLiveValueListener = function (handler) {
        console.log('starting listening to live values...');
        setLiveValueHandler(handler);
    };

    thiz.stopLiveValueListener = function () {
        setLiveValueHandler(null);
        console.log('listening to live values stopped.');
    };

    init();

    function init() {

        console.log ("init esp32 config and communication.");
        //activate Websocket or communication mockup 
        var promise = new Promise(function(resolve) {
            if(window.location.href.indexOf('mock') > -1) {
                _communicator = new MockCommunicator();
                resolve();
                return;
            }

            ws.initWebsocket(C.ESP32_WEBSOCKET_URL).then(function (socket) {
                _communicator = new WsCommunicator(C.ESP32_WEBSOCKET_URL, socket);
                resolve();
            }, function error () {
                    console.warn("could not establish any websocket connection - using mock mode!");
                    _communicator = new MockCommunicator();
                    resolve();
                });
        });

        promise.then(function () {
            thiz.refreshConfig().then(function () {
                if (L.isFunction(initFinished)) {
                    initFinished();
                }
            }, function () {
                if (L.isFunction(initFinished)) {
                    initFinished();
                }
            });
        });
    }

    function setLiveValueHandler(handler) {
        _valueHandler = handler;
        if (L.isFunction(_valueHandler)) {
            _communicator.setValueHandler(parseLiveValues);
        }
    }

    function parseLiveValues(data) {
        if (!L.isFunction(_valueHandler)) {
            _communicator.setValueHandler(null);
            return;
        }
        if (!data || data.indexOf(C.LIVE_VALUE_CONSTANT) == -1) { 
            console.log('error parsing live data: ' + data);
            return;
        }
        _valueHandler(data);   
    }    
}
