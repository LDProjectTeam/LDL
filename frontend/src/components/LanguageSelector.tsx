import { useState } from 'react';
import { motion } from 'framer-motion';
import { useI18n, translations } from '../i18n';
import type { Language } from '../i18n';

const RuFlag = () => (
    <svg viewBox="0 0 9 6" width="60" height="40" className="rounded-sm shadow-md mb-6">
        <rect fill="#fff" width="9" height="2" />
        <rect fill="#0039a6" y="2" width="9" height="2" />
        <rect fill="#d52b1e" y="4" width="9" height="2" />
    </svg>
);

const GbFlag = () => (
    <svg viewBox="0 0 60 30" width="60" height="40" className="rounded-sm shadow-md mb-6" preserveAspectRatio="none">
        <path d="M0,0H60V30H0Z" fill="#012169"/>
        <path d="M0,0L60,30M60,0L0,30" stroke="#fff" strokeWidth="6"/>
        <path d="M0,0L60,30" stroke="#C8102E" strokeWidth="4"/>
        <path d="M60,0L0,30" stroke="#C8102E" strokeWidth="4"/>
        <path d="M30,0V30M0,15H60" stroke="#fff" strokeWidth="10"/>
        <path d="M30,0V30M0,15H60" stroke="#C8102E" strokeWidth="6"/>
    </svg>
);

const UaFlag = () => (
    <svg viewBox="0 0 60 40" width="60" height="40" className="rounded-sm shadow-md mb-6">
        <rect fill="#0057b7" width="60" height="20" />
        <rect fill="#ffd700" y="20" width="60" height="20" />
    </svg>
);

export default function LanguageSelector() {
    const { lang, setLanguage, languages } = useI18n();
    const [selected, setSelected] = useState<Language>(lang);

    const handleConfirm = () => {
        setLanguage(selected);
    };

    // Use translations for the *selected* language, so the preview updates instantly
    const activeT = translations[selected];

    return (
        <div className="absolute inset-0 z-[100] bg-crt-bg/90 backdrop-blur-xl flex items-center justify-center overflow-hidden">
            <motion.div 
                initial={{ opacity: 0, y: 30 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ duration: 0.6, ease: "easeOut" }}
                className="relative z-10 flex flex-col items-center w-full max-w-4xl px-8"
            >
                <div className="text-center mb-16 h-32">
                    <motion.h1 
                        key={`title-${selected}`}
                        initial={{ opacity: 0, y: -10 }}
                        animate={{ opacity: 1, y: 0 }}
                        className="text-5xl font-black text-crt-glow text-glow-subtle mb-6 tracking-tight drop-shadow-xl"
                    >
                        {activeT.selectLanguageTitle}
                    </motion.h1>
                    <motion.p 
                        key={`subtitle-${selected}`}
                        initial={{ opacity: 0 }}
                        animate={{ opacity: 1 }}
                        className="text-crt-accent text-xl font-medium px-8"
                    >
                        {activeT.selectLanguageSubtitle}
                    </motion.p>
                </div>

                <div className="grid grid-cols-1 md:grid-cols-3 gap-8 w-full px-8">
                    {languages.map((l) => {
                        const isActive = selected === l.code;
                        return (
                            <button
                                key={l.code}
                                onClick={() => setSelected(l.code)}
                                className={`relative flex flex-col items-center justify-center p-12 rounded-2xl border-2 transition-all duration-150 hover:scale-105 hover:-translate-y-1 active:scale-95 ${
                                    isActive 
                                        ? 'bg-crt-glow/10 border-crt-glow shadow-[0_0_40px_rgba(124,252,0,0.2)]' 
                                        : 'bg-crt-bg border-crt-accent shadow-[0_10px_30px_rgba(0,0,0,0.5)] hover:border-[#888888]'
                                }`}
                            >
                                {/* Checkmark for active state */}
                                {isActive && (
                                    <motion.div 
                                        initial={{ scale: 0 }}
                                        animate={{ scale: 1 }}
                                        className="absolute top-4 right-4 text-crt-glow text-glow"
                                    >
                                        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="4" strokeLinecap="round" strokeLinejoin="round">
                                            <polyline points="20 6 9 17 4 12" />
                                        </svg>
                                    </motion.div>
                                )}
                                
                                {l.code === 'ru' && <RuFlag />}
                                {l.code === 'en' && <GbFlag />}
                                {l.code === 'ua' && <UaFlag />}
                                
                                <span className={`text-2xl font-bold tracking-wide ${isActive ? 'text-crt-glow text-glow-subtle' : 'text-crt-text'}`}>
                                    {l.label}
                                </span>
                            </button>
                        );
                    })}
                </div>

                {/* Continue button */}
                <motion.button
                    key={`btn-${selected}`}
                    initial={{ opacity: 0, y: 10 }}
                    animate={{ opacity: 1, y: 0 }}
                    transition={{ delay: 0.1 }}
                    whileHover={{ scale: 1.04 }}
                    whileTap={{ scale: 0.96 }}
                    onClick={handleConfirm}
                    className="mt-12 px-16 py-4 rounded-xl font-black text-lg uppercase tracking-widest text-black transition-all duration-200"
                    style={{
                        backgroundColor: '#7CFC00',
                        boxShadow: '0 10px 30px rgba(124,252,0,0.35)'
                    }}
                >
                    {activeT.continue}
                </motion.button>
            </motion.div>
        </div>
    );
}
