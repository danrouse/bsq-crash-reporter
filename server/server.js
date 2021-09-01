const express = require('express');
const fileUpload = require('express-fileupload');
const { Firestore } = require('@google-cloud/firestore');
const { gzip, ungzip } = require('node-gzip');
const PasswordGen = require('passwordgen');
const ndkStack = require('./util/ndk-stack');
const { renderTemplate, renderPage } = require('./util/templates');
const urgentMessage = require('./util/urgent-message');
const cacheMiddleware = require('./util/cache-middleware');
const { getLibReferences } = require('./util/tombstone-parser');

const PORT = process.env.PORT || 3000;

const app = express();
const passwordGen = new PasswordGen();
const firestore = new Firestore({
  projectId: 'bsq-crash-reporter',
  keyFilename: './firestore-key.json',
});
const tombstonesCollection = firestore.collection('tombstones');

app.use(express.static('static'));
// app.use(express.json());
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
    console.log(req);

    const references = getLibReferences(req.files.tombstone.data.toString());
    const ndkStackResult = await ndkStack(req.files.tombstone.data);
    const compressedLog = await gzip(ndkStackResult);

    const readableId = passwordGen.phrase(4, { symbols: false, separator: '-' });
    // Return an id to the client before doing slow remote ops
    res.status(201).send(readableId);

    const record = tombstonesCollection.doc();
    await record.set({
      readableId,
      version: req.body.version,
      uid: req.body.uid,
      os: req.body.os,
      time: Date.now(),
      backtraceRefs: references.backtrace,
      memoryRefs: references.memoryMap,
      backtraceRefIds: references.backtraceIds,
      memoryRefIds: references.memoryMapIds,
      log: compressedLog,
    });

    console.info(`Saved tombstone to Firestore with id ${record.id}`);    
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
    uncompressedLog = await ungzip(data.log);
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
      backtrace: uncompressedLog,
      memoryRefs: data.memoryRefs.map((libName, i) =>
        renderTemplate('tombstoneModItem', {
          libName,
          buildId: data.memoryRefIds[i],
          searchType: 'libs',
        })).join(''),
      backtraceRefs: data.backtraceRefs.map((libName, i) =>
        renderTemplate('tombstoneModItem', {
          libName,
          buildId: data.backtraceRefIds[i],
          searchType: 'backtraces',
        })).join(''),
    })
  );
});

function renderSearchResultsPage(query, search, searchType) {
  return renderPage('listing', `${search} (${searchType})`, {
    search,
    searchType,
    results: query.empty
      ? '<li>No results found</li>'
      : query.docs.map((doc) => {
        const data = doc.data();
        const buildIdIndex = data[`${searchType}Refs`].indexOf(search);
        const timestamp = new Date(data.time).toISOString();
        return renderTemplate('listingItem', {
          id: data.readableId,
          buildId: data[`${searchType}RefIds`][buildIdIndex],
          time: timestamp,
        });
      }).join(''),
  });
}

// TODO: Figure out caching strategy for search results that doesn't go stale...
app.get('/backtraces/:libName', async (req, res) => {
  const query = await tombstonesCollection.where('backtraceRefs', 'array-contains', req.params.libName).get();
  res.status(200).send(renderSearchResultsPage(query, req.params.libName, 'backtrace'));
});

app.get('/libs/:libName', async (req, res) => {
  const query = await tombstonesCollection.where('memoryRefs', 'array-contains', req.params.libName).get();
  res.status(200).send(renderSearchResultsPage(query, req.params.libName, 'memory'));
});

app.get('/', (req, res) => res.send(urgentMessage()));

app.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}...`);
});
