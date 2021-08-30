const express = require('express');
const fileUpload = require('express-fileupload');
const urgentMessage = require('./urgent-message');

const PORT = process.env.PORT || 3000;

const app = express();

app.use(fileUpload({
  limits: {
    fileSize: 50 * 1024 * 1024,
    useTempFiles: true,
    tempFileDir: '/tmp/',
  }
}));

app.post('/upload', (req, res) => {
  if (req.files.tombstone) {
    const tombstone = req.files.tombstone.data.toString();
    console.log('Received tombstone:');
    console.log(tombstone);

    // TODO: All of this :-)
    // (if necessary) copy tombstone somewhere useful
    // run ndk-stack on tombstone and save its output somewhere
    // push tombstone and ndk-stack output to S3
    // extract tokens from tombstone:
    // - all libs found in the crash backtrace
    // create database record for crash, plus join records for found libs
    // return database id (guid?) for crash log to client

    return res.send('ok');
  }

  res.status(418).send('Error: A tombstone must be included in the request');
});

app.get('/logs/:id', (req, res) => {
  // TODO: Also all of this
  // find individual crash by id
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

app.listen(PORT, () => {
  console.log(`Server listening onasdsd port ${PORT}...`);
});
