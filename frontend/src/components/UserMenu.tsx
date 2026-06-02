import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { useI18n } from '../i18n';
import type { Language } from '../i18n';
import { useAuth } from '../contexts/AuthContext';

interface UserMenuProps {
    isOpen: boolean;
    onClose: () => void;
    onOpenChangelog: () => void;
    onOpenProfileManagement: () => void;
    onOpenLegal: () => void;
    onOpenSettings: () => void;
    onOpenSupport: () => void;
}

export default function UserMenu({ isOpen, onClose, onOpenChangelog, onOpenProfileManagement, onOpenLegal, onOpenSettings, onOpenSupport }: UserMenuProps) {
    const { t, lang, setLanguage, languages } = useI18n();
    const { user, logout, updateStatus } = useAuth();
    const [showLangPicker, setShowLangPicker] = useState(false);

    const handleQuit = () => {
        window.electronAPI?.closeWindow();
    };

    const handleLogout = () => {
        logout();
        onClose();
    };

    const handleSelectLanguage = (code: Language) => {
        setLanguage(code);
        setShowLangPicker(false);
    };

    // Close lang picker when menu closes
    const handleClose = () => {
        setShowLangPicker(false);
        onClose();
    };

    return (
        <AnimatePresence>
            {isOpen && (
                <>
                    {/* Backdrop */}
                    <motion.div
                        initial={{ opacity: 0 }}
                        animate={{ opacity: 1 }}
                        exit={{ opacity: 0 }}
                        transition={{ duration: 0.2 }}
                        className="fixed inset-0 z-50"
                        onClick={handleClose}
                    />

                    {/* Menu Panel */}
                    <motion.div
                        initial={{ opacity: 0, x: -10, y: -5 }}
                        animate={{ opacity: 1, x: 0, y: 0 }}
                        exit={{ opacity: 0, x: -10, y: -5 }}
                        transition={{ duration: 0.2 }}
                        className="absolute left-[100px] top-[62px] z-50 rounded-none shadow-2xl flex flex-col"
                        style={{ width: '240px', background: '#020205', border: '1px solid #507090', boxShadow: '4px 4px 25px rgba(0,0,0,0.8)' }}
                    >
                        {/* User Info */}
                        <div className="flex items-start gap-4 p-4 pr-5">
                            {/* Avatar wrapper — relative so status dot anchors to it, not the panel */}
                            <div className="relative shrink-0 mt-0.5">
                                <div className="flex h-10 w-10 items-center justify-center rounded-none overflow-hidden bg-crt-accent/30 text-crt-accent">
                                    <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                                        <path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" />
                                    </svg>
                                    {user?.avatarUrl && (
                                        <img src={user.avatarUrl} className="absolute inset-0 h-full w-full object-cover" alt="" />
                                    )}
                                </div>
                                {/* Status dot — positioned relative to the avatar wrapper */}
                                <div
                                    className="absolute bottom-0 right-0 h-3 w-3 rounded-full border-[2.5px] border-crt-bg transition-colors duration-500"
                                    style={{
                                        backgroundColor: user?.status === 'offline' ? '#888888' : '#7CFC00',
                                        boxShadow: user?.status === 'offline' ? 'none' : '0 0 6px #7CFC00',
                                    }}
                                />
                            </div>
                            <div className="flex flex-col w-full overflow-hidden">
                                <div className="text-[14px] font-bold text-crt-glow text-glow-subtle tracking-wide truncate">
                                    {user?.username || 'Guest'}
                                </div>
                                <div 
                                    className="text-[13px] font-bold mt-0.5 flex items-center gap-1 text-left w-fit transition-colors"
                                    style={{ color: user?.status === 'offline' ? '#ff4444' : '#80b0ff' }}
                                >
                                    {user?.status === 'offline' ? 'OFFLINE' : ('ONLINE')}
                                </div>
                                {user?.showEmail !== false && (
                                    <div className="text-[12px] font-bold text-crt-accent mt-0.5 truncate">
                                        {user?.email || ''}
                                    </div>
                                )}
                            </div>
                        </div>

                        <div className="h-px w-full bg-crt-accent/30" />

                        <div className="flex flex-col py-2">
                            {/* Language button with flyout submenu */}
                            <div
                                className="relative flex flex-col"
                                onMouseEnter={() => setShowLangPicker(true)}
                                onMouseLeave={() => setShowLangPicker(false)}
                            >
                                <button
                                    className="group mx-2 flex items-center justify-between px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 rounded-none transition-all"
                                    onClick={() => setShowLangPicker(!showLangPicker)}
                                >
                                    <span className="flex items-center gap-3">
                                        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><circle cx="12" cy="12" r="10" /><line x1="2" y1="12" x2="22" y2="12" /><path d="M12 2a15.3 15.3 0 014 10 15.3 15.3 0 01-4 10 15.3 15.3 0 01-4-10 15.3 15.3 0 014-10z" /></svg>
                                        {t.appLanguage}
                                    </span>
                                    <div className="flex items-center gap-2">
                                        <div className="px-1 py-[1px] rounded bg-crt-bg text-[10px] text-crt-accent group-hover:text-crt-text font-bold tracking-wider border border-crt-accent transition-colors">A文</div>
                                        <svg
                                            width="10" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3"
                                            className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"
                                        >
                                            <polyline points="9 18 15 12 9 6" />
                                        </svg>
                                    </div>
                                </button>

                                {/* Language flyout — pops out to the right */}
                                <AnimatePresence>
                                    {showLangPicker && (
                                        <motion.div
                                            initial={{ opacity: 0, x: -5 }}
                                            animate={{ opacity: 1, x: 0 }}
                                            exit={{ opacity: 0, x: -5 }}
                                            transition={{ duration: 0.15 }}
                                            className="absolute z-[60] rounded-none overflow-hidden flex flex-col"
                                            style={{
                                                left: 'calc(100% + 6px)',
                                                top: 0,
                                                width: '200px',
                                                maxHeight: '450px',
                                                background: '#020205',
                                                border: '1px solid #507090',
                                                boxShadow: '4px 4px 25px rgba(0,0,0,0.8)',
                                            }}
                                        >
                                            <div className="overflow-y-auto ldl-scrollbar" style={{ paddingTop: '8px', paddingBottom: '8px' }}>
                                                {languages.map((l) => (
                                                    <button
                                                        key={l.code}
                                                        onClick={() => handleSelectLanguage(l.code)}
                                                        className={`flex w-full items-center justify-between px-5 py-3.5 text-[14px] font-bold transition-colors ${lang === l.code
                                                            ? 'text-crt-glow text-glow bg-crt-glow/5'
                                                            : 'text-crt-text hover:bg-crt-accent/20 hover:text-crt-glow text-glow-subtle'
                                                            }`}
                                                    >
                                                        <span>{l.label}</span>
                                                        {lang === l.code && (
                                                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3" strokeLinecap="round" strokeLinejoin="round">
                                                                <polyline points="20 6 9 17 4 12" />
                                                            </svg>
                                                        )}
                                                    </button>
                                                ))}
                                            </div>
                                        </motion.div>
                                    )}
                                </AnimatePresence>
                            </div>

                            <button
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 rounded-none transition-all mt-1"
                                onClick={onOpenProfileManagement}
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><path d="M20 21v-2a4 4 0 00-4-4H8a4 4 0 00-4 4v2" /><circle cx="12" cy="7" r="4" /></svg>
                                {t.profileManagement}
                            </button>
                            <button
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 rounded-none transition-all mt-1"
                                onClick={() => { handleClose(); setTimeout(() => onOpenSettings(), 200); }}
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><circle cx="12" cy="12" r="3" /><path d="M19.4 15a1.65 1.65 0 00.33 1.82l.06.06a2 2 0 010 2.83 2 2 0 01-2.83 0l-.06-.06a1.65 1.65 0 00-1.82-.33 1.65 1.65 0 00-1 1.51V21a2 2 0 01-2 2 2 2 0 01-2-2v-.09A1.65 1.65 0 009 19.4a1.65 1.65 0 00-1.82.33l-.06.06a2 2 0 01-2.83 0 2 2 0 010-2.83l.06-.06A1.65 1.65 0 004.68 15a1.65 1.65 0 00-1.51-1H3a2 2 0 01-2-2 2 2 0 012-2h.09A1.65 1.65 0 004.6 9a1.65 1.65 0 00-.33-1.82l-.06-.06a2 2 0 010-2.83 2 2 0 012.83 0l.06.06A1.65 1.65 0 009 4.6a1.65 1.65 0 001-1.51V3a2 2 0 012-2 2 2 0 012 2v.09a1.65 1.65 0 001 1.51 1.65 1.65 0 001.82-.33l.06-.06a2 2 0 012.83 0 2 2 0 010 2.83l-.06.06a1.65 1.65 0 00-.33 1.82V9a1.65 1.65 0 001.51 1H21a2 2 0 012 2 2 2 0 01-2 2h-.09a1.65 1.65 0 00-1.51 1z" /></svg>
                                {t.settings || 'Настройки'}
                            </button>
                            <button 
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 rounded-none transition-all mt-1" 
                                onClick={() => { handleClose(); setTimeout(() => onOpenLegal(), 200); }}
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8z" /><polyline points="14 2 14 8 20 8" /><line x1="16" y1="13" x2="8" y2="13" /><line x1="16" y1="17" x2="8" y2="17" /><polyline points="10 9 9 9 8 9" /></svg>
                                {t.legalInfo}
                            </button>
                            <button
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 rounded-none transition-all mt-1"
                                onClick={() => { handleClose(); setTimeout(() => onOpenSupport(), 200); }}
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><path d="M21 15a2 2 0 01-2 2H7l-4 4V5a2 2 0 012-2h14a2 2 0 012 2z" /></svg>
                                {t.support || 'Поддержка'}
                            </button>

                            <div className="w-full h-px bg-crt-accent/30 my-1.5" />

                            {/* Group 2 */}
                            <button
                                onClick={() => { handleClose(); setTimeout(() => onOpenChangelog(), 200); }}
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 rounded-none transition-all mt-1"
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2" /></svg>
                                {t.whatsNew}
                            </button>

                            <div className="w-full h-px bg-crt-accent/30 my-1.5" />

                            <button 
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-red-500 rounded-none transition-all mt-1" 
                                onClick={handleLogout} 
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><path d="M9 21H5a2 2 0 01-2-2V5a2 2 0 012-2h4" /><polyline points="16 17 21 12 16 7" /><line x1="21" y1="12" x2="9" y2="12" /></svg>
                                {t.logout}
                            </button>

                            <button 
                                className="group mx-2 flex items-center gap-3 px-3 py-2.5 text-left text-[13px] font-bold text-crt-text hover:text-crt-glow text-glow-subtle hover:bg-red-500 rounded-none transition-all mt-1" 
                                onClick={handleQuit} 
                            >
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-accent group-hover:text-crt-glow text-glow-subtle transition-colors"><path d="M18.36 6.64a9 9 0 11-12.73 0" /><line x1="12" y1="2" x2="12" y2="12" /></svg>
                                {t.quitApp}
                            </button>
                        </div>
                    </motion.div>
                </>
            )}
        </AnimatePresence>
    );
}
