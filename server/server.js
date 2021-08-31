const fs = require('fs');
const express = require('express');
const fileUpload = require('express-fileupload');
const { Firestore } = require('@google-cloud/firestore');
const ndkStack = require('./ndk-stack');
const urgentMessage = require('./urgent-message');
const { getLibReferences } = require('./tombstone-parser');
const templates = {
  listing: fs.readFileSync('templates/listing.html', 'utf-8'),
  tombstone: fs.readFileSync('templates/tombstone.html', 'utf-8'),
};

function renderTemplate(template, tokens) {
  let buf = templates[template];
  Object.keys(tokens).forEach((key) => {
    const regexp = new RegExp(`{{${key}}}`, 'g');
    buf = buf.replace(regexp, tokens[key]);
  });
  return buf;
}

const PORT = process.env.PORT || 3000;

const app = express();
const firestore = new Firestore({
  projectId: 'bsq-crash-reporter',
  keyFilename: './firestore-key.json',
});
const tombstonesCollection = firestore.collection('tombstones');

app.use(express.static('static'));
app.use(fileUpload({
  limits: {
    fileSize: 50 * 1024 * 1024,
    useTempFiles: true,
    tempFileDir: '/tmp/',
  }
}));

app.post('/upload', async (req, res) => {
  if (req.files.tombstone) {
    console.log('Received tombstone...');
    // TODO: Worth even storing the raw tombstone?

    const references = getLibReferences(req.files.tombstone.data.toString());
    const ndkStackResult = await ndkStack(req.files.tombstone.data);
    // TODO: Store ndk-stack output in persistent storage?

    const record = tombstonesCollection.doc();
    // Return an id to the client before doing slow remote ops
    res.status(201).send(record.id);

    await record.set({
      time: Date.now(),
      backtraceRefs: Array.from(references.backtrace),
      memoryRefs: Array.from(references.memory),
      log: ndkStackResult,
    });

    console.info(`Saved tombstone to Firestore with id ${record.id}`);    
  }

  res.status(418).send('Error: A tombstone must be included in the request');
});

app.get('/tombstones/:id', async (req, res) => {
  const ref = tombstonesCollection.doc(req.params.id);
  const record = await ref.get();
  if (!record) {
    return res.status(404).return('Not Found');
  }
  const data = record.data();
  res.status(200).send(renderTemplate('tombstone', {
    id: record.id,
    time: new Date(data.time).toISOString(),
    backtrace: data.log,
    memoryRefs: data.memoryRefs.map((libName) =>
      `<li><a href="/libs/${libName}">${libName}</a></li>`
    ).join(''),
    backtraceRefs: data.backtraceRefs.map((libName) =>
      `<li><a href="/backtraces/${libName}">${libName}</a></li>`
    ).join(''),
  }));
});

app.get('/backtraces/:libName', async (req, res) => {
  const query = await tombstonesCollection.where('backtraceRefs', 'array-contains', req.params.libName).get();
  if (query.empty) {
    return res.status(404).return('Not Found');
  }

  res.status(200).send(renderTemplate('listing', {
    search: req.params.libName,
    searchType: 'backtrace',
    results: query.docs.map((doc) => {
      const timestamp = new Date(doc.data().time).toISOString();
      return `<li><a href="/tombstones/${doc.id}">${doc.id} (<time class="date" datetime="${timestamp}">${timestamp}</time>)</a></li>`
    }).join(''),
  }));
});

app.get('/libs/:libName', async (req, res) => {
  const query = await tombstonesCollection.where('memoryRefs', 'array-contains', req.params.libName).get();
  if (query.empty) {
    return res.status(404).return('Not Found');
  }

  res.status(200).send(renderTemplate('listing', {
    search: req.params.libName,
    searchType: 'memory',
    results: query.docs.map((doc) => {
      const timestamp = new Date(doc.data().time).toISOString();
      return `<li><a href="/tombstones/${doc.id}">${doc.id} (<time class="date" datetime="${timestamp}">${timestamp}</time>)</a></li>`
    }).join(''),
  }));
});

app.get('/', (req, res) => res.send(urgentMessage()));

app.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}...`);
});
