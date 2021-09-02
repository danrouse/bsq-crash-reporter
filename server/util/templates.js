const fs = require('fs');

const templates = {
  base: fs.readFileSync('templates/base.html', 'utf-8'),
  latest: fs.readFileSync('templates/latest.html', 'utf-8'),
  listing: fs.readFileSync('templates/listing.html', 'utf-8'),
  listingItem: fs.readFileSync('templates/listing-item.html', 'utf-8'),
  tombstone: fs.readFileSync('templates/tombstone.html', 'utf-8'),
  tombstoneModItem: fs.readFileSync('templates/tombstone-mod-item.html', 'utf-8'),
};

// TODO (in-template): Add input + radios to search for mods

function renderTemplate(template, tokens) {
  if (!template in templates) throw new Error(`Template not found: ${template}`);
  let buf = templates[template];
  Object.keys(tokens).forEach((key) => {
    const regexp = new RegExp(`{{${key}}}`, 'g');
    buf = buf.replace(regexp, tokens[key]);
  });
  return buf;
};

function renderPage(template, title, tokens) {
  return renderTemplate('base', {
    title: `Beat Saber Quest Crash Logs${title ? ` | ${title}` : ''}`,
    page: renderTemplate(template, tokens),
  });
}

module.exports = {
  renderTemplate,
  renderPage,
};
