#!/usr/bin/env node
/**
 * LDLauncher Interactive Release Builder
 * Запрашивает версию, обновляет package.json, собирает .exe и portable.
 * 
 * Запуск: npm run release:build
 */

const { execSync } = require("child_process");
const readline = require("readline");
const fs = require("fs");
const path = require("path");

const PKG_PATH = path.join(__dirname, "..", "package.json");

// ── Helpers ────────────────────────────────────────────────────────────────────
function readPkg() {
  return JSON.parse(fs.readFileSync(PKG_PATH, "utf-8"));
}

function writePkg(pkg) {
  fs.writeFileSync(PKG_PATH, JSON.stringify(pkg, null, 2) + "\n");
}

function isValidVersion(v) {
  return /^\d+\.\d+\.\d+$/.test(v.trim());
}

function run(cmd) {
  console.log(`\n▶  ${cmd}`);
  execSync(cmd, { stdio: "inherit", cwd: path.join(__dirname, "..") });
}

function colorize(code, text) {
  return `\x1b[${code}m${text}\x1b[0m`;
}
const green  = (t) => colorize("32;1", t);
const yellow = (t) => colorize("33;1", t);
const cyan   = (t) => colorize("36;1", t);
const dim    = (t) => colorize("2",    t);
const red    = (t) => colorize("31;1", t);

// ── Main ───────────────────────────────────────────────────────────────────────
async function main() {
  const pkg = readPkg();
  const currentVersion = pkg.version;

  console.log("");
  console.log(cyan("╔════════════════════════════════════════╗"));
  console.log(cyan("║   LDLauncher — Interactive Builder     ║"));
  console.log(cyan("╚════════════════════════════════════════╝"));
  console.log(dim(`  Текущая версия в package.json: ${yellow(currentVersion)}`));
  console.log("");

  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });

  const ask = (question) =>
    new Promise((resolve) => rl.question(question, resolve));

  // 1. Спросить версию
  let newVersion;
  while (true) {
    const input = await ask(
      `  Введи новую версию ${dim(`(enter = оставить ${currentVersion})`)} : `
    );
    const trimmed = input.trim();

    if (trimmed === "") {
      newVersion = currentVersion;
      break;
    }

    if (isValidVersion(trimmed)) {
      newVersion = trimmed;
      break;
    }

    console.log(red("  ✗ Неверный формат. Нужно: X.Y.Z  (напр. 3.1.0)"));
  }

  // 2. Подтверждение
  console.log("");
  if (newVersion !== currentVersion) {
    console.log(`  Версия будет изменена: ${yellow(currentVersion)} → ${green(newVersion)}`);
  } else {
    console.log(`  Версия без изменений: ${green(newVersion)}`);
  }
  console.log("");

  const confirm = await ask("  Начать сборку? [Y/n] : ");
  rl.close();

  if (confirm.trim().toLowerCase() === "n") {
    console.log(dim("\n  Сборка отменена.\n"));
    process.exit(0);
  }

  // 3. Обновить package.json
  if (newVersion !== currentVersion) {
    pkg.version = newVersion;
    writePkg(pkg);
    console.log(green(`\n  ✓ package.json обновлён: ${newVersion}`));
  }

  // 4. Сборка
  console.log(cyan("\n─── Сборка фронтенда (tsc + vite) ────────────────"));
  run("npm run build");

  console.log(cyan("\n─── Сборка установщика (electron-builder) ─────────"));
  run("electron-builder");

  console.log("");
  console.log(green("╔════════════════════════════════════════╗"));
  console.log(green("║   ✓ Сборка завершена успешно!          ║"));
  console.log(green("╚════════════════════════════════════════╝"));
  console.log(dim(`  Версия: ${newVersion}`));
  console.log(dim(`  Файлы: LDLauncher_Test_Build/`));
  console.log("");
  console.log("  Следующий шаг:");
  console.log(`  ${cyan("GitHub")} → Draft new release → tag: ${yellow(newVersion)} → загрузи LDLauncher_Setup.exe`);
  console.log("");
}

main().catch((err) => {
  console.error(red("\n  ✗ Ошибка сборки:"), err.message);
  process.exit(1);
});
