const fs = require('fs');
const path = require('path');
const javaManager = require('./JavaManager');

class GameValidator {
    /**
     * Checks if a game is installed by verifying its directory and integrity.
     * Returns: 'installed' | 'damaged' | 'missing'
     * @param {string} appRoot 
     * @param {string} gameId 
     * @param {string} type 
     * @param {string} version 
     */
    checkGameStatus(appRoot, gameId, type, version) {
        try {
            const gameDir = path.join(appRoot, 'games', gameId);

            if (!fs.existsSync(gameDir)) return 'missing';

            // Check if directory has any contents at all
            const files = fs.readdirSync(gameDir);
            if (files.length === 0) return 'missing';

            // Base integrity check: the marker file must exist
            const isIntact = this.verifyBaseIntegrity(gameDir);
            if (!isIntact) return 'damaged';

            // For custom Fabric setups, verify the custom profile folder AND Java 17 exist
            // Read modLoader from installation metadata to check generically
            if (type === 'minecraft') {
                let installedModLoader = null;
                try {
                    const markerPath = path.join(gameDir, '.ldl_installed');
                    if (fs.existsSync(markerPath)) {
                        const meta = JSON.parse(fs.readFileSync(markerPath, 'utf-8'));
                        installedModLoader = meta.modLoader;
                    }
                } catch (e) { /* ignore parse errors */ }

                if (installedModLoader === 'fabric') {
                    const customProfileName = `${gameId}_${version || '1.20.1'}`;
                    const profileDir = path.join(gameDir, 'versions', customProfileName);
                    const profileJson = path.join(profileDir, `${customProfileName}.json`);

                    if (!fs.existsSync(profileDir) || !fs.existsSync(profileJson)) {
                        console.log(`Validation failed: Missing custom Fabric profile ${profileJson}`);
                        return 'damaged';
                    }
                }

                if (!javaManager.verifyJavaIntegrity(appRoot, gameId)) {
                    console.log(`Validation failed: Missing standalone Java 17 for ${gameId}`);
                    return 'damaged';
                }
            }

            // Universal deep check for a populated minecraft instance
            if (type === 'minecraft') {
                const librariesDir = path.join(gameDir, 'libraries');
                const baseVersionDir = path.join(gameDir, 'versions', version || '1.20.1');
                const baseVersionJar = path.join(baseVersionDir, `${version || '1.20.1'}.jar`);

                if (!fs.existsSync(librariesDir)) {
                    console.log(`Validation failed: Missing libraries directory for ${gameId}`);
                    return 'damaged';
                }

                if (!fs.existsSync(baseVersionJar)) {
                    console.log(`Validation failed: Missing base version jar ${baseVersionJar}`);
                    return 'damaged';
                }
            }

            return 'installed';
        } catch (e) {
            console.error("Error checking game installation:", e);
            return 'missing';
        }
    }

    /**
     * Basic check for `.ldl_installed`
     */
    verifyBaseIntegrity(gameDir) {
        if (!fs.existsSync(gameDir)) return false;
        const markerPath = path.join(gameDir, '.ldl_installed');
        return fs.existsSync(markerPath);
    }
}

module.exports = new GameValidator();
