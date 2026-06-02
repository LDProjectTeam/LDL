const {
  app,
  BrowserWindow,
  ipcMain,
  shell,
  Notification,
  Menu,
  dialog,
} = require("electron");
const path = require("path");
const http = require("http");
const https = require("https");
const url = require("url");
const crypto = require("crypto");
const fs = require("fs");
const dns = require("dns");

// Fix for ETIMEDOUT on broken IPv6 networks (Node 18+ defaults to IPv6 first)
dns.setDefaultResultOrder("ipv4first");
http.globalAgent.options.family = 4;
https.globalAgent.options.family = 4;

const launcherService = require("./LauncherService");

const GeneralSettingsManager = require("./managers/GeneralSettingsManager");

// ─── Apply Hardware Acceleration setting BEFORE app is ready ───
// Must be done synchronously here, cannot be done after app.whenReady()
try {
  let settingsRoot;
  if (process.env.PORTABLE_EXECUTABLE_DIR) {
    settingsRoot = process.env.PORTABLE_EXECUTABLE_DIR;
  } else if (process.env.APPIMAGE) {
    settingsRoot = require("path").dirname(process.env.APPIMAGE);
  } else if (require("electron").app.isPackaged) {
    settingsRoot = require("path").dirname(process.execPath);
  } else {
    settingsRoot = require("path").join(__dirname, "..", "..");
  }
  const _settingsMgr = new GeneralSettingsManager(settingsRoot);
  const _s = _settingsMgr.getSettings();
  if (_s.hwAcceleration === false) {
    console.log("[Main] Hardware acceleration DISABLED (from settings)");
    require("electron").app.disableHardwareAcceleration();
  }
} catch (e) {
  // If settings can't be read, just proceed with defaults (hw accel ON)
  console.warn("[Main] Could not read hwAcceleration setting:", e.message);
}

// ─── Windows Branding Fix ───
if (process.platform === "win32") {
  app.setAppUserModelId("com.ldlauncher.app");
}

let mainWindow;

// Clean up default Menu
Menu.setApplicationMenu(null);

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 760,
    minWidth: 1024,
    minHeight: 600,
    frame: false,
    transparent: false,
    backgroundColor: "#0a0a0f",
    resizable: false,
    maximizable: false,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, "preload.js"),
    },
    title: "LDLauncher",
    icon: path.join(__dirname, "..", "src", "assets", "icon.png"),
  });

  // In dev mode, load from Vite dev server; in production, load built files
  const isDev = process.argv.includes("--dev");
  if (isDev) {
    mainWindow.loadURL("http://localhost:5173");
  } else {
    mainWindow.loadFile(path.join(__dirname, "..", "dist", "index.html"));
  }

  mainWindow.webContents.on(
    "console-message",
    (event, level, message, line, sourceId) => {
      console.log(`[Frontend] ${message}`);
    },
  );

  // Disable default right-click menu for production feel
  mainWindow.webContents.on("context-menu", (e) => {
    e.preventDefault();
  });

  mainWindow.on("closed", () => {
    mainWindow = null;
  });
}

// Ensure True Portability: Store Electron's Cache and UserData near the EXE, not in AppData.
// ONLY apply this for Portable builds. The NSIS installer MUST use default AppData 
// so that sessions survive the update process (NSIS wipes the install dir during updates).
if (app.isPackaged) {
  if (process.env.PORTABLE_EXECUTABLE_DIR) {
    const portableDataPath = path.join(process.env.PORTABLE_EXECUTABLE_DIR, "local_data");
    app.setPath("userData", portableDataPath);
    app.setPath("sessionData", portableDataPath);
    app.setPath("crashDumps", portableDataPath);
    app.setPath("logs", portableDataPath);
  } else if (process.env.APPIMAGE) {
    const portableDataPath = path.join(path.dirname(process.env.APPIMAGE), "local_data");
    app.setPath("userData", portableDataPath);
    app.setPath("sessionData", portableDataPath);
    app.setPath("crashDumps", portableDataPath);
    app.setPath("logs", portableDataPath);
  }
  // If it's the NSIS installed version, do nothing!
  // Electron will safely use %AppData%/Roaming/<productName>
}

// Store reference to OAuth server so we can close it if needed
let oauthServer = null;

// ─── OAuth Callback Page Builder ───
// Reads saved launcher language and renders a styled page matching the launcher UI.
function buildOAuthCallbackPage(appRootPath, success) {
  // Read language from launcher_settings.json
  let lang = 'ru';
  try {
    const settingsFile = path.join(appRootPath, 'launcher_settings.json');
    if (fs.existsSync(settingsFile)) {
      const s = JSON.parse(fs.readFileSync(settingsFile, 'utf-8'));
      if (s.language && ['ru', 'en', 'ua'].includes(s.language)) lang = s.language;
    }
  } catch (e) { /* use default */ }

  const i18n = {
    ru: {
      title: '\u0423\u0441\u043f\u0435\u0448\u043d\u044b\u0439 \u0432\u0445\u043e\u0434',
      heading: '\u0423\u0441\u043f\u0435\u0448\u043d\u044b\u0439 \u0432\u0445\u043e\u0434!',
      sub: '\u0412\u043e\u0437\u0432\u0440\u0430\u0449\u0430\u0435\u043c \u0432\u0430\u0441 \u0432 \u043b\u0430\u0443\u043d\u0447\u0435\u0440...',
      errTitle: '\u041e\u0448\u0438\u0431\u043a\u0430 \u0432\u0445\u043e\u0434\u0430',
      errSub: '\u041f\u043e\u043f\u0440\u043e\u0431\u0443\u0439\u0442\u0435 \u0435\u0449\u0451 \u0440\u0430\u0437 \u0432 \u043b\u0430\u0443\u043d\u0447\u0435\u0440\u0435.',
    },
    en: {
      title: 'Authorization Successful',
      heading: 'Login Successful!',
      sub: 'Returning you to the launcher\u2026',
      errTitle: 'Login Failed',
      errSub: 'Please try again inside the launcher.',
    },
    ua: {
      title: '\u0423\u0441\u043f\u0456\u0448\u043d\u0438\u0439 \u0432\u0445\u0456\u0434',
      heading: '\u0412\u0445\u0456\u0434 \u0432\u0438\u043a\u043e\u043d\u0430\u043d\u043e!',
      sub: '\u041f\u043e\u0432\u0435\u0440\u0442\u0430\u0454\u043c\u043e \u0432\u0430\u0441 \u0434\u043e \u043b\u0430\u0443\u043d\u0447\u0435\u0440\u0430\u2026',
      errTitle: '\u041f\u043e\u043c\u0438\u043b\u043a\u0430 \u0432\u0445\u043e\u0434\u0443',
      errSub: '\u0421\u043f\u0440\u043e\u0431\u0443\u0439\u0442\u0435 \u0449\u0435 \u0440\u0430\u0437 \u0443 \u043b\u0430\u0443\u043d\u0447\u0435\u0440\u0456.',
    },
  };

  const T = i18n[lang] || i18n.ru;
  const accentColor = success ? '#7fff7f' : '#ff6060';
  const icon = success ? '\u2713' : '\u2717';
  const title = success ? T.heading : T.errTitle;
  const sub = success ? T.sub : T.errSub;
  const pageTitle = success ? T.title : T.errTitle;

  return `<!DOCTYPE html>
<html lang="${lang}">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>${pageTitle}</title>
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link href="https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Inter:wght@400;700&display=swap" rel="stylesheet" />
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    html, body {
      height: 100%;
      background: #0a0a0f;
      color: #c8d4c8;
      font-family: 'Share Tech Mono', 'Courier New', monospace;
      display: flex;
      align-items: center;
      justify-content: center;
      overflow: hidden;
    }
    /* CRT scanline overlay */
    body::before {
      content: '';
      position: fixed;
      inset: 0;
      background: repeating-linear-gradient(
        to bottom,
        transparent 0px,
        transparent 2px,
        rgba(0,0,0,0.18) 2px,
        rgba(0,0,0,0.18) 4px
      );
      pointer-events: none;
      z-index: 100;
    }
    /* Ambient glow blobs */
    .blob {
      position: fixed;
      border-radius: 50%;
      filter: blur(120px);
      pointer-events: none;
    }
    .blob-1 {
      width: 45vw; height: 45vh;
      background: ${accentColor};
      opacity: 0.04;
      top: -10%; left: -5%;
    }
    .blob-2 {
      width: 35vw; height: 35vh;
      background: ${accentColor};
      opacity: 0.03;
      bottom: -10%; right: -5%;
    }
    .card {
      position: relative;
      background: rgba(14, 20, 14, 0.92);
      border: 1px solid ${accentColor}44;
      box-shadow: 0 0 40px ${accentColor}22, inset 0 0 40px rgba(0,0,0,0.4);
      padding: 48px 52px;
      min-width: 340px;
      max-width: 420px;
      text-align: center;
      animation: fadeIn 0.4s ease-out both;
    }
    .card::before {
      content: '';
      position: absolute;
      inset: 0;
      border: 1px solid ${accentColor}22;
      pointer-events: none;
    }
    .icon {
      font-size: 52px;
      color: ${accentColor};
      text-shadow: 0 0 20px ${accentColor}cc, 0 0 6px ${accentColor};
      margin-bottom: 20px;
      display: block;
      animation: pulse 2.5s ease-in-out infinite;
    }
    h1 {
      font-size: 20px;
      font-weight: 700;
      letter-spacing: 0.12em;
      text-transform: uppercase;
      color: ${accentColor};
      text-shadow: 0 0 12px ${accentColor}99;
      margin-bottom: 12px;
    }
    p {
      font-size: 13px;
      color: #6a826a;
      letter-spacing: 0.05em;
    }
    .ldlogo {
      display: flex;
      align-items: center;
      justify-content: center;
      width: 52px;
      height: 52px;
      border: 1px solid ${accentColor}66;
      box-shadow: 0 0 14px ${accentColor}44;
      margin: 0 auto 28px;
      font-size: 22px;
      font-weight: 700;
      color: ${accentColor};
      text-shadow: 0 0 10px ${accentColor};
      letter-spacing: -1px;
    }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(14px) scale(0.98); }
      to   { opacity: 1; transform: translateY(0)  scale(1); }
    }
    @keyframes pulse {
      0%, 100% { text-shadow: 0 0 20px ${accentColor}cc, 0 0 6px ${accentColor}; }
      50%       { text-shadow: 0 0 32px ${accentColor}, 0 0 12px ${accentColor}cc; }
    }
  </style>
</head>
<body>
  <div class="blob blob-1"></div>
  <div class="blob blob-2"></div>
  <div class="card">
    <div class="ldlogo">LD</div>
    <span class="icon">${icon}</span>
    <h1>${title}</h1>
    <p>${sub}</p>
  </div>
  <script>setTimeout(() => { window.close(); }, 600);<\/script>
</body>
</html>`;
}

const gotTheLock = app.requestSingleInstanceLock();

if (!gotTheLock) {
  app.quit();
} else {
  // If a second instance tries to launch, focus the existing window instead
  app.on("second-instance", () => {
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.show();
      mainWindow.focus();
    }
  });
}

app.whenReady().then(createWindow);

app.on("window-all-closed", () => {
  app.quit();
});

// Window control IPC handlers
ipcMain.on("window-minimize", () => {
  if (mainWindow) mainWindow.minimize();
});

ipcMain.on("window-close", () => {
  if (mainWindow) mainWindow.close();
});

// ─── Email Magic Link Server ───
// Starts a local HTTP server that catches the Supabase magic link redirect.
// ─── Microsoft Authentication (Minecraft License Check) ───
ipcMain.handle("start-microsoft-auth", async () => {
  try {
    console.log("[MSMC] Starting Microsoft authentication...");
    const { Auth } = require("msmc");
    const authManager = new Auth("login");

    // В Electron-приложениях правильный и безопасный метод - это внутреннее окно
    const xboxManager = await authManager.launch("electron");

    // Используем встроенный в Electron (Chromium) net.fetch вместо проблемного node-fetch
    console.log("[MSMC] Getting XSTS token...");
    const authString = await xboxManager.xAuth(
      "rp://api.minecraftservices.com/",
    );

    console.log("[MSMC] Authenticating with Minecraft Services...");
    const { net } = require("electron");
    const rlogin = await net.fetch(
      "https://api.minecraftservices.com/authentication/login_with_xbox",
      {
        method: "POST",
        body: JSON.stringify({ identityToken: authString }),
        headers: {
          "Content-Type": "application/json",
          Accept: "application/json",
          "User-Agent":
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/115.0",
        },
      },
    );

    if (!rlogin.ok) {
      console.error(
        "[MSMC] Minecraft API rejected login:",
        await rlogin.text(),
      );
      return {
        success: false,
        error: "Ошибка авторизации на серверах Minecraft.",
      };
    }
    const MCauth = await rlogin.json();

    console.log("[MSMC] Fetching Minecraft Profile...");
    const rprofile = await net.fetch(
      "https://api.minecraftservices.com/minecraft/profile",
      {
        headers: {
          "Content-Type": "application/json",
          Accept: "application/json",
          Authorization: `Bearer ${MCauth.access_token}`,
          "User-Agent":
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/115.0",
        },
      },
    );

    const MCprofile = await rprofile.json();

    if (MCprofile.error || !MCprofile.id) {
      console.log("[MSMC] User does not have a Minecraft license (Demo).");
      return {
        success: false,
        error: "У вас нет лицензии Minecraft (Demo-аккаунт).",
      };
    }

    console.log(`[MSMC] Login successful for: ${MCprofile.name}`);

    // Return profile in MCLC compatible format
    return {
      success: true,
      profile: {
        id: MCprofile.id,
        name: MCprofile.name,
        token: {
          access_token: MCauth.access_token,
          client_token: "ldlauncher-client",
          uuid: MCprofile.id,
          name: MCprofile.name,
          user_properties: "{}",
          meta: { type: "msa", demo: false },
        },
      },
    };
  } catch (err) {
    console.error("[MSMC] Authentication error:", err);
    return {
      success: false,
      error: err.message || "Ошибка авторизации Microsoft.",
    };
  }
});

ipcMain.on("start-magic-link-server", () => {
  if (oauthServer) {
    // Server already running (e.g. Google OAuth in progress), don't override
    return;
  }

  const port = 5174;

  oauthServer = http.createServer(async (req, res) => {
    try {
      const parsedUrl = url.parse(req.url, true);

      if (parsedUrl.pathname === "/oauth2callback") {
        // Supabase puts tokens in URL hash — serve JS to extract and re-send them
        if (!parsedUrl.query.error && !parsedUrl.query.access_token) {
          res.writeHead(200, { "Content-Type": "text/html" });
          res.end(`<html><body><script>
                        const hash = window.location.hash.substring(1);
                        window.location.href = '/oauth2callback?' + hash;
                    </script></body></html>`);
          return;
        }

        const accessToken = parsedUrl.query.access_token;
        const refreshToken = parsedUrl.query.refresh_token;

        if (accessToken && mainWindow) {
          mainWindow.show();
          mainWindow.setAlwaysOnTop(true);
          mainWindow.focus();
          mainWindow.setAlwaysOnTop(false);

          mainWindow.webContents.send("oauth-callback", {
            success: true,
            session: { access_token: accessToken, refresh_token: refreshToken },
          });

          res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
          const appRoot = launcherService.getAppRoot ? launcherService.getAppRoot() : __dirname;
          res.end(buildOAuthCallbackPage(appRoot, true));

          req.socket.destroy();
          try {
            oauthServer.close();
            oauthServer = null;
          } catch (e) {}
        } else if (parsedUrl.query.error) {
          mainWindow.webContents.send("oauth-callback", {
            success: false,
            error: parsedUrl.query.error_description || parsedUrl.query.error,
          });
          res.writeHead(400);
          res.end(`Error: ${parsedUrl.query.error}`);
          try {
            oauthServer.close();
            oauthServer = null;
          } catch (e) {}
        }
      } else {
        res.writeHead(404);
        res.end();
      }
    } catch (e) {
      console.error("[MagicLink Server] Error:", e);
      res.writeHead(500);
      res.end();
    }
  });

  oauthServer.listen(port, "127.0.0.1", () => {
    console.log(
      `[MagicLink] Local server listening on port ${port}, waiting for magic link click...`,
    );
  });

  oauthServer.on("error", (e) => {
    // Port may already be in use — silently fail, it means another flow has the server
    console.warn("[MagicLink] Server error:", e.message);
    oauthServer = null;
  });
});

// ─── Supabase Google OAuth Flow ───
ipcMain.on("start-google-oauth", (event, authUrl) => {
  // 1. Close any existing server
  if (oauthServer) {
    oauthServer.close();
    oauthServer = null;
  }

  const port = 5174;

  // 2. Start Local HTTP Server
  oauthServer = http.createServer(async (req, res) => {
    try {
      const parsedUrl = url.parse(req.url, true);

      if (parsedUrl.pathname === "/oauth2callback") {
        // Supabase puts tokens in the URL hash (#access_token=...&refresh_token=...)
        if (!parsedUrl.query.error && !parsedUrl.query.access_token) {
          res.writeHead(200, { "Content-Type": "text/html" });
          res.end(`
                        <html><body><script>
                            const hash = window.location.hash.substring(1);
                            window.location.href = '/oauth2callback?' + hash;
                        </script></body></html>
                    `);
          return;
        }

        const accessToken = parsedUrl.query.access_token;
        const refreshToken = parsedUrl.query.refresh_token;

        if (accessToken && mainWindow) {
          // Force focus hack for Linux/Windows
          mainWindow.show();
          mainWindow.setAlwaysOnTop(true);
          mainWindow.focus();
          mainWindow.setAlwaysOnTop(false);

          // Send tokens directly to React so Supabase JS can establish session
          mainWindow.webContents.send("oauth-callback", {
            success: true,
            session: { access_token: accessToken, refresh_token: refreshToken },
          });

          // Show success page
          res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
          const appRoot = launcherService.getAppRoot ? launcherService.getAppRoot() : __dirname;
          res.end(buildOAuthCallbackPage(appRoot, true));

          // 3. Immediately close the server
          req.socket.destroy();
          try {
            oauthServer.close();
            oauthServer = null;
          } catch (e) {}
        } else if (parsedUrl.query.error) {
          mainWindow.webContents.send("oauth-callback", {
            success: false,
            error: parsedUrl.query.error_description || parsedUrl.query.error,
          });
          res.writeHead(400, { "Content-Type": "text/plain; charset=utf-8" });
          res.end(
            `Ошибка авторизации: ${parsedUrl.query.error_description || parsedUrl.query.error}`,
          );
          try {
            oauthServer.close();
            oauthServer = null;
          } catch (e) {}
        }
      } else {
        res.writeHead(404);
        res.end();
      }
    } catch (e) {
      console.error("Local OAuth Server Error", e);
      res.writeHead(500);
      res.end();
    }
  });

  oauthServer.listen(port, "127.0.0.1", () => {
    console.log(`[OAuth] Opening system browser to: ${authUrl}`);
    shell.openExternal(authUrl);
  });
});

// Open URL in default browser
ipcMain.handle("open-external", async (event, url) => {
  if (url && typeof url === "string") {
    await shell.openExternal(url);
  }
});

// Open folder in Explorer
ipcMain.handle("open-path", async (event, folderPath) => {
  if (folderPath && typeof folderPath === "string") {
    await shell.openPath(folderPath);
  }
});

// Get absolute path to game folder
ipcMain.handle("get-game-path", (event, gameId) => {
  return launcherService.getGamePath(gameId);
});

// Get global app root folder
ipcMain.handle("get-app-root", () => {
  return launcherService.getAppRoot();
});

// Install game IPC handler
ipcMain.handle("install-game", async (event, gameConfig) => {
  try {
    const gameId = gameConfig.gameId || "";
    const gameType = gameConfig.type || "minecraft";
    const version = gameConfig.version || "1.20.1";
    const modLoader = gameConfig.modLoader || "forge";
    const modLoaderVersion = gameConfig.modLoaderVersion || "";
    const downloadUrl = gameConfig.downloadUrl || "";
    const downloadToken = gameConfig.downloadToken || "";
    const installPath = gameConfig.installPath || "";

    const settingsManager = new GeneralSettingsManager(
      launcherService.getAppRoot(),
    );

    if (gameType === "minecraft") {
      await launcherService.installMinecraft(
        mainWindow,
        version,
        modLoader,
        modLoaderVersion,
        gameId,
        downloadUrl,
        downloadToken,
      );
    } else if (gameType === "modpack_zip") {
      await launcherService.extractModpack(
        mainWindow,
        downloadUrl,
        installPath,
        gameId,
        downloadToken,
      );
    }

    return {
      success: true,
      output: `${gameId} installation completed successfully`,
    };
  } catch (error) {
    console.error("Install Error:", error);
    launcherService.onTerminalForward = null;

    // CRITICAL: always signal frontend so button unlocks
    if (mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.webContents.send("game-progress", {
        status: "error",
        gameId: gameConfig.gameId || "",
        message: error.message || String(error),
        progress: 0,
      });
    }

    return { success: false, error: error.message || String(error) };
  }
});

// Game launch IPC handler
ipcMain.handle("launch-game", async (event, gameConfig) => {
  try {
    const gameId = gameConfig.gameId || "";
    const gameType = gameConfig.type || "minecraft";
    const version = gameConfig.version || "1.20.1";
    const modLoader = gameConfig.modLoader || "forge";
    const modLoaderVersion = gameConfig.modLoaderVersion || "";
    const launcherLang = gameConfig.launcherLang || "en";
    const sessionToken = gameConfig.sessionToken || null; // Supabase JWT for DRM
    const minecraftLicense = gameConfig.minecraftLicense || null; // Microsoft Token

    if (gameType === "minecraft") {
      await launcherService.launchMinecraft(
        mainWindow,
        version,
        modLoader,
        modLoaderVersion,
        gameId,
        launcherLang,
        sessionToken,
        minecraftLicense,
      );
    }

    return { success: true, output: `${gameId} started successfully` };
  } catch (error) {
    console.error("Launch Error:", error);
    return { success: false, error: error.message || String(error) };
  }
});

// Kill game IPC handler
ipcMain.handle("kill-game", (event, gameId) => {
  try {
    return launcherService.killGame(gameId);
  } catch (error) {
    console.error("Kill Game Error:", error);
    return false;
  }
});

// Game status IPC handler
ipcMain.handle("get-game-status", (event, config) => {
  try {
    const gameId = typeof config === "string" ? config : config.gameId;
    const type = typeof config === "object" ? config.type : "minecraft";
    const version = typeof config === "object" ? config.version : "1.20.1";

    const statusStr = launcherService.checkGameInstalled(gameId, type, version);

    return {
      status: statusStr,
      running: launcherService.isGameRunning(gameId),
    };
  } catch (error) {
    console.error("Status Error:", error);
    return { status: "missing", running: false };
  }
});

// Game delete IPC handler
ipcMain.handle("delete-game", async (event, gameId) => {
  try {
    await launcherService.deleteGame(gameId);
    return { success: true };
  } catch (error) {
    console.error("Delete Error:", error);
    return { success: false, error: error.message || String(error) };
  }
});

// ─── Auto-Optimization IPC Handlers ───
const SystemOptimizer = require("./managers/SystemOptimizer");
const os = require("os");

function getOptimizerSettingsPath() {
  const appRoot = launcherService.getAppRoot();
  return path.join(appRoot, "optimizer_settings.json");
}

ipcMain.handle("get-optimizer-settings", () => {
  try {
    const settingsPath = getOptimizerSettingsPath();
    if (fs.existsSync(settingsPath)) {
      return JSON.parse(fs.readFileSync(settingsPath, "utf-8"));
    }
  } catch (e) {
    console.warn("[Main] Could not read optimizer settings:", e.message);
  }
  return { enabled: true, mode: "balance" };
});

ipcMain.handle("set-optimizer-settings", (event, settings) => {
  try {
    const settingsPath = getOptimizerSettingsPath();
    const dir = path.dirname(settingsPath);
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
    fs.writeFileSync(settingsPath, JSON.stringify(settings, null, 2));
    return { success: true };
  } catch (e) {
    console.error("[Main] Could not save optimizer settings:", e.message);
    return { success: false, error: e.message };
  }
});

ipcMain.handle("get-system-specs", () => {
  try {
    const appRoot = launcherService.getAppRoot();
    const optimizer = new SystemOptimizer(appRoot);
    const specs = optimizer.getSystemSpecs();
    // Detected hardware tier for the UI
    const tier = optimizer._detectTier(specs);
    // Monitor refresh rate — Electron exposes this via screen.getPrimaryDisplay().
    // displayFrequency is Windows-specific; refreshRate is the cross-platform alias.
    const primaryDisplay = require("electron").screen.getPrimaryDisplay();
    const refreshRate =
      primaryDisplay.displayFrequency || primaryDisplay.refreshRate || 60;

    // VSync is always OFF (FPS is capped at refreshRate to prevent tearing).
    // hasVRR detection is no longer needed and has been removed.
    console.log(`[Main] Monitor: ${refreshRate} Hz | VSync: always OFF`);
    return { ...specs, tier, refreshRate };
  } catch (e) {
    console.error("[Main] Could not get system specs:", e.message);
    return null;
  }
});

// ─── General Settings IPC Handlers ───
ipcMain.handle("get-general-settings", () => {
  try {
    const appRoot = launcherService.getAppRoot();
    const settingsManager = new GeneralSettingsManager(appRoot);
    return settingsManager.getSettings();
  } catch (e) {
    console.error("[Main] Could not get general settings:", e.message);
    return null;
  }
});

ipcMain.handle("set-general-settings", (event, settings) => {
  try {
    const appRoot = launcherService.getAppRoot();
    const settingsManager = new GeneralSettingsManager(appRoot);
    const success = settingsManager.saveSettings(settings);

    // Apply Autostart if it was changed
    if (settings.autostart !== undefined) {
      console.log("[Main] Setting Autostart:", settings.autostart);
      app.setLoginItemSettings({
        openAtLogin: settings.autostart,
        path: app.getPath("exe"),
      });
    }

    // Apply hwAcceleration — requires restart, we just save it
    // On next launch, main.js will check this before app is ready
    if (settings.hwAcceleration !== undefined) {
      console.log(
        "[Main] Hardware Acceleration setting saved:",
        settings.hwAcceleration,
        "(will apply on next launch)",
      );
    }

    return { success };
  } catch (e) {
    console.error("[Main] Could not save general settings:", e.message);
    return { success: false, error: e.message };
  }
});

ipcMain.handle("open-devtools", () => {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.openDevTools({ mode: "detach" });
  }
});

ipcMain.handle("close-devtools", () => {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.closeDevTools();
  }
});

ipcMain.handle("select-directory", async (event, defaultPath) => {
  if (!mainWindow) return null;
  return await dialog.showOpenDialog(mainWindow, {
    title: "Выберите папку установки игр",
    defaultPath: defaultPath || "C:\\",
    properties: ["openDirectory", "createDirectory"],
  });
});

ipcMain.handle("reset-game-settings", () => {
  try {
    const appRoot = launcherService.getAppRoot();
    const p = require("path");
    const fs = require("fs");
    const gamesDir = p.join(appRoot, "games");

    // All hash files the optimizer may have written.
    // Deleting them forces the optimizer to re-apply its profile on next launch.
    const HASH_FILES = [
      ".ldl_default_hash", // default (nice-visuals) profile
      ".ldl_perf_hash", // legacy single-profile hash (older builds)
      ".ldl_perf_low_hash", // tier: LOW
      ".ldl_perf_medium_hash", // tier: MEDIUM
      ".ldl_perf_high_hash", // tier: HIGH
      ".ldl_perf_ultra_hash", // tier: ULTRA
      ".ldl_sodium_hash", // sodium-options.json guard
      ".ldl_options_hash", // legacy options hash (older builds)
    ];

    if (fs.existsSync(gamesDir)) {
      const dirs = fs.readdirSync(gamesDir);
      for (const d of dirs) {
        for (const hf of HASH_FILES) {
          const fp = p.join(gamesDir, d, hf);
          if (fs.existsSync(fp)) {
            fs.unlinkSync(fp);
            console.log(`[Reset] Removed ${fp}`);
          }
        }
      }
    }

    console.log("[Reset] All game optimizer hashes cleared.");
    return { success: true };
  } catch (e) {
    console.error("Failed to reset game settings", e.message);
    return { success: false, error: e.message };
  }
});

// ─── Build Update IPC Handlers ───
const gameInstaller = require("./managers/GameInstaller");
const installationManager = require("./managers/InstallationManager");
const axios = require("axios");

ipcMain.handle(
  "check-build-update",
  async (event, { gameId, downloadUrl, downloadToken }) => {
    try {
      // Use gamesRoot (respects custom install directory from settings)
      const gamesRoot = launcherService.getGamesRoot
        ? launcherService.getGamesRoot()
        : launcherService.getAppRoot();
      const installed = gameInstaller.readInstalled(gamesRoot, gameId);
      const currentTag = installed?.buildTag || null;

      // Parse GitHub URL to get owner/repo
      const ghMatch = downloadUrl?.match(
        /^https?:\/\/github\.com\/([^/]+)\/([^/]+)\/releases\/download\/([^/]+)\/(.+)$/,
      );

      if (!ghMatch || !downloadToken) {
        return {
          hasUpdate: false,
          currentTag,
          latestTag: null,
          error: "No GitHub URL or token",
        };
      }

      const [, owner, repo] = ghMatch;
      const apiHeaders = {
        Authorization: `Bearer ${downloadToken}`,
        Accept: "application/vnd.github+json",
        "User-Agent": "LDLauncher/1.0",
      };

      // Get latest release
      const resp = await axios.get(
        `https://api.github.com/repos/${owner}/${repo}/releases/latest`,
        { headers: apiHeaders },
      );

      const latestTag = resp.data.tag_name;
      const hasUpdate = currentTag !== latestTag;

      console.log(
        `[UpdateCheck] ${gameId}: installed=${currentTag}, latest=${latestTag}, hasUpdate=${hasUpdate}`,
      );

      return {
        hasUpdate,
        currentTag,
        latestTag,
        releaseName: resp.data.name || latestTag,
        publishedAt: resp.data.published_at,
      };
    } catch (e) {
      console.error("[UpdateCheck] Failed:", e.message);
      return {
        hasUpdate: false,
        currentTag: null,
        latestTag: null,
        error: e.message,
      };
    }
  },
);

ipcMain.handle(
  "update-game",
  async (event, { gameId, downloadUrl, downloadToken, newBuildTag }) => {
    try {
      // Use gamesRoot (respects custom install directory from settings).
      // getAppRoot() = launcher .exe directory.
      // getGamesRoot() = custom games folder (may differ if user changed install path in settings).
      const gamesRoot = launcherService.getGamesRoot
        ? launcherService.getGamesRoot()
        : launcherService.getAppRoot();

      // NOTE: do NOT shadow the module-level `mainWindow` with `const` here.
      const updateWindow = mainWindow || BrowserWindow.getAllWindows()[0];
      const scopedSendProgress = (win, data) =>
        launcherService.sendProgress(win, { ...data, gameId });

      await installationManager.updateMinecraftBuild(
        gamesRoot,
        updateWindow,
        scopedSendProgress,
        gameId,
        downloadUrl,
        downloadToken,
        newBuildTag,
      );

      return { success: true };
    } catch (e) {
      console.error("[UpdateGame] Failed:", e.message);
      return { success: false, error: e.message };
    }
  },
);

// ─── Desktop Notifications IPC Handler ───
ipcMain.handle("show-notification", (event, { title, body }) => {
  if (Notification.isSupported()) {
    const notification = new Notification({
      title: title || "LDLauncher",
      body: body,
      icon: path.join(__dirname, "..", "src", "assets", "icon.png"),
    });

    // When notification is clicked, bring app to front
    notification.on("click", () => {
      if (mainWindow) {
        if (mainWindow.isMinimized()) mainWindow.restore();
        mainWindow.show();
        mainWindow.focus();
      }
    });

    notification.show();
    return { success: true };
  }
  return { success: false, error: "Notifications not supported on this OS" };
});

// ─── Custom Launcher Auto Updater (For Inno Setup) ───
// Note: launcher updates are downloaded via axios streams (no electron-dl needed)
function sendLauncherUpdateStatus(status, progressObj = null, version = null) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send("launcher-update-status", {
      status,
      progress: progressObj,
      version,
    });
  }
}

let latestDownloadUrl = null;
let downloadedFilePath = null;

ipcMain.handle("check-launcher-updates", async () => {
  try {
    if (!app.isPackaged) {
      console.log(
        "[CustomUpdater] Skipped update check because app is not packaged (Dev Mode).",
      );
      sendLauncherUpdateStatus("latest", null, app.getVersion());
      return { success: true };
    }

    sendLauncherUpdateStatus("checking");
    const resp = await axios.get(
      "https://api.github.com/repos/LDProjectTeam/LDL/releases/latest",
    );

    const latestTag = resp.data.tag_name.replace(/^v/, "");
    const currentVersion = app.getVersion();

    // Simple string comparison for versions (assuming semantic formatting like x.y.z)
    const isNewer =
      latestTag.localeCompare(currentVersion, undefined, {
        numeric: true,
        sensitivity: "base",
      }) > 0;

    if (isNewer) {
      // Find the asset
      const asset = resp.data.assets.find(
        (a) => a.name === "LDLauncher_Setup.exe" || a.name.endsWith(".exe"),
      );
      if (asset) {
        latestDownloadUrl = asset.browser_download_url;
        sendLauncherUpdateStatus("available", null, latestTag);
      } else {
        sendLauncherUpdateStatus("error");
        console.error(
          "[CustomUpdater] No .exe asset found in the latest release.",
        );
      }
    } else {
      sendLauncherUpdateStatus("latest", null, currentVersion);
    }

    return { success: true };
  } catch (error) {
    console.error("[CustomUpdater] Check failed:", error.message);
    sendLauncherUpdateStatus("error");
    return { success: false, error: error.message };
  }
});

ipcMain.handle("install-launcher-update", async () => {
  try {
    if (!latestDownloadUrl)
      throw new Error(
        "No download URL available. Please check for updates first.",
      );

    // Ensure temp folder exists
    const tempDir = app.getPath("temp");
    downloadedFilePath = path.join(tempDir, "LDLauncher_Update.exe");

    sendLauncherUpdateStatus("downloading", { percent: 0 });

    // Download natively using axios streams
    const response = await axios({
      url: latestDownloadUrl,
      method: "GET",
      responseType: "stream",
    });

    const totalLength = parseInt(response.headers["content-length"], 10);
    let downloadedLength = 0;
    const writer = fs.createWriteStream(downloadedFilePath);

    response.data.on("data", (chunk) => {
      downloadedLength += chunk.length;
      const percent = (downloadedLength / totalLength) * 100;
      sendLauncherUpdateStatus("downloading", { percent });
    });

    // Pipe BEFORE attaching finish/error so no data is missed if the stream
    // emits events synchronously before the Promise callbacks are wired up.
    response.data.pipe(writer);

    return new Promise((resolve, reject) => {
      writer.on("close", () => {
        sendLauncherUpdateStatus("ready");
        // Execute the installer detached so it survives app.quit()
        const { spawn } = require("child_process");
        const child = spawn(downloadedFilePath, [], {
          detached: true,
          stdio: "ignore",
        });
        child.unref();

        // Quit ourselves immediately so the installer can overwrite the files
        app.quit();

        resolve({ success: true });
      });
      writer.on("error", (err) => {
        console.error("[CustomUpdater] Write stream error:", err);
        sendLauncherUpdateStatus("error");
        reject({ success: false, error: err.message });
      });
    });
  } catch (error) {
    console.error("[CustomUpdater] Install failed:", error);
    sendLauncherUpdateStatus("error");
    return { success: false, error: error.message };
  }
});
