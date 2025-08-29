// main.js
const { app, BrowserWindow } = require('electron');
const path = require('path');
const { spawn } = require('child_process');

let childProcess = null;

function createWindow() {
  // Create a browser window
  const win = new BrowserWindow({
    width: 1000,
    height: 700,
    webPreferences: {
      nodeIntegration: true,  // allow Node.js in renderer (easier for beginners)
      contextIsolation: false // makes window + document available in renderer
    }
  });

  // Load your UI (index.html)
  win.loadFile('index.html');

  // Optional: open DevTools (good for debugging)
  // win.webContents.openDevTools();
}

app.whenReady().then(() => {
  // If you have a compiled C++ executable, try to run it
  const exeName = process.platform === 'win32' ? 'server.exe' : './server';
  const exePath = path.join(__dirname, exeName);

  try {
    childProcess = spawn(exePath, [], { stdio: ['pipe', 'pipe', 'pipe'] });

    childProcess.stdout.on('data', (data) => {
      console.log('C++ stdout:', data.toString());
    });

    childProcess.stderr.on('data', (data) => {
      console.error('C++ stderr:', data.toString());
    });

    childProcess.on('close', (code) => {
      console.log('C++ process exited with code', code);
    });
  } catch (err) {
    console.log('Could not start C++ executable:', err.message);
  }

  // Create the app window
  createWindow();
});

app.on('window-all-closed', () => {
  // Kill the C++ child process when app is closed
  if (childProcess) childProcess.kill();

  // Quit app except on macOS (where apps stay in the dock)
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  // On macOS, re-create a window if none exist
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
