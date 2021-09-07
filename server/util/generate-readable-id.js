const words = require('./wordlist.json');

module.exports = () => [
  words[Math.floor(Math.random() * words.length)],
  words[Math.floor(Math.random() * words.length)],
  words[Math.floor(Math.random() * words.length)],
].join('-');
