const ignoredLibs = ['libc', 'libil2cpp', 'libunity', 'libmain'];

function collectLibReferences(set, line) {
  const match = line.match(/\/com\.beatgames\.beatsaber\/files\/([^/\s]+)\.so/);
  if (match && !ignoredLibs.includes(match[1])) {
    set.add(match[1]);
  }
}

function getLibReferences(tombstoneText) {
  const tombstoneLines = tombstoneText.split('\n');

  const backtraceStart = tombstoneLines.indexOf('backtrace:') + 1;
  const backtraceEnd = tombstoneLines.indexOf('stack:') - 1;
  const backtraceSymbols = new Set();
  tombstoneLines.slice(backtraceStart, backtraceEnd).forEach(
    collectLibReferences.bind(null, backtraceSymbols)
  );

  const memoryMapStart = tombstoneLines.findIndex((line) => line.startsWith('memory map'));
  const memoryMapSymbols = new Set();
  tombstoneLines.slice(memoryMapStart).forEach(
    collectLibReferences.bind(null, memoryMapSymbols)
  );

  return {
    backtrace: backtraceSymbols,
    memory: memoryMapSymbols,
  };
}

module.exports = {
  getLibReferences,
};
