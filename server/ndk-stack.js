const { spawn } = require('child_process');

const NDK_STACK_PATH = '/usr/src/android-ndk/ndk-stack';
const NDK_STACK_SYMBOLS = '/usr/src/debug-libs';

module.exports = function(dataBuffer) {
  return new Promise((resolve, reject) => {
    let buf = '';
    const proc = spawn(NDK_STACK_PATH, ['-sym', NDK_STACK_SYMBOLS]);
    proc.stdout.on('data', (data) => buf += data.toString());
    proc.stdin.write(dataBuffer);
    proc.stdin.end();
    proc.on('error', (err) => reject(err));
    proc.on('close', () => resolve(buf));
  });
};
