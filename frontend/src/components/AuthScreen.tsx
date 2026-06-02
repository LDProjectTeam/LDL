import React, { useState } from 'react';
import { useI18n } from '../i18n';
import { motion, AnimatePresence } from 'framer-motion';
import { supabase } from '../supabaseClient';
import { useAuth } from '../contexts/AuthContext';

export default function AuthScreen() {
    const { t } = useI18n();
    const [activeTab, setActiveTab] = useState<'login' | 'register'>('login');
    const [step, setStep] = useState<'email' | 'code'>('email');

    const [email, setEmail] = useState('');
    const [username, setUsername] = useState('');
    const [code, setCode] = useState('');

    const [isLoading, setIsLoading] = useState(false);
    const [error, setError] = useState('');
    const [successMessage, setSuccessMessage] = useState('');

    React.useEffect(() => {
        // @ts-ignore
        if (window.electronAPI && window.electronAPI.onOAuthCallback) {
            // @ts-ignore
            const cleanup = window.electronAPI.onOAuthCallback(async (authData: any) => {
                if (authData.error) {
                    setError(authData.error);
                } else if (authData.success && authData.session) {
                    const { error } = await supabase.auth.setSession({
                        access_token: authData.session.access_token,
                        refresh_token: authData.session.refresh_token
                    });
                    if (error) {
                        setError('Failed to establish session: ' + error.message);
                    }
                    // If successful, AuthContext will catch the session change and automatically log in
                }
            });
            return cleanup;
        }
    }, []);

    const handleGoogleLogin = async () => {
        setError('');
        try {
            const { data, error } = await supabase.auth.signInWithOAuth({
                provider: 'google',
                options: {
                    redirectTo: 'http://127.0.0.1:5174/oauth2callback',
                    skipBrowserRedirect: true
                }
            });
            if (error) throw error;
            if (data?.url) {
                // @ts-ignore
                window.electronAPI.startGoogleOAuth(data.url);
            }
        } catch (err: any) {
            setError(err.message || 'Error starting Google Auth');
        }
    };

    const handleSendCode = async (e: React.FormEvent) => {
        e.preventDefault();
        setError('');
        setSuccessMessage('');
        
        if (!email.includes('@')) {
            setError(t.authEmailInvalid);
            return;
        }

        setIsLoading(true);
        try {
            // Check if user exists first
            const { data: userExists, error: rpcError } = await supabase.rpc('check_user_exists', { input_email: email });
            if (rpcError) throw rpcError;

            // Validate based on the current tab
            if (activeTab === 'login' && !userExists) {
                throw new Error(t.authErrNoAccount);
            }

            if (activeTab === 'register' && userExists) {
                throw new Error(t.authErrExists);
            }

            const { error } = await supabase.auth.signInWithOtp({
                email,
                options: {
                    data: {
                        username: activeTab === 'register' ? username : undefined
                    }
                }
            });
            
            if (error) throw error;

            // @ts-ignore
            if (window.electronAPI?.startMagicLinkServer) {
                // @ts-ignore
                window.electronAPI.startMagicLinkServer();
            }

            setStep('code');
            setSuccessMessage(t.authCodeSent);
        } catch (err: any) {
            setError(err.message || 'Network error');
        } finally {
            setIsLoading(false);
        }
    };

    const handleVerifyCode = async (e: React.FormEvent) => {
        e.preventDefault();
        setError('');

        if (code.length < 4) {
            setError(t.authCodeError);
            return;
        }

        setIsLoading(true);
        try {
            const { error } = await supabase.auth.verifyOtp({
                email,
                token: code,
                type: 'email'
            });
            
            if (error) throw error;
            // If successful, AuthContext catches the session change
        } catch (err: any) {
            setError(err.message || 'Verification failed');
        } finally {
            setIsLoading(false);
        }
    };

    const handleBack = () => {
        setStep('email');
        setCode('');
        setError('');
        setSuccessMessage('');
    };

    return (
        <div className="fixed inset-0 bg-transparent flex items-center justify-center z-50 overflow-hidden font-mono select-none">
            {/* Background elements (subtle CRT glow) */}
            <div className="absolute top-[-20%] left-[-10%] w-[50%] h-[50%] bg-crt-glow opacity-[0.05] blur-[150px] rounded-full pointer-events-none"></div>
            <div className="absolute bottom-[-20%] right-[-10%] w-[40%] h-[40%] bg-crt-glow opacity-[0.04] blur-[120px] rounded-full pointer-events-none"></div>

            <motion.div
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ duration: 0.5, ease: "easeOut" }}
                className="w-full max-w-md bg-crt-bg/90 backdrop-blur-xl rounded-none border border-crt-accent shadow-crt-glow p-8 relative"
            >
                {/* Logo & Title */}
                <div className="flex flex-col items-center mb-8">
                    <div className="w-16 h-16 bg-crt-accent/20 border border-crt-glow rounded-none flex items-center justify-center mb-4 shadow-crt-glow">
                        <span className="text-crt-glow font-bold text-3xl tracking-tighter text-glow">LD</span>
                    </div>
                    <h1 className="text-crt-text text-2xl font-bold tracking-tight text-glow-subtle uppercase">
                        {step === 'email' ? t.authWelcomeBack : t.authVerifyTitle}
                    </h1>
                </div>

                {/* Tabs (Only show on email step) */}
                {step === 'email' && (
                    <div className="flex bg-crt-bg p-1 rounded-none mb-8 border border-crt-accent relative">
                        <div
                            className="absolute top-1 bottom-1 w-[calc(50%-4px)] bg-crt-accent/30 rounded-none transition-all duration-300 ease-in-out border border-crt-accent/50"
                            style={{ left: activeTab === 'login' ? '4px' : 'calc(50%)' }}
                        />
                        <button
                            className={`flex-1 py-2 text-sm font-bold uppercase relative z-10 transition-colors ${activeTab === 'login' ? 'text-crt-glow text-glow-subtle' : 'text-crt-accent hover:text-crt-text'}`}
                            onClick={() => { setActiveTab('login'); setError(''); }}
                        >
                            {t.authLoginTab}
                        </button>
                        <button
                            className={`flex-1 py-2 text-sm font-bold uppercase relative z-10 transition-colors ${activeTab === 'register' ? 'text-crt-glow text-glow-subtle' : 'text-crt-accent hover:text-crt-text'}`}
                            onClick={() => { setActiveTab('register'); setError(''); }}
                        >
                            {t.authRegisterTab}
                        </button>
                    </div>
                )}

                {/* Alerts */}
                <AnimatePresence>
                    {error && (
                        <motion.div
                            initial={{ opacity: 0, height: 0, marginBottom: 0 }}
                            animate={{ opacity: 1, height: 'auto', marginBottom: 16 }}
                            exit={{ opacity: 0, height: 0, marginBottom: 0 }}
                            className="bg-red-500/10 border border-red-500/30 text-red-400 px-4 py-3 rounded-lg text-sm font-medium"
                        >
                            {error}
                        </motion.div>
                    )}
                    {successMessage && (
                        <motion.div
                            initial={{ opacity: 0, height: 0, marginBottom: 0 }}
                            animate={{ opacity: 1, height: 'auto', marginBottom: 16 }}
                            exit={{ opacity: 0, height: 0, marginBottom: 0 }}
                            className="bg-crt-glow/10 border border-crt-glow text-crt-glow text-glow px-4 py-3 rounded-lg text-sm font-medium"
                        >
                            {successMessage}
                        </motion.div>
                    )}
                </AnimatePresence>

                {/* Forms */}
                <AnimatePresence mode="wait">
                    {step === 'email' ? (
                        <motion.form
                            key="email-form"
                            initial={{ opacity: 0, x: -20 }}
                            animate={{ opacity: 1, x: 0 }}
                            exit={{ opacity: 0, x: 20 }}
                            transition={{ duration: 0.3 }}
                            onSubmit={handleSendCode}
                            className="space-y-4"
                        >
                            <button
                                type="button"
                                onClick={handleGoogleLogin}
                                className="w-full bg-crt-bg border-2 border-crt-accent text-crt-text font-bold py-3.5 rounded-none hover:bg-crt-accent/20 hover:border-crt-glow hover:text-crt-glow hover:shadow-crt-glow active:scale-[0.98] transition-all flex justify-center items-center h-[52px] mb-4 gap-3 uppercase tracking-widest text-glow-subtle"
                            >
                                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                                    <path d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z" fill="#a0d0a0" />
                                    <path d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z" fill="#a0d0a0" />
                                    <path d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z" fill="#a0d0a0" />
                                    <path d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z" fill="#a0d0a0" />
                                </svg>
                                {t.authGoogleBtn}
                            </button>

                            <div className="flex items-center gap-4 py-2">
                                <div className="flex-1 h-px border-t border-dashed border-crt-accent"></div>
                                <span className="text-crt-accent text-xs font-bold uppercase tracking-widest">{t.authOrViaEmail}</span>
                                <div className="flex-1 h-px border-t border-dashed border-crt-accent"></div>
                            </div>

                            {activeTab === 'register' && (
                                <div>
                                    <input
                                        type="text"
                                        placeholder={t.authUsernamePlaceholder}
                                        value={username}
                                        onChange={(e) => setUsername(e.target.value)}
                                        required
                                        className="w-full bg-crt-bg/50 border-b-2 border-crt-accent text-crt-text px-4 py-3.5 rounded-none outline-none focus:border-crt-glow transition-colors placeholder:text-crt-accent/50 font-mono text-glow-subtle"
                                    />
                                </div>
                            )}
                            <div>
                                <input
                                    type="email"
                                    placeholder={t.authEmailPlaceholder}
                                    value={email}
                                    onChange={(e) => setEmail(e.target.value)}
                                    required
                                    className="w-full bg-crt-bg/50 border-b-2 border-crt-accent text-crt-text px-4 py-3.5 rounded-none outline-none focus:border-crt-glow transition-colors placeholder:text-crt-accent/50 font-mono text-glow-subtle"
                                />
                            </div>
                            <button
                                type="submit"
                                disabled={isLoading || !email}
                                className="w-full bg-crt-bg border-2 border-crt-accent hover:border-crt-glow hover:bg-crt-accent/10 text-crt-glow font-bold py-3.5 rounded-none hover:shadow-crt-glow active:scale-[0.98] transition-all disabled:opacity-50 disabled:active:scale-100 flex justify-center items-center h-[52px] uppercase tracking-widest text-glow"
                            >
                                {isLoading ? (
                                    <div className="w-5 h-5 border-2 border-crt-accent border-t-crt-glow rounded-full animate-spin" />
                                ) : (
                                    activeTab === 'login' ? t.authSendCode : t.authRegisterBtn
                                )}
                            </button>
                        </motion.form>
                    ) : (
                        <motion.form
                            key="code-form"
                            initial={{ opacity: 0, x: 20 }}
                            animate={{ opacity: 1, x: 0 }}
                            exit={{ opacity: 0, x: -20 }}
                            transition={{ duration: 0.3 }}
                            onSubmit={handleVerifyCode}
                            className="space-y-6"
                        >
                            <div>
                                <p className="text-crt-accent text-sm mb-1 text-center font-mono">
                                    {t.authEmailSentTo} <span className="text-crt-glow text-glow-subtle font-bold">{email}</span>
                                </p>
                                <p className="text-crt-accent/70 text-xs mb-4 text-center font-mono whitespace-pre-line">
                                    {t.authEmailSentHint}
                                </p>
                                <input
                                    type="text"
                                    placeholder={t.authCodePlaceholder}
                                    value={code}
                                    onChange={(e) => setCode(e.target.value.replace(/\D/g, '').slice(0, 8))}
                                    required
                                    className="w-full bg-crt-bg/50 border-b-2 border-crt-accent text-crt-glow px-4 py-4 rounded-none outline-none focus:border-crt-glow transition-colors placeholder:text-crt-accent/50 font-mono text-center text-2xl tracking-[0.5em] text-glow"
                                />
                            </div>
                            <div className="flex flex-col space-y-3">
                                <button
                                    type="submit"
                                    disabled={isLoading || code.length < 4}
                                    className="w-full bg-crt-bg border-2 border-crt-accent hover:border-crt-glow hover:bg-crt-accent/10 text-crt-glow font-bold py-3.5 rounded-none hover:shadow-crt-glow active:scale-[0.98] transition-all disabled:opacity-50 disabled:active:scale-100 flex justify-center items-center h-[52px] uppercase tracking-widest text-glow"
                                >
                                    {isLoading ? (
                                        <div className="w-5 h-5 border-2 border-crt-accent border-t-crt-glow rounded-full animate-spin" />
                                    ) : (
                                        t.authEnter
                                    )}
                                </button>
                                <button
                                    type="button"
                                    onClick={handleBack}
                                    className="text-crt-accent hover:text-crt-glow text-sm font-bold uppercase tracking-widest transition-colors text-glow-subtle"
                                >
                                    {t.authBack}
                                </button>
                            </div>
                        </motion.form>
                    )}
                </AnimatePresence>
            </motion.div>
        </div>
    );
}
