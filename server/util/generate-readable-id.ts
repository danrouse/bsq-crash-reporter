import words from './wordlist.json';

export default () => [
  words[Math.floor(Math.random() * words.length)],
  words[Math.floor(Math.random() * words.length)],
  words[Math.floor(Math.random() * words.length)],
].join('-');
