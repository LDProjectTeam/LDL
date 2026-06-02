/// <reference types="vite/client" />

interface Window {
    electronAPI?: {
        closeWindow: () => void;
        openExternal: (url: string) => void;
        [key: string]: any;
    };
}
