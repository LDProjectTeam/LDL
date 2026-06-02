import { useState, useEffect, useRef, useCallback } from "react";
import LauncherUpdateScreen from "./components/LauncherUpdateScreen";
import Sidebar from "./components/Sidebar";
import UserMenu from "./components/UserMenu";
import HeroBanner from "./components/HeroBanner";
import GameOverlay from "./components/GameOverlay";
import LanguageSelector from "./components/LanguageSelector";
import TitleBar from "./components/TitleBar";
import SettingsModal from "./components/modals/SettingsModal";
import ProfileModal from "./components/modals/ProfileModal";
import LegalModal from "./components/modals/LegalModal";
import ChangelogModal from "./components/modals/ChangelogModal";
import SupportModal from "./components/modals/SupportModal";
import TonPaymentModal from "./components/modals/TonPaymentModal";
import AuthScreen from "./components/AuthScreen";
import { games } from "./data/games";
import { useI18n } from "./i18n";
import { useAuth } from "./contexts/AuthContext";
import { supabase } from "./supabaseClient";

// Edge Function endpoints
const SUPABASE_FN_URL =
  "https://bqcmictdinprbjsrjbcq.supabase.co/functions/v1/get-download-url";
const SUPABASE_CHECK_FN_URL =
  "https://bqcmictdinprbjsrjbcq.supabase.co/functions/v1/check-entitlement";
const SUPABASE_CREATE_TON_URL =
  "https://bqcmictdinprbjsrjbcq.supabase.co/functions/v1/create-ton-payment";
const SUPABASE_CHECK_TON_URL =
  "https://bqcmictdinprbjsrjbcq.supabase.co/functions/v1/check-ton-payment";

/**
 * Exchanges a private GitHub release URL for a short-lived signed S3 URL
 * by calling the Supabase Edge Function with the user's JWT.
 * Returns the signed URL on success, or throws a user-friendly Error.
 */
async function resolveDownloadUrl(githubUrl: string, gameId?: string): Promise<string> {
  const {
    data: { session },
  } = await supabase.auth.getSession();

  if (!session?.access_token) {
    throw new Error("You must be logged in to download the game.");
  }

  const resp = await fetch(SUPABASE_FN_URL, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Bearer ${session.access_token}`,
    },
    body: JSON.stringify({ downloadUrl: githubUrl, gameId }),
  });

  if (!resp.ok) {
    let msg = `Download server error (${resp.status})`;
    try {
      const body = await resp.json();
      if (body.error) msg = body.error;
    } catch { /* ignore */ }
    throw new Error(msg);
  }

  const data = await resp.json();
  if (!data.signedUrl) {
    throw new Error("Could not get a download URL from the server.");
  }

  return data.signedUrl;
}

export default function App() {
  const { isInitialized, lang } = useI18n();
  const { user } = useAuth();
  const [activeGameId, setActiveGameId] = useState(games[0].id);
  const [isUserMenuOpen, setIsUserMenuOpen] = useState(false);

  // ── Launcher update gate ───────────────────────────────────────────────────
  // Show update screen first; only render the main UI once done.
  const [updateCheckDone, setUpdateCheckDone] = useState(false);

  // Modal states
  const [activeModal, setActiveModal] = useState<
    "profile" | "settings" | "legal" | "changelog" | "support" | null
  >(null);

  // Mock states for installation/launch
  const [installingGameId, setInstallingGameId] = useState<string | null>(null);
  const [launchingGameId, setLaunchingGameId] = useState<string | null>(null);
  const [runningGameId, setRunningGameId] = useState<string | null>(null);
  const [installProgress, setInstallProgress] = useState(0);
  const [installMessage, setInstallMessage] = useState("");
  const [installSpeed, setInstallSpeed] = useState("");
  // useRef to avoid stale closure in progress listener
  const installingGameIdRef = useRef<string | null>(null);

  // Entitlement state — tracks which paid games the user has purchased
  const [entitlements, setEntitlements] = useState<Record<string, boolean>>({});
  // isPaying = gameId currently awaiting payment confirmation (polling)
  const [isPaying, setIsPaying] = useState<string | null>(null);
  const payingPollRef = useRef<ReturnType<typeof setInterval> | null>(null);
  const currentPaymentIdRef = useRef<string | null>(null);

  // TON payment modal state
  const [tonModal, setTonModal] = useState<{
    isOpen: boolean;
    paymentId: string;
    walletAddress: string;
    amountTon: number;
    amountNano: number;
    comment: string;
    expiresAt: string;
    gameId: string;
    gameName: string;
    tgLink: string;
  } | null>(null);

  useEffect(() => {
    if (!window.electronAPI?.onGameProgress) return;
    const cleanup = window.electronAPI.onGameProgress((rawData: any) => {
      let data;
      if (typeof rawData === "string") {
        try {
          data = JSON.parse(rawData);
        } catch (e) {
          return; // Ignore invalid data
        }
      } else {
        data = rawData;
      }

      if (data.status === "progress") {
        // Update progress regardless — only one game installs at a time
        setInstallProgress(data.progress ?? 0);
        if (data.message) setInstallMessage(data.message);
        if (data.speed) setInstallSpeed(data.speed);
      } else if (data.status === "success") {
        setInstallingGameId(null);
        installingGameIdRef.current = null;
        setInstallProgress(100);
        setInstallMessage("");
        // Give the progress bar a moment to show 100% then reset
        setTimeout(() => setInstallProgress(0), 800);
      } else if (data.status === "error") {
        console.error("Install Error:", data.message);
        setInstallingGameId(null);
        installingGameIdRef.current = null;
        setInstallProgress(0);
        setInstallMessage("");
        setInstallSpeed("");
      } else if (data.status === "launched") {
        setLaunchingGameId(null);
        setRunningGameId(data.gameId);
      } else if (data.status === "closed") {
        setRunningGameId(null);
      }
    });
    return cleanup;
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // ── Entitlement helpers ────────────────────────────────────────────────────
  const checkEntitlement = useCallback(async (gameId: string): Promise<boolean> => {
    try {
      const { data: { session } } = await supabase.auth.getSession();
      if (!session?.access_token) return false;
      const resp = await fetch(SUPABASE_CHECK_FN_URL, {
        method: "POST",
        headers: { "Content-Type": "application/json", Authorization: `Bearer ${session.access_token}` },
        body: JSON.stringify({ gameId }),
      });
      if (!resp.ok) return false;
      const data = await resp.json();
      return !!data.hasAccess;
    } catch { return false; }
  }, []);

  // Check entitlements for all paid games whenever user logs in
  useEffect(() => {
    if (!user) { setEntitlements({}); return; }
    games.filter(g => g.isPaid && g.config).forEach(async (g) => {
      const has = await checkEntitlement(g.id);
      setEntitlements(prev => ({ ...prev, [g.id]: has }));
    });
  }, [user, checkEntitlement]);

  // TON payment — create session, open wallet deeplink, poll until confirmed
  const handleBuy = useCallback(async (gameId: string) => {
    const { data: { session } } = await supabase.auth.getSession();
    if (!session?.access_token) return;

    // 1. Create payment session on server
    let paymentData: any;
    try {
      const resp = await fetch(SUPABASE_CREATE_TON_URL, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${session.access_token}`,
        },
        body: JSON.stringify({ gameId }),
      });
      if (!resp.ok) { console.error("[TON] create-ton-payment failed", resp.status); return; }
      paymentData = await resp.json();
    } catch (e) {
      console.error("[TON] network error:", e);
      return;
    }

    const { paymentId, walletAddress, amountTon, amountNano, comment, expiresAt } = paymentData;
    currentPaymentIdRef.current = paymentId;

    // Build Telegram Stars link for this game
    const GAME_SHORT_IDS: Record<string, string> = { "lost-death-2": "ld2" };
    const shortId = GAME_SHORT_IDS[gameId] ?? gameId;
    const tgLink = `https://t.me/LDProjectbot?start=buy_${shortId}_${session.user.id}`;

    const game = games.find(g => g.id === gameId);
    setTonModal({
      isOpen: true,
      paymentId,
      walletAddress,
      amountTon,
      amountNano,
      comment,
      expiresAt,
      gameId,
      gameName: game?.title ?? gameId,
      tgLink,
    });

    setIsPaying(gameId);
    if (payingPollRef.current) clearInterval(payingPollRef.current);

    // 3. Poll check-ton-payment every 5s for up to 15 min
    let attempts = 0;
    payingPollRef.current = setInterval(async () => {
      attempts++;
      if (attempts > 180) {
        clearInterval(payingPollRef.current!);
        setIsPaying(null);
        currentPaymentIdRef.current = null;
        return;
      }
      try {
        const { data: { session: s } } = await supabase.auth.getSession();
        if (!s?.access_token) return;
        const checkResp = await fetch(SUPABASE_CHECK_TON_URL, {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            Authorization: `Bearer ${s.access_token}`,
          },
          body: JSON.stringify({ paymentId: currentPaymentIdRef.current }),
        });
        if (!checkResp.ok) return;
        const data = await checkResp.json();
        if (data.hasAccess) {
          clearInterval(payingPollRef.current!);
          setEntitlements(prev => ({ ...prev, [gameId]: true }));
          setIsPaying(null);
          currentPaymentIdRef.current = null;
          // Keep modal open so user sees success state
          setTonModal(prev => prev ? { ...prev } : null);
        }
      } catch { /* ignore network errors, keep polling */ }
    }, 5000);
  }, [checkEntitlement]);

  // Switches polling to Telegram Stars mode (check-entitlement) after user clicks Stars button
  const handlePayWithStars = useCallback((gameId: string) => {
    if (payingPollRef.current) clearInterval(payingPollRef.current);
    currentPaymentIdRef.current = null;
    setIsPaying(gameId);
    let attempts = 0;
    payingPollRef.current = setInterval(async () => {
      attempts++;
      if (attempts > 100) { // ~5 min
        clearInterval(payingPollRef.current!);
        setIsPaying(null);
        return;
      }
      const has = await checkEntitlement(gameId);
      if (has) {
        clearInterval(payingPollRef.current!);
        setEntitlements(prev => ({ ...prev, [gameId]: true }));
        setIsPaying(null);
      }
    }, 3000);
  }, [checkEntitlement]);

  // Cleanup polling on unmount
  useEffect(() => () => { if (payingPollRef.current) clearInterval(payingPollRef.current); }, []);

  const activeGame = games.find((g) => g.id === activeGameId) || games[0];

  const handleInstall = async (gameId: string) => {
    const game = games.find((g) => g.id === gameId);
    if (!game || !game.config) return;
    // Debounce: ignore if already installing or launching any game
    if (installingGameId || launchingGameId) return;

    setInstallingGameId(gameId);
    installingGameIdRef.current = gameId;
    setInstallProgress(0);
    setInstallMessage("Preparing download...");

    try {
      let { downloadUrl } = game.config;

      // ── Secure path: resolve via Supabase Edge Function ───────────────────
      // useSupabaseGate = true means the GitHub PAT lives in Supabase secrets.
      // We exchange the GitHub URL for a signed S3 URL before passing to Electron.
      if (game.config.useSupabaseGate && downloadUrl) {
        try {
          downloadUrl = await resolveDownloadUrl(downloadUrl, gameId);
        } catch (resolveErr: any) {
          console.error("[Install] Failed to resolve download URL:", resolveErr.message);
          // Surface the error to the user through the standard error flow
          setInstallingGameId(null);
          installingGameIdRef.current = null;
          setInstallProgress(0);
          setInstallMessage("");
          // Re-throw so the UI can show a toast/alert if needed
          throw resolveErr;
        }
      }

      await window.electronAPI?.installGame({
        ...game.config,
        gameId,
        downloadUrl, // may be a pre-signed S3 URL now — no token needed
        downloadToken: undefined, // never send PAT to Electron process
      });
    } catch (error) {
      console.error("Failed to start installation:", error);
      setInstallingGameId(null);
      installingGameIdRef.current = null;
    }
  };

  const handleLaunch = async (gameId: string) => {
    const game = games.find((g) => g.id === gameId);
    if (!game || !game.config) return;
    // Debounce: ignore if already launching or installing
    if (launchingGameId || installingGameId || runningGameId) return;

    setLaunchingGameId(gameId);
    try {
      // Get Supabase JWT for DRM challenge-response handshake
      const { data: { session } } = await supabase.auth.getSession();
      const sessionToken = session?.access_token ?? null;

      await window.electronAPI?.launchGame({
        ...game.config,
        gameId,
        launcherLang: lang,
        sessionToken, // used by DRM server in LaunchManager — never logged
        minecraftLicense: user?.minecraftLicense, // pass Microsoft token if linked
      });
      // Do NOT unset launchingGameId here; wait for 'launched' event.
    } catch (error) {
      console.error("Failed to launch:", error);
      setLaunchingGameId(null);
    }
  };

  // ── Update gate ────────────────────────────────────────────────────────────
  if (!updateCheckDone) {
    return <LauncherUpdateScreen onDone={() => setUpdateCheckDone(true)} />;
  }

  // If the language hasn't been chosen yet, show only the Language Selector
  if (!isInitialized) {
    return (
      <div className="relative w-full h-screen bg-crt-bg overflow-hidden select-none text-crt-text font-mono">
        <HeroBanner game={activeGame} />
        <div className="absolute top-0 left-0 right-0 h-10 z-[60] drag-region">
          <TitleBar />
        </div>
        <LanguageSelector />
      </div>
    );
  }

  // If the user is not logged in, show the Auth Screen
  if (!user) {
    return (
      <div className="relative w-full h-screen bg-crt-bg overflow-hidden select-none text-crt-text font-mono">
        <HeroBanner game={activeGame} />
        <div className="absolute top-0 left-0 right-0 h-10 z-[60] drag-region">
          <TitleBar />
        </div>
        <AuthScreen />
      </div>
    );
  }

  return (
    <div className="relative w-full h-screen bg-crt-bg overflow-hidden select-none text-crt-text font-mono">
      <HeroBanner game={activeGame} />

      {/* Title Bar drag region and window controls */}
      <div className="absolute top-0 left-0 right-0 h-10 z-[60] drag-region">
        <TitleBar />
      </div>

      {/* Sidebar */}
      <div className="absolute left-6 top-[48px] bottom-6 w-[70px] z-40">
        <Sidebar
          games={games}
          activeGameId={activeGameId}
          onSelectGame={setActiveGameId}
          onOpenProfile={() => setIsUserMenuOpen(!isUserMenuOpen)}
          onOpenSettings={() => setActiveModal("settings")}
          installingGameId={installingGameId}
          launchingGameId={launchingGameId}
          installProgress={installProgress}
          installSpeed={installSpeed}
        />
      </div>

      <UserMenu
        isOpen={isUserMenuOpen}
        onClose={() => setIsUserMenuOpen(false)}
        onOpenChangelog={() => setActiveModal("changelog")}
        onOpenProfileManagement={() => setActiveModal("profile")}
        onOpenLegal={() => setActiveModal("legal")}
        onOpenSettings={() => setActiveModal("settings")}
        onOpenSupport={() => { setIsUserMenuOpen(false); setTimeout(() => setActiveModal("support"), 150); }}
      />

      <GameOverlay
        game={activeGame}
        installingGameId={installingGameId}
        launchingGameId={launchingGameId}
        runningGameId={runningGameId}
        installProgress={installProgress}
        installMessage={installMessage}
        installSpeed={installSpeed}
        onInstall={handleInstall}
        onLaunch={handleLaunch}
        hasEntitlement={entitlements[activeGame.id]}
        isPaying={isPaying === activeGame.id}
        onBuy={handleBuy}
      />

      {/* Modals */}
      <SettingsModal
        isOpen={activeModal === "settings"}
        onClose={() => setActiveModal(null)}
      />
      <ProfileModal
        isOpen={activeModal === "profile"}
        onClose={() => setActiveModal(null)}
      />
      <LegalModal
        isOpen={activeModal === "legal"}
        onClose={() => setActiveModal(null)}
      />
      <ChangelogModal
        isOpen={activeModal === "changelog"}
        onClose={() => setActiveModal(null)}
      />
      <SupportModal
        isOpen={activeModal === "support"}
        onClose={() => setActiveModal(null)}
      />

      {/* TON Payment Modal */}
      {tonModal && (
        <TonPaymentModal
          isOpen={tonModal.isOpen}
          onClose={() => {
            setTonModal(null);
            if (isPaying) {
              setIsPaying(null);
              if (payingPollRef.current) clearInterval(payingPollRef.current);
              currentPaymentIdRef.current = null;
            }
          }}
          paymentId={tonModal.paymentId}
          walletAddress={tonModal.walletAddress}
          amountTon={tonModal.amountTon}
          amountNano={tonModal.amountNano}
          comment={tonModal.comment}
          expiresAt={tonModal.expiresAt}
          gameName={tonModal.gameName}
          isPaid={!!entitlements[tonModal.gameId]}
          tgLink={tonModal.tgLink}
          onPayWithStars={() => handlePayWithStars(tonModal.gameId)}
        />
      )}
    </div>
  );
}
