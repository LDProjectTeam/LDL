import { useState, useRef, useEffect, useCallback } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { supabase } from '../../supabaseClient';
import { useI18n } from '../../i18n';
import { useAuth } from '../../contexts/AuthContext';

const TICKET_KEY = 'ldl_support_ticket_id';

type Status = 'idle' | 'sending' | 'error';

interface SupportModalProps {
    isOpen: boolean;
    onClose: () => void;
}

interface ChatMessage {
    id: string;
    from: 'support' | 'user';
    text: string;
    time: string;
}

function getNow() {
    return new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}
function fmtTime(iso: string) {
    return new Date(iso).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

function Avatar({ from, avatarUrl }: { from: 'support' | 'user'; avatarUrl?: string }) {
    if (from === 'support') {
        return (
            <div className="flex-shrink-0 w-7 h-7 rounded-full flex items-center justify-center" style={{ background: 'linear-gradient(135deg,#1e3a5f,#4a8fd4)' }}>
                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="white" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                    <path d="M21 15a2 2 0 01-2 2H7l-4 4V5a2 2 0 012-2h14a2 2 0 012 2z"/>
                </svg>
            </div>
        );
    }
    return (
        <div className="flex-shrink-0 w-7 h-7 rounded-full flex items-center justify-center overflow-hidden relative" style={{ background: 'linear-gradient(135deg,#3a6040,#60c070)' }}>
            {avatarUrl
                ? <img src={avatarUrl} className="absolute inset-0 w-full h-full object-cover" alt=""/>
                : <svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor" className="text-white"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/></svg>
            }
        </div>
    );
}

const bubbleVariants = {
    hidden: { opacity: 0, y: 8, scale: 0.95 },
    visible: { opacity: 1, y: 0, scale: 1, transition: { duration: 0.2, ease: 'easeOut' } },
};

export default function SupportModal({ isOpen, onClose }: SupportModalProps) {
    const { t } = useI18n();
    const { user } = useAuth();

    const [status, setStatus]     = useState<Status>('idle');
    const [message, setMessage]   = useState('');
    const [chatLog, setChatLog]   = useState<ChatMessage[]>([]);
    const [inputFocused, setInputFocused] = useState(false);
    const [showForm, setShowForm] = useState(false);
    const [ticketId, setTicketId] = useState<string | null>(null);
    const [name, setName]         = useState('');
    const [email, setEmail]       = useState('');

    const chatEndRef    = useRef<HTMLDivElement>(null);
    const textareaRef   = useRef<HTMLTextAreaElement>(null);
    const subRef        = useRef<any>(null);
    const ticketIdRef   = useRef<string | null>(null);

    // ── OPEN MODAL ──
    useEffect(() => {
        if (!isOpen) return;
        setStatus('idle');
        setMessage('');
        setName(user?.username || '');
        setEmail(user?.email || '');
        setChatLog([{ id: '0', from: 'support', text: t.supportWelcome, time: getNow() }]);
        setTimeout(() => setShowForm(true), 500);

        // Try to restore existing ticket
        const saved = localStorage.getItem(TICKET_KEY);
        if (saved) {
            loadExistingTicket(saved);
        }

        return () => {
            if (subRef.current) { supabase.removeChannel(subRef.current); subRef.current = null; }
        };
    // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [isOpen]);

    async function loadExistingTicket(id: string) {
        const { data } = await supabase
            .from('support_tickets')
            .select('id')
            .eq('id', id)
            .maybeSingle();
        if (!data) { localStorage.removeItem(TICKET_KEY); return; }

        setTicketId(id);
        ticketIdRef.current = id;

        // Load previous messages
        const { data: msgs } = await supabase
            .from('support_messages')
            .select('*')
            .eq('ticket_id', id)
            .order('created_at', { ascending: true });

        if (msgs && msgs.length) {
            const restored: ChatMessage[] = msgs.map((m: any) => ({
                id: m.id,
                from: m.from_admin ? 'support' : 'user',
                text: m.content,
                time: fmtTime(m.created_at),
            }));
            // Replace welcome with history
            setChatLog(restored);
        }

        subscribeToMessages(id);
    }

    function subscribeToMessages(tid: string) {
        if (subRef.current) supabase.removeChannel(subRef.current);
        const ch = supabase.channel('support-user-' + tid)
            .on('postgres_changes', {
                event: 'INSERT', schema: 'public',
                table: 'support_messages',
                filter: `ticket_id=eq.${tid}`,
            }, (payload: any) => {
                const m = payload.new;
                // Only show admin replies (user messages we add optimistically)
                if (!m.from_admin) return;
                const msg: ChatMessage = {
                    id: m.id,
                    from: 'support',
                    text: m.content,
                    time: fmtTime(m.created_at),
                };
                setChatLog(prev => [...prev, msg]);
            })
            .subscribe();
        subRef.current = ch;
    }

    // ── ESCAPE ──
    useEffect(() => {
        const h = (e: KeyboardEvent) => { if (e.key === 'Escape' && isOpen) onClose(); };
        window.addEventListener('keydown', h);
        return () => window.removeEventListener('keydown', h);
    }, [isOpen, onClose]);

    // ── AUTO-SCROLL ──
    useEffect(() => {
        chatEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [chatLog, showForm]);

    // ── SEND ──
    const handleSend = useCallback(async () => {
        if (!message.trim() || status === 'sending') return;

        const text = message.trim();
        const userMsg: ChatMessage = { id: Date.now().toString(), from: 'user', text, time: getNow() };
        setChatLog(prev => [...prev, userMsg]);
        setMessage('');
        setStatus('sending');

        try {
            let tid = ticketIdRef.current;

            // Create ticket on first message
            if (!tid) {
                const uname = name || user?.username || 'Unknown';
                const uemail = email || user?.email || 'no-email';
                const { data: ticket, error: tErr } = await supabase
                    .from('support_tickets')
                    .upsert(
                        { user_email: uemail, username: uname, avatar_url: user?.avatarUrl || null,
                          last_message: text, updated_at: new Date().toISOString(), unread_count: 1 },
                        { onConflict: 'user_email', ignoreDuplicates: false }
                    )
                    .select('id')
                    .single();

                if (tErr) throw tErr;
                tid = ticket.id;
                setTicketId(tid!);
                ticketIdRef.current = tid!;
                localStorage.setItem(TICKET_KEY, tid!);
                subscribeToMessages(tid!);
            } else {
                // Read current unread then bump it
                const { data: cur } = await supabase
                    .from('support_tickets')
                    .select('unread_count')
                    .eq('id', tid)
                    .single();
                await supabase.from('support_tickets').update({
                    last_message: text,
                    updated_at: new Date().toISOString(),
                    unread_count: (cur?.unread_count ?? 0) + 1,
                }).eq('id', tid);
            }

            // Insert message
            const { error: mErr } = await supabase
                .from('support_messages')
                .insert({ ticket_id: tid, content: text, from_admin: false });

            if (mErr) throw mErr;
            setStatus('idle');

        } catch (err) {
            console.error('[Support]', err);
            setStatus('error');
            setChatLog(prev => [...prev, { id: Date.now() + '_e', from: 'support', text: t.supportError, time: getNow() }]);
        }
    }, [message, status, name, email, user, t]);

    const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
        if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); handleSend(); }
    };

    const canSend = message.trim().length > 0 && status !== 'sending';
    const needsForm = showForm && (!user?.email || !user?.username) && !ticketId;

    return (
        <AnimatePresence>
            {isOpen && (
                <div className="fixed inset-0 z-[100] flex items-end justify-start" style={{ paddingLeft: '90px', paddingBottom: '24px' }}>
                    {/* Backdrop */}
                    <motion.div
                        initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }}
                        transition={{ duration: 0.2 }}
                        className="absolute inset-0 bg-black/40 backdrop-blur-[2px]"
                        onClick={onClose}
                    />

                    {/* Chat Window */}
                    <motion.div
                        initial={{ opacity: 0, y: 30, scale: 0.96 }}
                        animate={{ opacity: 1, y: 0, scale: 1 }}
                        exit={{ opacity: 0, y: 30, scale: 0.96 }}
                        transition={{ duration: 0.25, ease: 'easeOut' }}
                        className="relative z-10 flex flex-col rounded-xl overflow-hidden"
                        style={{ width: '360px', height: '520px', background: 'linear-gradient(180deg,#0d1118 0%,#090c12 100%)', border: '1px solid #1e3a5a', boxShadow: '0 20px 60px rgba(0,0,0,.75), 0 0 40px rgba(74,143,212,.12)' }}
                    >
                        {/* Header */}
                        <div className="flex items-center justify-between px-4 py-3 flex-shrink-0"
                            style={{ background: 'linear-gradient(90deg,#0b1420,#0f1e30)', borderBottom: '1px solid rgba(30,58,90,.8)' }}>
                            <div className="flex items-center gap-3">
                                <div className="relative">
                                    <div className="w-9 h-9 rounded-full flex items-center justify-center" style={{ background: 'linear-gradient(135deg,#1e3a5f,#4a8fd4)' }}>
                                        <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="white" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                                            <path d="M21 15a2 2 0 01-2 2H7l-4 4V5a2 2 0 012-2h14a2 2 0 012 2z"/>
                                        </svg>
                                    </div>
                                    <div className="absolute bottom-0 right-0 w-2.5 h-2.5 rounded-full border-2"
                                        style={{ background: '#7CFC00', borderColor: '#0b1420', boxShadow: '0 0 6px #7CFC00' }}/>
                                </div>
                                <div>
                                    <div className="text-[13px] font-bold text-white tracking-wide">LDL Support</div>
                                    <div className="text-[11px] font-medium" style={{ color: '#7CFC00' }}>● {t.online}</div>
                                </div>
                            </div>
                            <button onClick={onClose} className="p-1.5 rounded-lg transition-colors hover:bg-white/10" style={{ color: '#4a8fd4' }}>
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                    <line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/>
                                </svg>
                            </button>
                        </div>

                        {/* Messages */}
                        <div className="flex-1 overflow-y-auto px-4 py-3 flex flex-col gap-3" style={{ scrollbarWidth: 'none' }}>
                            <AnimatePresence initial={false}>
                                {chatLog.map(msg => (
                                    <motion.div key={msg.id} variants={bubbleVariants} initial="hidden" animate="visible"
                                        className={`flex gap-2 ${msg.from === 'user' ? 'flex-row-reverse' : 'flex-row'}`}>
                                        <Avatar from={msg.from} avatarUrl={msg.from === 'user' ? user?.avatarUrl : undefined}/>
                                        <div className={`flex flex-col max-w-[75%] ${msg.from === 'user' ? 'items-end' : 'items-start'}`}>
                                            <div className="px-3 py-2 rounded-xl text-[13px] leading-relaxed"
                                                style={msg.from === 'support'
                                                    ? { background: 'rgba(74,143,212,.12)', border: '1px solid rgba(74,143,212,.28)', color: '#c8d8ea', borderRadius: '4px 14px 14px 14px' }
                                                    : { background: 'linear-gradient(135deg,#1a3a52,#1e4060)', border: '1px solid rgba(74,143,212,.3)', color: '#d0e8fa', borderRadius: '14px 4px 14px 14px' }
                                                }>
                                                {msg.text}
                                            </div>
                                            <div className="text-[10px] mt-1 px-1" style={{ color: '#3d5a78' }}>{msg.time}</div>
                                        </div>
                                    </motion.div>
                                ))}
                            </AnimatePresence>

                            {/* Typing dots while sending */}
                            <AnimatePresence>
                                {status === 'sending' && (
                                    <motion.div initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} exit={{ opacity: 0 }} className="flex items-center gap-2">
                                        <Avatar from="support"/>
                                        <div className="px-3 py-2.5 rounded-xl flex gap-1.5 items-center"
                                            style={{ background: 'rgba(74,143,212,.12)', border: '1px solid rgba(74,143,212,.25)', borderRadius: '4px 14px 14px 14px' }}>
                                            {[0, 1, 2].map(i => (
                                                <motion.div key={i} className="w-1.5 h-1.5 rounded-full"
                                                    style={{ background: '#4a8fd4' }}
                                                    animate={{ y: [0, -4, 0] }}
                                                    transition={{ duration: 0.7, repeat: Infinity, delay: i * 0.15 }}/>
                                            ))}
                                        </div>
                                    </motion.div>
                                )}
                            </AnimatePresence>

                            <div ref={chatEndRef}/>
                        </div>

                        {/* Name / Email form (only if not logged in fully) */}
                        <AnimatePresence>
                            {needsForm && (
                                <motion.div initial={{ opacity: 0, height: 0 }} animate={{ opacity: 1, height: 'auto' }} exit={{ opacity: 0, height: 0 }}
                                    className="px-4 pb-2 flex gap-2" style={{ borderTop: '1px solid rgba(30,58,90,.5)' }}>
                                    {!user?.username && (
                                        <input type="text" value={name} onChange={e => setName(e.target.value)}
                                            placeholder={t.supportNamePlaceholder}
                                            className="flex-1 px-3 py-1.5 rounded-lg text-[12px] outline-none mt-2"
                                            style={{ background: 'rgba(74,143,212,.08)', border: '1px solid rgba(74,143,212,.25)', color: '#c8d8ea' }}/>
                                    )}
                                    {!user?.email && (
                                        <input type="email" value={email} onChange={e => setEmail(e.target.value)}
                                            placeholder={t.supportEmailPlaceholder}
                                            className="flex-1 px-3 py-1.5 rounded-lg text-[12px] outline-none mt-2"
                                            style={{ background: 'rgba(74,143,212,.08)', border: '1px solid rgba(74,143,212,.25)', color: '#c8d8ea' }}/>
                                    )}
                                </motion.div>
                            )}
                        </AnimatePresence>

                        {/* Input area */}
                        <div className="flex-shrink-0 px-3 pb-3 pt-2 flex gap-2 items-end"
                            style={{ borderTop: '1px solid rgba(30,58,90,.6)' }}>
                            <div className="flex-1 flex items-end rounded-xl transition-all duration-200"
                                style={{
                                    background: 'rgba(74,143,212,.07)',
                                    border: `1px solid ${inputFocused ? 'rgba(74,143,212,.6)' : 'rgba(30,58,90,.8)'}`,
                                    boxShadow: inputFocused ? '0 0 12px rgba(74,143,212,.15)' : 'none',
                                }}>
                                <textarea
                                    ref={textareaRef}
                                    value={message}
                                    onChange={e => setMessage(e.target.value)}
                                    onKeyDown={handleKeyDown}
                                    onFocus={() => setInputFocused(true)}
                                    onBlur={() => setInputFocused(false)}
                                    placeholder={t.supportMessagePlaceholder}
                                    rows={1}
                                    className="flex-1 px-3 py-2.5 text-[13px] bg-transparent outline-none resize-none leading-relaxed"
                                    style={{ color: '#c8d8ea', maxHeight: '100px', minHeight: '40px', scrollbarWidth: 'none' }}
                                    onInput={e => {
                                        const el = e.target as HTMLTextAreaElement;
                                        el.style.height = 'auto';
                                        el.style.height = Math.min(el.scrollHeight, 100) + 'px';
                                    }}
                                />
                            </div>

                            <motion.button
                                whileHover={canSend ? { scale: 1.08 } : {}}
                                whileTap={canSend ? { scale: 0.92 } : {}}
                                onClick={handleSend}
                                disabled={!canSend}
                                className="flex-shrink-0 w-10 h-10 rounded-xl flex items-center justify-center transition-all duration-200"
                                style={{
                                    background: canSend ? 'linear-gradient(135deg,#1e4a6e,#2d6090)' : 'rgba(74,143,212,.1)',
                                    border: `1px solid ${canSend ? 'rgba(74,143,212,.5)' : 'rgba(30,58,90,.5)'}`,
                                    boxShadow: canSend ? '0 4px 16px rgba(45,96,144,.4)' : 'none',
                                    cursor: canSend ? 'pointer' : 'default',
                                }}>
                                <svg width="15" height="15" viewBox="0 0 24 24" fill="none"
                                    stroke={canSend ? '#80b8ff' : '#3d5a78'} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                                    <line x1="22" y1="2" x2="11" y2="13"/><polygon points="22 2 15 22 11 13 2 9 22 2"/>
                                </svg>
                            </motion.button>
                        </div>

                        {/* Error retry */}
                        <AnimatePresence>
                            {status === 'error' && (
                                <motion.div initial={{ opacity: 0, height: 0 }} animate={{ opacity: 1, height: 'auto' }} exit={{ opacity: 0, height: 0 }}
                                    className="px-4 pb-3 flex justify-center">
                                    <button onClick={() => setStatus('idle')}
                                        className="text-[12px] font-bold px-4 py-1.5 rounded-lg transition-all"
                                        style={{ color: '#ff8080', border: '1px solid rgba(255,100,100,.3)', background: 'rgba(255,80,80,.08)' }}>
                                        {t.supportRetry}
                                    </button>
                                </motion.div>
                            )}
                        </AnimatePresence>

                        {/* Hint */}
                        <div className="text-center text-[10px] pb-2" style={{ color: '#1e3a5a' }}>
                            Enter — {t.supportSendHint} · Shift+Enter — {t.supportNewLine}
                        </div>
                    </motion.div>
                </div>
            )}
        </AnimatePresence>
    );
}
