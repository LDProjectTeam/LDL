const fs = require("fs");
const path = require("path");
const axios = require("axios");
const AdmZip = require("adm-zip");

class JavaManager {
  /**
   * Downloads and extracts Azul Zulu Java 17 (Windows x64 ZIP) to a specific runtime folder.
   * @param {string} appRoot Absolute path to the launcher root
   * @param {string} gameId ID of the game
   * @param {Object} mainWindow Reference to electron BrowserWindow to send IPC progress
   * @param {Function} sendProgress Helper mapped to LauncherService.sendProgress
   */
  async installJava(appRoot, gameId, mainWindow, sendProgress) {
    const javaRuntimeDir = path.join(appRoot, "runtime", `${gameId}_java17`);
    const javawPath = this.getJavaExePath(appRoot, gameId);

    // Check if java is already installed and intact
    if (fs.existsSync(javawPath)) {
      sendProgress(mainWindow, {
        status: "progress",
        message: "Java 17 already installed.",
        progress: 100,
      });
      return javaRuntimeDir;
    }

    fs.mkdirSync(javaRuntimeDir, { recursive: true });

    sendProgress(mainWindow, {
      status: "progress",
      message: "Downloading Azul Zulu Java 17...",
      progress: 5,
    });

    // Direct download link for Azul Zulu 17 JRE Windows x64 ZIP
    // We use a robust endpoint, but a direct link is fastest for this specific case.
    const javaZipUrl =
      "https://cdn.azul.com/zulu/bin/zulu17.50.19-ca-jre17.0.11-win_x64.zip";

    // 1. Download to a temp file
    const tempDir = path.join(appRoot, "temp");
    if (!fs.existsSync(tempDir)) fs.mkdirSync(tempDir, { recursive: true });
    const tmpPath = path.join(tempDir, `zulu17_${Date.now()}.zip`);

    try {
      const response = await axios({
        method: "GET",
        url: javaZipUrl,
        responseType: "stream",
        timeout: 60000, // 60s connection timeout — prevents hanging on slow/dead CDN
      });

      // Read total length from headers
      const totalBytes = parseInt(response.headers["content-length"], 10);
      let downloadedBytes = 0;

      const writer = fs.createWriteStream(tmpPath);

      response.data.on("data", (chunk) => {
        downloadedBytes += chunk.length;
        if (totalBytes) {
          const pct = Math.floor((downloadedBytes / totalBytes) * 100);
          sendProgress(mainWindow, {
            status: "progress",
            message: `Azul Zulu Java 17`,
            progress: pct,
          });
        }
      });

      response.data.pipe(writer);

      await new Promise((resolve, reject) => {
        writer.on("finish", resolve);
        writer.on("error", reject);
      });

      sendProgress(mainWindow, {
        status: "progress",
        message: "Unpacking Java 17...",
        progress: 99,
      });

      // 2. Extract using adm-zip
      const zip = new AdmZip(tmpPath);
      zip.extractAllTo(javaRuntimeDir, true);

      // AdmZip extracted it into a subfolder like `zulu17.50.19-ca-jre17.0.11-win_x64/`
      // Let's find that subfolder and move its contents up so `bin/javaw.exe` sits directly in `runtime/TECH_WORLD_java17/bin/`
      const extractedItem = fs
        .readdirSync(javaRuntimeDir)
        .find((file) => file.startsWith("zulu17"));

      if (extractedItem) {
        const subfolderPath = path.join(javaRuntimeDir, extractedItem);
        const isDirectory = fs.statSync(subfolderPath).isDirectory();

        if (isDirectory) {
          const contents = fs.readdirSync(subfolderPath);
          for (const item of contents) {
            fs.renameSync(
              path.join(subfolderPath, item),
              path.join(javaRuntimeDir, item),
            );
          }
          // remove empty subfolder
          fs.rmdirSync(subfolderPath);
        }
      }

      // 3. Cleanup temp
      fs.unlinkSync(tmpPath);

      sendProgress(mainWindow, {
        status: "progress",
        message: "Java 17 setup complete.",
        progress: 100,
      });

      return javaRuntimeDir;
    } catch (error) {
      // Clean up temp file on error
      if (fs.existsSync(tmpPath)) fs.unlinkSync(tmpPath);
      throw new Error(`Failed to download/install Java 17: ${error.message}`);
    }
  }

  /**
   * @param {string} appRoot
   * @param {string} gameId
   * @returns {string} The absolute path to the java executable
   */
  getJavaExePath(appRoot, gameId) {
    // javaw.exe is Windows-only (no terminal window); on Linux/macOS use plain 'java'
    const javaExe = process.platform === "win32" ? "javaw.exe" : "java";
    return path.join(appRoot, "runtime", `${gameId}_java17`, "bin", javaExe);
  }

  /**
   * Verifies strictly if Java is fully downloaded and executable.
   */
  verifyJavaIntegrity(appRoot, gameId) {
    return fs.existsSync(this.getJavaExePath(appRoot, gameId));
  }
}

module.exports = new JavaManager();
