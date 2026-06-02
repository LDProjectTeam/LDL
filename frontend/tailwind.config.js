/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        crt: {
          bg: '#020205',
          text: '#d0e0ff',
          accent: '#507090',
          glow: '#80b0ff'
        }
      },
      fontFamily: {
        mono: ['"IBM Plex Mono"', 'monospace'],
        sans: ['"IBM Plex Mono"', 'monospace'], // Force monospace everywhere
      },
      boxShadow: {
        'crt-glow': '0 0 10px rgba(128, 176, 255, 0.25)',
      }
    },
  },
  plugins: [],
}
