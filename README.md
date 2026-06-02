# LDLauncher

A custom Minecraft desktop launcher built with Electron and React.

## Features
- **Modern CRT Dark-Fantasy Aesthetic**: Designed with a high-performance visual style, smooth animations, and custom UI panels.
- **Multiple Game Support**: Manage and run multiple instances (Lost Death series).
- **In-App Payments**: Integrated payment option via Telegram Stars / TON.
- **Support Chat**: Built-in support messaging portal linked directly with Supabase.
- **System-Aware Optimizer**: Detects hardware tier (RAM, CPU cores) and auto-optimizes Minecraft launcher configuration (options.txt and Java arguments).

## Tech Stack
- **Frontend / Desktop Core**: Electron, React, TypeScript, TailwindCSS, Vite, Framer Motion
- **Backend / Services**: Supabase (Edge Functions, Database, Auth)
- **Mod Guard**: Fabric API Minecraft Mod (Java)
- **Support Website**: HTML5, Vanilla JS, Supabase integration

## Setup & Running

### Prerequisites
- Node.js (v18+)
- Java (JRE 17+ or equivalent for Minecraft runtime)

### Development
1. Navigate to the frontend directory:
   `ash
   cd frontend
   `
2. Install dependencies:
   `ash
   npm install
   `
3. Run the development server with Electron:
   `ash
   npm run dev
   `
   Or to start Electron directly:
   `ash
   npm start
   `

### Production Build
To bundle the frontend and generate a portable Windows installer:
`ash
npm run build:exe
`

## Licensing
This project is open-source and licensed under the [MIT License](LICENSE).
