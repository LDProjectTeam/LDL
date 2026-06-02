#!/usr/bin/env bash

# =========================================
#       LDLauncher Test Environment
# =========================================

# Step 1: Switch to the directory where this script lives (frontend/)
cd "$(dirname "$0")" || exit 1

# Step 2: Try to load Node.js from nvm / fnm if available
if [ -s "$HOME/.nvm/nvm.sh" ]; then
    source "$HOME/.nvm/nvm.sh"
elif [ -s "$HOME/.config/nvm/nvm.sh" ]; then
    source "$HOME/.config/nvm/nvm.sh"
elif command -v fnm &>/dev/null; then
    eval "$(fnm env)"
fi

# Step 3: Verify npm is available
if ! command -v npm &>/dev/null; then
    echo ""
    echo " [ERROR] npm not found! Please install Node.js:"
    echo "         https://nodejs.org/  or  use nvm: https://github.com/nvm-sh/nvm"
    echo ""
    exit 1
fi

echo ""
echo "========================================="
echo "      LDLauncher Test Environment"
echo "========================================="
echo ""
echo " [1] Test in Browser  (Hot Reload, very fast)"
echo " [2] Test as Desktop App  (Build + Electron)"
echo ""
read -rp "Choose an option (1 or 2): " choice

case "$choice" in
    1)
        echo ""
        echo "Starting Vite Dev Server..."
        npm run dev
        if [ $? -ne 0 ]; then
            echo ""
            echo " [ERROR] Dev server failed to start."
        fi
        ;;
    2)
        echo ""
        echo "Building React files..."
        npm run build
        if [ $? -ne 0 ]; then
            echo ""
            echo " [ERROR] Build failed. Fix the errors above and try again."
            exit 1
        fi
        echo ""
        echo "Starting Electron app..."
        npm start
        if [ $? -ne 0 ]; then
            echo ""
            echo " [ERROR] Electron failed to start."
        fi
        ;;
    *)
        echo ""
        echo " [ERROR] Invalid choice. Please enter 1 or 2."
        exit 1
        ;;
esac
