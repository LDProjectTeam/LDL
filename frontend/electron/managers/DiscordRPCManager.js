const DiscordRPC = require('discord-rpc');

class DiscordRPCManager {
    constructor() {
        // You MUST replace this Client ID with your own from the Discord Developer Portal
        this.clientId = '1342616231221153835'; // A temporary valid placeholder to prevent instant crash
        this.rpc = new DiscordRPC.Client({ transport: 'ipc' });
        this.isConnected = false;
        this.startTimestamp = new Date();

        this.rpc.on('ready', () => {
            this.isConnected = true;
            this.setLauncherPresence();
            console.log('[DiscordRPC] Connected as', this.rpc.user.username);
        });

        // Suppress errors if Discord is not running or ID is totally invalid
        this.rpc.login({ clientId: this.clientId }).catch(err => {
            console.log('[DiscordRPC] Could not connect to Discord:', err.message);
        });
    }

    setLauncherPresence() {
        if (!this.isConnected) return;
        this.rpc.setActivity({
            details: 'В главном меню',
            state: 'Лаунчер',
            startTimestamp: this.startTimestamp,
            largeImageKey: 'icon',
            largeImageText: 'LDLauncher',
            instance: false,
        }).catch(err => console.error('[DiscordRPC] Error setting presence:', err.message));
    }

    setGamePresence(gameName) {
        if (!this.isConnected) return;
        this.rpc.setActivity({
            details: `Играет в ${gameName}`,
            state: 'В игре',
            startTimestamp: new Date(),
            largeImageKey: 'icon',
            largeImageText: 'LDLauncher',
            instance: false,
        }).catch(err => console.error('[DiscordRPC] Error setting presence:', err.message));
    }
}

module.exports = new DiscordRPCManager();
