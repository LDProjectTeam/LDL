const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    minimizeWindow: () => ipcRenderer.send('window-minimize'),
    closeWindow: () => ipcRenderer.send('window-close'),
    launchGame: (config) => ipcRenderer.invoke('launch-game', config),
    installGame: (config) => ipcRenderer.invoke('install-game', config),
    deleteGame: (gameId) => ipcRenderer.invoke('delete-game', gameId),
    killGame: (gameId) => ipcRenderer.invoke('kill-game', gameId),
    getGameStatus: (args) => ipcRenderer.invoke('get-game-status', args),
    onGameProgress: (callback) => {
        const handler = (_event, data) => callback(data);
        ipcRenderer.on('game-progress', handler);
        return () => ipcRenderer.removeListener('game-progress', handler);
    },
    startGoogleOAuth: (authUrl) => ipcRenderer.send('start-google-oauth', authUrl),
    startMagicLinkServer: () => ipcRenderer.send('start-magic-link-server'),
    startMicrosoftAuth: () => ipcRenderer.invoke('start-microsoft-auth'),
    onOAuthCallback: (callback) => {
        const handler = (_event, data) => callback(data);
        ipcRenderer.on('oauth-callback', handler);
        return () => ipcRenderer.removeListener('oauth-callback', handler);
    },
    openExternal: (url) => ipcRenderer.invoke('open-external', url),
    openPath: (path) => ipcRenderer.invoke('open-path', path),
    getGamePath: (gameId) => ipcRenderer.invoke('get-game-path', gameId),
    getAppRoot: () => ipcRenderer.invoke('get-app-root'),

    // ─── Auto-Optimization API ───
    getOptimizerSettings: () => ipcRenderer.invoke('get-optimizer-settings'),
    setOptimizerSettings: (settings) => ipcRenderer.invoke('set-optimizer-settings', settings),
    getGeneralSettings: () => ipcRenderer.invoke('get-general-settings'),
    setGeneralSettings: (settings) => ipcRenderer.invoke('set-general-settings', settings),
    getSystemSpecs: () => ipcRenderer.invoke('get-system-specs'),
    resetGameSettings: () => ipcRenderer.invoke('reset-game-settings'),

    // ─── Build Update API ───
    checkBuildUpdate: (config) => ipcRenderer.invoke('check-build-update', config),
    updateGame: (config) => ipcRenderer.invoke('update-game', config),

    // ─── Desktop Notifications ───
    showNotification: (options) => ipcRenderer.invoke('show-notification', options),

    // ─── Launcher Auto Updates ───
    checkLauncherUpdates: () => ipcRenderer.invoke('check-launcher-updates'),
    installLauncherUpdate: () => ipcRenderer.invoke('install-launcher-update'),
    onLauncherUpdateStatus: (callback) => {
        const handler = (_event, data) => callback(data);
        ipcRenderer.on('launcher-update-status', handler);
        return () => ipcRenderer.removeListener('launcher-update-status', handler);
    },

    // ─── Settings ───
    openDevTools: () => ipcRenderer.invoke('open-devtools'),
    closeDevTools: () => ipcRenderer.invoke('close-devtools'),
    selectDirectory: (defaultPath) => ipcRenderer.invoke('select-directory', defaultPath),
});

