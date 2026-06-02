import BaseModal from './BaseModal';
import { useI18n } from '../../i18n';

interface LegalModalProps {
    isOpen: boolean;
    onClose: () => void;
}

export default function LegalModal({ isOpen, onClose }: LegalModalProps) {
    const { t } = useI18n();

    return (
        <BaseModal isOpen={isOpen} onClose={onClose} title={t.legalInfo} width="600px">
            <div className="flex flex-col space-y-6 text-crt-text text-[13px] leading-relaxed">
                <div>
                    <h3 className="text-crt-glow text-glow-subtle font-bold text-sm mb-2">{t.legalTermsTitle}</h3>
                    <p className="mb-2">{t.legalTermsP1}</p>
                    <p>{t.legalTermsP2}</p>
                </div>

                <div>
                    <h3 className="text-crt-glow text-glow-subtle font-bold text-sm mb-2">{t.legalPrivacyTitle}</h3>
                    <p className="mb-2">{t.legalPrivacyP1}</p>
                    <p>{t.legalPrivacyP2}</p>
                </div>

                <div>
                    <h3 className="text-crt-glow text-glow-subtle font-bold text-sm mb-2">{t.legalLicenseTitle}</h3>
                    <p>{t.legalLicenseP1}</p>
                </div>

                <div className="pt-4 border-t border-crt-accent text-center text-crt-accent text-xs">
                    <p>© 2026 LDProject. All rights reserved.</p>
                    <p className="mt-1">{t.versionStr} 3.1.0</p>
                </div>
            </div>
        </BaseModal>
    );
}
