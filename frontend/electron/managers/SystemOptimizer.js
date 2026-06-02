const os = require("os");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

// ─────────────────────────────────────────────────────────────────────────────
// HARDWARE TIERS
//
// The optimizer scores the CPU and RAM, picks a tier, then applies the
// matching options.txt / sodium / JVM profile automatically.
//
// Tier    Score   Typical machine
// LOW      0-2    Budget laptop, old PC (≤8 GB, ≤4 threads)
// MEDIUM   3-4    Average gaming PC    (8-16 GB, 6-8 threads)
// HIGH     5-6    Good gaming PC       (16-32 GB, 8-12 threads)
// ULTRA    7-8    Enthusiast PC        (≥32 GB, ≥16 threads)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * DEFAULT profile — nice-looking baseline written when optimizer is OFF.
 * Based on the reference PrismLauncher setup.
 *
 * graphicsMode: 0=Fancy  1=Fast  2=Fabulous  (numbers, NOT strings like "fancy")
 * particles:    0=All    1=Decreased  2=Minimal
 */
const DEFAULT_OPTIONS = {
  enableVsync: "false", // vsync OFF = less input lag
  graphicsMode: "1", // Fast — good quality/perf balance
  particles: "0", // All — full visuals
  entityShadows: "true",
  renderDistance: "10",
  simulationDistance: "8",
  entityDistanceScaling: "0.75",
  biomeBlendRadius: "2",
  renderClouds: '"false"', // quotes are part of MC format
  syncChunkWrites: "true", // safe vanilla default
  mipmapLevels: "4", // sharp textures
  ao: "true", // smooth lighting
  maxFps: "150",
};

/**
 * Per-tier PERFORMANCE profiles.
 * Each tier only lists settings that differ from DEFAULT.
 * The engine merges DEFAULT ← tier overrides when building the final profile.
 *
 * Reasoning per setting:
 *   renderDistance        — chunk rendering is the #1 GPU bottleneck
 *   simulationDistance    — server-side tick load; 6 is enough for single-player
 *   particles             — huge impact in combat / explosions
 *   entityShadows         — shadow projection under every mob; negligible visuals
 *   mipmapLevels          — GPU VRAM & bandwidth; 4→2 saves ~25% texture memory
 *   ao (smooth lighting)  — soft shadows on blocks; costs a CPU pass per chunk
 *   syncChunkWrites       — false = async disk I/O = no main-thread stalls
 *   entityDistanceScaling — how far mobs/players render their full LOD
 *   biomeBlendRadius      — color interpolation across biome borders
 */
const TIER_OVERRIDES = {
  low: {
    // Very weak machine — every FPS counts
    renderDistance: "6",
    simulationDistance: "4",
    particles: "2", // Minimal
    entityShadows: "false",
    mipmapLevels: "1",
    ao: "false",
    syncChunkWrites: "false",
    entityDistanceScaling: "0.5",
    biomeBlendRadius: "0",
    maxFps: "60",
  },
  medium: {
    renderDistance: "8",
    simulationDistance: "6",
    particles: "1", // Decreased
    entityShadows: "false",
    mipmapLevels: "2",
    syncChunkWrites: "false",
    entityDistanceScaling: "0.65",
    biomeBlendRadius: "2",
    maxFps: "120",
  },
  high: {
    renderDistance: "10",
    simulationDistance: "8",
    particles: "1", // Decreased
    entityShadows: "true",
    mipmapLevels: "4",
    syncChunkWrites: "false",
    entityDistanceScaling: "0.85",
    biomeBlendRadius: "2",
    maxFps: "144",
  },
  ultra: {
    // Strong machine — we can afford nice settings + still help FPS
    renderDistance: "12",
    simulationDistance: "10",
    particles: "0", // All
    entityShadows: "true",
    mipmapLevels: "4",
    syncChunkWrites: "false", // async is always better
    entityDistanceScaling: "1.0",
    biomeBlendRadius: "3",
    maxFps: "240",
  },
};

// Max chunk builder threads to give sodium per tier (rest stays for game + OS)
const TIER_SODIUM_THREADS = { low: 1, medium: 2, high: 4, ultra: 6 };

// Sodium advanced: how many frames the CPU renders ahead of the GPU.
// More = smoother frame delivery, more VRAM usage and tiny input lag increase.
const TIER_RENDER_AHEAD = { low: 2, medium: 3, high: 4, ultra: 5 };

// Memory caps per tier (GB)  [min, max]
const TIER_MEMORY_GB = {
  low: [2, 3],
  medium: [2, 5],
  high: [3, 7],
  ultra: [4, 10],
};

// ─────────────────────────────────────────────────────────────────────────────

class SystemOptimizer {
  constructor(gameDir) {
    this.gameDir = gameDir;
    this.optionsFile = path.join(gameDir, "options.txt");
    this.backupFile = path.join(gameDir, "options.txt.bak");
  }

  // ─── Hardware detection ───────────────────────────────────────────────────

  getSystemSpecs() {
    const cpus = os.cpus();
    const totalMem = os.totalmem();
    const freeMem = os.freemem();
    const cpuModel = cpus[0]?.model || "Unknown CPU";
    const threads = cpus.length;
    const speed = cpus[0]?.speed || 2000; // MHz

    let cores = threads;
    try {
      const unique = new Set(cpus.map((c) => c.coreId));
      cores =
        unique.size > 0 && !unique.has(undefined)
          ? unique.size
          : Math.max(1, Math.floor(threads / 2));
    } catch {
      cores = Math.max(1, Math.floor(threads / 2));
    }

    return {
      totalMem, // bytes
      freeMem, // bytes
      totalMemGB: totalMem / 1024 ** 3,
      freeMemGB: freeMem / 1024 ** 3,
      cpuModel,
      cores,
      threads,
      speed,
    };
  }

  /**
   * Score the hardware and return a tier name.
   *
   * RAM score (0–3):  <8→0  ≥8→1  ≥16→2  ≥32→3
   * CPU threads (0–3):<6→0  ≥6→1  ≥8→2   ≥16→3
   * CPU speed  (0–2): <2.5GHz→0  ≥2.5GHz→1  ≥4GHz→2
   *
   * NOTE: os.cpus()[0].speed returns the CURRENT clock on Windows, which
   * can be as low as the base clock when the system is idle. Modern laptop
   * CPUs (e.g. i5-12450HX: 2.4 GHz base, 4.4 GHz boost) would score 0 on
   * the old 3 GHz threshold despite being capable machines.
   * Threshold lowered to 2000 MHz to correctly tier modern hybrid CPUs.
   *
   * Total 0–8 → LOW(0-2) / MEDIUM(3-4) / HIGH(5-6) / ULTRA(7-8)
   */
  _detectTier(specs) {
    let score = 0;

    // RAM (Windows hardware-reserves some memory, so 16GB physically is ~15.8GB OS-visible)
    const memGB = Math.round(specs.totalMemGB);
    if (memGB >= 32) score += 3;
    else if (memGB >= 16) score += 2;
    else if (memGB >= 8) score += 1;

    // CPU threads
    if (specs.threads >= 16) score += 3;
    else if (specs.threads >= 8) score += 2;
    else if (specs.threads >= 6) score += 1;

    // CPU speed
    // Lower threshold: laptop CPUs report base clock (~2.4 GHz) at idle
    // but boost to 4+ GHz under load. 2000 MHz covers all modern hybrid CPUs.
    if (specs.speed >= 4000) score += 2;
    else if (specs.speed >= 2000) score += 1;

    if (score >= 7) return "ultra";
    if (score >= 5) return "high";
    if (score >= 3) return "medium";
    return "low";
  }

  /**
   * Merge DEFAULT_OPTIONS with the tier-specific overrides.
   */
  _buildTierProfile(tier) {
    return { ...DEFAULT_OPTIONS, ...(TIER_OVERRIDES[tier] || {}) };
  }

  // ─── Memory + JVM ─────────────────────────────────────────────────────────

  calculateMemory(tier) {
    const specs = this.getSystemSpecs();
    const [minGB, maxGB] = TIER_MEMORY_GB[tier] || [2, 6];

    // Allocate 60% of free RAM, clamped to the tier range
    const safeGB = specs.freeMemGB * 0.6;
    const allocGB = Math.max(minGB, Math.min(maxGB, Math.floor(safeGB)));

    return {
      min: Math.min(minGB, allocGB) * 1024,
      max: allocGB * 1024,
    };
  }

  getJvmArgs(tier) {
    const specs = this.getSystemSpecs();
    const mem = this.calculateMemory(tier);
    const maxRamGB = Math.floor(mem.max / 1024);

    // NOTE: Launcher ships Java 17.
    // -XX:+ZGenerational / -XX:+ZProactive require Java 21+ and crash on Java 17.
    let gcArgs;

    if (tier === "ultra" || (maxRamGB >= 8 && specs.cores > 6)) {
      gcArgs = [
        "-XX:+UseG1GC",
        "-XX:MaxGCPauseMillis=50",
        "-XX:G1HeapRegionSize=32M",
        "-XX:+ParallelRefProcEnabled",
        "-XX:G1NewSizePercent=30",
        "-XX:G1MaxNewSizePercent=40",
        "-XX:G1ReservePercent=20",
        "-XX:G1MixedGCCountTarget=4",
        "-XX:InitiatingHeapOccupancyPercent=15",
      ];
    } else if (tier === "high" || (specs.cores > 4 && specs.speed >= 2800)) {
      gcArgs = [
        "-XX:+UseG1GC",
        "-XX:MaxGCPauseMillis=80",
        "-XX:G1HeapRegionSize=16M",
        "-XX:+ParallelRefProcEnabled",
        "-XX:G1NewSizePercent=20",
        "-XX:G1MaxNewSizePercent=40",
        "-XX:G1ReservePercent=20",
      ];
    } else if (tier === "medium") {
      gcArgs = [
        "-XX:+UseG1GC",
        "-XX:MaxGCPauseMillis=120",
        "-XX:G1HeapRegionSize=8M",
      ];
    } else {
      // LOW — conservative, avoid GC pressure on weak machines
      gcArgs = ["-XX:+UseSerialGC"];
    }

    const common = [
      "-XX:+DisableExplicitGC",
      "-XX:+AlwaysPreTouch",
      "-XX:+UseStringDeduplication",
      "-Dsun.rmi.dgc.server.gcInterval=2147483646",
      "-Dsun.rmi.dgc.client.gcInterval=2147483646",
      "-Djava.net.preferIPv4Stack=true",
    ];

    // SerialGC doesn't support some flags
    if (tier === "low") {
      return [...gcArgs, "-Djava.net.preferIPv4Stack=true"];
    }

    return [...gcArgs, ...common];
  }

  // ─── options.txt ──────────────────────────────────────────────────────────

  _hash(obj) {
    return crypto.createHash("md5").update(JSON.stringify(obj)).digest("hex");
  }

  _readOptions(keys) {
    if (!fs.existsSync(this.optionsFile)) return null;
    const result = {};
    try {
      for (const line of fs
        .readFileSync(this.optionsFile, "utf-8")
        .split(/\r?\n/)) {
        const idx = line.indexOf(":");
        if (idx === -1) continue;
        const k = line.substring(0, idx);
        if (keys.includes(k)) result[k] = line.substring(idx + 1);
      }
    } catch {
      return null;
    }
    return result;
  }

  /**
   * Apply a key/value profile to options.txt.
   * Hash guard: if the user manually edited any managed key since the last
   * apply, we skip to respect their choice. Otherwise we always re-apply
   * the current profile (catches tier changes, VSync policy updates, etc.).
   */
  _applyOptionsProfile(profile, hashFile) {
    if (!fs.existsSync(this.optionsFile)) return false;

    const keys = Object.keys(profile);
    const current = this._readOptions(keys);

    if (current && fs.existsSync(hashFile)) {
      try {
        const saved = fs.readFileSync(hashFile, "utf-8").trim();
        if (saved !== this._hash(current)) {
          console.log(
            "[Optimizer] options.txt manually changed by user — skipping.",
          );
          return false;
        }
        // Hash matches → no manual change detected → fall through and re-apply
        // (profile may have changed since last write, e.g. VSync policy update)
      } catch {
        /* hash unreadable → re-apply */
      }
    }

    try {
      fs.copyFileSync(this.optionsFile, this.backupFile);

      const lines = fs.readFileSync(this.optionsFile, "utf-8").split(/\r?\n/);
      const out = lines.map((line) => {
        const idx = line.indexOf(":");
        if (idx === -1) return line;
        const k = line.substring(0, idx);
        return profile[k] !== undefined ? `${k}:${profile[k]}` : line;
      });

      fs.writeFileSync(this.optionsFile, out.join("\n"));

      const written = this._readOptions(keys);
      if (written) fs.writeFileSync(hashFile, this._hash(written));

      console.log(
        `[Optimizer] options.txt updated (${path.basename(hashFile)})`,
      );
      return true;
    } catch (e) {
      console.error("[Optimizer] Failed to modify options.txt:", e.message);
      return false;
    }
  }

  // ─── sodium-options.json ──────────────────────────────────────────────────

  _applySodiumOptions(tier) {
    // Sodium stores ALL fields in snake_case — NOT camelCase!
    const SODIUM_FILE = path.join(
      this.gameDir,
      "config",
      "sodium-options.json",
    );
    const SODIUM_HASH = path.join(this.gameDir, ".ldl_sodium_hash");

    if (!fs.existsSync(SODIUM_FILE)) return false;

    try {
      const config = JSON.parse(fs.readFileSync(SODIUM_FILE, "utf-8"));
      if (!config.performance) return false;

      // ── Tier-calculated values ────────────────────────────────────────────
      const specs = this.getSystemSpecs();
      const tierMax = TIER_SODIUM_THREADS[tier] || 2;
      const threads = Math.max(1, Math.min(tierMax, specs.threads - 2));
      const renderAhead = TIER_RENDER_AHEAD[tier] || 3;
      // Use persistent buffer mapping for all except very weak machines.
      // On LOW tier the GPU might be old/integrated and can have driver issues.
      const persistentMapping = tier !== "low";

      // ── Fields we want to control ─────────────────────────────────────────
      // Collect current values of EVERY field we touch so the hash guard
      // can detect if the user changed any of them in-game.
      const snapshot = {
        // performance section
        chunk_builder_threads: config.performance.chunk_builder_threads,
        always_defer_chunk_updates_v2:
          config.performance.always_defer_chunk_updates_v2,
        use_block_face_culling: config.performance.use_block_face_culling,
        use_fog_occlusion: config.performance.use_fog_occlusion,
        use_entity_culling: config.performance.use_entity_culling,
        animate_only_visible_textures:
          config.performance.animate_only_visible_textures,
        use_no_error_g_l_context: config.performance.use_no_error_g_l_context,
        // advanced section
        use_advanced_staging_buffers:
          config.advanced?.use_advanced_staging_buffers,
        cpu_render_ahead_limit: config.advanced?.cpu_render_ahead_limit,
      };

      // ── Always apply — optimizer wins over manual in-game changes ───────────
      // The sodium hash is only used to DETECT changes for logging purposes.
      // We don't skip; the optimizer profile always takes precedence.
      if (fs.existsSync(SODIUM_HASH)) {
        try {
          if (
            fs.readFileSync(SODIUM_HASH, "utf-8").trim() !==
            this._hash(snapshot)
          ) {
            console.log(
              "[Optimizer] sodium-options.json was manually changed — overwriting with optimizer profile.",
            );
          }
        } catch {
          /* hash unreadable → apply anyway */
        }
      }

      // ── Apply ─────────────────────────────────────────────────────────────
      let changed = false;
      const setPerf = (key, val) => {
        if (config.performance[key] !== val) {
          config.performance[key] = val;
          changed = true;
        }
      };
      const setAdv = (key, val) => {
        if (!config.advanced) {
          config.advanced = {};
        }
        if (config.advanced[key] !== val) {
          config.advanced[key] = val;
          changed = true;
        }
      };

      // Performance section
      setPerf("chunk_builder_threads", threads);
      setPerf("always_defer_chunk_updates_v2", false); // false = no defer → smoother frames
      setPerf("use_block_face_culling", true); // skip hidden block faces
      setPerf("use_fog_occlusion", true); // cull geometry behind fog
      setPerf("use_entity_culling", true); // skip invisible entities
      setPerf("animate_only_visible_textures", true); // only animate what player sees
      setPerf("use_no_error_g_l_context", true); // skip GL error checks

      // Advanced section
      setAdv("use_advanced_staging_buffers", persistentMapping); // persistent buffer mapping
      setAdv("cpu_render_ahead_limit", renderAhead); // frames CPU renders ahead

      if (changed || !fs.existsSync(SODIUM_HASH)) {
        fs.writeFileSync(SODIUM_FILE, JSON.stringify(config, null, 4));

        // Re-read and hash what we actually wrote
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
            config.advanced.use_advanced_staging_buffers,
          cpu_render_ahead_limit: config.advanced.cpu_render_ahead_limit,
        };
        fs.writeFileSync(SODIUM_HASH, this._hash(written));

        console.log(
          `[Optimizer] Sodium: threads=${threads}, render_ahead=${renderAhead},` +
            ` persistent_mapping=${persistentMapping}, all culling=ON, defer=OFF`,
        );
      }

      return true;
    } catch (e) {
      console.error(
        "[Optimizer] Failed to modify sodium-options.json:",
        e.message,
      );
      return false;
    }
  }

  // ─── Public API ───────────────────────────────────────────────────────────

  /**
   * Write the DEFAULT (nice visuals) config.
   * Called when the optimizer toggle is OFF.
   * Still optimises Sodium chunk threads (no visual trade-off).
   * @param {number} monitorHz  Primary display refresh rate in Hz (0 = unknown)
   */
  applyDefaultConfig(monitorHz = 0) {
    // Clone DEFAULT_OPTIONS so we never mutate the module-level constant.
    const profile = { ...DEFAULT_OPTIONS };

    // VSync is always OFF — we cap FPS at monitor Hz instead to prevent tearing
    // with zero latency penalty (software frame limiter acts like hardware sync).
    profile.enableVsync = "false";

    if (monitorHz > 0) {
      profile.maxFps = String(monitorHz);
      console.log(`[Optimizer] Default config: maxFps capped at ${monitorHz} Hz (monitor)`);
    } else {
      console.log(`[Optimizer] Default config: maxFps = ${profile.maxFps} (fallback, monitor Hz unknown)`);
    }

    const hashFile = path.join(this.gameDir, ".ldl_default_hash");
    this._applyOptionsProfile(profile, hashFile);
    // Even in default mode, set optimal sodium threads (pure perf, no visual cost)
    const specs = this.getSystemSpecs();
    const tier = this._detectTier(specs);
    this._applySodiumOptions(tier);
    console.log("[Optimizer] Default config applied.");
  }

  /**
   * Detect hardware tier and apply the matching performance profile.
   * Called when the optimizer toggle is ON.
   *
   * VSync strategy: always OFF.
   * Tearing is prevented by capping maxFps at the monitor's refresh rate.
   * A software frame limiter at ~Hz is visually equivalent to VSync with
   * zero added input latency — this is the approach used by competitive titles.
   *
   * @param {number} monitorHz  Primary display refresh rate in Hz (0 = unknown, use tier default)
   * @returns {{ memory, jvmArgs, specs, tier, monitorHz, isMemoryCritical }}
   */
  applyOptimization(monitorHz = 0) {
    const specs = this.getSystemSpecs();
    const tier = this._detectTier(specs);

    console.log(
      `[Optimizer] ${specs.cpuModel}\n` +
        `[Optimizer] ${specs.cores}C / ${specs.threads}T @ ${specs.speed}MHz | ` +
        `RAM: ${specs.totalMemGB.toFixed(1)} GB total, ${specs.freeMemGB.toFixed(1)} GB free\n` +
        `[Optimizer] Tier: ${tier.toUpperCase()} | Monitor: ${monitorHz > 0 ? monitorHz + " Hz" : "unknown (using tier default)"}`,
    );

    const memory = this.calculateMemory(tier);
    const jvmArgs = this.getJvmArgs(tier);

    // Build the options profile for this tier.
    const profile = this._buildTierProfile(tier);

    // VSync — always OFF. Tearing is eliminated by capping FPS at monitor Hz.
    // Minecraft's frame limiter (and Sodium's) fires precisely at the target,
    // effectively giving VSync-level smoothness with half the input latency.
    profile.enableVsync = "false";
    console.log("[Optimizer] VSync=OFF (FPS cap at monitor Hz prevents tearing)");

    // maxFps — always match the monitor's refresh rate if known.
    // Never render more frames than the screen can show (wasted GPU work).
    if (monitorHz > 0) {
      profile.maxFps = String(monitorHz);
      console.log(`[Optimizer] maxFps = ${monitorHz} (monitor Hz)`);
    } else {
      console.log(
        `[Optimizer] maxFps = ${profile.maxFps} (tier default, monitor Hz unknown)`,
      );
    }

    const hashFile = path.join(this.gameDir, `.ldl_perf_${tier}_hash`);
    this._applyOptionsProfile(profile, hashFile);
    this._applySodiumOptions(tier);

    console.log(`[Optimizer] Allocating ${memory.max / 1024} GB to JVM`);

    return {
      memory,
      jvmArgs,
      specs,
      tier,
      monitorHz,
      isMemoryCritical: specs.freeMemGB < 2.5,
    };
  }
}

module.exports = SystemOptimizer;
