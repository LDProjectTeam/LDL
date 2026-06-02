const path = require("path");
const fs = require("fs");
const { exec } = require("child_process");
const { Client } = require("minecraft-launcher-core");
const javaManager = require("./JavaManager");
const gameValidator = require("./GameValidator");
const SystemOptimizer = require("./SystemOptimizer");
const discordRPC = require("./DiscordRPCManager");

class LaunchManager {
  constructor() {
    this.launcher = new Client();
    this.runningGames = new Set();
    this.gameProcesses = new Map(); // gameId -> child process
  }

  isGameRunning(gameId) {
    return this.runningGames.has(gameId);
  }

  killGame(gameId) {
    const proc = this.gameProcesses.get(gameId);
    if (!proc) {
      console.warn(`[LaunchManager] No process found for gameId: ${gameId}`);
      return false;
    }
    try {
      if (process.platform === "win32") {
        // taskkill with /F /T kills the whole process tree (JVM + children)
        const { execSync } = require("child_process");
        execSync(`taskkill /PID ${proc.pid} /F /T`, { timeout: 5000 });
      } else {
        proc.kill("SIGKILL");
      }
      this.gameProcesses.delete(gameId);
      this.runningGames.delete(gameId);
      console.log(`[LaunchManager] Killed game process for: ${gameId} (pid=${proc.pid})`);
      return true;
    } catch (e) {
      console.error(`[LaunchManager] Failed to kill game ${gameId}:`, e.message);
      return false;
    }
  }

  /**
   * Starts a one-shot TCP challenge-response server for DRM handshake.
   * The Fabric Guard mod connects to this port and proves it was launched
   * by LDLauncher by signing a random challenge with the session token.
   *
   * Performance impact: ZERO. This runs once at startup on a local socket.
   * The server automatically closes after 60 seconds if no mod connects.
   *
   * @param {string|null} sessionToken  Supabase JWT used as the HMAC key
   * @returns {{ port: number|null }}
   */
  _startDRMServer(sessionToken) {
    const net = require('net');
    const crypto = require('crypto');

    if (!sessionToken) return Promise.resolve({ port: null });

    return new Promise((resolve) => {
      const server = net.createServer((socket) => {
        // Send a random 32-byte hex challenge
        const challenge = crypto.randomBytes(32).toString('hex');
        socket.write(JSON.stringify({ challenge }) + '\n');

        let buf = '';
        socket.on('data', (data) => {
          buf += data.toString();
          const nl = buf.indexOf('\n');
          if (nl === -1) return;

          const line = buf.slice(0, nl).trim();
          buf = buf.slice(nl + 1);

          try {
            const { response } = JSON.parse(line);
            // Expected: HMAC-SHA256(challenge, sessionToken)
            const expected = crypto
              .createHmac('sha256', sessionToken)
              .update(challenge)
              .digest('hex');

            const ok = response === expected;
            socket.write(JSON.stringify({ ok }) + '\n');
            console.log(`[DRM] Handshake ${ok ? 'OK ✓' : 'FAILED ✗ (wrong token or not our mod)'}`);
          } catch (e) {
            socket.write(JSON.stringify({ ok: false }) + '\n');
          }

          // One-shot: close server after first connection
          server.close();
          socket.destroy();
        });

        socket.on('error', () => { /* ignore connection errors */ });
      });

      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port;
        console.log(`[DRM] Challenge server ready on 127.0.0.1:${port}`);
        resolve({ port, server });
      });

      server.on('error', (e) => {
        console.warn('[DRM] Could not start challenge server:', e.message);
        resolve({ port: null });
      });

      // Auto-close after 5 min — heavy modpacks can take 60–90s to load,
      // so 60s was too short. 5 min is a safe upper bound.
      setTimeout(() => {
        if (server.listening) {
          server.close();
          console.log('[DRM] Server auto-closed (mod did not connect within 5 min)');
        }
      }, 300000);
    });
  }

  /**
   * Reads the user's optimization settings from a local JSON file.
   * Returns { enabled: boolean, mode: 'save'|'balance'|'performance' }
   */
  _getOptimizerSettings(appRoot) {
    const settingsPath = path.join(appRoot, "optimizer_settings.json");
    try {
      if (fs.existsSync(settingsPath)) {
        return JSON.parse(fs.readFileSync(settingsPath, "utf-8"));
      }
    } catch (e) {
      console.warn(
        "[LaunchManager] Could not read optimizer settings:",
        e.message,
      );
    }
    return { enabled: true };
  }

  _syncGameLanguage(gameDir, launcherLang) {
    try {
      const optionsPath = path.join(gameDir, "options.txt");
      let mcLang = "en_us";
      if (launcherLang === "ru") mcLang = "ru_ru";
      else if (launcherLang === "ua") mcLang = "uk_ua";

      if (fs.existsSync(optionsPath)) {
        let content = fs.readFileSync(optionsPath, "utf8");
        if (content.includes("lang:")) {
          content = content.replace(/lang:.*(\r?\n|$)/, `lang:${mcLang}$1`);
        } else {
          content += `\nlang:${mcLang}\n`;
        }
        fs.writeFileSync(optionsPath, content, "utf8");
      } else {
        fs.writeFileSync(optionsPath, `lang:${mcLang}\n`, "utf8");
      }
    } catch (e) {
      console.error("[LaunchManager] Failed to sync game language:", e.message);
    }
  }

  _syncSodiumThreads(gameDir, threads) {
    // Sodium stores its config in snake_case. The field is 'chunk_builder_threads',
    // NOT 'chunkBuilderThreads'. Using the wrong key silently creates a dead field
    // and the actual setting is never changed.
    const THREAD_KEY = "chunk_builder_threads";
    const DEFER_KEY = "always_defer_chunk_updates_v2";
    const sodiumFile = path.join(gameDir, "config", "sodium-options.json");
    const sodiumHash = path.join(gameDir, ".ldl_sodium_hash");

    try {
      if (!fs.existsSync(sodiumFile)) return;

      const config = JSON.parse(fs.readFileSync(sodiumFile, "utf-8"));
      if (!config.performance) return;

      config.performance[THREAD_KEY] = threads;
      fs.writeFileSync(sodiumFile, JSON.stringify(config, null, 4));

      // Update the hash so the SystemOptimizer hash-guard on next launch
      // sees "no user change" and doesn't overwrite the manual value.
      // IMPORTANT: must include the same fields that SystemOptimizer hashes.
      const crypto = require("crypto");
      const written = {
        chunk_builder_threads: config.performance.chunk_builder_threads,
        always_defer_chunk_updates_v2:
          config.performance.always_defer_chunk_updates_v2,
        use_block_face_culling: config.performance.use_block_face_culling,
        use_fog_occlusion: config.performance.use_fog_occlusion,
        use_entity_culling: config.performance.use_entity_culling,
        animate_only_visible_textures:
          config.performance.animate_only_visible_textures,
        use_no_error_g_l_context: config.performance.use_no_error_g_l_context,
        use_advanced_staging_buffers:
          config.advanced?.use_advanced_staging_buffers,
        cpu_render_ahead_limit: config.advanced?.cpu_render_ahead_limit,
      };
      fs.writeFileSync(
        sodiumHash,
        crypto.createHash("md5").update(JSON.stringify(written)).digest("hex"),
      );

      console.log(
        `[LaunchManager] Manual sodium ${THREAD_KEY} set to ${threads}`,
      );
    } catch (e) {
      console.warn(
        "[LaunchManager] Could not sync sodium manual threads:",
        e.message,
      );
    }
  }

  /**
   * Executes the game strictly without downloading new assets.
   * Assumes GameValidator has already cleared the build.
   */
  async launchMinecraft(
    appRoot,
    mainWindow,
    sendProgress,
    version,
    modLoader,
    modLoaderVersion,
    gameId,
    launcherLang,
    sessionToken = null,   // Supabase JWT for DRM handshake (optional, backward-compat)
    minecraftLicense = null, // Microsoft token from Supabase user metadata
  ) {
    // Deep verification one last time
    const status = gameValidator.checkGameStatus(
      appRoot,
      gameId,
      "minecraft",
      version,
    );
    if (status !== "installed") {
      throw new Error(
        `Cannot launch: Build status is ${status}. Please Reinstall/Repair.`,
      );
    }

    const gameDir = path.join(appRoot, "games", gameId);
    this.runningGames.add(gameId);

    // --- Sync Language ---
    this._syncGameLanguage(gameDir, launcherLang);

    sendProgress(mainWindow, {
      status: "launch_progress",
      message: `Starting ${gameId}...`,
    });

    // ─── Dynamic System Optimization ───
    const optimizerSettings = this._getOptimizerSettings(appRoot);
    let memoryOpts = { max: "4G", min: "2G" };
    let extraJvmArgs = [];

    if (optimizerSettings.enabled) {
      const optimizer = new SystemOptimizer(gameDir);
      // Detect monitor refresh rate so the frame limiter can cap FPS exactly at Hz.
      // VSync is always OFF — the software frame cap prevents tearing with less latency.
      let monitorHz = 0;
      try {
        const { screen } = require("electron");
        const display = screen.getPrimaryDisplay();
        monitorHz = display.displayFrequency || display.refreshRate || 0;
        if (monitorHz > 0) {
          console.log(`[LaunchManager] Monitor detected: ${monitorHz} Hz`);
        }
      } catch (e) {
        console.warn(
          "[LaunchManager] Could not detect monitor refresh rate:",
          e.message,
        );
      }
      const result = optimizer.applyOptimization(monitorHz);

      memoryOpts = {
        max: `${result.memory.max}M`,
        min: `${result.memory.min}M`,
      };
      extraJvmArgs = result.jvmArgs;

      if (result.isMemoryCritical) {
        console.warn(
          "[LaunchManager] WARNING: System memory is critically low!",
        );
        sendProgress(mainWindow, {
          status: "warning",
          message: `Warning: Low system memory (${(result.specs.freeMem / 1024 / 1024 / 1024).toFixed(1)}GB free). Performance may be affected.`,
        });
      }

      console.log(
        `[LaunchManager] Optimizer: Tier=${result.tier?.toUpperCase()} | RAM ${memoryOpts.min}-${memoryOpts.max} | JVM args: ${extraJvmArgs.length}`,
      );
    } else {
      // ─── Manual mode ───
      // When the optimizer is OFF the user configures options.txt and
      // sodium-options.json themselves — the launcher must NOT touch them.
      // We only control the JVM process (memory allocation + thread count).
      const rawRam = optimizerSettings.manualRam || 4;
      const ramMB = Math.floor(rawRam * 1024);
      memoryOpts = {
        max: `${ramMB}M`,
        min: "2048M", // Keep min at 2G for stability
      };

      // Sanitize and split JVM args
      const rawArgs = optimizerSettings.manualJvmArgs || "";
      // Strip -Xmx and -Xms to prevent crashes, then split by whitespace
      const userArgs = rawArgs
        .split(/\s+/)
        .filter(
          (arg) =>
            arg.trim() !== "" &&
            !arg.toLowerCase().startsWith("-xmx") &&
            !arg.toLowerCase().startsWith("-xms"),
        );

      extraJvmArgs.push(...userArgs);

      const manualThreads = optimizerSettings.manualThreads || 0;
      if (manualThreads > 0) {
        extraJvmArgs.push(`-XX:ActiveProcessorCount=${manualThreads}`);
        this._syncSodiumThreads(gameDir, manualThreads);
      }

      console.log(
        `[LaunchManager] Manual: RAM ${memoryOpts.max} | Threads: ${manualThreads || "Auto"} | JVM args count: ${extraJvmArgs.length}`,
      );
    }

    // Determine correct version folder name
    let versionDirStr = version;
    if (modLoader === "fabric" && modLoaderVersion) {
      versionDirStr = `${gameId}_${version}`; // e.g., lost-death-1_1.20.1
    }

    let opts = {
      clientPackage: null,
      authorization: minecraftLicense ? minecraftLicense.token : {
        // Offline Auth fallback
        access_token: "0",
        client_token: "0",
        uuid: "0",
        name: "Player",
        user_properties: "{}",
        meta: {
          type: "mojang",
          demo: false,
        },
      },
      root: gameDir,
      version: {
        // number = base vanilla version, custom = our Fabric profile folder/json name
        number: version,
        custom: versionDirStr !== version ? versionDirStr : undefined,
        type: "release",
      },
      memory: memoryOpts,
    };

    // NOTE: Optimizer's GC JVM args (UseG1GC, etc.) cannot be passed via customArgs
    // because MCLC already sets -XX:-UseAdaptiveSizePolicy which conflicts with G1GC flags.
    // These args were previously in overrides.jvmArguments (silently ignored by MCLC),
    // so removing them now has no effect on game performance vs the previous behavior.
    // Memory allocation (memoryOpts) is still applied correctly via opts.memory.

    // Enforce bundled Azul Zulu Java 17 — no system Java fallback.
    // Using system Java risks wrong version (Java 8/21) or missing installation entirely.
    {
      const standaloneJavaPath = javaManager.getJavaExePath(appRoot, gameId);
      if (!fs.existsSync(standaloneJavaPath)) {
        throw new Error(
          `Java 17 not found at: ${standaloneJavaPath}\n` +
          `Please reinstall the game so Java can be downloaded automatically.`,
        );
      }
      opts.javaPath = standaloneJavaPath;
      console.log(`[LaunchManager] Using bundled Java 17: ${standaloneJavaPath}`);
    }

    // Wire MCLC progress events back to frontend.
    // Remove any listeners from a previous launch first to prevent memory leaks
    // and duplicate event handling if the game is re-launched in the same session.
    this.launcher.removeAllListeners("debug");
    this.launcher.removeAllListeners("data");
    this.launcher.removeAllListeners("close");

    this.launcher.on("debug", (e) => console.log(e));
    this.launcher.on("data", (e) => {
      sendProgress(mainWindow, {
        status: "launch_progress",
        message: e.toString(),
      });
    });
    this.launcher.on("close", (e) => {
      if (this.runningGames.has(gameId)) {
        this.runningGames.delete(gameId);
      }
      this.gameProcesses.delete(gameId);
      sendProgress(mainWindow, {
        status: "closed",
        message: `Minecraft has closed.`,
        code: e,
        gameId: gameId,
      });

      // Revert Discord Status to Launcher
      discordRPC.setLauncherPresence();
    });

    sendProgress(mainWindow, {
      status: "launch_progress",
      message: "Executing JVM...",
    });

    // ─── DRM Handshake ───────────────────────────────────────────────────────
    console.log(`[DRM] sessionToken present: ${!!sessionToken} (length: ${sessionToken?.length ?? 0})`);

    const { port: drmPort } = await this._startDRMServer(sessionToken);
    console.log(`[DRM] TCP server port: ${drmPort}`);

    if (drmPort) {
      // Add DRM port via customArgs — the ONLY way MCLC passes args to the JVM
      opts.customArgs = [...(opts.customArgs || []), `-Dldl.port=${drmPort}`];
      process.env.LDL_SESSION = sessionToken;
      console.log(`[DRM] LDL_SESSION set on process.env, length=${sessionToken.length}`);
      console.log(`[DRM] Injected -Dldl.port=${drmPort} via customArgs`);
    } else {
      console.warn(`[DRM] No port — handshake SKIPPED. sessionToken was ${sessionToken ? 'present' : 'NULL/EMPTY'}`);
    }

    discordRPC.setGamePresence(gameId);
    const gameProc = await this.launcher.launch(opts);
    if (gameProc) {
      this.gameProcesses.set(gameId, gameProc);
      console.log(`[LaunchManager] Stored process for ${gameId} (pid=${gameProc.pid})`);
    }

    // Clean up session token from process env — JVM already inherited it at spawn time
    if (process.env.LDL_SESSION) delete process.env.LDL_SESSION;

    sendProgress(mainWindow, {
      status: "launched",
      message: "Game Launched!",
      progress: 100,
      gameId: gameId,
    });

    // Elevate CPU Priority to HIGH on Windows to ensure smoother frametimes.
    // wmic is deprecated/removed in Windows 11, use PowerShell instead.
    if (process.platform === "win32") {
      setTimeout(() => {
        const psCmd =
          "Get-Process javaw -ErrorAction SilentlyContinue | ForEach-Object { $_.PriorityClass = [System.Diagnostics.ProcessPriorityClass]::High }";
        exec(
          `powershell.exe -NoProfile -NonInteractive -Command "${psCmd}"`,
          (err) => {
            if (!err) {
              console.log(
                "[LaunchManager] Successfully elevated CPU priority of javaw.exe to High.",
              );
            } else {
              console.log(
                "[LaunchManager] Note: Could not elevate CPU priority (can be ignored):",
                err.message,
              );
            }
          },
        );
      }, 5000); // 5 seconds delay to ensure the process is fully registered in the OS
    }
  }
}

module.exports = new LaunchManager();
