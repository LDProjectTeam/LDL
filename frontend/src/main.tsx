import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import './index.css';
import { AuthProvider } from './contexts/AuthContext';
import { I18nProvider } from './i18n';

ReactDOM.createRoot(document.getElementById('root')!).render(
    <React.StrictMode>
        <I18nProvider>
            <AuthProvider>
                <App />
            </AuthProvider>
        </I18nProvider>
    </React.StrictMode>
);
