const fs = require('fs');
const path = require('path');
const axios = require('axios');

class GameInstaller {
    /**
     * Downloads Fabric metadata and constructs a strictly named version.json
     * @param {string} appRoot Absolute path to launcher root
     * @param {string} gameId ID of the game
     * @param {string} vanillaVersion i.e "1.20.1"
     * @param {string} fabricVersion i.e "0.15.0"
     * @param {Object} mainWindow Ref to electron window
     * @param {Function} sendProgress mapping to LauncherService.sendProgress 
     */
    async installFabricProfile(appRoot, gameId, vanillaVersion, fabricVersion, mainWindow, sendProgress) {
        const gameDir = path.join(appRoot, 'games', gameId);

        // Let's use standard naming for the custom profile: <gameId>_<vanillaVersion>
        const customProfileName = `${gameId}_${vanillaVersion}`;
        const versionsDir = path.join(gameDir, 'versions', customProfileName);
        const jsonPath = path.join(versionsDir, `${customProfileName}.json`);

        sendProgress(mainWindow, {
            status: 'progress',
            message: 'Configuring custom Fabric profile...',
            progress: 55
        });

        // Ensure directories exist
        fs.mkdirSync(versionsDir, { recursive: true });

        if (fs.existsSync(jsonPath)) {
            sendProgress(mainWindow, {
                status: 'progress',
                message: 'Fabric profile already configured.',
                progress: 60
            });
            return customProfileName;
        }

        try {
            // Fetch Fabric meta profile format for the target loader and vanilla version
            const fabricMetaUrl = `https://meta.fabricmc.net/v2/versions/loader/${vanillaVersion}/${fabricVersion}/profile/json`;
            const response = await axios.get(fabricMetaUrl);
            const fabricJson = response.data;

            // VERY IMPORTANT: Mutate the "id" to strictly match our custom folder name.
            // If we don't do this, minecraft-launcher-core will literally crash 
            // because it expects the JSON ID to match the container folder.
            fabricJson.id = customProfileName;

            // Optionally set the display name shown in logs/options
            fabricJson.name = gameId.replace(/-/g, ' ').toUpperCase();

            // Write modified JSON to disk
            fs.writeFileSync(jsonPath, JSON.stringify(fabricJson, null, 2));

            sendProgress(mainWindow, {
                status: 'progress',
                message: 'Fabric profile generated.',
                progress: 60
            });

            return customProfileName;
        } catch (error) {
            throw new Error(`Failed to generate Fabric profile: ${error.message}`);
        }
    }

    /**
     * Create the .ldl_installed marker block indicating the core process
     * has completed flawlessly.
     */
    markInstalled(appRoot, gameId, version, modLoader, buildTag = null) {
        const gameDir = path.join(appRoot, 'games', gameId);
        if (!fs.existsSync(gameDir)) fs.mkdirSync(gameDir, { recursive: true });

        fs.writeFileSync(
            path.join(gameDir, '.ldl_installed'),
            JSON.stringify({
                version,
                modLoader,
                buildTag,
                installedAt: new Date().toISOString()
            })
        );
    }

    /**
     * Read the current installation metadata for a game.
     * Returns null if not installed.
     */
    readInstalled(appRoot, gameId) {
        const markerPath = path.join(appRoot, 'games', gameId, '.ldl_installed');
        try {
            if (fs.existsSync(markerPath)) {
                return JSON.parse(fs.readFileSync(markerPath, 'utf-8'));
            }
        } catch (e) {
            console.warn('[GameInstaller] Could not read .ldl_installed:', e.message);
        }
        return null;
    }
}

module.exports = new GameInstaller();
