import { useEffect, useState, useRef } from "react";
import { motion, AnimatePresence } from "framer-motion";

// ─── Types ────────────────────────────────────────────────────────────────────
type UpdateStage =
  | "checking"   // Connecting to GitHub, spinner
  | "latest"     // Already up to date, auto-dismiss
  | "available"  // New version found, show prompt
  | "downloading"// Progress bar active
  | "ready"      // Downloaded, launching installer
  | "error";     // Something went wrong

interface UpdateInfo {
  stage: UpdateStage;
  version: string | null;       // Latest available version
  currentVersion: string | null;
  progress: number;              // 0–100
}

interface LauncherUpdateScreenProps {
  onDone: () => void; // Called when update is skipped, up-to-date, or installed
}

// ─── Helpers ─────────────────────────────────────────────────────────────────
function formatPercent(p: number): string {
  return `${Math.min(100, Math.floor(p))}%`;
}

// ─── Component ───────────────────────────────────────────────────────────────
export default function LauncherUpdateScreen({ onDone }: LauncherUpdateScreenProps) {
  const [info, setInfo] = useState<UpdateInfo>({
    stage: "checking",
    version: null,
    currentVersion: null,
    progress: 0,
  });
  const [dots, setDots] = useState(".");
  const doneCalled = useRef(false);

  // Animated dots for the "Checking…" state
  useEffect(() => {
    const id = setInterval(() => {
      setDots((d) => (d.length >= 3 ? "." : d + "."));
    }, 500);
    return () => clearInterval(id);
  }, []);

  // Listen for status updates from the main process
  useEffect(() => {
    const api = window.electronAPI;
    if (!api?.onLauncherUpdateStatus) {
      // Not in Electron (dev browser), skip immediately
      onDone();
      return;
    }

    const cleanup = api.onLauncherUpdateStatus((data: any) => {
      const status: UpdateStage = data.status;
      const version: string | null = data.version ?? null;
      const progressVal: number = data.progress?.percent ?? 0;

      setInfo((prev) => {
        const next = { ...prev, stage: status, progress: progressVal };
        if (version) {
          if (status === "latest") {
            next.currentVersion = version;
          } else {
            next.version = version;
          }
        }
        return next;
      });

      // Auto-proceed when already on latest
      if (status === "latest" && !doneCalled.current) {
        doneCalled.current = true;
        setTimeout(() => onDone(), 1200);
      }
    });

    // Kick off the check
    api.checkLauncherUpdates?.();

    return cleanup;
  }, [onDone]);

  // ── Button handlers ────────────────────────────────────────────────────────
  const handleUpdate = async () => {
    setInfo((prev) => ({ ...prev, stage: "downloading", progress: 0 }));
    await window.electronAPI?.installLauncherUpdate?.();
  };

  const installedVersion = info.currentVersion;

  const handleSkip = () => {
    if (!doneCalled.current) {
      doneCalled.current = true;
      onDone();
    }
  };

  // ── Render ─────────────────────────────────────────────────────────────────
  return (
    <div className="fixed inset-0 bg-crt-bg flex items-center justify-center z-50 overflow-hidden font-mono select-none">
      {/* Background elements (subtle CRT glow) */}
      <div className="absolute top-[-20%] left-[-10%] w-[50%] h-[50%] bg-crt-glow opacity-[0.05] blur-[150px] rounded-full pointer-events-none"></div>
      <div className="absolute bottom-[-20%] right-[-10%] w-[40%] h-[40%] bg-crt-glow opacity-[0.04] blur-[120px] rounded-full pointer-events-none"></div>

      <motion.div
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        transition={{ duration: 0.4, ease: "easeOut" }}
        className="w-full max-w-md bg-crt-bg/90 backdrop-blur-xl rounded-none border border-crt-accent shadow-crt-glow p-8 relative flex flex-col items-center"
      >
        {/* Logo */}
        <div className="flex flex-col items-center mb-8">
          <div className="w-16 h-16 bg-crt-accent/20 border border-crt-glow rounded-none flex items-center justify-center mb-4 shadow-crt-glow">
            <span className="text-crt-glow font-bold text-3xl tracking-tighter text-glow">LD</span>
          </div>
          <h1 className="text-crt-text text-xl font-bold tracking-widest text-glow-subtle uppercase">
            LDLauncher
          </h1>
        </div>

        {/* Dynamic Stage Content */}
        <div className="w-full min-h-[140px] flex flex-col items-center justify-center">
          <AnimatePresence mode="wait">
            <motion.div
              key={info.stage}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              transition={{ duration: 0.3 }}
              className="w-full flex flex-col items-center"
            >
              {info.stage === "checking" && <StageChecking dots={dots} />}
              {info.stage === "latest" && <StageLatest version={info.currentVersion} />}
              {info.stage === "available" && (
                <StageAvailable
                  version={info.version}
                  installedVersion={installedVersion}
                  onUpdate={handleUpdate}
                  onSkip={handleSkip}
                />
              )}
              {info.stage === "downloading" && <StageDownloading progress={info.progress} />}
              {info.stage === "ready" && <StageReady />}
              {info.stage === "error" && <StageError onSkip={handleSkip} />}
            </motion.div>
          </AnimatePresence>
        </div>
      </motion.div>

      {/* Version label */}
      <span className="absolute bottom-6 text-crt-accent/40 text-xs font-mono tracking-widest uppercase">
        {info.currentVersion ? `v${info.currentVersion}` : "Updating..."}
      </span>
    </div>
  );
}

// ─── Sub-stages ──────────────────────────────────────────────────────────────

function StageChecking({ dots }: { dots: string }) {
  return (
    <motion.div initial="hidden" animate="visible" variants={staggerContainer} className="flex flex-col items-center">
      <motion.div variants={fadeInUp} className="w-12 h-12 border-2 border-crt-accent border-t-crt-glow border-r-crt-glow rounded-full animate-spin mb-5 shadow-crt-glow" />
      <motion.p variants={fadeInUp} className="text-crt-text font-bold uppercase tracking-[0.2em] text-glow-subtle text-sm">
        ПРОВЕРКА ВЕРСИИ{dots}
      </motion.p>
      <motion.p variants={fadeInUp} className="text-crt-accent text-[10px] uppercase tracking-widest mt-2">
        Установка безопасного соединения...
      </motion.p>
    </motion.div>
  );
}

function StageLatest({ version }: { version: string | null }) {
  return (
    <motion.div initial="hidden" animate="visible" variants={staggerContainer} className="flex flex-col items-center">
      <motion.div variants={scaleIn} className="text-crt-glow mb-5 text-glow">
        <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
          <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
          <polyline points="22 4 12 14.01 9 11.01"></polyline>
        </svg>
      </motion.div>
      <motion.p variants={fadeInUp} className="text-crt-text font-bold uppercase tracking-[0.2em] text-glow-subtle text-sm">
        СИСТЕМА АКТУАЛЬНА
      </motion.p>
      {version && (
        <motion.p variants={fadeInUp} className="text-crt-accent text-xs uppercase tracking-widest mt-2 opacity-80">
          Установлена версия v{version}
        </motion.p>
      )}
    </motion.div>
  );
}

function StageAvailable({
  version,
  installedVersion,
  onUpdate,
  onSkip,
}: {
  version: string | null;
  installedVersion: string | null;
  onUpdate: () => void;
  onSkip: () => void;
}) {
  return (
    <motion.div initial="hidden" animate="visible" variants={staggerContainer} className="flex flex-col items-center w-full">
      <motion.div variants={scaleIn} className="text-crt-glow mb-4 text-glow animate-pulse">
        <svg width="42" height="42" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
          <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
          <polyline points="7 10 12 15 17 10"></polyline>
          <line x1="12" y1="15" x2="12" y2="3"></line>
        </svg>
      </motion.div>
      <motion.p variants={fadeInUp} className="text-crt-text font-bold uppercase tracking-[0.15em] text-glow-subtle mb-4 text-sm">
        НАЙДЕНО ОБНОВЛЕНИЕ
      </motion.p>

      <motion.div variants={fadeInUp} className="flex items-center justify-center gap-4 mb-6 bg-crt-bg/50 border border-crt-accent/40 px-5 py-2 w-full">
        {installedVersion && (
          <span className="text-crt-accent font-mono text-xs tracking-widest">v{installedVersion}</span>
        )}
        {installedVersion && version && (
          <span className="text-crt-text/30">→</span>
        )}
        {version && (
          <span className="text-crt-glow font-mono font-bold text-xs tracking-widest text-glow">v{version}</span>
        )}
      </motion.div>

      <motion.div variants={fadeInUp} className="w-full flex flex-col gap-3">
        <button
          onClick={onUpdate}
          className="w-full bg-crt-bg border-2 border-crt-accent hover:border-crt-glow hover:bg-crt-accent/10 text-crt-glow font-bold py-3.5 rounded-none hover:shadow-crt-glow active:scale-[0.98] transition-all flex justify-center items-center uppercase tracking-[0.15em] text-glow text-xs"
        >
          ИНИЦИИРОВАТЬ ЗАГРУЗКУ
        </button>
        <button
          onClick={onSkip}
          className="w-full text-crt-accent/70 hover:text-crt-glow text-[10px] font-bold py-2 uppercase tracking-[0.2em] transition-colors text-glow-subtle"
        >
          ПРОИГНОРИРОВАТЬ
        </button>
      </motion.div>
    </motion.div>
  );
}

function StageDownloading({ progress }: { progress: number }) {
  return (
    <motion.div initial="hidden" animate="visible" variants={staggerContainer} className="flex flex-col items-center w-full">
      <motion.div variants={fadeInUp} className="text-crt-glow mb-5 text-glow">
        <svg width="42" height="42" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
          <rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect>
          <line x1="8" y1="21" x2="16" y2="21"></line>
          <line x1="12" y1="17" x2="12" y2="21"></line>
        </svg>
      </motion.div>
      <motion.p variants={fadeInUp} className="text-crt-text font-bold uppercase tracking-[0.15em] text-glow-subtle mb-4 text-sm">
        ЗАГРУЗКА ПАКЕТОВ...
      </motion.p>
      
      <motion.div variants={fadeInUp} className="w-full h-1.5 bg-crt-bg border border-crt-accent/30 relative overflow-hidden mb-3">
        <div
          className="absolute top-0 left-0 bottom-0 bg-crt-glow transition-all duration-300 shadow-crt-glow"
          style={{ width: `${Math.min(100, progress)}%` }}
        />
      </motion.div>
      <motion.p variants={fadeInUp} className="text-crt-glow font-mono text-xs tracking-[0.2em] text-glow">
        {formatPercent(progress)}
      </motion.p>
    </motion.div>
  );
}

function StageReady() {
  return (
    <motion.div initial="hidden" animate="visible" variants={staggerContainer} className="flex flex-col items-center">
      <motion.div variants={scaleIn} className="text-crt-glow mb-5 text-glow">
        <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
          <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
          <polyline points="22 4 12 14.01 9 11.01"></polyline>
        </svg>
      </motion.div>
      <motion.p variants={fadeInUp} className="text-crt-text font-bold uppercase tracking-[0.15em] text-glow-subtle text-sm">
        ПАКЕТЫ ЗАГРУЖЕНЫ
      </motion.p>
      <motion.p variants={fadeInUp} className="text-crt-accent text-[10px] uppercase tracking-[0.2em] mt-2">
        ЗАПУСК ИНСТАЛЛЯТОРА...
      </motion.p>
    </motion.div>
  );
}

function StageError({ onSkip }: { onSkip: () => void }) {
  return (
    <motion.div initial="hidden" animate="visible" variants={staggerContainer} className="flex flex-col items-center w-full">
      <motion.div variants={scaleIn} className="text-red-400 mb-5" style={{ filter: "drop-shadow(0 0 8px rgba(248, 113, 113, 0.5))" }}>
        <svg width="42" height="42" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
          <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"></path>
          <line x1="12" y1="9" x2="12" y2="13"></line>
          <line x1="12" y1="17" x2="12.01" y2="17"></line>
        </svg>
      </motion.div>
      <motion.p variants={fadeInUp} className="text-red-400 font-bold uppercase tracking-[0.15em] mb-2 text-sm" style={{ filter: "drop-shadow(0 0 4px rgba(248, 113, 113, 0.5))" }}>
        ОШИБКА ПОДКЛЮЧЕНИЯ
      </motion.p>
      <motion.p variants={fadeInUp} className="text-crt-accent text-[10px] uppercase tracking-[0.2em] text-center mb-6">
        СЕРВЕР ОБНОВЛЕНИЙ НЕДОСТУПЕН
      </motion.p>
      
      <motion.button
        variants={fadeInUp}
        onClick={onSkip}
        className="w-full bg-crt-bg border border-crt-accent hover:border-crt-glow hover:bg-crt-accent/10 text-crt-glow font-bold py-3.5 rounded-none hover:shadow-crt-glow active:scale-[0.98] transition-all flex justify-center items-center uppercase tracking-[0.15em] text-glow text-xs"
      >
        ПРОДОЛЖИТЬ АВТОНОМНО
      </motion.button>
    </motion.div>
  );
}

// ─── Animation Variants ──────────────────────────────────────────────────────

const staggerContainer = {
  hidden: { opacity: 0 },
  visible: {
    opacity: 1,
    transition: {
      staggerChildren: 0.1,
      delayChildren: 0.1
    }
  }
};

const fadeInUp = {
  hidden: { opacity: 0, y: 10 },
  visible: { opacity: 1, y: 0, transition: { duration: 0.4, ease: "easeOut" } }
};

const scaleIn = {
  hidden: { opacity: 0, scale: 0.8 },
  visible: { opacity: 1, scale: 1, transition: { type: "spring", bounce: 0.4, duration: 0.6 } }
};
