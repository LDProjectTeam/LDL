const path = require('path');
const fs = require('fs');
const javaManager = require('./JavaManager');
const gameInstaller = require('./GameInstaller');
const Handler = require('minecraft-launcher-core/components/handler');
const EventEmitter = require('events').EventEmitter;
const axios = require('axios');
const { execFile, spawn } = require('child_process');
const GeneralSettingsManager = require('./GeneralSettingsManager');

function createProgressWrapper(originalSendProgress, minPct, maxPct) {
    return (mainWindow, data) => {
        if (data.status === 'progress') {
            const range = maxPct - minPct;
            let absoluteProgress = minPct;
            if (typeof data.progress === 'number') {
                absoluteProgress = minPct + (data.progress * range / 100);
            }
            originalSendProgress(mainWindow, { ...data, progress: Math.floor(absoluteProgress) });
        } else {
            originalSendProgress(mainWindow, data); // let errors and success pass through usually
        }
    };
}

class DummyClient extends EventEmitter {
    constructor(mainWindow, sendProgress) {
        super();
        this.mainWindow = mainWindow;
        this.originalSendProgress = sendProgress;
        this.sendProgress = sendProgress;

        this.on('debug', (msg) => {
            console.log(msg);
        });

        this.lastTime = Date.now();
        this.lastCurrent = 0;
        this.speedStr = '0.0 MB/s';

        this.on('download-status', (data) => {
            if (data.type === 'version-package') return;
            const pct = data.current / data.total;

            const now = Date.now();
            const timeDiff = (now - this.lastTime) / 1000;
            if (timeDiff >= 1) {
                let bytesDiff = data.current - this.lastCurrent;
                if (bytesDiff < 0) {
                    bytesDiff = data.current; // New file started, reset difference
                }
                const speedMB = (bytesDiff / (1024 * 1024)) / timeDiff;
                this.speedStr = `${speedMB.toFixed(1)} MB/s`;
                this.lastTime = now;
                this.lastCurrent = data.current;
            }

            // We intentionally omit 'progress' here because MCLC fires this per-file.
            // Emitting per-file progress causes the global bar to jump wildly between 0-100%.
            // The progress will instead advance smoothly via phase transitions.
            this.sendProgress(this.mainWindow, {
                status: 'progress',
                message: data.name || 'Preparing game files...',
                speed: this.speedStr,
                downloaded: data.current,
                total: data.total
            });
        });
    }

    setRange(minPct, maxPct) {
        // DummyClient's sendProgress is expected to be the top-level one or already wrapped.
        // It's cleaner to just update the wrapper instance it uses, but it's simpler
        // to pass a fresh wrapper during phase execution. We will re-assign this.sendProgress.
        this.sendProgress = createProgressWrapper(this.originalSendProgress, minPct, maxPct);
    }
}

class InstallationManager {
    /**
     * Installs Java, configures Fabric Profile, and pre-downloads ALL assets and libraries.
     */
    async installMinecraftDeep(appRoot, mainWindow, sendProgress, version, modLoader, modLoaderVersion, gameId, downloadUrl, downloadToken, buildTag = null) {
        // Auto-resolve latest GitHub release for fresh installs
        if (downloadUrl && downloadToken) {
            const ghMatch = downloadUrl.match(
                /^https?:\/\/github\.com\/([^/]+)\/([^/]+)\/releases\/download\/([^/]+)\/(.+)$/
            );
            if (ghMatch) {
                const [, owner, repo, , assetName] = ghMatch;
                try {
                    const apiHeaders = {
                        Authorization: `Bearer ${downloadToken}`,
                        Accept: 'application/vnd.github+json',
                        'User-Agent': 'LDLauncher/1.0'
                    };
                    const resp = await axios.get(
                        `https://api.github.com/repos/${owner}/${repo}/releases/latest`,
                        { headers: apiHeaders }
                    );
                    const latestTag = resp.data.tag_name;
                    const asset = resp.data.assets.find(a => a.name === assetName);
                    if (asset) {
                        // Reconstruct download URL with latest tag
                        downloadUrl = `https://github.com/${owner}/${repo}/releases/download/${latestTag}/${assetName}`;
                        buildTag = latestTag;
                        console.log(`[Install] Resolved latest release: tag=${latestTag}, asset=${assetName}`);
                    }
                } catch (e) {
                    console.warn('[Install] Could not resolve latest release, using provided URL:', e.message);
                }
            }
        }

        // Step 1 + 2 in PARALLEL: Install Java AND resolve GitHub release + setup Fabric
        const javaProgress = createProgressWrapper(sendProgress, 0, 15);
        const fabricProgress = createProgressWrapper(sendProgress, 15, 17);

        let customProfileName = version;

        await Promise.all([
            // Task A: Install Java (skip if already present)
            javaManager.installJava(appRoot, gameId, mainWindow, javaProgress).catch(e => {
                console.error("Java Installation failed:", e);
                throw e;
            }),

            // Task B: Setup Fabric profile (skip if JSON already exists)
            (async () => {
                if (modLoader === 'fabric' && modLoaderVersion) {
                    try {
                        customProfileName = await gameInstaller.installFabricProfile(
                            appRoot, gameId, version, modLoaderVersion, mainWindow, fabricProgress
                        );
                    } catch (e) {
                        console.error("Fabric Configuration failed:", e);
                        throw e;
                    }
                }
            })()
        ]);

        const gameDir = path.join(appRoot, 'games', gameId);
        fs.mkdirSync(gameDir, { recursive: true });

        // Step 3: Deep download of assets, libs, and jar via MCLC internal Handler
        // 
        // IMPORTANT: Fabric JSON has "inheritsFrom: 1.20.1" but NO downloads.client field.
        // We must first resolve vanilla 1.20.1 to get the jar and assets, then use the
        // Fabric JSON as modifyJson for library downloads. This mirrors MCLC's internal logic.
        try {
            const mclcProgressWrapper = createProgressWrapper(sendProgress, 17, 70);

            mclcProgressWrapper(mainWindow, {
                status: 'progress',
                message: 'Minecraft client',
                progress: 0
            });

            const dummyClient = new DummyClient(mainWindow, sendProgress);

            // Phase A: Resolve vanilla version (jar, assets, base libraries)
            const vanillaVersionDir = path.join(gameDir, 'versions', version);
            fs.mkdirSync(vanillaVersionDir, { recursive: true });

            const vanillaOptions = {
                root: gameDir,
                version: {
                    number: version,
                    type: 'release'
                },
                directory: vanillaVersionDir,
                overrides: {
                    maxSockets: 30,
                    detached: true,
                    url: {
                        meta: 'https://piston-meta.mojang.com',
                        resource: 'https://resources.download.minecraft.net',
                        mavenForge: 'https://files.minecraftforge.net/maven/',
                        defaultRepoForge: 'https://libraries.minecraft.net/',
                        fallbackMaven: 'https://search.maven.org/remotecontent?filepath=',
                    },
                    fw: {
                        baseUrl: 'https://github.com/ZekerZhayard/ForgeWrapper/releases/download/',
                        version: '1.6.0',
                        sh1: '035a51fe6439792a61507630d89382f621da0f1f',
                        size: 28679,
                    }
                }
            };

            dummyClient.options = vanillaOptions;
            const vanillaHandler = new Handler(dummyClient);

            // Phase A1: Download vanilla version manifest (17% - 20%)
            dummyClient.setRange(17, 20);
            await vanillaHandler.getVersion();

            // Phase A2: Download client jar (20% - 25%)
            dummyClient.setRange(20, 25);
            await vanillaHandler.getJar();

            // Phase A3: Download all game assets (25% - 45%)
            dummyClient.setRange(25, 45);
            await vanillaHandler.getAssets();

            // Phase A4: Download vanilla libraries (45% - 55%)
            dummyClient.setRange(45, 55);
            await vanillaHandler.getClasses(null);

            // Phase B: Download Fabric-specific libraries using the custom JSON (55% - 65%)
            if (customProfileName !== version) {
                dummyClient.setRange(55, 65);
                const fabricJsonPath = path.join(gameDir, 'versions', customProfileName, `${customProfileName}.json`);
                if (fs.existsSync(fabricJsonPath)) {
                    const fabricJson = JSON.parse(fs.readFileSync(fabricJsonPath, 'utf-8'));
                    // Use the vanilla handler (which has already resolved version info) to download Fabric libs
                    await vanillaHandler.getClasses(fabricJson);
                }
            }

            // Phase C: Download natives (65% - 70%)
            dummyClient.setRange(65, 70);
            await vanillaHandler.getNatives();

        } catch (e) {
            console.error("Minecraft asset download failed:", e);
            throw e;
        }

        // Step 3: If a Modpack download URL is provided, download and extract it OVER the installation (70% - 99%)
        const modpackDownloadProgress = createProgressWrapper(sendProgress, 70, 90);
        const modpackExtractProgress = createProgressWrapper(sendProgress, 90, 99);

        if (downloadUrl) {
            try {
                modpackDownloadProgress(mainWindow, {
                    status: 'progress',
                    message: 'Resolving modpack download...',
                    progress: 0
                });

                const tempDir = path.join(appRoot, 'temp');
                if (!fs.existsSync(tempDir)) fs.mkdirSync(tempDir, { recursive: true });
                const tmpPath = path.join(tempDir, `modpack_${gameId}_${Date.now()}.zip`);

                try {
                    // For GitHub private repos, the browser-style URL (github.com/.../releases/download/...)
                    // returns 404 even with a Bearer token. We must use the GitHub API instead.
                    let finalDownloadUrl = downloadUrl;
                    let finalHeaders = {};

                    const ghMatch = downloadUrl.match(
                        /^https?:\/\/github\.com\/([^/]+)\/([^/]+)\/releases\/download\/([^/]+)\/(.+)$/
                    );

                    if (ghMatch && downloadToken) {
                        const [, owner, repo, tag, assetName] = ghMatch;
                        console.log(`[Modpack] Resolving GitHub release: owner=${owner}, repo=${repo}, tag=${tag}, asset=${assetName}`);

                        sendProgress(mainWindow, {
                            status: 'progress',
                            message: 'Authenticating with GitHub...',
                            progress: 96
                        });

                        const apiHeaders = {
                            Authorization: `Bearer ${downloadToken}`,
                            Accept: 'application/vnd.github+json',
                            'User-Agent': 'LDLauncher/1.0'
                        };

                        let releaseData = null;

                        // Attempt 1: Try to find release by tag name
                        try {
                            const tagUrl = `https://api.github.com/repos/${owner}/${repo}/releases/tags/${tag}`;
                            console.log(`[Modpack] Attempt 1: GET ${tagUrl}`);
                            const resp = await axios.get(tagUrl, { headers: apiHeaders });
                            releaseData = resp.data;
                            console.log(`[Modpack] Found release by tag: "${releaseData.name || releaseData.tag_name}"`);
                        } catch (e1) {
                            console.log(`[Modpack] Tag lookup failed (${e1.response?.status}). Trying by release ID...`);
                        }

                        // Attempt 2: Try as release ID (the "4" in the URL might be a numeric release ID, not a tag)
                        if (!releaseData) {
                            try {
                                const idUrl = `https://api.github.com/repos/${owner}/${repo}/releases/${tag}`;
                                console.log(`[Modpack] Attempt 2: GET ${idUrl}`);
                                const resp = await axios.get(idUrl, { headers: apiHeaders });
                                releaseData = resp.data;
                                console.log(`[Modpack] Found release by ID: "${releaseData.name || releaseData.tag_name}"`);
                            } catch (e2) {
                                console.log(`[Modpack] Release ID lookup failed (${e2.response?.status}). Listing all releases...`);
                            }
                        }

                        // Attempt 3: List all releases and find the one that contains our asset
                        if (!releaseData) {
                            try {
                                const listUrl = `https://api.github.com/repos/${owner}/${repo}/releases?per_page=30`;
                                console.log(`[Modpack] Attempt 3: GET ${listUrl}`);
                                const resp = await axios.get(listUrl, { headers: apiHeaders });
                                const releases = resp.data;

                                if (!Array.isArray(releases)) {
                                    throw new Error(
                                        `GitHub API returned unexpected response (not an array). ` +
                                        `This may indicate a rate-limit or auth error.`
                                    );
                                }

                                // Find the release that contains our target asset file
                                releaseData = releases.find(r =>
                                    r.assets.some(a => a.name === assetName)
                                );

                                if (releaseData) {
                                    console.log(`[Modpack] Matched release "${releaseData.name}" (tag: ${releaseData.tag_name}) containing asset "${assetName}"`);
                                }
                            } catch (e3) {
                                console.error(`[Modpack] Failed to list releases:`, e3.response?.status, e3.response?.data?.message || e3.message);
                                throw new Error(
                                    `Cannot access GitHub repo "${owner}/${repo}". ` +
                                    `Status: ${e3.response?.status || 'unknown'}. ` +
                                    `Make sure your PAT token has "Contents: Read" permission for this repository.`
                                );
                            }
                        }

                        if (!releaseData) {
                            throw new Error(
                                `No release found containing asset "${assetName}" in repo "${owner}/${repo}". ` +
                                `Check: 1) the file exists in a release, 2) the PAT has access to this repo.`
                            );
                        }

                        // Find the exact asset in the resolved release
                        const asset = releaseData.assets.find(a => a.name === assetName);
                        if (!asset) {
                            throw new Error(`Asset "${assetName}" not found. Available: ${releaseData.assets.map(a => a.name).join(', ')}`);
                        }

                        console.log(`[Modpack] Resolved asset ID: ${asset.id}, size: ${(asset.size / 1024 / 1024).toFixed(1)} MB`);

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

                    modpackDownloadProgress(mainWindow, {
                        status: 'progress',
                        message: 'Downloading modpack archive...',
                        progress: 0
                    });

                    const response = await axios({
                        method: 'GET',
                        url: finalDownloadUrl,
                        responseType: 'stream',
                        headers: finalHeaders,
                        maxRedirects: 5 // GitHub API redirects to a signed S3 URL
                    });

                    const writer = fs.createWriteStream(tmpPath);
                    response.data.pipe(writer);

                    // Track download progress for modpack
                    const totalLength = parseInt(response.headers['content-length'], 10) || 1;
                    let downloaded = 0;
                    let lastTime = Date.now();
                    let lastDownloaded = 0;
                    let speedStr = '0.0 MB/s';
                    
                    response.data.on('data', (chunk) => {
                        downloaded += chunk.length;
                        const now = Date.now();
                        const timeDiff = (now - lastTime) / 1000;
                        if (timeDiff >= 1) {
                            const speedMB = ((downloaded - lastDownloaded) / (1024 * 1024)) / timeDiff;
                            speedStr = `${speedMB.toFixed(1)} MB/s`;
                            lastTime = now;
                            lastDownloaded = downloaded;
                        }
                        
                        const pct = Math.floor((downloaded / totalLength) * 100);
                        modpackDownloadProgress(mainWindow, {
                            status: 'progress',
                            message: `minecraft.zip`,
                            progress: pct,
                            speed: speedStr,
                            downloaded: downloaded,
                            total: totalLength
                        });
                    });

                    await new Promise((resolve, reject) => {
                        writer.on('finish', resolve);
                        writer.on('error', reject);
                    });

                    // Send exactly 100 on complete (90% absolute) 
                    modpackDownloadProgress(mainWindow, {
                        status: 'progress',
                        message: `minecraft.zip`,
                        progress: 100
                    });

                    // To prevent ghost/duplicate files, wipe existing mods and config folders before extracting
                    const modsDir = path.join(gameDir, 'mods');
                    const configDir = path.join(gameDir, 'config');
                    if (fs.existsSync(modsDir)) fs.rmSync(modsDir, { recursive: true, force: true });
                    if (fs.existsSync(configDir)) fs.rmSync(configDir, { recursive: true, force: true });

                    // Refined extraction with per-file logging via tar.exe
                    await new Promise((resolve, reject) => {
                        const settingsManager = new GeneralSettingsManager(appRoot);
                        const settings = settingsManager.getSettings();

                        const args = ['-xf', tmpPath, '-C', gameDir, '-v'];
                        console.log(`[Modpack] Extracting via tar: ${tmpPath} -> ${gameDir}`);
                        const ps = spawn('tar.exe', args, { windowsHide: !settings.showInstallConsole });

                        ps.stdout.on('data', (data) => {
                            const lines = data.toString().split(/\r?\n/);
                            for (const line of lines) {
                                const trimmedLine = line.trim();
                                if (trimmedLine) {
                                    // Keep constant progress and fixed message to reduce noise
                                    modpackExtractProgress(mainWindow, {
                                        status: 'progress',
                                        message: 'Unpacking...',
                                        progress: 50
                                    });
                                }
                            }
                        });

                        ps.stderr.on('data', (data) => {
                            const lines = data.toString().split(/\r?\n/);
                            for (const line of lines) {
                                const trimmedLine = line.trim();
                                if (trimmedLine && !trimmedLine.toLowerCase().includes('error')) {
                                    modpackExtractProgress(mainWindow, {
                                        status: 'progress',
                                        message: 'Unpacking...',
                                        progress: 50
                                    });
                                } else if (trimmedLine.toLowerCase().includes('error')) {
                                    console.error('[Modpack] tar stderr:', trimmedLine);
                                }
                            }
                        });

                        ps.on('close', (code) => {
                            if (code === 0) {
                                console.log('[Modpack] Extraction complete via tar');

                                // Flatten structure: if the archive extracted to a SINGLE subfolder,
                                // move its contents up (handles LD2/, minecraft/, .minecraft/, etc.)
                                // NOTE: only count DIRECTORIES — files like .ldl_installed must not interfere.
                                const SKIP_DIRS = ['versions', 'assets', 'libraries', 'runtime', 'temp'];
                                const extractedDirs = fs.readdirSync(gameDir).filter(item => {
                                    if (SKIP_DIRS.includes(item)) return false;
                                    try {
                                        return fs.statSync(path.join(gameDir, item)).isDirectory();
                                    } catch { return false; }
                                });
                                const singleFolder = extractedDirs.length === 1
                                    ? path.join(gameDir, extractedDirs[0])
                                    : null;
                                if (singleFolder) {
                                    console.log(`[Modpack] Flattening single root folder: ${extractedDirs[0]}`);
                                    const items = fs.readdirSync(singleFolder);
                                    for (const item of items) {
                                        const src = path.join(singleFolder, item);
                                        const dest = path.join(gameDir, item);
                                        try {
                                            if (fs.existsSync(dest)) {
                                                fs.rmSync(dest, { recursive: true, force: true, maxRetries: 5 });
                                            }
                                            fs.renameSync(src, dest);
                                        } catch (renameErr) {
                                            console.warn(`[Modpack] Rename failed for ${item}:`, renameErr.message);
                                            try {
                                                fs.cpSync(src, dest, { recursive: true, force: true, preserveTimestamps: true });
                                                fs.rmSync(src, { recursive: true, force: true, maxRetries: 5 });
                                            } catch (cpErr) {
                                                console.warn(`[Modpack] Could not move ${src}:`, cpErr.message);
                                            }
                                        }
                                    }
                                    try { fs.rmdirSync(singleFolder, { maxRetries: 5, retryDelay: 500 }); } catch (e) { /* ignore */ }
                                }

                                modpackExtractProgress(mainWindow, {
                                    status: 'progress',
                                    message: 'Extraction complete',
                                    progress: 100
                                });
                                resolve();
                            } else {
                                reject(new Error(`tar exited with code ${code}`));
                            }
                        });
                    });
                } finally {
                    if (fs.existsSync(tmpPath)) {
                        fs.unlinkSync(tmpPath); // Cleanup safely
                    }
                }
            } catch (err) {
                console.error("Modpack zip download/extraction failed:", err);
                throw new Error("Failed to download or extract the modpack specific files: " + err.message);
            }
        }

        // Step 4: Verification Phase (Cyberpunk Terminal request)
        sendProgress(mainWindow, {
            status: 'progress',
            message: 'Verifying file integrity...',
            progress: 99
        });

        const filesToCheck = [
            { name: 'client.jar', path: path.join(gameDir, 'versions', version, `${version}.jar`) },
            { name: 'libraries', path: path.join(gameDir, 'libraries'), isDir: true },
            { name: 'assets', path: path.join(gameDir, 'assets'), isDir: true },
        ];

        if (downloadUrl) {
            filesToCheck.push({ name: 'mods', path: path.join(gameDir, 'mods'), isDir: true, required: false });
            filesToCheck.push({ name: 'config', path: path.join(gameDir, 'config'), isDir: true, required: false });
        }

        for (const file of filesToCheck) {
            const exists = fs.existsSync(file.path);
            if (!exists) {
                if (file.required !== false) {
                    sendProgress(mainWindow, { status: 'verify-end', message: 'ERROR', progress: 99 });
                    throw new Error(`Critical integrity check failed: ${file.name} is missing.`);
                } else {
                    console.warn(`[Verify] Optional item missing: ${file.name}`);
                }
            }
        }

        // Step 5: Mark installation as complete and valid
        gameInstaller.markInstalled(appRoot, gameId, version, modLoader, buildTag || null);

        sendProgress(mainWindow, {
            status: 'success',
            message: 'Installation complete!',
            progress: 100
        });
    }

    /**
     * Recursively copies a directory from src to dest, preserving structure.
     */
    _copyDirRecursive(src, dest) {
        fs.mkdirSync(dest, { recursive: true });
        const entries = fs.readdirSync(src, { withFileTypes: true });
        for (const entry of entries) {
            const srcPath = path.join(src, entry.name);
            const destPath = path.join(dest, entry.name);
            if (entry.isDirectory()) {
                this._copyDirRecursive(srcPath, destPath);
            } else {
                fs.copyFileSync(srcPath, destPath);
            }
        }
    }

    /**
     * Updates a modpack build while preserving user data (worlds, configs, options).
     * 
     * Flow:
     * 1. Backup user data (saves, config, options.txt, servers.dat, etc.) to temp
     * 2. Wipe only pack-managed folder (mods)
     * 3. Download + extract new modpack ZIP
     * 4. Restore user data from backup
     * 5. Update .ldl_installed with new build tag
     */
    async updateMinecraftBuild(appRoot, mainWindow, sendProgress, gameId, downloadUrl, downloadToken, newBuildTag) {
        const gameDir = path.join(appRoot, 'games', gameId);

        if (!fs.existsSync(gameDir)) {
            throw new Error(`Game ${gameId} is not installed. Cannot update.`);
        }

        // Read current installation info
        const currentInfo = gameInstaller.readInstalled(appRoot, gameId);
        if (!currentInfo) {
            throw new Error(`No installation metadata found for ${gameId}. Please reinstall.`);
        }

        const backupDir = path.join(appRoot, 'temp', `update_backup_${gameId}_${Date.now()}`);
        fs.mkdirSync(backupDir, { recursive: true });

        // ─── Step 1: Backup user data ───
        sendProgress(mainWindow, {
            status: 'progress',
            message: 'Backing up your worlds and configs...',
            progress: 5
        });

        const userFolders = ['saves', 'config', 'resourcepacks', 'shaderpacks', 'screenshots'];
        const userFiles = [
            'options.txt', 'servers.dat', 'optionsof.txt', 'options.txt.bak',
            // All optimizer hash files — preserve them so optimizer doesn't
            // re-apply settings after update when user already has the correct profile
            '.ldl_options_hash',
            '.ldl_default_hash',
            '.ldl_perf_hash',
            '.ldl_perf_low_hash',
            '.ldl_perf_medium_hash',
            '.ldl_perf_high_hash',
            '.ldl_perf_ultra_hash',
            '.ldl_sodium_hash',
        ];

        for (const folder of userFolders) {
            const src = path.join(gameDir, folder);
            if (fs.existsSync(src)) {
                const dest = path.join(backupDir, folder);
                this._copyDirRecursive(src, dest);
                console.log(`[Update] Backed up folder: ${folder}`);
            }
        }

        for (const file of userFiles) {
            const src = path.join(gameDir, file);
            if (fs.existsSync(src)) {
                fs.copyFileSync(src, path.join(backupDir, file));
                console.log(`[Update] Backed up file: ${file}`);
            }
        }

        sendProgress(mainWindow, {
            status: 'progress',
            message: 'Backup complete. Downloading update...',
            progress: 15
        });

        // ─── Step 2: Wipe only pack-managed content (mods) ───
        const modsDir = path.join(gameDir, 'mods');
        if (fs.existsSync(modsDir)) {
            fs.rmSync(modsDir, { recursive: true, force: true });
            console.log('[Update] Wiped mods/ folder');
        }

        // ─── Step 3: Download + extract new modpack ───
        if (downloadUrl) {
            const tempDir = path.join(appRoot, 'temp');
            fs.mkdirSync(tempDir, { recursive: true });
            const tmpPath = path.join(tempDir, `ldl_update_${Date.now()}.zip`);

            try {
                // Resolve GitHub download URL (same logic as install)
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

                    // Try release by tag, then by ID, then search all
                    try {
                        const resp = await axios.get(
                            `https://api.github.com/repos/${owner}/${repo}/releases/tags/${tag}`,
                            { headers: apiHeaders }
                        );
                        releaseData = resp.data;
                    } catch (e) {
                        try {
                            const resp = await axios.get(
                                `https://api.github.com/repos/${owner}/${repo}/releases/${tag}`,
                                { headers: apiHeaders }
                            );
                            releaseData = resp.data;
                        } catch (e2) {
                            const resp = await axios.get(
                                `https://api.github.com/repos/${owner}/${repo}/releases?per_page=30`,
                                { headers: apiHeaders }
                            );
                            releaseData = resp.data.find(r => r.assets.some(a => a.name === assetName));
                        }
                    }

                    if (!releaseData) throw new Error(`No release found for update`);

                    const asset = releaseData.assets.find(a => a.name === assetName);
                    if (!asset) throw new Error(`Asset "${assetName}" not found in release`);

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

                sendProgress(mainWindow, {
                    status: 'progress',
                    message: 'Downloading update...',
                    progress: 20
                });

                const response = await axios({
                    method: 'GET',
                    url: finalDownloadUrl,
                    responseType: 'stream',
                    headers: finalHeaders,
                    maxRedirects: 5
                });

                const writer = fs.createWriteStream(tmpPath);
                response.data.pipe(writer);

                const totalLength = parseInt(response.headers['content-length'], 10) || 1; // guard against NaN
                let downloaded = 0;
                response.data.on('data', (chunk) => {
                    downloaded += chunk.length;
                    const pct = Math.floor(20 + (downloaded / totalLength) * 50);
                    sendProgress(mainWindow, {
                        status: 'progress',
                        message: 'minecraft.zip',
                        progress: pct
                    });
                });

                await new Promise((resolve, reject) => {
                    writer.on('finish', resolve);
                    writer.on('error', reject);
                });

                sendProgress(mainWindow, {
                    status: 'progress',
                    message: 'minecraft.zip',
                    progress: 100
                });

                // Extract - only mods/config from the archive will overwrite
                sendProgress(mainWindow, {
                    status: 'progress',
                    message: 'Extracting update...',
                    progress: 75
                });

                await new Promise((resolve, reject) => {
                    const settingsManager = new GeneralSettingsManager(appRoot);
                    const settings = settingsManager.getSettings();

                    const args = ['-xf', tmpPath, '-C', gameDir, '-v'];
                    const ps = spawn('tar.exe', args, { windowsHide: !settings.showInstallConsole });

                    ps.stdout.on('data', (data) => {
                        const lines = data.toString().split(/\r?\n/);
                        for (const line of lines) {
                            const trimmed = line.trim();
                            if (trimmed) {
                                sendProgress(mainWindow, {
                                    status: 'progress',
                                    message: `Unpacking: ${path.basename(trimmed)}`,
                                    progress: 75
                                });
                            }
                        }
                    });

                    ps.stderr.on('data', (data) => {
                        const trimmed = data.toString().trim();
                        if (trimmed && trimmed.toLowerCase().includes('error')) {
                            console.error('[Update] tar stderr:', trimmed);
                        }
                    });

                    ps.on('close', (code) => {
                        if (code === 0) resolve();
                        else reject(new Error(`tar exited with code ${code}`));
                    });
                });

                // Cleanup temp file
                if (fs.existsSync(tmpPath)) fs.unlinkSync(tmpPath);

            } catch (err) {
                // Restore backup on failure
                console.error('[Update] Download/extract failed, restoring backup...', err);
                sendProgress(mainWindow, {
                    status: 'warning',
                    message: 'Update failed. Restoring your data...',
                    progress: 80
                });

                for (const folder of userFolders) {
                    const src = path.join(backupDir, folder);
                    if (fs.existsSync(src)) {
                        const dest = path.join(gameDir, folder);
                        if (fs.existsSync(dest)) fs.rmSync(dest, { recursive: true, force: true });
                        this._copyDirRecursive(src, dest);
                    }
                }
                for (const file of userFiles) {
                    const src = path.join(backupDir, file);
                    if (fs.existsSync(src)) fs.copyFileSync(src, path.join(gameDir, file));
                }

                // Cleanup backup
                if (fs.existsSync(backupDir)) fs.rmSync(backupDir, { recursive: true, force: true });
                throw new Error('Update failed: ' + err.message);
            }
        }

        // ─── Step 4: Restore user data from backup ───
        sendProgress(mainWindow, {
            status: 'progress',
            message: 'Restoring your worlds and configs...',
            progress: 85
        });

        for (const folder of userFolders) {
            const src = path.join(backupDir, folder);
            if (fs.existsSync(src)) {
                const dest = path.join(gameDir, folder);
                // For config: merge — user config files take priority over archive
                if (folder === 'config' && fs.existsSync(dest)) {
                    // Copy user configs on top (overwriting archive versions)
                    this._copyDirRecursive(src, dest);
                } else {
                    // For saves, resourcepacks etc: replace entirely with backup
                    if (fs.existsSync(dest)) fs.rmSync(dest, { recursive: true, force: true });
                    this._copyDirRecursive(src, dest);
                }
                console.log(`[Update] Restored folder: ${folder}`);
            }
        }

        for (const file of userFiles) {
            const src = path.join(backupDir, file);
            if (fs.existsSync(src)) {
                fs.copyFileSync(src, path.join(gameDir, file));
                console.log(`[Update] Restored file: ${file}`);
            }
        }

        // ─── Step 5: Update marker with new build tag ───
        gameInstaller.markInstalled(appRoot, gameId, currentInfo.version, currentInfo.modLoader, newBuildTag);

        // Cleanup backup
        if (fs.existsSync(backupDir)) fs.rmSync(backupDir, { recursive: true, force: true });

        sendProgress(mainWindow, {
            status: 'success',
            message: 'Update complete! Your worlds and configs are safe.',
            progress: 100
        });
    }
}

module.exports = new InstallationManager();
