const fs = require('fs');
const file = 'src/components/UserMenu.tsx';
let content = fs.readFileSync(file, 'utf8');

// Panel background and border
content = content.replace(
  /background: '#2B2B2B', border: '1px solid #3D3D3D', boxShadow: '4px 4px 25px rgba\(0,0,0,0\.8\)'/g,
  "background: '#020205', border: '1px solid #507090', boxShadow: '4px 4px 25px rgba(0,0,0,0.9), 0 0 15px rgba(80,112,144,0.2)'"
);

// Lang flyout background
content = content.replace(
  /background: '#2B2B2B',\s*\n\s*border: '1px solid #3D3D3D',\s*\n\s*boxShadow: '4px 4px 25px rgba\(0,0,0,0\.8\)',/g,
  "background: '#020205',\n                                                border: '1px solid #507090',\n                                                boxShadow: '4px 4px 25px rgba(0,0,0,0.9)',"
);

// Rounded corners -> none
content = content.replace(/rounded-md/g, 'rounded-none');
content = content.replace(/rounded-full(?!\s*object)/g, 'rounded-none');

// Panel borders, separators
content = content.replace(/bg-\[#3D3D3D\]/g, 'border-crt-accent/50');
content = content.replace(/h-px w-full bg-crt-accent\/50/g, 'h-px w-full border-t border-dashed border-crt-accent');
content = content.replace(/w-full h-px bg-crt-accent\/50/g, 'w-full h-px border-t border-dashed border-crt-accent');

// User info styles  
content = content.replace(/bg-\[#3D3D3D\] text-\[#A0A0A0\]/g, 'bg-crt-accent/20 border border-crt-accent text-crt-accent');
content = content.replace(/text-white tracking-wide/g, 'text-crt-glow tracking-widest text-glow-subtle');
content = content.replace(/border-\[#2B2B2B\]/g, 'border-crt-bg');
content = content.replace(/bg-\[#7CFC00\](?!\/)/g, 'bg-crt-glow shadow-crt-glow');
content = content.replace(/bg-\[#E50000\]/g, 'bg-red-500');

// Status text
content = content.replace(/'Не в сети'/g, "'OFFLINE'");
content = content.replace(/t\.online \|\| 'В сети'/g, "'ONLINE'");
content = content.replace(/#E50000/g, '#ff4444');
content = content.replace(/#7CFC00'/g, "#80b0ff'");

// Menu buttons
content = content.replace(/text-\[#CCCCCC\] hover:text-white hover:bg-white\/10/g, 'text-crt-text hover:text-crt-glow hover:bg-crt-accent/20');
content = content.replace(/text-\[#CCCCCC\] hover:text-white hover:bg-\[#E50000\]/g, 'text-crt-text hover:text-red-400 hover:bg-red-900/30');
content = content.replace(/text-\[#888888\] group-hover:text-white transition-colors/g, 'text-crt-accent group-hover:text-crt-glow transition-colors');
content = content.replace(/text-\[#888888\] group-hover:text-\[#CCCCCC\]/g, 'text-crt-accent group-hover:text-crt-text');

// Language picker active state
content = content.replace(/text-\[#7CFC00\] bg-\[#7CFC00\]\/5/g, 'text-crt-glow text-glow bg-crt-glow/10');
content = content.replace(/text-\[#CCCCCC\] hover:bg-white\/5 hover:text-white/g, 'text-crt-text hover:bg-crt-accent/20 hover:text-crt-glow');

// Language badge
content = content.replace(/bg-\[#1A1A1A\] text-\[10px\] text-\[#888888\] group-hover:text-\[#CCCCCC\] font-bold tracking-wider border border-\[#3D3D3D\]/g, 
  'bg-crt-bg text-[10px] text-crt-accent group-hover:text-crt-glow font-bold tracking-wider border border-crt-accent');

fs.writeFileSync(file, content, 'utf8');
console.log('Done.');
