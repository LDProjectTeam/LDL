const fs = require('fs');
const path = require('path');

const componentsDir = path.join(__dirname, 'src', 'components');

const replacements = [
    // Backgrounds
    { regex: /bg-\[#(1A1A1A|1a1a1a|2B2B2B|2b2b2b|0F0F0F|0f0f0f)\]/g, replacement: 'bg-crt-bg' },
    { regex: /bg-black\/[0-9]+/g, replacement: 'bg-crt-bg/90' },
    { regex: /bg-\[#3D3D3D\]/g, replacement: 'bg-crt-accent/30' },
    { regex: /bg-white\/[0-9]+/g, replacement: 'bg-crt-accent/20' },
    { regex: /hover:bg-white\/[0-9]+/g, replacement: 'hover:bg-crt-accent/30' },
    
    // Borders
    { regex: /border-\[#(3D3D3D|3d3d3d)\]/g, replacement: 'border-crt-accent' },
    { regex: /border-white\/[0-9]+/g, replacement: 'border-crt-accent' },
    
    // Texts
    { regex: /text-\[#(A0A0A0|a0a0a0|888888|555555)\]/g, replacement: 'text-crt-accent' },
    { regex: /text-\[#(CCCCCC|cccccc)\]/g, replacement: 'text-crt-text' },
    { regex: /text-white\/[0-9]+/g, replacement: 'text-crt-text text-glow-subtle' },
    { regex: /text-white(?!(\/|\]))/g, replacement: 'text-crt-glow text-glow-subtle' },
    { regex: /hover:text-white/g, replacement: 'hover:text-crt-glow' },
    
    // Green (Old Theme) to CRT Glow
    { regex: /bg-\[#(7CFC00|7cfc00)\]/g, replacement: 'bg-crt-glow' },
    { regex: /text-\[#(7CFC00|7cfc00)\]/g, replacement: 'text-crt-glow text-glow' },
    { regex: /border-\[#(7CFC00|7cfc00)\](\/[0-9]+)?/g, replacement: 'border-crt-glow' },
    
    // Hardcoded inline colors
    { regex: /background:\s*'#2B2B2B'/g, replacement: "background: '#020205'" },
    { regex: /border:\s*'1px solid #3D3D3D'/g, replacement: "border: '1px solid #507090'" },
    
    // Modals specific
    { regex: /bg-\[#161616\]/g, replacement: 'bg-crt-bg' },
];

function processDirectory(directory) {
    const files = fs.readdirSync(directory);
    for (const file of files) {
        const fullPath = path.join(directory, file);
        if (fs.statSync(fullPath).isDirectory()) {
            processDirectory(fullPath);
        } else if (fullPath.endsWith('.tsx') || fullPath.endsWith('.ts')) {
            let content = fs.readFileSync(fullPath, 'utf8');
            let originalContent = content;
            
            for (const { regex, replacement } of replacements) {
                content = content.replace(regex, replacement);
            }
            
            if (content !== originalContent) {
                fs.writeFileSync(fullPath, content, 'utf8');
                console.log(`Updated ${fullPath}`);
            }
        }
    }
}

processDirectory(componentsDir);
console.log("Done.");
