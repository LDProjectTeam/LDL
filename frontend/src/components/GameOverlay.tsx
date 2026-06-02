import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import type { Game } from '../data/games';
import { useI18n } from '../i18n';

type GameStatus = 'checking' | 'missing' | 'damaged' | 'installed';

interface GameOverlayProps {
    game: Game;
    installingGameId: string | null;
    launchingGameId: string | null;
    runningGameId: string | null;
    installProgress: number;
    installMessage?: string;
    installSpeed?: string;
    onInstall: (gameId: string) => void;
    onLaunch: (gameId: string) => void;
    hasEntitlement?: boolean;
    isPaying?: boolean;
    onBuy?: (gameId: string) => void;
}

export default function GameOverlay({
    game,
    installingGameId,
    launchingGameId,
    runningGameId,
    installProgress,
    installMessage,
    installSpeed,
    onInstall,
    onLaunch,
    hasEntitlement,
    isPaying,
    onBuy
}: GameOverlayProps) {
    const { t } = useI18n();
    const isInstalling = installingGameId === game.id;
    const isLaunching = launchingGameId === game.id;
    const isRunning = runningGameId === game.id;
    const isBusy = isInstalling || isLaunching;
    
    // UI state for mouse tracking tooltip
    const [mousePos, setMousePos] = useState({ x: 0, y: 0 });
    const [isHoveringInstall, setIsHoveringInstall] = useState(false);

    const [gameStatus, setGameStatus] = useState<GameStatus>('checking');
    const [gearOpen, setGearOpen] = useState(false);
    const [confirmDelete, setConfirmDelete] = useState(false);

    useEffect(() => {
        let isMounted = true;
        const checkStatus = async () => {
            if (!game.config) {
                if (isMounted) setGameStatus('missing');
                return;
            }
            try {
                const response = await window.electronAPI?.getGameStatus({
                    gameId: game.id,
                    type: game.config.type,
                    version: game.config.version
                });
                // getGameStatus returns { status: string, running: bool }
                const statusStr = response?.status ?? response;
                if (isMounted) {
                    if (statusStr === 'installed') setGameStatus('installed');
                    else if (statusStr === 'damaged') setGameStatus('damaged');
                    else setGameStatus('missing');
                }
            } catch (e) {
                if (isMounted) setGameStatus('missing');
            }
        };

        checkStatus();
        const interval = setInterval(checkStatus, 2000);
        return () => {
            isMounted = false;
            clearInterval(interval);
        };
    }, [game.id, game.config, isInstalling]);

    const handleOpenDir = async () => {
        setGearOpen(false);
        const gamePath = await window.electronAPI?.getGamePath(game.id);
        if (gamePath) window.electronAPI?.openPath(gamePath);
    };

    const handleDelete = async () => {
        if (!confirmDelete) {
            setConfirmDelete(true);
            setTimeout(() => setConfirmDelete(false), 3000);
            return;
        }
        setGearOpen(false);
        setConfirmDelete(false);
        await window.electronAPI?.deleteGame(game.id);
    };

    const handleKill = async () => {
        await window.electronAPI?.killGame(game.id);
    };

    // Close gear on outside click
    useEffect(() => {
        if (!gearOpen) return;
        const handler = () => setGearOpen(false);
        window.addEventListener('click', handler);
        return () => window.removeEventListener('click', handler);
    }, [gearOpen]);

    // No config = coming soon / unavailable
    const isUnavailable = !game.config;
    // Paid game logic: show buy/waiting if user has no entitlement
    const needsToPay = !!game.isPaid && !hasEntitlement;

    // Button appearance logic
    const getButton = () => {
        if (isUnavailable) {
            return {
                label: t.comingSoon,
                icon: (
                    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                        <circle cx="12" cy="12" r="10" />
                        <path d="M12 8v4l3 3" />
                    </svg>
                ),
                disabled: true,
                style: 'bg-crt-accent/10 text-crt-accent/50 cursor-not-allowed border border-crt-accent/20',
                shadow: {},
                action: null as null | 'buy'
            };
        }
        // Paid & not entitled: show buy or waiting
        if (needsToPay) {
            if (isPaying) {
                return {
                    label: t.waitingPayment,
                    icon: (
                        <svg className="animate-spin h-6 w-6" viewBox="0 0 24 24" fill="none">
                            <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
                            <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
                        </svg>
                    ),
                    disabled: true,
                    style: 'bg-yellow-500/10 text-yellow-300/70 cursor-not-allowed border border-yellow-500/20',
                    shadow: {},
                    action: null as null | 'buy'
                };
            }
            return {
                label: `${t.buyNow} — 0.3 TON (~$1)`,
                icon: (
                    /* TON crystal logo */
                    <svg width="22" height="22" viewBox="0 0 56 56" fill="none">
                        <path d="M28 6L6 18.5V28L28 52L50 28V18.5L28 6Z"
                              fill="#78350f" opacity="0.9"/>
                        <path d="M6 18.5H50" stroke="#78350f" strokeWidth="3" opacity="0.6"/>
                        <path d="M28 6L17 18.5H39L28 6Z" fill="#92400e" opacity="0.5"/>
                    </svg>
                ),
                disabled: false,
                style: 'text-[#3d1a00] hover:brightness-110 active:scale-95',
                shadow: {
                    background: 'linear-gradient(135deg, #fef9c3 0%, #fde047 18%, #f59e0b 42%, #d97706 64%, #92400e 84%, #78350f 100%)',
                    boxShadow: '0 20px 55px -10px rgba(202,138,4,0.8), 0 0 0 1px rgba(253,224,71,0.25)',
                },
                action: 'buy' as null | 'buy'
            };
        }
        if (isBusy) {
            return {
                label: isLaunching ? t.launching : t.installing,
                icon: (
                    <svg className="animate-spin h-6 w-6" viewBox="0 0 24 24" fill="none">
                        <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
                        <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
                    </svg>
                ),
                disabled: true,
                style: 'bg-crt-accent/20 text-crt-text cursor-not-allowed',
                shadow: {},
                action: null as null | 'buy'
            };
        }
        if (isRunning) {
            return {
                label: t.gameRunning,
                icon: (
                    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                        <circle cx="12" cy="12" r="10" />
                        <rect x="9" y="9" width="6" height="6" fill="currentColor" />
                    </svg>
                ),
                disabled: true,
                style: 'bg-crt-accent/20 text-crt-text cursor-not-allowed border border-crt-accent/30',
                shadow: {},
                action: null as null | 'buy'
            };
        }
        if (gameStatus === 'installed') {
            return {
                label: t.play,
                icon: <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3" /></svg>,
                disabled: false,
                style: 'bg-white text-black hover:bg-white',
                shadow: { boxShadow: `0 20px 40px -10px ${game.accentColor}80` },
                action: null as null | 'buy'
            };
        }
        if (gameStatus === 'damaged') {
            return {
                label: t.repair,
                icon: (
                    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                        <path d="M14.7 6.3a1 1 0 000 1.4l1.6 1.6a1 1 0 001.4 0l3.77-3.77a6 6 0 01-7.94 7.94l-6.91 6.91a2.12 2.12 0 01-3-3l6.91-6.91a6 6 0 017.94-7.94l-3.76 3.76z"/>
                    </svg>
                ),
                disabled: false,
                style: 'bg-amber-500/20 text-amber-300 border border-amber-500/50 hover:bg-amber-500/30',
                shadow: { boxShadow: '0 20px 40px -10px rgba(245,158,11,0.4)' },
                action: null as null | 'buy'
            };
        }
        // missing
        return {
            label: t.install,
            icon: <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5"><path d="M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>,
            disabled: false,
            style: 'bg-white text-black hover:bg-white',
            shadow: { boxShadow: `0 20px 40px -10px ${game.accentColor}80` },
            action: null as null | 'buy'
        };
    };

    const btn = getButton();

    return (
        <div className="absolute bottom-0 left-0 right-0 p-10 pl-[110px] pb-14 flex items-end justify-between z-10 bg-gradient-to-t from-[#0f0f0f] via-[#0f0f0f]/80 to-transparent">
            {/* Floating Tooltip */}
            {isHoveringInstall && isInstalling && installSpeed && (
                <div 
                    className="fixed z-[100] px-3 py-2 bg-[#0a0a0f]/95 border border-crt-accent rounded-xl pointer-events-none drop-shadow-2xl backdrop-blur-md min-w-[150px]"
                    style={{ left: mousePos.x + 20, top: mousePos.y + 20 }}
                >
                    <div className="flex items-center justify-between gap-4 text-xs font-mono mb-1">
                        <span className="text-[#a0a0b0]">{t.networkSpeed}</span>
                        <span className="text-emerald-400 font-bold">{installSpeed}</span>
                    </div>
                    <div className="flex items-center justify-between gap-4 text-xs font-mono">
                        <span className="text-[#a0a0b0]">{t.diskSpeed}</span>
                        <span className="text-emerald-400 font-bold">{installSpeed}</span>
                    </div>
                </div>
            )}

            <div className="flex flex-col max-w-3xl">
                <motion.div
                    key={game.id + '-badge'}
                    initial={{ opacity: 0, y: 10 }}
                    animate={{ opacity: 1, y: 0 }}
                    className="mb-4"
                >
                    <span
                        className="px-4 py-1.5 rounded-full text-xs font-bold uppercase tracking-widest"
                        style={{ backgroundColor: `${game.accentColor}20`, color: game.accentColor, border: `1px solid ${game.accentColor}40` }}
                    >
                        {game.badgeKey ? (t as any)[game.badgeKey] : t.featuredGame}
                    </span>
                </motion.div>
                <motion.h1
                    key={game.id + '-title'}
                    initial={{ opacity: 0, y: 20 }}
                    animate={{ opacity: 1, y: 0 }}
                    className="text-7xl font-black text-crt-glow text-glow-subtle drop-shadow-2xl tracking-tighter"
                    style={{ textShadow: `0 0 40px ${game.accentColor}40` }}
                >
                    {game.title}
                </motion.h1>
                <motion.p
                    key={game.id + '-desc'}
                    initial={{ opacity: 0 }}
                    animate={{ opacity: 1 }}
                    transition={{ delay: 0.1 }}
                    className="mt-6 text-xl text-crt-text text-glow-subtle max-w-2xl font-medium drop-shadow-md leading-relaxed"
                >
                    {game.descriptionKey ? (t as any)[game.descriptionKey] : game.description}
                </motion.p>

                {/* Repair notice */}
                <AnimatePresence>
                    {gameStatus === 'damaged' && !isBusy && (
                        <motion.p
                            initial={{ opacity: 0, y: 5 }}
                            animate={{ opacity: 1, y: 0 }}
                            exit={{ opacity: 0 }}
                            className="mt-3 text-sm text-amber-400/80 flex items-center gap-2"
                        >
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                                <path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/>
                                <line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>
                            </svg>
                            {t.repairDesc}
                        </motion.p>
                    )}
                </AnimatePresence>
            </div>

            <div className="flex flex-col items-end gap-3">
                {/* Launch hint */}
                <AnimatePresence>
                    {(isRunning || isLaunching) && (
                        <motion.p
                            initial={{ opacity: 0, y: 6 }}
                            animate={{ opacity: 1, y: 0 }}
                            exit={{ opacity: 0 }}
                            className="text-xs font-mono text-crt-accent/70 tracking-wide text-right max-w-xs"
                        >
                            <span className="inline-block w-1.5 h-1.5 rounded-full bg-crt-glow mr-2 animate-pulse align-middle" />
                            {t.launchHint}
                        </motion.p>
                    )}
                </AnimatePresence>

                {/* Progress card during install */}
                <AnimatePresence>
                    {isInstalling && (
                        <motion.div
                            initial={{ opacity: 0, x: 20 }}
                            animate={{ opacity: 1, x: 0 }}
                            exit={{ opacity: 0, x: 20 }}
                            className="flex flex-col items-end w-72 bg-crt-bg/90 p-4 rounded-2xl border border-crt-accent backdrop-blur-md"
                            onMouseMove={(e) => setMousePos({ x: e.clientX, y: e.clientY })}
                            onMouseEnter={() => setIsHoveringInstall(true)}
                            onMouseLeave={() => setIsHoveringInstall(false)}
                        >
                            <div className="flex justify-between w-full mb-2">
                                <span className="text-sm font-bold text-crt-text text-glow-subtle truncate max-w-[180px]">
                                    {installMessage || t.loading}
                                </span>
                                <span className="text-sm font-black text-crt-glow text-glow-subtle ml-2">{Math.round(installProgress)}%</span>
                            </div>
                            <div className="w-full h-2 bg-crt-bg/90 rounded-full overflow-hidden">
                                <motion.div
                                    className="h-full rounded-full"
                                    style={{ backgroundColor: game.accentColor, boxShadow: `0 0 10px ${game.accentColor}` }}
                                    animate={{ width: `${installProgress}%` }}
                                    transition={{ duration: 0.3 }}
                                />
                            </div>
                        </motion.div>
                    )}
                </AnimatePresence>

                {/* Buttons row: gear + main action */}
                <div className="flex items-center gap-2">

                    {/* Gear button — only when installed or damaged */}
                    {(gameStatus === 'installed' || gameStatus === 'damaged') && !isBusy && (
                        <div className="relative" onClick={e => e.stopPropagation()}>
                            <button
                                onClick={() => setGearOpen(v => !v)}
                                className="w-14 h-14 rounded-2xl flex items-center justify-center bg-white/10 border border-white/20 text-white/60 hover:text-white hover:bg-white/20 transition-colors duration-150 group"
                            >
                                <svg
                                    width="20" height="20" viewBox="0 0 24 24" fill="none"
                                    stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"
                                    className="transition-transform duration-300 group-hover:rotate-45"
                                >
                                    <circle cx="12" cy="12" r="3"/>
                                    <path d="M19.4 15a1.65 1.65 0 00.33 1.82l.06.06a2 2 0 010 2.83 2 2 0 01-2.83 0l-.06-.06a1.65 1.65 0 00-1.82-.33 1.65 1.65 0 00-1 1.51V21a2 2 0 01-4 0v-.09A1.65 1.65 0 009 19.4a1.65 1.65 0 00-1.82.33l-.06.06a2 2 0 01-2.83-2.83l.06-.06A1.65 1.65 0 004.68 15a1.65 1.65 0 00-1.51-1H3a2 2 0 010-4h.09A1.65 1.65 0 004.6 9a1.65 1.65 0 00-.33-1.82l-.06-.06a2 2 0 012.83-2.83l.06.06A1.65 1.65 0 009 4.68a1.65 1.65 0 001-1.51V3a2 2 0 014 0v.09a1.65 1.65 0 001 1.51 1.65 1.65 0 001.82-.33l.06-.06a2 2 0 012.83 2.83l-.06.06A1.65 1.65 0 0019.4 9a1.65 1.65 0 001.51 1H21a2 2 0 010 4h-.09a1.65 1.65 0 00-1.51 1z"/>
                                </svg>
                            </button>

                            <AnimatePresence>
                                {gearOpen && (
                                    <motion.div
                                        initial={{ opacity: 0, y: 8, scale: 0.95 }}
                                        animate={{ opacity: 1, y: 0, scale: 1 }}
                                        exit={{ opacity: 0, y: 8, scale: 0.95 }}
                                        transition={{ duration: 0.15 }}
                                        className="absolute bottom-16 right-0 w-52 bg-[#111]/95 border border-white/10 rounded-2xl overflow-hidden backdrop-blur-xl shadow-2xl z-50"
                                    >
                                        {/* Open directory */}
                                        <button
                                            onClick={handleOpenDir}
                                            className="w-full flex items-center gap-3 px-4 py-3 text-sm text-white/80 hover:bg-white/10 hover:text-white transition-colors"
                                        >
                                            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                                <path d="M22 19a2 2 0 01-2 2H4a2 2 0 01-2-2V5a2 2 0 012-2h5l2 3h9a2 2 0 012 2z"/>
                                            </svg>
                                            {t.openDirectory}
                                        </button>

                                        <div className="h-px bg-white/10 mx-3"/>

                                        {/* Delete */}
                                        <button
                                            onClick={handleDelete}
                                            className={`w-full flex items-center gap-3 px-4 py-3 text-sm transition-colors ${
                                                confirmDelete
                                                    ? 'bg-red-500/20 text-red-400 font-bold'
                                                    : 'text-red-400/70 hover:bg-red-500/10 hover:text-red-400'
                                            }`}
                                        >
                                            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                                <polyline points="3 6 5 6 21 6"/>
                                                <path d="M19 6l-1 14a2 2 0 01-2 2H8a2 2 0 01-2-2L5 6"/>
                                                <path d="M10 11v6M14 11v6"/>
                                            </svg>
                                            {confirmDelete ? t.deleteConfirm : t.deleteGame}
                                        </button>
                                    </motion.div>
                                )}
                            </AnimatePresence>
                        </div>
                    )}

                    {/* Main action button */}
                    <motion.button
                        whileHover={{ scale: btn.disabled ? 1 : 1.05 }}
                        whileTap={{ scale: btn.disabled ? 1 : 0.95 }}
                        onClick={() => {
                            if (btn.disabled) return;
                            if (btn.action === 'buy') { onBuy?.(game.id); return; }
                            if (gameStatus === 'installed') onLaunch(game.id);
                            else onInstall(game.id);
                        }}
                        disabled={btn.disabled}
                        className={`relative overflow-hidden px-14 py-5 rounded-2xl font-black text-2xl uppercase tracking-widest transition-all duration-300 ${btn.style}`}
                        style={btn.shadow}
                    >
                        {!isBusy && !isUnavailable && !needsToPay && (
                            <div
                                className="absolute inset-0 opacity-20 hover:opacity-40 transition-opacity"
                                style={{ background: `linear-gradient(90deg, transparent, ${game.accentColor}, transparent)` }}
                            />
                        )}
                        <span className="relative z-10 flex items-center gap-3">
                            {btn.icon}
                            {btn.label}
                        </span>
                    </motion.button>

                    {/* Kill game button (shows only when running) */}
                    <AnimatePresence>
                        {isRunning && (
                            <motion.button
                                initial={{ opacity: 0, scale: 0.8, x: -10 }}
                                animate={{ opacity: 1, scale: 1, x: 0 }}
                                exit={{ opacity: 0, scale: 0.8, x: -10 }}
                                whileHover={{ scale: 1.05 }}
                                whileTap={{ scale: 0.95 }}
                                onClick={handleKill}
                                className="h-16 px-6 rounded-2xl flex items-center justify-center bg-red-500/20 border border-red-500/50 text-red-400 hover:bg-red-500 hover:text-white hover:shadow-[0_0_20px_rgba(239,68,68,0.5)] transition-all duration-300 ml-2"
                                title={t.killGame}
                            >
                                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                    <path d="M18 6L6 18M6 6l12 12"/>
                                </svg>
                            </motion.button>
                        )}
                    </AnimatePresence>
                </div>
            </div>
        </div>
    );
}
