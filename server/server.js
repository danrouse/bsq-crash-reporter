const fs = require('fs');
const http = require('http');
const https = require('https');
const express = require('express');
const fileUpload = require('express-fileupload');
const { Firestore } = require('@google-cloud/firestore');
const { gzip, ungzip } = require('node-gzip');
const ndkStack = require('./util/ndk-stack');
const generateReadableId = require('./util/generate-readable-id');
const { renderTemplate, renderPage } = require('./util/templates');
const { cacheMiddleware, invalidateCache, clearCache } = require('./util/cache-middleware');
const { getTombstoneDetails } = require('./util/tombstone-parser');

const PORT = process.env.PORT || 3000;

const app = express();
const firestore = new Firestore({
  projectId: 'bsq-crash-reporter',
  keyFilename: './secrets/firestore-key.json',
});
const tombstonesCollection = firestore.collection('tombstones');

app.use(express.static('static'));
app.use('/.well-known', express.static('.well-known', { dotfiles: 'allow' }));
app.use(fileUpload({
  limits: {
    fileSize: 50 * 1024 * 1024,
    useTempFiles: true,
    tempFileDir: '/tmp/',
  }
}));
// TODO: Default error handler for prettier error output

app.post('/upload-tombstone', async (req, res) => {
  if (req.files.tombstone) {
    console.log('Received tombstone...');

    const details = getTombstoneDetails(req.files.tombstone.data.toString());
    const ndkStackResult = await ndkStack(req.files.tombstone.data);
    const compressedLog = await gzip(ndkStackResult + details.appendToBacktrace);
    const readableId = generateReadableId();

    // Return an id to the client before doing slow remote ops
    res.status(201).send(readableId);

    const record = tombstonesCollection.doc();
    const fields = {
      readableId,
      version: req.body.version || '<empty>',
      uid: req.body.uid || '<empty>',
      os: req.body.os || '<empty>',
      time: Date.now(),
      backtraceRefs: details.backtrace,
      memoryRefs: details.memoryMap,
      backtraceRefIds: details.backtraceIds,
      memoryRefIds: details.memoryMapIds,
      signalInfo: details.signalInfo,
      log: compressedLog,
    };
    console.log('Saving fields with serialized size of', JSON.stringify(fields).length);
    await record.set(fields);
    console.info(`Saved tombstone to Firestore with id ${record.id}`);    

    details.backtrace.forEach((libName) => invalidateCache(`backtraces/${libName}`));
    details.memoryMap.forEach((libName) => invalidateCache(`libs/${libName}`));
    invalidateCache('/');
    return;
  }

  res.status(418).send('Error: A tombstone must be included in the request');
});

app.get('/tombstones/:id', cacheMiddleware, async (req, res) => {
  const query = await tombstonesCollection.where('readableId', '==', req.params.id).get();
  if (query.empty) return res.status(404).send('Not Found');

  const data = query.docs[0].data();
  let uncompressedLog;
  try {
    uncompressedLog = (await ungzip(data.log)).toString();
  } catch(err) {
    uncompressedLog = data.log;
  }

  res.status(200).send(
    renderPage('tombstone', `Crash Log ${req.params.id}`, {
      id: req.params.id,
      time: new Date(data.time).toISOString(),
      gameVersion: data.version,
      deviceUniqueId: data.uid,
      operatingSystem: data.os,
      signalInfo: data.signalInfo,
      backtrace: uncompressedLog.split('\n').map((line, i) =>
        `<a name="L${i+1}" href="#L${i+1}">${line}</a>`).join(''),
      memoryRefs: data.memoryRefs.map((libName, i) =>
        renderTemplate('tombstone-mod-item', {
          libName,
          buildId: data.memoryRefIds[i],
          searchType: 'libs',
        })).join(''),
      backtraceRefs: data.backtraceRefs.map((libName, i) =>
        renderTemplate('tombstone-mod-item', {
          libName,
          buildId: data.backtraceRefIds[i],
          searchType: 'backtraces',
        })).join(''),
    })
  );
});

const ignoreLibsInBacktrace = ['libCrashMod', 'libanxiety'];
async function renderSearchResultsPage(query, search, searchType) {
  const results = await Promise.all(query.docs.map(async (doc) => {
    const data = doc.data();
    const uncompressedLog = (await ungzip(data.log)).toString();
    return {
      ...data,
      uncompressedLog,
    };
  }));
  const filteredResults = results.filter((row) => {
    return !ignoreLibsInBacktrace.filter((ignoredLib) => ignoredLib !== search).some((ignoredLib) =>
      row.backtraceRefs.includes(ignoredLib));
  });
  return renderPage('listing', `${search} (${searchType})`, {
    search,
    searchType,
    results: query.empty
      ? '<li>No results found</li>'
      : filteredResults.map((data) => {
        const buildIdIndex = data[`${searchType}Refs`].indexOf(search);
        const timestamp = new Date(data.time).toISOString();
        return renderTemplate('listing-item', {
          id: data.readableId,
          buildId: data[`${searchType}RefIds`][buildIdIndex],
          time: timestamp,
        });
      }).join(''),
  });
}

app.get('/backtraces/:libName', cacheMiddleware, async (req, res) => {
  const query = await tombstonesCollection.where('backtraceRefs', 'array-contains', req.params.libName).get();
  res.status(200).send(await renderSearchResultsPage(query, req.params.libName, 'backtrace'));
});

app.get('/libs/:libName', cacheMiddleware, async (req, res) => {
  const query = await tombstonesCollection.where('memoryRefs', 'array-contains', req.params.libName).get();
  res.status(200).send(await renderSearchResultsPage(query, req.params.libName, 'memory'));
});

app.get('/', cacheMiddleware, async (req, res) => {
  const query = await tombstonesCollection.orderBy('time', 'desc').limit(25).get();
  res.status(200).send(
    renderPage('latest', `Latest crashes`, {
      results: query.empty
        ? '<li>No results found</li>'
        : query.docs.map((doc) => {
          const data = doc.data();
          const timestamp = new Date(data.time).toISOString();
          return renderTemplate('latest-item', {
            id: data.readableId,
            time: timestamp,
          });
        }).join('')
      }
    )
  );
});

app.get('/clear-cache', async (req, res) => {
  clearCache();
  res.send(renderTemplate('base', {
    title: 'Cache cleared',
    page: '<div class="container">Cache cleared.</div>',
    urgentMessage: 'what',
  }));
});

app.get('/manual-upload', async (req, res) => {
  res.send(renderPage('upload-tombstone'));
});

http.createServer(app).listen(PORT, () => console.log(`HTTP server listening on port ${PORT}`));
https.createServer({
  key: fs.readFileSync('./secrets/private.key'),
  cert: fs.readFileSync('./secrets/certificate.crt'),
}, app).listen(443, () => console.log('HTTP server listening on port 443'));
