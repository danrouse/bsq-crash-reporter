const ignoredLibs = ['libc', 'libil2cpp', 'libunity', 'libmain'];

function collectLibReferences(symbols, buildIds, line) {
  const match = line.match(/\/com\.beatgames\.beatsaber\/files\/([^/\s]+)\.so .*\(BuildId: ([^)]+)\)/);
  if (match && !ignoredLibs.includes(match[1]) && !symbols.includes(match[1])) {
    symbols.push(match[1]);
    buildIds.push(match[2]);
  }
}

function getTombstoneDetails(tombstoneText) {
  const tombstoneLines = tombstoneText.split('\n');

  let appendToBacktrace = '';

  const failedToUnwindIndex = tombstoneLines.indexOf('Failed to unwind');
  const backtraceStart = tombstoneLines.indexOf('backtrace:') + 1;
  const backtraceEnd = tombstoneLines.indexOf('stack:') - 1;
  const backtraceSymbols = [];
  const backtraceIds = [];
  if (failedToUnwindIndex === -1) {
    tombstoneLines.slice(backtraceStart, backtraceEnd).forEach(
      collectLibReferences.bind(null, backtraceSymbols, backtraceIds)
    );
  } else {
    appendToBacktrace += tombstoneLines[failedToUnwindIndex - 1].trim();
    const memoryNearLrIndex = tombstoneLines.findIndex((line) => line.startsWith('memory near lr ('));
    if (memoryNearLrIndex !== -1) {
      appendToBacktrace += '\n' + tombstoneLines[memoryNearLrIndex].slice(0, -1);
    }
  }

  const memoryMapStart = tombstoneLines.findIndex((line) => line.startsWith('memory map'));
  const memoryMapSymbols = [];
  const memoryMapIds = [];
  tombstoneLines.slice(memoryMapStart).forEach(
    collectLibReferences.bind(null, memoryMapSymbols, memoryMapIds)
  );

  const signalInfoIndex = tombstoneLines.findIndex((line) => line.startsWith('signal '));
  const signalInfo = tombstoneLines[signalInfoIndex] + ', ' + tombstoneLines[signalInfoIndex + 1];

  return {
    backtrace: backtraceSymbols,
    memoryMap: memoryMapSymbols,
    backtraceIds,
    memoryMapIds,
    signalInfo,
    appendToBacktrace,
  };
}

module.exports = {
  getTombstoneDetails,
};
