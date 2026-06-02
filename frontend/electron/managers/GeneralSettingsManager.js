const fs = require('fs');
const path = require('path');

class GeneralSettingsManager {
    constructor(appRoot) {
        this.settingsFile = path.join(appRoot, 'launcher_settings.json');
        this.defaultSettings = {
            showInstallConsole: true,
            autostart: false,
            notifications: true,
            hwAcceleration: true
        };
    }

    getSettings() {
        try {
            if (fs.existsSync(this.settingsFile)) {
                const data = JSON.parse(fs.readFileSync(this.settingsFile, 'utf-8'));
                return { ...this.defaultSettings, ...data };
            }
        } catch (e) {
            console.error('[GeneralSettingsManager] Error reading settings:', e);
        }
        return this.defaultSettings;
    }

    saveSettings(settings) {
        try {
            const current = this.getSettings();
            const merged = { ...current, ...settings };
            fs.writeFileSync(this.settingsFile, JSON.stringify(merged, null, 2), 'utf-8');
            return true;
        } catch (e) {
            console.error('[GeneralSettingsManager] Error saving settings:', e);
            return false;
        }
    }
}

module.exports = GeneralSettingsManager;
