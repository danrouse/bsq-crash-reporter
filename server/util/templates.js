const fs = require('fs');
const path = require('path');
const urgentMessage = require('./urgent-message');

const templates = {};
fs.readdirSync(path.join(__dirname, '../templates')).forEach((fileName) => {
  const contents = fs.readFileSync(path.join(__dirname, '../templates', fileName), 'utf-8');
  templates[fileName.split('.').slice(0, -1).join('')] = contents;
});

function renderTemplate(template, tokens) {
  if (!template in templates) throw new Error(`Template not found: ${template}`);
  let buf = templates[template];
  if (tokens) {
    Object.keys(tokens).forEach((key) => {
      const regexp = new RegExp(`{{${key}}}`, 'g');
      buf = buf.replace(regexp, tokens[key]);
    });
  }
  return buf;
};

function renderPage(template, title, tokens) {
  return renderTemplate('base', {
    title: `Beat Saber Quest Crash Logs${title ? ` | ${title}` : ''}`,
    page: renderTemplate(template, tokens),
    urgentMessage: urgentMessage(),
  });
}

module.exports = {
  renderTemplate,
  renderPage,
};
