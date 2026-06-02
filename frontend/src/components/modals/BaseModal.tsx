import { ReactNode, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';

interface BaseModalProps {
    isOpen: boolean;
    onClose: () => void;
    title: string;
    children: ReactNode;
    width?: string;
}

export default function BaseModal({ isOpen, onClose, title, children, width = '500px' }: BaseModalProps) {
    // Close on Escape key
    useEffect(() => {
        const handleKeyDown = (e: KeyboardEvent) => {
            if (e.key === 'Escape' && isOpen) {
                onClose();
            }
        };
        window.addEventListener('keydown', handleKeyDown);
        return () => window.removeEventListener('keydown', handleKeyDown);
    }, [isOpen, onClose]);

    return (
        <AnimatePresence>
            {isOpen && (
                <div className="fixed inset-0 z-[100] flex items-center justify-center">
                    {/* Backdrop */}
                    <motion.div
                        initial={{ opacity: 0 }}
                        animate={{ opacity: 1 }}
                        exit={{ opacity: 0 }}
                        transition={{ duration: 0.2 }}
                        className="absolute inset-0 bg-crt-bg/90 backdrop-blur-sm"
                        onClick={onClose}
                    />

                    {/* Modal Content */}
                    <motion.div
                        initial={{ opacity: 0, scale: 0.95, y: 10 }}
                        animate={{ opacity: 1, scale: 1, y: 0 }}
                        exit={{ opacity: 0, scale: 0.95, y: 10 }}
                        transition={{ duration: 0.2, ease: "easeOut" }}
                        className="relative z-10 flex flex-col rounded-xl overflow-hidden shadow-[0_10px_40px_rgba(0,0,0,0.5)]"
                        style={{
                            width,
                            maxHeight: '85vh',
                            background: 'linear-gradient(180deg, #2B2B2B 0%, #1A1A1A 100%)',
                            border: '1px solid #507090'
                        }}
                    >
                        {/* Header */}
                        <div className="flex items-center justify-between px-6 py-4 border-b border-crt-accent/50 bg-crt-bg/90">
                            <h2 className="text-lg font-bold text-crt-glow text-glow-subtle tracking-wide">{title}</h2>
                            <button
                                onClick={onClose}
                                className="p-1 rounded-md text-crt-accent hover:text-crt-glow text-glow-subtle hover:bg-crt-accent/20 transition-colors"
                            >
                                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                    <line x1="18" y1="6" x2="6" y2="18" />
                                    <line x1="6" y1="6" x2="18" y2="18" />
                                </svg>
                            </button>
                        </div>

                        {/* Body */}
                        <div className="flex-1 overflow-y-auto ldl-scrollbar p-6">
                            {children}
                        </div>
                    </motion.div>
                </div>
            )}
        </AnimatePresence>
    );
}
