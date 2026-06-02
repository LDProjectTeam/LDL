const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('terminalAPI', {
    onLog: (callback) => {
        ipcRenderer.on('terminal-log', (_event, data) => callback(data));
    },
    close: () => {
        ipcRenderer.send('terminal-close');
    }
});
