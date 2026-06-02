export default function TitleBar() {
    return (
        <div className="absolute top-0 right-0 h-10 flex items-center z-50 no-drag">
            <button 
                onClick={() => window.electronAPI?.minimizeWindow?.()} 
                className="w-12 h-10 flex items-center justify-center hover:bg-crt-accent/20 transition-colors"
                title="Minimize"
            >
                <svg width="12" height="1" viewBox="0 0 12 1" fill="white">
                    <rect width="12" height="1" />
                </svg>
            </button>
            <button 
                onClick={() => window.electronAPI?.closeWindow?.()} 
                className="w-12 h-10 flex items-center justify-center hover:bg-red-500 transition-colors group"
                title="Close"
            >
                <svg width="12" height="12" viewBox="0 0 12 12" className="text-crt-glow text-glow-subtle">
                    <path fill="currentColor" d="M11.5 1.5l-1-1-4.5 4.5-4.5-4.5-1 1 4.5 4.5-4.5 4.5 1 1 4.5-4.5 4.5 4.5 1-1-4.5-4.5z" />
                </svg>
            </button>
        </div>
    );
}
