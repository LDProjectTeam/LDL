import BaseModal from "./BaseModal";
import { useI18n } from "../../i18n";
import { useState, useEffect } from "react";

interface SettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export default function SettingsModal({ isOpen, onClose }: SettingsModalProps) {
  const { t, lang, setLanguage, languages } = useI18n();

  const [autoStart, setAutoStart] = useState(false);
  const [hwAccel, setHwAccel] = useState(true);
  const [loaded, setLoaded] = useState(false);
  const [restartNeeded, setRestartNeeded] = useState(false);
  const [resetSuccess, setResetSuccess] = useState(false);
  const [installDirectory, setInstallDirectory] = useState<string>(
    "C:\\LDLauncher\\Games",
  );

  const [optEnabled, setOptEnabled] = useState(true);
  const [manualRam, setManualRam] = useState(4);
  const [manualThreads, setManualThreads] = useState(0);
  const [sysMaxRam, setSysMaxRam] = useState(16);
  const [sysCores, setSysCores] = useState(8);
  const [detectedTier, setDetectedTier] = useState<string | null>(null);
  const [monitorHz, setMonitorHz] = useState<number>(0);

  // Load real settings from Electron when modal opens.
  // Reset `loaded` when the modal closes so that settings are always
  // freshly read from disk the next time the user opens the modal.
  useEffect(() => {
    if (!isOpen) {
      setLoaded(false);
      return;
    }
    if (loaded) return;
    if (isOpen && !loaded) {
      window.electronAPI?.getGeneralSettings?.().then((s: any) => {
        if (s) {
          setAutoStart(!!s.autostart);
          setHwAccel(s.hwAcceleration !== false);
          if (s.installDirectory) {
            setInstallDirectory(s.installDirectory);
          }
        }
        setLoaded(true);
      });

      window.electronAPI?.getSystemSpecs?.().then((specs: any) => {
        if (specs) {
          setSysMaxRam(Math.floor(specs.totalMemGB));
          setSysCores(specs.cores);
          if (specs.tier) setDetectedTier(specs.tier);
          if (specs.refreshRate) setMonitorHz(specs.refreshRate);
        }
      });

      window.electronAPI?.getOptimizerSettings?.().then((opt: any) => {
        if (opt) {
          setOptEnabled(opt.enabled !== false);
          setManualRam(opt.manualRam || 4);
          setManualThreads(opt.manualThreads || 0);
        }
      });
    }
  }, [isOpen]);

  const handleOptChange = async (key: string, value: any) => {
    let newOpt = { enabled: optEnabled, manualRam, manualThreads };
    if (key === "enabled") {
      setOptEnabled(value);
      newOpt.enabled = value;
    }
    if (key === "manualRam") {
      setManualRam(value);
      newOpt.manualRam = value;
    }
    if (key === "manualThreads") {
      setManualThreads(value);
      newOpt.manualThreads = value;
    }
    await window.electronAPI?.setOptimizerSettings?.(newOpt);
  };

  const handleToggle = async (key: string, value: boolean) => {
    if (key === "autoStart") setAutoStart(value);
    if (key === "hwAccel") {
      setHwAccel(value);
      setRestartNeeded(true);
    }
    // Save to file
    const settings: Record<string, any> = {
      autostart: key === "autoStart" ? value : autoStart,
      hwAcceleration: key === "hwAccel" ? value : hwAccel,
      installDirectory: installDirectory,
    };
    await window.electronAPI?.setGeneralSettings?.(settings);
  };

  const handleResetGameSettings = async () => {
    if (window.electronAPI?.resetGameSettings) {
      const result = await window.electronAPI.resetGameSettings();
      if (result && result.success) {
        setResetSuccess(true);
        setTimeout(() => setResetSuccess(false), 3000);
      }
    }
  };

  const handleSelectDirectory = async () => {
    if (window.electronAPI?.selectDirectory) {
      const result = await window.electronAPI.selectDirectory(installDirectory);
      if (result && !result.canceled && result.filePaths.length > 0) {
        const newPath = result.filePaths[0];
        setInstallDirectory(newPath);

        // Save immediately
        const settings: Record<string, any> = {
          autostart: autoStart,
          hwAcceleration: hwAccel,
          installDirectory: newPath,
        };
        await window.electronAPI?.setGeneralSettings?.(settings);
      }
    }
  };

  const SettingToggle = ({
    label,
    description,
    checked,
    onChange,
    badge,
  }: {
    label: string;
    description: string;
    checked: boolean;
    onChange: (val: boolean) => void;
    badge?: string;
  }) => (
    <div className="flex items-start justify-between py-4 border-b border-crt-accent/30 last:border-0">
      <div className="flex flex-col pr-8">
        <div className="flex items-center gap-2">
          <span className="text-[14px] font-bold text-crt-glow text-glow-subtle">
            {label}
          </span>
          {badge && (
            <span className="text-[10px] font-bold px-1.5 py-0.5 rounded bg-crt-glow/20 text-crt-glow text-glow border border-crt-glow uppercase tracking-wider">
              {badge}
            </span>
          )}
        </div>
        <span className="text-[12px] text-crt-accent mt-1 leading-relaxed">
          {description}
        </span>
      </div>
      <button
        className={`relative inline-flex h-6 w-11 flex-shrink-0 cursor-pointer rounded-full border-2 border-transparent transition-colors duration-200 ease-in-out focus:outline-none ${checked ? "bg-crt-glow" : "bg-crt-accent/30"}`}
        onClick={() => onChange(!checked)}
      >
        <span
          className={`pointer-events-none inline-block h-5 w-5 transform rounded-full bg-white shadow ring-0 transition duration-200 ease-in-out ${checked ? "translate-x-5" : "translate-x-0"}`}
        />
      </button>
    </div>
  );

  return (
    <BaseModal
      isOpen={isOpen}
      onClose={onClose}
      title={t.settings}
    >
      <div className="flex flex-col space-y-2">
        {restartNeeded && (
          <div className="mb-2 px-4 py-3 rounded-md bg-amber-500/10 border border-amber-500/30 text-amber-400 text-[12px] font-medium flex items-center gap-2">
            <svg
              width="14"
              height="14"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2.5"
            >
              <path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z" />
              <line x1="12" y1="9" x2="12" y2="13" />
              <line x1="12" y1="17" x2="12.01" y2="17" />
            </svg>
            {t.settingsRestartNotice}
          </div>
        )}

        <div className="mb-4">
          <h3 className="text-xs font-bold uppercase tracking-wider text-crt-glow text-glow mb-2">
            {t.settingsGeneral}
          </h3>
          <div className="flex flex-col py-4 border-b border-crt-accent/30 last:border-0">
            <div className="flex flex-col mb-3">
              <span className="text-[14px] font-bold text-crt-glow text-glow-subtle">
                {t.appLanguage}
              </span>
            </div>
            <div className="flex items-center gap-3">
              {languages.map((l) => (
                <button
                  key={l.code}
                  onClick={() => setLanguage(l.code)}
                  className={`px-4 py-2 border rounded text-[13px] font-bold transition-colors ${
                    lang === l.code
                      ? "bg-crt-glow/20 border-crt-glow text-crt-glow shadow-[0_0_10px_rgba(255,215,0,0.3)]"
                      : "bg-black/40 border-crt-accent/30 text-crt-accent hover:bg-crt-glow/10 hover:text-crt-glow"
                  }`}
                >
                  {l.flag} {l.label}
                </button>
              ))}
            </div>
          </div>
          <SettingToggle
            label={t.settingsAutoStart}
            description={t.settingsAutoStartDesc}
            checked={autoStart}
            onChange={(v) => handleToggle("autoStart", v)}
          />
          <SettingToggle
            label={t.settingsHwAccel}
            description={t.settingsHwAccelDesc}
            checked={hwAccel}
            onChange={(v) => handleToggle("hwAccel", v)}
            badge={t.settingsRestartBadge}
          />
        </div>
        <div className="mb-4">
          <h3 className="text-xs font-bold uppercase tracking-wider text-crt-glow text-glow mb-2">
            {t.settingsOptimizer}
          </h3>
          <SettingToggle
            label={t.settingsAutoOptimize}
            description={t.settingsAutoOptimizeDesc}
            checked={optEnabled}
            onChange={(v) => handleOptChange("enabled", v)}
          />

          {/* Hardware tier indicator — visible when optimizer is ON */}
          {optEnabled &&
            detectedTier &&
            (() => {
              const tierConfig: Record<
                string,
                { label: string; color: string; bg: string; icon: string }
              > = {
                low: {
                  label: "LOW",
                  color: "#f97316",
                  bg: "rgba(249,115,22,0.1)",
                  icon: "⚡",
                },
                medium: {
                  label: "MEDIUM",
                  color: "#eab308",
                  bg: "rgba(234,179,8,0.1)",
                  icon: "⚡⚡",
                },
                high: {
                  label: "HIGH",
                  color: "#22c55e",
                  bg: "rgba(34,197,94,0.1)",
                  icon: "⚡⚡⚡",
                },
                ultra: {
                  label: "ULTRA",
                  color: "#8b5cf6",
                  bg: "rgba(139,92,246,0.1)",
                  icon: "⚡⚡⚡⚡",
                },
              };
              const cfg = tierConfig[detectedTier] ?? tierConfig.medium;
              const tierDesc: Record<string, string> = {
                low: t.tierLow,
                medium: t.tierMedium,
                high: t.tierHigh,
                ultra: t.tierUltra,
              };
              return (
                <div
                  className="mt-2 mx-1 px-4 py-3 rounded-lg flex items-center justify-between"
                  style={{
                    background: cfg.bg,
                    border: `1px solid ${cfg.color}40`,
                  }}
                >
                  <div className="flex flex-col">
                    <span
                      className="text-[11px] font-bold uppercase tracking-widest"
                      style={{ color: cfg.color }}
                    >
                      {t.tierDetected}
                    </span>
                    <span className="text-[12px] text-crt-accent mt-0.5">
                      {tierDesc[detectedTier]}
                    </span>
                    {monitorHz > 0 && (
                      <span className="text-[11px] text-crt-accent/70 mt-0.5">
                        {t.tierMonitor}: {monitorHz} Hz → maxFps = {monitorHz}
                      </span>
                    )}
                    <span className="text-[11px] mt-0.5" style={{ color: "#22c55e" }}>
                      ✔ VSync {t.vsyncOff} — FPS {t.fpsCapped}
                    </span>
                  </div>
                  <div
                    className="px-3 py-1.5 rounded-md text-[13px] font-black tracking-wider"
                    style={{
                      background: cfg.bg,
                      border: `1px solid ${cfg.color}`,
                      color: cfg.color,
                    }}
                  >
                    {cfg.icon} {cfg.label}
                  </div>
                </div>
              );
            })()}

          {!optEnabled && (
            <div className="flex flex-col py-4 border-b border-crt-accent/30 last:border-0 pl-4 border-l-2 border-l-crt-glow/50 ml-2 mt-2 space-y-4">
              <div className="flex flex-col">
                <span className="text-[14px] font-bold text-crt-glow text-glow-subtle">
                  {t.settingsManualRam}: {manualRam} ГБ
                </span>
                <span className="text-[12px] text-crt-accent mt-1 leading-relaxed mb-2">
                  {t.settingsManualRamDesc}
                </span>
                <input
                  type="range"
                  min="2"
                  max={sysMaxRam}
                  value={manualRam}
                  onChange={(e) =>
                    handleOptChange("manualRam", Number(e.target.value))
                  }
                  className="w-full accent-crt-glow cursor-pointer"
                />
              </div>
              <div className="flex flex-col">
                <span className="text-[14px] font-bold text-crt-glow text-glow-subtle">
                  {t.settingsManualThreads}:{" "}
                  {manualThreads === 0 ? t.autoStr : manualThreads}
                </span>
                <span className="text-[12px] text-crt-accent mt-1 leading-relaxed mb-2">
                  {t.settingsManualThreadsDesc}
                </span>
                <input
                  type="range"
                  min="0"
                  max={sysCores}
                  value={manualThreads}
                  onChange={(e) =>
                    handleOptChange("manualThreads", Number(e.target.value))
                  }
                  className="w-full accent-crt-glow cursor-pointer"
                />
              </div>
            </div>
          )}

          <div className="mt-4 pt-4 border-t border-crt-accent/30">
            <div className="flex flex-col mb-3">
              <span className="text-[14px] font-bold text-crt-glow text-glow-subtle">
                {t.settingsResetGameSettings}
              </span>
              <span className="text-[12px] text-crt-accent mt-1 leading-relaxed">
                {t.settingsResetGameSettingsDesc}
              </span>
            </div>
            <button
              onClick={handleResetGameSettings}
              className={`px-4 py-2 border rounded text-[13px] font-bold transition-colors w-full sm:w-auto ${
                resetSuccess
                  ? "bg-green-500/20 border-green-500 text-green-400"
                  : "bg-red-500/10 border-red-500/50 text-red-400 hover:bg-red-500/20 hover:text-red-300"
              }`}
            >
              {resetSuccess
                ? t.settingsResetSuccess
                : t.settingsResetGameSettings}
            </button>
          </div>
        </div>

        <div className="mb-4">
          <h3 className="text-xs font-bold uppercase tracking-wider text-crt-glow text-glow mb-2">
            {t.settingsDownloads}
          </h3>
          <div className="flex flex-col py-4 border-b border-crt-accent/30 last:border-0">
            <div className="flex flex-col mb-3">
              <span className="text-[14px] font-bold text-crt-glow text-glow-subtle">
                {t.settingsInstallDir}
              </span>
              <span className="text-[12px] text-crt-accent mt-1 leading-relaxed">
                {t.settingsInstallDirDesc}
              </span>
            </div>
            <div className="flex items-center gap-3">
              <div
                className="flex-1 bg-black/40 border border-crt-accent/30 rounded px-3 py-2 text-[13px] text-white font-mono truncate select-text cursor-text"
                title={installDirectory}
              >
                {installDirectory}
              </div>
              <button
                onClick={handleSelectDirectory}
                className="px-4 py-2 bg-crt-glow/10 hover:bg-crt-glow/20 border border-crt-glow/50 rounded text-[13px] font-bold text-crt-glow transition-colors whitespace-nowrap"
              >
                {t.settingsChangeDir}
              </button>
            </div>
          </div>
        </div>
      </div>
    </BaseModal>
  );
}
