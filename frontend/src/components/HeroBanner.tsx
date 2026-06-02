import { motion, AnimatePresence } from 'framer-motion';
import type { Game } from '../data/games';

interface HeroBannerProps {
    game: Game;
}

export default function HeroBanner({ game }: HeroBannerProps) {
    return (
        <div className="absolute inset-0 z-0 bg-crt-bg overflow-hidden">
            <AnimatePresence mode="wait">
                <motion.div
                    key={game.id}
                    initial={{ opacity: 0, scale: 1.05 }}
                    animate={{ opacity: 1, scale: 1 }}
                    exit={{ opacity: 0 }}
                    transition={{ duration: 0.6, ease: "easeOut" }}
                    className="absolute inset-0"
                >
                    {game.bannerUrl ? (
                        <img 
                            src={game.bannerUrl} 
                            alt={game.title} 
                            className="w-full h-full object-cover opacity-40 mix-blend-screen"
                            style={{ filter: 'grayscale(100%) sepia(100%) hue-rotate(180deg) saturate(150%) brightness(1.1) contrast(1.3)' }}
                        />
                    ) : (
                        <div 
                            className="w-full h-full opacity-30" 
                            style={{ 
                                background: `radial-gradient(ellipse at top right, #507090, transparent 65%), radial-gradient(ellipse at bottom left, #020205, transparent 60%)` 
                            }} 
                        />
                    )}
                    
                    {/* Gradient Overlay for text readability */}
                    <div className="absolute inset-0 bg-gradient-to-t from-crt-bg via-crt-bg/70 to-transparent" />
                    <div className="absolute inset-0 bg-gradient-to-r from-crt-bg via-crt-bg/50 to-transparent" />
                </motion.div>
            </AnimatePresence>
        </div>
    );
}
