const express = require('express');
const fileUpload = require('express-fileupload');
const { Firestore } = require('@google-cloud/firestore');
const ndkStack = require('./ndk-stack');
const urgentMessage = require('./urgent-message');
const { getLibReferences } = require('./tombstone-parser');

const PORT = process.env.PORT || 3000;

const app = express();
const firestore = new Firestore({
  projectId: 'bsq-crash-reporter',
  keyFilename: './firestore-key.json',
});
const tombstonesCollection = firestore.collection('tombstones');
// const doc = coll.doc();
// doc.set({ foo: 'bar' });
// console.log('doc id', doc.id);


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
    await record.set({
      time: Date.now(),
      backtraceRefs: Array.from(references.backtrace),
      memoryRefs: Array.from(references.memory),
      log: ndkStackResult,
    });

    console.info(`Saved tombstone to Firestore with id ${record.id}`);

    return res.status(201).send(record.id); 
  }

  res.status(418).send('Error: A tombstone must be included in the request');
});

app.get('/tombstones/:id', (req, res) => {
  // TODO: Retrieve individual crash by id
  const record = null;
  if (!record) {
    return res.status(404).return('Not Found');
  }

  // display it along with any available metadata
  // make it _extremely pretty_
  /*
  crash log $id - $datetime
  $userUniqueId
  [-] ndk stack output
  [+] full tombstone
  */

  res.send(urgentMessage());
});

app.get('/libs/:libName', (req, res) => {
  // TODO: Also also all of this
  // find all crashes in database with libName in backtrace
  // return list of crashes linked to /log/:id

  res.send(urgentMessage());
});

app.get('/', (req, res) => res.send(urgentMessage()));

app.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}...`);
});
