window.C = {};

C.IS_ELECTRON = navigator.userAgent.toLowerCase().indexOf(' electron/') > -1;

C.ESP32_WEBSOCKET_URL = 'ws://' + window.location.hostname + ':80/ws';
C.RESET_DRAWPOSITION_CONSTANT = '*';
C.LIVE_VALUE_CONSTANT = 'VALUES:';

/**
 * Command names for callback functions in the backend
*/ 
C.CMD_ATTENTION='AT';
C.CMD_LOAD_CONFIG='load';
C.CMD_SAVE_CONFIG='save';
C.CMD_START_DATA='start';
C.CMD_STOP_DATA='stop';

/**
 * Array for mapping configuration parameters to their GUI element, command and value
 * see also: config.c for according backend implementation !
 * note that the WebGui has own default values because it can work "backless" in mockup mode!
*/ 
C.CONFIG_MAPPING = [];
C.CONFIG_MAPPING.push({ guiId: 'RED',               cmd: 'red',             value: '100' });
C.CONFIG_MAPPING.push({ guiId: 'GREEN',             cmd: 'green',           value: '200' });
C.CONFIG_MAPPING.push({ guiId: 'BLUE',              cmd: 'blue',            value: '50' });
C.CONFIG_MAPPING.push({ guiId: 'COLORSENSING',      cmd: 'colorsensing',    value: '0' });
C.CONFIG_MAPPING.push({ guiId: 'AREA1ONOF',         cmd: 'area1onof',       value: '0' });
C.CONFIG_MAPPING.push({ guiId: 'AREA2ONOF',         cmd: 'area2onof',       value: '0' });
C.CONFIG_MAPPING.push({ guiId: 'AREA3ONOF',         cmd: 'area3onof',       value: '0' });
C.CONFIG_MAPPING.push({ guiId: 'AREA4ONOF',         cmd: 'area4onof',       value: '0' });
C.CONFIG_MAPPING.push({ guiId: 'ONOFBOX',           cmd: 'onof',            value: '0' }); 
C.CONFIG_MAPPING.push({ guiId: 'SIGNAL_MODE_SELECT',cmd: 'signal_mode',     value: '1' });
C.CONFIG_MAPPING.push({ guiId: 'Brightness_SLIDER', cmd: 'brightness',      value: '30' });
C.CONFIG_MAPPING.push({ guiId: 'GAIN_SLIDER',       cmd: 'gain',            value: '30' })
C.CONFIG_MAPPING.push({ guiId: 'FREQUENCY_SLIDER',  cmd: 'frequency',       value: '20' });
C.CONFIG_MAPPING.push({ guiId: 'WIFI_MODE_SELECT',  cmd: 'wifi_mode',       value: '1' });
C.CONFIG_MAPPING.push({ guiId: 'MQTT_BROKER_TEXT',  cmd: 'mqtt_broker',     value: 'testBroker' });
C.CONFIG_MAPPING.push({ guiId: 'MQTT_DELIMITER_TEXT',cmd: 'mqtt_delimiter', value: ';' });
C.CONFIG_MAPPING.push({ guiId: 'WIFI_NAME_TEXT',    cmd: 'wifi_name',       value: 'myWifi' });
C.CONFIG_MAPPING.push({ guiId: 'WIFI_PASSWORD_TEXT',cmd: 'wifi_password',   value: 'myPassword' });


C.findConfigIndexByGuiId = function (guiId) {
    for (i=0;i<C.CONFIG_MAPPING.length;i++) 
        if (C.CONFIG_MAPPING[i].guiId == guiId) 
            return(i);
    return(-1);
}
C.findConfigIndexByCmd = function (cmd) {
    for (i=0;i<C.CONFIG_MAPPING.length;i++) 
        if (C.CONFIG_MAPPING[i].cmd == cmd) 
            return(i);
    return(-1);
}
C.getConfigValueByGuiId = function (guiId) {
    for (i=0;i<C.CONFIG_MAPPING.length;i++) 
        if (C.CONFIG_MAPPING[i].guiId == guiId) 
            return(C.CONFIG_MAPPING[i].value);
    return(-1);
}
