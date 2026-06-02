import { motion, AnimatePresence } from 'framer-motion';
import type { Game } from '../data/games';
import { useAuth } from '../contexts/AuthContext';
import defaultAvatar from '../assets/icon.png';

interface SidebarProps {
    games: Game[];
    activeGameId: string;
    onSelectGame: (id: string) => void;
    onOpenProfile: () => void;
    onOpenSettings: () => void;
    installingGameId?: string | null;
    launchingGameId?: string | null;
    installProgress?: number;
    installSpeed?: string;
}

// Monoline outline SVG icons for games without a custom iconUrl
function GameSvgIcon({ gameId }: { gameId: string }) {
    const style = "w-[22px] h-[22px]";
    const props = { width: 22, height: 22, viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: 2, strokeLinecap: "round" as const, strokeLinejoin: "round" as const, className: style };

    switch (gameId) {
        case 'lost-death-3': // Sword
            return <svg {...props}><path d="M14.5 17.5L3 6V3h3l11.5 11.5" /><path d="M13 19l6-6" /><path d="M16 16l4 4" /><path d="M19 21l2-2" /></svg>;
        case 'lost-death-2': // Shield
            return <svg {...props}><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" /></svg>;
        case 'lost-death-1': // Dragon/flame
            return <svg {...props}><path d="M8.5 14.5A2.5 2.5 0 0011 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 11-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 002.5 2.5z" /></svg>;
        case 'witcher-ld': // Wolf
            return <svg {...props}><path d="M12 3c-1.5 2-3 3.5-3 6 0 1.5.5 3 2 4.5" /><path d="M12 3c1.5 2 3 3.5 3 6 0 1.5-.5 3-2 4.5" /><circle cx="9" cy="10" r="1" /><circle cx="15" cy="10" r="1" /><path d="M9 16c1 1 2 1.5 3 1.5s2-.5 3-1.5" /><path d="M4 8l2 4" /><path d="M20 8l-2 4" /></svg>;
        case 'cyberpunk-ld': // City/Skyline
            return <svg {...props}><rect x="3" y="11" width="4" height="10" /><rect x="10" y="5" width="4" height="16" /><rect x="17" y="8" width="4" height="13" /><line x1="5" y1="8" x2="5" y2="11" /><line x1="12" y1="2" x2="12" y2="5" /></svg>;
        default:
            return <svg {...props}><circle cx="12" cy="12" r="10" /><line x1="12" y1="8" x2="12" y2="12" /><line x1="12" y1="16" x2="12.01" y2="16" /></svg>;
    }
}

export default function Sidebar({
    games,
    activeGameId,
    onSelectGame,
    onOpenProfile,
    onOpenSettings,
    installingGameId,
    launchingGameId,
    installProgress,
    installSpeed
}: SidebarProps) {
    const { user } = useAuth();
    const avatarSrc = user?.avatarUrl || defaultAvatar;
    return (
        <motion.div
            initial={{ x: -60, opacity: 0 }}
            animate={{ x: 0, opacity: 1 }}
            transition={{ duration: 0.4, ease: 'easeOut' }}
            className="flex h-full w-full flex-col items-center rounded-none bg-crt-bg/80 pb-3 border-r border-crt-accent backdrop-blur-sm"
            style={{ paddingTop: '12px' }}
        >
            {/* Profile / Logo button */}
            <motion.button
                whileHover={{ scale: 1.1 }}
                whileTap={{ scale: 0.95 }}
                onClick={onOpenProfile}
                style={{ marginTop: '8px' }}
                className="relative flex flex-shrink-0 h-[42px] w-[42px] items-center justify-center rounded-full transition-colors hover:bg-crt-accent/20"
            >
                <img src={avatarSrc} alt="Launcher Logo" className="h-[34px] w-[34px] rounded-full object-cover" />
                {/* Online indicator - color depends on user.status */}
                <div
                    className="absolute bottom-0 right-0 h-3 w-3 rounded-full border-[2.5px] border-crt-bg transition-colors duration-500"
                    style={{
                        backgroundColor: user?.status === 'online' ? '#7CFC00' : '#888888',
                        boxShadow: user?.status === 'online' ? '0 0 6px #7CFC00' : 'none'
                    }}
                />
            </motion.button>
            {/* Separator line */}
            <div className="w-6 h-px border-t border-dashed border-crt-accent mt-3 mb-8 shrink-0" />

            {/* Game Icons - scrollable */}
            <div className="flex w-full flex-1 flex-col items-center gap-1.5 overflow-y-auto overflow-x-hidden [&::-webkit-scrollbar]:hidden" style={{ scrollbarWidth: 'none' }}>
                {
                    games.map((game, index) => (
                        <div key={game.id} className="relative flex w-full justify-center group">
                            {/* Active Indicator Line */}
                            <AnimatePresence>
                                {activeGameId === game.id && (
                                    <div className="absolute left-0 top-0 bottom-0 flex items-center z-10">
                                        <motion.div
                                            layoutId="activeSidebarIndicator"
                                            initial={{ opacity: 0, height: 0 }}
                                            animate={{ opacity: 1, height: 32 }}
                                            exit={{ opacity: 0, height: 0 }}
                                            transition={{ duration: 0.2 }}
                                            className="w-[4px] rounded-none"
                                            style={{ backgroundColor: '#80b0ff', boxShadow: `0 0 10px #80b0ff` }}
                                        />
                                    </div>
                                )}
                            </AnimatePresence>

                            {/* Hover Indicator Line (Only shows when not active) */}
                            {activeGameId !== game.id && (
                                <div className="absolute left-0 top-0 bottom-0 flex items-center z-10">
                                    <div
                                        className="w-[4px] rounded-none transition-all duration-200 h-0 group-hover:h-[20px]"
                                        style={{ backgroundColor: '#507090' }}
                                    />
                                </div>
                            )}

                            <motion.button
                                initial={{ x: -30, opacity: 0 }}
                                animate={{ x: 0, opacity: 1, backgroundColor: activeGameId === game.id ? `${game.accentColor}20` : 'rgba(0,0,0,0)' }}
                                transition={{ duration: 0.25, delay: 0.05 * index, ease: 'easeOut' }}
                                whileHover={{ scale: 1.08, backgroundColor: `${game.accentColor}20` }}
                                whileTap={{ scale: 0.95 }}
                                onClick={() => onSelectGame(game.id)}
                                className={`relative sidebar-icon flex h-[42px] w-[42px] items-center justify-center rounded-lg text-lg ${activeGameId === game.id
                                    ? 'shadow-lg'
                                    : ''
                                    }`}
                                title={game.title}
                            >
                                <span
                                    className="relative z-10 flex items-center justify-center text-xl"
                                    style={{
                                        filter: activeGameId === game.id ? 'none' : 'grayscale(0.5) opacity(0.7)',
                                    }}
                                >
                                    {game.iconUrl ? (
                                        <img src={game.iconUrl} alt={game.title} className="w-[34px] h-[34px] rounded-full object-cover shadow-sm" style={{ mixBlendMode: 'screen' }} />
                                    ) : (
                                        <GameSvgIcon gameId={game.id} />
                                    )}
                                </span>

                                {/* Status Indicators */}
                                {(installingGameId === game.id || launchingGameId === game.id) && (
                                    <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
                                        <svg className="h-full w-full -rotate-90 overflow-visible" viewBox="0 0 40 40">
                                            <circle
                                                cx="20"
                                                cy="20"
                                                r="18"
                                                fill="none"
                                                stroke="rgba(255,255,255,0.1)"
                                                strokeWidth="2"
                                            />
                                            <motion.circle
                                                cx="20"
                                                cy="20"
                                                r="18"
                                                fill="none"
                                                stroke={game.accentColor}
                                                strokeWidth="2"
                                                strokeLinecap="round"
                                                initial={{ pathLength: 0 }}
                                                animate={{
                                                    pathLength: installingGameId === game.id ? (installProgress || 0) / 100 : 1,
                                                    opacity: [0.4, 0.8, 0.4]
                                                }}
                                                transition={{
                                                    pathLength: { duration: 0.5 },
                                                }}
                                                strokeDasharray="113.097"
                                                strokeDashoffset={113.097 - (113.097 * (installProgress || 0)) / 100}
                                            />
                                        </svg>
                                    </div>
                                )}
                            </motion.button>
                        </div>
                    ))
                }
            </div >
        </motion.div >
    );
}
