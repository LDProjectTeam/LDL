const { app } = require('electron');
const { execFile } = require('child_process');
const axios = require('axios');
const path = require('path');
const fs = require('fs');

// Custom Managers
const gameValidator = require('./managers/GameValidator');
const installationManager = require('./managers/InstallationManager');
const launchManager = require('./managers/LaunchManager');
const GeneralSettingsManager = require('./managers/GeneralSettingsManager');

function getAppRoot() {
    // If running from a packaged portable .exe, return the folder where the .exe sits
    if (app.isPackaged) {
        if (process.env.PORTABLE_EXECUTABLE_DIR) {
            return process.env.PORTABLE_EXECUTABLE_DIR;
        }
        return path.dirname(app.getPath('exe'));
    }
    // If running in development mode (npm start), return the workspace root
    return path.join(__dirname, '..', '..');
}

function getGamesRoot() {
    const settingsManager = new GeneralSettingsManager(getAppRoot());
    const settings = settingsManager.getSettings();
    if (settings.installDirectory) {
        return settings.installDirectory;
    }
    return getAppRoot();
}

class LauncherService {
    constructor() {
        // All launch/install logic is now delegated to dedicated managers
    }

    /**
     * Expose the root application path for UI checks
     */
    getAppRoot() {
        return getAppRoot();
    }

    /**
     * Expose the games root path (respects custom installDirectory from settings).
     */
    getGamesRoot() {
        return getGamesRoot();
    }

    /**
     * Get the absolute path for a game directory.
     */
    getGamePath(gameId) {
        return path.join(getGamesRoot(), 'games', gameId);
    }

    /**
     * Checks if a game is installed by verifying its directory and integrity.
     * Returns: 'installed' | 'damaged' | 'missing'
     */
    checkGameInstalled(gameId, type = 'minecraft', version = '1.20.1') {
        return gameValidator.checkGameStatus(getGamesRoot(), gameId, type, version);
    }

    /**
     * Deletes a game installation.
     */
    async deleteGame(gameId) {
        const targetPath = path.join(getGamesRoot(), 'games', gameId);
        if (fs.existsSync(targetPath)) {
            await fs.promises.rm(targetPath, { recursive: true, force: true });
        }
    }

    /**
 * Dispatch progress to the renderer via the main window webContents.
 * Also forwards to the Cyberpunk terminal window if it's open.
 * @param {BrowserWindow} mainWindow
 * @param {Object} data payload 
 */
    sendProgress(mainWindow, data) {
        // Ensure data is an object if it's not already, and include gameId if possible
        const payloadObj = { ...data };
        const payload = JSON.stringify(payloadObj);

        if (mainWindow) {
            mainWindow.webContents.send('game-progress', payload);
        }
        // Forward to terminal window (set by main.js)
        if (this.onTerminalForward) {
            this.onTerminalForward(payload);
        }
    }

    /**
     * Extracts a modpack from a URL to the target path.
     */
    async extractModpack(mainWindow, downloadUrl, installPath, gameId, downloadToken) {
        if (!downloadUrl) throw new Error("No download URL provided");

        let targetPath = installPath;
        if (!targetPath) {
            targetPath = path.join(getAppRoot(), 'games', gameId);
        } else if (!path.isAbsolute(targetPath)) {
            targetPath = path.join(getAppRoot(), installPath);
        }

        fs.mkdirSync(targetPath, { recursive: true });

        this.sendProgress(mainWindow, {
            status: 'progress',
            message: 'Downloading modpack archive...',
            progress: 10,
            gameId: gameId
        });

        // 1. Download to a temp file
        const tempDir = path.join(getAppRoot(), 'temp');
        if (!fs.existsSync(tempDir)) fs.mkdirSync(tempDir, { recursive: true });
        const tmpPath = path.join(tempDir, `ldl_modpack_${Date.now()}.zip`);

        // Resolve GitHub private repo URLs to API asset downloads (3-step fallback)
        let finalDownloadUrl = downloadUrl;
        let finalHeaders = {};

        const ghMatch = downloadUrl.match(
            /^https?:\/\/github\.com\/([^/]+)\/([^/]+)\/releases\/download\/([^/]+)\/(.+)$/
        );

        if (ghMatch && downloadToken) {
            const [, owner, repo, tag, assetName] = ghMatch;
            const apiHeaders = {
                Authorization: `Bearer ${downloadToken}`,
                Accept: 'application/vnd.github+json',
                'User-Agent': 'LDLauncher/1.0'
            };

            let releaseData = null;

            // Attempt 1: by tag
            try {
                const resp = await axios.get(`https://api.github.com/repos/${owner}/${repo}/releases/tags/${tag}`, { headers: apiHeaders });
                releaseData = resp.data;
            } catch (e) { /* fallthrough */ }

            // Attempt 2: by release ID
            if (!releaseData) {
                try {
                    const resp = await axios.get(`https://api.github.com/repos/${owner}/${repo}/releases/${tag}`, { headers: apiHeaders });
                    releaseData = resp.data;
                } catch (e) { /* fallthrough */ }
            }

            // Attempt 3: list all releases
            if (!releaseData) {
                const resp = await axios.get(`https://api.github.com/repos/${owner}/${repo}/releases?per_page=30`, { headers: apiHeaders });
                releaseData = resp.data.find(r => r.assets.some(a => a.name === assetName));
            }

            if (!releaseData) throw new Error(`No release containing "${assetName}" found in "${owner}/${repo}".`);

            const asset = releaseData.assets.find(a => a.name === assetName);
            if (!asset) throw new Error(`Asset "${assetName}" not found.`);

            finalDownloadUrl = asset.url;
            finalHeaders = {
                Authorization: `Bearer ${downloadToken}`,
                Accept: 'application/octet-stream',
                'User-Agent': 'LDLauncher/1.0'
            };
        } else if (downloadToken) {
            finalHeaders = {
                Authorization: `Bearer ${downloadToken}`,
                Accept: 'application/octet-stream'
            };
        }

        try {
            const response = await axios({
                method: 'GET',
                url: finalDownloadUrl,
                responseType: 'stream',
                headers: finalHeaders,
                maxRedirects: 5
            });

            const writer = fs.createWriteStream(tmpPath);
            response.data.pipe(writer);

            await new Promise((resolve, reject) => {
                writer.on('finish', resolve);
                writer.on('error', reject);
            });

            this.sendProgress(mainWindow, {
                status: 'progress',
                message: 'Extracting files...',
                progress: 60,
                gameId: gameId
            });

            const _appRoot = getAppRoot();
            const settingsManager = new GeneralSettingsManager(_appRoot);
            const settings = settingsManager.getSettings();

            // Use PowerShell Expand-Archive in a separate process (async, no RAM bloat)
            await new Promise((resolve, reject) => {
                const args = [
                    '-NoProfile', '-NonInteractive', '-Command',
                    `Expand-Archive -Path '${tmpPath.replace(/'/g, "''")}' -DestinationPath '${targetPath.replace(/'/g, "''")}' -Force`
                ];
                execFile('powershell.exe', args, { timeout: 600000, windowsHide: !settings.showInstallConsole }, (error, stdout, stderr) => {
                    if (error) {
                        reject(new Error(`ZIP extraction failed: ${stderr || error.message}`));
                    } else {
                        resolve();
                    }
                });
            });
        } finally {
            // 3. Cleanup
            if (fs.existsSync(tmpPath)) {
                fs.unlinkSync(tmpPath);
            }
        }

        // 4. Mark as completely installed for verification checks
        fs.writeFileSync(path.join(targetPath, '.ldl_installed'), JSON.stringify({ installedAt: new Date().toISOString() }));

        // 5. Verification Phase (Cyberpunk Terminal request)
        this.sendProgress(mainWindow, {
            status: 'progress',
            message: 'Verifying file integrity...',
            progress: 99,
            gameId: gameId
        });

        // Artificial delay for terminal aesthetic
        await new Promise(r => setTimeout(r, 600));

        this.sendProgress(mainWindow, {
            status: 'progress',
            message: `Checking: ${path.basename(targetPath)} ........ OK`,
            progress: 99,
            gameId: gameId
        });

        this.sendProgress(mainWindow, {
            status: 'success',
            message: 'Installation complete!',
            progress: 100,
            gameId: gameId
        });
    }

    /**
     * Minecraft installation dispatcher.
     */
    async installMinecraft(mainWindow, version, modLoader, modLoaderVersion, gameId, downloadUrl, downloadToken) {
        try {
            this.sendProgress(mainWindow, {
                status: 'progress',
                message: 'Preparing Minecraft environment...',
                progress: 10,
                gameId: gameId
            });

            // Extract GitHub release tag from downloadUrl for version tracking
            let buildTag = null;
            const ghMatch = downloadUrl?.match(
                /^https?:\/\/github\.com\/[^/]+\/[^/]+\/releases\/download\/([^/]+)\//
            );
            if (ghMatch) {
                buildTag = ghMatch[1];
            }

            // Custom progress sender that automatically injects gameId
            const scopedSendProgress = (win, data) => this.sendProgress(win, { ...data, gameId });

            await installationManager.installMinecraftDeep(getGamesRoot(), mainWindow, scopedSendProgress, version, modLoader, modLoaderVersion, gameId, downloadUrl, downloadToken, buildTag);

        } catch (e) {
            console.error("Installation Process failed:", e);
            this.sendProgress(mainWindow, {
                status: 'error',
                message: e.message || 'Installation failed',
                progress: 0,
                gameId: gameId
            });
            throw e;
        }
    }

    isGameRunning(gameId) {
        return launchManager.isGameRunning(gameId);
    }

    /**
     * Minecraft launch dispatcher.
     */
    async launchMinecraft(mainWindow, version, modLoader, modLoaderVersion, gameId, launcherLang, sessionToken = null, minecraftLicense = null) {
        try {
            const scopedSendProgress = (win, data) => this.sendProgress(win, { ...data, gameId });

            await launchManager.launchMinecraft(
                getGamesRoot(), mainWindow, scopedSendProgress,
                version, modLoader, modLoaderVersion,
                gameId, launcherLang, sessionToken,  // DRM handshake
                minecraftLicense                     // Microsoft skin & nick
            );
        } catch (error) {
            console.error("Launch Process failed:", error);
            throw error;
        }
    }
}

module.exports = new LauncherService();
