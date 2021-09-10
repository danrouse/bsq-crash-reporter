import fs from 'fs';
import http from 'http';
import https from 'https';
import express from 'express';
import fileUpload from 'express-fileupload';
import { Firestore } from '@google-cloud/firestore';
import { gzip, ungzip } from 'node-gzip';
import ndkStack from './util/ndk-stack';
import generateReadableId from './util/generate-readable-id';
import { renderTemplate, renderPage } from './util/templates';
import { cacheMiddleware, invalidateCache, clearCache } from './util/cache-middleware';
import { getTombstoneDetails } from './util/tombstone-parser';

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

app.post('/upload-crash-details', async (req, res) => {
  const readableId = generateReadableId();
  let compressedLog;
  if (req.body.backtrace) {
    const ndkStackResult = await ndkStack(Buffer.from(req.body.backtrace, 'utf-8'));
    compressedLog = await gzip(ndkStackResult);
  }

  res.status(201).send(readableId);

  const backtraceRefs: string[] = (req.body.backtraceLibs || '').split('\n');
  const memoryRefs: string[] = (req.body.memoryLibs || '').split('\n');
  
  const record = tombstonesCollection.doc();
  const fields: TombstoneV2 = {
    v: 2,
    readableId,
    time: Date.now(),
    version: req.body.gameVersion || '',
    uid: req.body.deviceUniqueIdentifier || '',
    os: req.body.operatingSystem || '',
    prevScene: req.body.prevSceneName || '',
    nextScene: req.body.nextSceneName || '',
    sceneTime: req.body.secondsInScene || '',
    rsp: req.body.registerSP || '',
    rlr: req.body.registerLR || '',
    rpc: req.body.registerPC || '',
    sig: req.body.signal || '',
    faddr: req.body.faultAddress || '',
    backtraceRefs,
    memoryRefs,
    backtraceRefIds: (req.body.backtraceLibBuildIds || '').split('\n'),
    memoryRefIds: (req.body.memoryLibBuildIds || '').split('\n'),
    logcat: req.body.log || '',
    backtrace: compressedLog || '',
  };
  console.log('Saving fields with serialized size of', JSON.stringify(fields).length);
  await record.set(fields);
  console.info(`Saved tombstone to Firestore with id ${record.id}`);    

  backtraceRefs.forEach((libName) => invalidateCache(`backtraces/${libName}`));
  memoryRefs.forEach((libName) => invalidateCache(`libs/${libName}`));
  invalidateCache('/');
});

app.post('/upload-tombstone', async (req, res) => {
  if (req.files && req.files.tombstone) {
    console.log('Received tombstone...');

    if (Array.isArray(req.files.tombstone)) req.files.tombstone = req.files.tombstone[0];
    const details = getTombstoneDetails(req.files.tombstone.data.toString());
    const ndkStackResult = await ndkStack(req.files.tombstone.data);
    const compressedLog = await gzip(ndkStackResult + details.appendToBacktrace);
    const readableId = generateReadableId();

    // Return an id to the client before doing slow remote ops
    res.status(201).send(readableId);

    const record = tombstonesCollection.doc();
    const fields: TombstoneV1 = {
      readableId,
      version: req.body.version || '&lt;empty&gt;',
      uid: req.body.uid || '&lt;empty&gt;',
      os: req.body.os || '&lt;empty&gt;',
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

// TODO: handle new format tombstone data
app.get('/tombstones/:id', cacheMiddleware, async (req, res) => {
  const query = await tombstonesCollection.where('readableId', '==', req.params.id).get();
  if (query.empty) return res.status(404).send('Not Found');
  const rawData = query.docs[0].data();
  
  let page; 
  if (rawData.v && rawData.v === 2) {
    const data = rawData as TombstoneV2;
    const uncompressedLog = (await ungzip(data.backtrace)).toString();
    page = renderPage('tombstone-v2', `Crash Log ${req.params.id}`, {
      id: req.params.id,
      time: new Date(data.time).toISOString(),
      gameVersion: data.version || '&lt;empty&gt;',
      deviceUniqueId: data.uid || '&lt;empty&gt;',
      operatingSystem: data.os || '&lt;empty&gt;',
      prevSceneName: data.prevScene || '&lt;empty&gt;',
      nextSceneName: data.nextScene || '&lt;empty&gt;',
      secondsInScene: data.sceneTime || '&lt;empty&gt;',
      registerSP: data.rsp || '&lt;empty&gt;',
      registerLR: data.rlr || '&lt;empty&gt;',
      registerPC: data.rpc || '&lt;empty&gt;',
      signal: data.sig || '&lt;empty&gt;',
      faultAddress: data.faddr || '&lt;empty&gt;',
      log: data.logcat || '<no log>',
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
    });
  } else {
    const data = rawData as TombstoneV1;
    let uncompressedLog: string;
    try {
      uncompressedLog = (await ungzip(data.log)).toString();
    } catch(err) {
      uncompressedLog = data.log as string;
    }
    page = renderPage('tombstone-v1', `Crash Log ${req.params.id}`, {
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
    });
  }

  res.status(200).send(page);
});

const ignoreLibsInBacktrace = ['libCrashMod', 'libanxiety'];
async function renderSearchResultsPage(
  query: FirebaseFirestore.QuerySnapshot<FirebaseFirestore.DocumentData>,
  search: string,
  searchType: 'backtrace' | 'memory'
) {
  const results = await Promise.all(query.docs.map(async (doc) => {
    const data = doc.data() as TombstoneV1;
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
