import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { useI18n } from '../../i18n';

interface TonPaymentModalProps {
  isOpen: boolean;
  onClose: () => void;
  paymentId: string;
  walletAddress: string;
  amountTon: number;
  amountNano: number;
  comment: string;
  expiresAt: string;
  gameName: string;
  isPaid: boolean;
  tgLink?: string;
  onPayWithStars?: () => void;
}

export default function TonPaymentModal({
  isOpen, onClose,
  walletAddress, amountTon, amountNano, comment,
  expiresAt, gameName, isPaid,
  tgLink, onPayWithStars,
}: TonPaymentModalProps) {
  const { t } = useI18n();
  const [secondsLeft, setSecondsLeft] = useState(0);

  const tonLink = `ton://transfer/${walletAddress}?amount=${amountNano}&text=${encodeURIComponent(comment)}`;
  // Standard black-on-white QR — works with ALL scanners
  const qrUrl = `https://api.qrserver.com/v1/create-qr-code/?size=220x220&color=000000&bgcolor=ffffff&qzone=2&data=${encodeURIComponent(tonLink)}`;

  // Countdown timer
  useEffect(() => {
    if (!isOpen || !expiresAt) return;
    const update = () => {
      const diff = Math.max(0, Math.floor((new Date(expiresAt).getTime() - Date.now()) / 1000));
      setSecondsLeft(diff);
    };
    update();
    const t = setInterval(update, 1000);
    return () => clearInterval(t);
  }, [isOpen, expiresAt]);

  const minutes = Math.floor(secondsLeft / 60);
  const seconds = secondsLeft % 60;

  return (
    <AnimatePresence>
      {isOpen && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          className="no-drag fixed inset-0 flex items-center justify-center px-4 py-12 overflow-y-auto"
          style={{
            background: 'rgba(0,0,0,0.88)',
            backdropFilter: 'blur(14px)',
            zIndex: 10001,
            scrollbarWidth: 'none',
            msOverflowStyle: 'none',
          } as React.CSSProperties}
          onClick={(e) => { if (e.target === e.currentTarget) onClose(); }}
        >
          <motion.div
            initial={{ scale: 0.9, opacity: 0, y: 20 }}
            animate={{ scale: 1, opacity: 1, y: 0 }}
            exit={{ scale: 0.9, opacity: 0, y: 20 }}
            transition={{ type: 'spring', damping: 20 }}
            className="no-drag relative w-full max-w-[420px] rounded-3xl overflow-hidden my-auto"
            onClick={(e) => e.stopPropagation()}
            style={{
              background: 'linear-gradient(160deg, #0d0d20 0%, #0a0a15 100%)',
              border: '1px solid rgba(100,200,255,0.15)',
              boxShadow: '0 40px 80px -20px rgba(0,0,0,0.8), 0 0 0 1px rgba(100,200,255,0.08)',
            }}
          >
            {/* Header */}
            <div className="px-6 pt-6 pb-4 flex items-center justify-between">
              <div className="flex items-center gap-3">
                <div className="w-9 h-9 rounded-xl flex items-center justify-center shrink-0"
                     style={{ background: 'linear-gradient(135deg, #0088cc, #00aaff)' }}>
                  <svg width="20" height="20" viewBox="0 0 56 56" fill="none">
                    <path d="M28 6L6 18.5V28L28 52L50 28V18.5L28 6Z" fill="white" opacity="0.9"/>
                    <path d="M6 18.5H50" stroke="white" strokeWidth="3" opacity="0.5"/>
                    <path d="M28 6L17 18.5H39L28 6Z" fill="white" opacity="0.4"/>
                  </svg>
                </div>
                <div>
                  <h2 className="text-white font-bold text-base leading-tight">{t.tonPayTitle}</h2>
                  <p className="text-white/40 text-xs">{gameName}</p>
                </div>
              </div>
              <button
                onClick={(e) => { e.stopPropagation(); onClose(); }}
                className="no-drag w-8 h-8 rounded-xl flex items-center justify-center text-white/40 hover:text-white hover:bg-white/10 transition-colors"
              >
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                  <path d="M18 6L6 18M6 6l12 12"/>
                </svg>
              </button>
            </div>

            {isPaid ? (
              /* ── Success state ── */
              <div className="px-6 pb-6 flex flex-col items-center gap-4 pt-4">
                <motion.div
                  initial={{ scale: 0 }}
                  animate={{ scale: 1 }}
                  transition={{ type: 'spring', damping: 15 }}
                  className="w-20 h-20 rounded-full flex items-center justify-center"
                  style={{ background: 'rgba(16,185,129,0.15)', border: '2px solid rgba(16,185,129,0.4)' }}
                >
                  <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#10b981" strokeWidth="2.5">
                    <path d="M20 6L9 17l-5-5"/>
                  </svg>
                </motion.div>
                <p className="text-white font-bold text-xl">{t.tonPayPaid}</p>
                <p className="text-white/50 text-sm text-center">
                  <span className="text-white font-semibold">{gameName}</span> {t.tonPayPaidDesc}
                </p>
                <button
                  onClick={(e) => { e.stopPropagation(); onClose(); }}
                  className="no-drag mt-2 px-8 py-3 rounded-2xl font-bold text-white transition-all hover:brightness-110 active:scale-95"
                  style={{ background: 'linear-gradient(135deg, #059669, #10b981)' }}
                >
                  {t.tonPayDone}
                </button>
              </div>
            ) : (
              <div className="px-6 pb-6">
                {/* Amount */}
                <div className="flex items-center justify-between mb-5 px-4 py-3 rounded-2xl"
                     style={{ background: 'rgba(0,136,204,0.1)', border: '1px solid rgba(0,136,204,0.2)' }}>
                  <span className="text-white/50 text-sm">{t.tonPayAmount}</span>
                  <div>
                    <span className="text-white font-black text-2xl">{amountTon} TON</span>
                    <span className="text-white/40 text-sm ml-2">(~$1)</span>
                  </div>
                </div>

                {/* QR Code */}
                <div className="flex justify-center mb-4">
                  <div className="p-3 rounded-2xl bg-white">
                    <img src={qrUrl} alt="QR" width="186" height="186" className="rounded-lg block" />
                  </div>
                </div>

                <p className="text-white/40 text-xs text-center mb-5">
                  {t.tonPayScanHint}<br/>
                  <span className="text-white/60">{t.tonPayWallets}</span>
                </p>

                {/* Telegram Stars button */}
                {tgLink && (
                  <button
                    onClick={() => {
                      if ((window as any).electronAPI?.openExternal) {
                        (window as any).electronAPI.openExternal(tgLink);
                      } else {
                        window.open(tgLink, '_blank');
                      }
                      onPayWithStars?.();
                    }}
                    className="no-drag w-full flex items-center justify-center gap-2 py-3 rounded-2xl font-bold text-sm mb-5 transition-all hover:brightness-110 active:scale-95"
                    style={{
                      background: 'linear-gradient(135deg, #7c3aed, #a855f7)',
                      color: 'white',
                      boxShadow: '0 8px 24px -6px rgba(168,85,247,0.5)',
                    }}
                  >
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="white">
                      <path d="M12 0C5.373 0 0 5.373 0 12s5.373 12 12 12 12-5.373 12-12S18.627 0 12 0zm5.894 8.221-1.97 9.28c-.145.658-.537.818-1.084.508l-3-2.21-1.447 1.394c-.16.16-.295.295-.605.295l.213-3.053 5.56-5.023c.242-.213-.054-.333-.373-.12L7.19 13.6l-2.965-.924c-.643-.204-.657-.643.136-.953l11.57-4.461c.537-.194 1.006.131.963.959z"/>
                    </svg>
                    {t.tonPayStarsBtn}
                  </button>
                )}

                {/* Waiting indicator */}
                <div className="flex items-center justify-between px-4 py-3 rounded-2xl"
                     style={{ background: 'rgba(255,255,255,0.03)', border: '1px solid rgba(255,255,255,0.06)' }}>
                  <div className="flex items-center gap-2">
                    <span className="inline-block w-2 h-2 rounded-full bg-amber-400 animate-pulse"/>
                    <span className="text-white/50 text-sm">{t.tonPayWaiting}</span>
                  </div>
                  {secondsLeft > 0 && (
                    <span className="text-white/30 text-xs font-mono">
                      {minutes}:{String(seconds).padStart(2, '0')}
                    </span>
                  )}
                </div>
              </div>
            )}
          </motion.div>
        </motion.div>
      )}
    </AnimatePresence>
  );
}
