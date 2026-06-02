import BaseModal from './BaseModal';
import { useI18n } from '../../i18n';
import { useAuth } from '../../contexts/AuthContext';
import { useState, useEffect, useRef } from 'react';

/** Resize an image File to maxSize×maxSize JPEG and return a base64 data URL. */
function resizeImageToDataURL(file: File, maxSize = 128): Promise<string> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (e) => {
            const img = new Image();
            img.onload = () => {
                const ratio = Math.min(maxSize / img.width, maxSize / img.height);
                const canvas = document.createElement('canvas');
                canvas.width  = Math.round(img.width  * ratio);
                canvas.height = Math.round(img.height * ratio);
                canvas.getContext('2d')!.drawImage(img, 0, 0, canvas.width, canvas.height);
                resolve(canvas.toDataURL('image/jpeg', 0.8));
            };
            img.onerror = reject;
            img.src = e.target?.result as string;
        };
        reader.onerror = reject;
        reader.readAsDataURL(file);
    });
}

interface ProfileModalProps {
    isOpen: boolean;
    onClose: () => void;
}

export default function ProfileModal({ isOpen, onClose }: ProfileModalProps) {
    const { t } = useI18n();
    const { user, updateProfile, linkMicrosoft, unlinkMicrosoft } = useAuth();

    const [username, setUsername] = useState('');
    const [avatarPreview, setAvatarPreview] = useState<string>('');
    const [avatarFile, setAvatarFile] = useState<File | null>(null);
    const [isSaving, setIsSaving] = useState(false);
    const [isLinkingMS, setIsLinkingMS] = useState(false);
    const [isUnlinkingMS, setIsUnlinkingMS] = useState(false);
    const [saveError, setSaveError] = useState<string | null>(null);
    const [msConfirmProfile, setMsConfirmProfile] = useState<{ name: string; id: string } | null>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);

    // Sync form state when modal opens or user changes
    useEffect(() => {
        if (isOpen) {
            setUsername(user?.username || '');
            setAvatarPreview(user?.avatarUrl || '');
            setAvatarFile(null);
            setSaveError(null);
        }
    }, [isOpen, user]);

    const handleAvatarClick = () => fileInputRef.current?.click();

    const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;
        if (file.size > 2 * 1024 * 1024) {
            setSaveError(t.profileErrTooBig);
            return;
        }
        setAvatarFile(file);
        setAvatarPreview(URL.createObjectURL(file));
        setSaveError(null);
    };

    const handleSave = async () => {
        if (!username.trim()) {
            setSaveError(t.profileErrEmpty);
            return;
        }
        setIsSaving(true);
        setSaveError(null);
        try {
            let newAvatarUrl: string | undefined;

            // Resize image client-side (128×128 JPEG ~3-5 KB) and store as base64
            // in user_metadata — no Supabase Storage bucket needed.
            if (avatarFile) {
                newAvatarUrl = await resizeImageToDataURL(avatarFile, 128);
            }

            await updateProfile(username.trim(), newAvatarUrl);
            onClose();
        } catch (e: any) {
            setSaveError(e?.message || t.profileErrSave);
        } finally {
            setIsSaving(false);
        }
    };

    const handleLinkMicrosoft = async () => {
        setSaveError(null);
        setIsLinkingMS(true);
        try {
            // @ts-ignore
            const result = await window.electronAPI.startMicrosoftAuth();
            if (!result.success) {
                throw new Error(result.error || 'Failed to link Microsoft account');
            }

            const msProfile = result.profile;
            await linkMicrosoft(msProfile);

            // Show the custom confirm dialog instead of native confirm()
            setMsConfirmProfile({ name: msProfile.name, id: msProfile.id });
        } catch (e: any) {
            setSaveError(e?.message || t.profileErrLink);
        } finally {
            setIsLinkingMS(false);
        }
    };

    const handleMsConfirmApply = async () => {
        if (!msConfirmProfile) return;
        const msAvatarUrl = `https://mc-heads.net/avatar/${msConfirmProfile.id}/128`;
        setUsername(msConfirmProfile.name);
        setAvatarPreview(msAvatarUrl);
        setAvatarFile(null);
        await updateProfile(msConfirmProfile.name, msAvatarUrl);
        setMsConfirmProfile(null);
    };

    const handleMsConfirmSkip = () => setMsConfirmProfile(null);

    const handleUnlinkMicrosoft = async () => {
        setSaveError(null);
        setIsUnlinkingMS(true);
        try {
            await unlinkMicrosoft();
        } catch (e: any) {
            setSaveError(e?.message || t.profileErrUnlink);
        } finally {
            setIsUnlinkingMS(false);
        }
    };

    return (
        <BaseModal isOpen={isOpen} onClose={onClose} title={t.profileManagement} width="400px">
            {/* Custom MS confirm overlay */}
            {msConfirmProfile && (
                <div className="absolute inset-0 bg-crt-bg/95 backdrop-blur-sm z-10 flex flex-col items-center justify-center p-8 rounded-none">
                    <img
                        src={`https://mc-heads.net/avatar/${msConfirmProfile.id}/96`}
                        alt="Minecraft Avatar"
                        className="w-16 h-16 mb-4 border-2 border-crt-accent"
                        style={{ imageRendering: 'pixelated' }}
                    />
                    <p className="text-crt-glow font-bold text-base text-glow-subtle text-center mb-1">{msConfirmProfile.name}</p>
                    <p className="text-crt-accent text-xs font-mono text-center mb-6 uppercase tracking-widest">{t.profileUseMicrosoftDataTitle}</p>
                    <p className="text-crt-text text-sm text-center mb-8 leading-relaxed opacity-80">{t.profileUseMicrosoftDataDesc}</p>
                    <div className="flex gap-3 w-full">
                        <button
                            onClick={handleMsConfirmSkip}
                            className="flex-1 py-2.5 border border-crt-accent text-crt-accent font-bold text-sm uppercase tracking-widest hover:border-crt-glow hover:text-crt-glow transition-all"
                        >
                            {t.profileUseMicrosoftDataSkip}
                        </button>
                        <button
                            onClick={handleMsConfirmApply}
                            className="flex-1 py-2.5 bg-crt-glow text-black font-bold text-sm uppercase tracking-widest hover:bg-[#68D900] active:scale-[0.98] transition-all"
                        >
                            {t.profileUseMicrosoftDataApply}
                        </button>
                    </div>
                </div>
            )}
            <div className="flex flex-col items-center">
                {/* Avatar */}
                <div className="relative group cursor-pointer mb-8" onClick={handleAvatarClick}>
                    <input
                        ref={fileInputRef}
                        type="file"
                        accept="image/*"
                        className="hidden"
                        onChange={handleFileChange}
                    />
                    <div className="h-24 w-24 overflow-hidden border-4 border-crt-accent group-hover:border-crt-glow transition-colors bg-crt-bg">
                        {avatarPreview ? (
                            <img src={avatarPreview} alt="Avatar" className="w-full h-full object-cover" />
                        ) : (
                            <svg className="w-full h-full text-crt-accent p-4" viewBox="0 0 24 24" fill="currentColor">
                                <path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" />
                            </svg>
                        )}
                    </div>
                    <div className="absolute inset-0 bg-crt-bg/80 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="text-crt-glow">
                            <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
                            <polyline points="17 8 12 3 7 8" />
                            <line x1="12" y1="3" x2="12" y2="15" />
                        </svg>
                    </div>
                </div>
                <p className="text-xs text-crt-accent mb-6 -mt-5 opacity-60">{t.profileAvatarHint}</p>

                <div className="w-full space-y-4">
                    <div>
                        <label className="block text-xs font-bold text-crt-accent uppercase tracking-wide mb-1">
                            {t.profileUsername}
                        </label>
                        <input
                            type="text"
                            value={username}
                            onChange={(e) => { setUsername(e.target.value); setSaveError(null); }}
                            className="w-full bg-crt-bg border border-crt-accent rounded-md px-4 py-2.5 text-crt-glow font-medium focus:outline-none focus:border-crt-glow transition-colors"
                        />
                    </div>
                </div>

                {saveError && (
                    <p className="mt-4 text-xs text-red-400 w-full text-center">{saveError}</p>
                )}

                {/* Microsoft Account Linking */}
                <div className="w-full mt-6 pt-6 border-t border-crt-accent/30 space-y-3">
                    {/* Header row: title + description */}
                    <div>
                        <p className="text-sm font-bold text-crt-text">{t.profileLinkMicrosoft}</p>
                        <p className="text-xs text-crt-accent mt-0.5">{t.profileLinkMicrosoftDesc}</p>
                    </div>

                    {/* Action row */}
                    {user?.minecraftLicense ? (
                        <div className="flex items-center gap-2 w-full">
                            {/* Green "linked" badge — takes all space */}
                            <div className="flex-1 flex items-center gap-2 px-3 py-2 bg-[#107C10]/10 border border-[#107C10] text-[#107C10] text-xs font-bold">
                                <svg width="13" height="13" viewBox="0 0 24 24" fill="none">
                                    <path d="M11.4 24H0V12.6h11.4V24zM24 24H12.6V12.6H24V24zM11.4 11.4H0V0h11.4v11.4zM24 11.4H12.6V0H24v11.4z" fill="currentColor" />
                                </svg>
                                {t.profileMicrosoftLinked}
                            </div>
                            {/* Red unlink button */}
                            <button
                                onClick={handleUnlinkMicrosoft}
                                disabled={isUnlinkingMS}
                                className="flex items-center justify-center gap-1.5 px-4 py-2 border border-red-500/50 text-red-400 text-xs font-bold hover:border-red-500 hover:bg-red-500/10 transition-all disabled:opacity-50 shrink-0"
                            >
                                {isUnlinkingMS ? (
                                    <div className="w-3 h-3 border-2 border-red-400 border-t-transparent rounded-full animate-spin" />
                                ) : (
                                    t.profileUnlinkMicrosoft
                                )}
                            </button>
                        </div>
                    ) : (
                        /* Full-width link button */
                        <button
                            onClick={handleLinkMicrosoft}
                            disabled={isLinkingMS}
                            className="w-full flex items-center justify-center gap-2 px-4 py-2.5 bg-crt-bg border border-[#107C10]/50 text-[#107C10] text-sm font-bold hover:bg-[#107C10]/20 hover:border-[#107C10] hover:shadow-[0_0_8px_rgba(16,124,16,0.4)] transition-all disabled:opacity-50"
                        >
                            {isLinkingMS ? (
                                <div className="w-4 h-4 border-2 border-[#107C10] border-t-transparent rounded-full animate-spin" />
                            ) : (
                                <>
                                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none">
                                        <path d="M11.4 24H0V12.6h11.4V24zM24 24H12.6V12.6H24V24zM11.4 11.4H0V0h11.4v11.4zM24 11.4H12.6V0H24v11.4z" fill="currentColor" />
                                    </svg>
                                    {t.profileLinkMicrosoft}
                                </>
                            )}
                        </button>
                    )}
                </div>

                <button
                    onClick={handleSave}
                    disabled={isSaving}
                    className="w-full mt-6 bg-crt-glow text-black font-bold py-3 rounded-md hover:bg-[#68D900] active:scale-[0.98] transition-all disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2"
                >
                    {isSaving ? (
                        <>
                            <div className="w-4 h-4 border-2 border-black border-t-transparent rounded-full animate-spin" />
                            {t.profileSaving}
                        </>
                    ) : (
                        t.profileSaveBtn
                    )}
                </button>
            </div>
        </BaseModal>
    );
}
